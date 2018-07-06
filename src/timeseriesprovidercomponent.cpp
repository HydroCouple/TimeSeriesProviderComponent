#include "stdafx.h"
#include "timeseriesprovidercomponent.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"
#include "timeseriesprovider.h"
#include "spatial/geometryfactory.h"
#include "core/abstractoutput.h"
#include "progresschecker.h"
#include "temporal/timedata.h"
#include "core/idbasedargument.h"
#include "spatial/envelope.h"
#include "timeseriesinput.h"
#include "spatial/geometry.h"
#include "timeseriesoutput.h"

#include <QTextStream>

using namespace HydroCouple;
using namespace std;
using namespace SDKTemporal;

TimeSeriesProviderComponent::TimeSeriesProviderComponent(const QString &id, TimeSeriesProviderComponentInfo *modelComponentInfo)
  : AbstractTimeModelComponent(id, modelComponentInfo),
    m_inputFilesArgument(nullptr),
    m_parent(nullptr)
{

  m_timeDimension = new Dimension("TimeDimension",this);
  m_geometryDimension = new Dimension("ElementGeometryDimension", this);

  createArguments();
}

TimeSeriesProviderComponent::~TimeSeriesProviderComponent()
{
  if(m_parent)
  {
    m_parent->removeClone(this);
  }

  while (m_clones.size())
  {
    TimeSeriesProviderComponent *clone =  dynamic_cast<TimeSeriesProviderComponent*>(m_clones.first());
    removeClone(clone);
    delete clone;
  }
}

QList<QString> TimeSeriesProviderComponent::validate()
{
  return QList<QString>();
}

void TimeSeriesProviderComponent::prepare()
{
  if(!isPrepared() && isInitialized())
  {
    for(auto output :  outputsInternal())
    {
      for(auto adaptedOutput : output->adaptedOutputs())
      {
        adaptedOutput->initialize();
      }
    }

    progressChecker()->reset(m_beginDateTime, m_endDateTime);

    updateOutputValues(QList<HydroCouple::IOutput*>());

    setStatus(IModelComponent::Updated ,"Finished preparing model");
    setPrepared(true);
  }
  else
  {
    setPrepared(false);
    setStatus(IModelComponent::Failed ,"Error occured when preparing model");
  }
}

void TimeSeriesProviderComponent::update(const QList<HydroCouple::IOutput *> &requiredOutputs)
{
  if(status() == IModelComponent::Updated)
  {
    setStatus(IModelComponent::Updating);

    applyInputValues();

    double minDate = getMinDateTime();

    updateOutputValues(requiredOutputs);

    currentDateTimeInternal()->setJulianDay(minDate);

    if(minDate >=  m_endDateTime)
    {
      setStatus(IModelComponent::Done , "Simulation finished successfully", 100);
    }
    else
    {
      if(progressChecker()->performStep(minDate))
      {
        setStatus(IModelComponent::Updated , "Simulation performed time-step | DateTime: " + QString::number(minDate) , progressChecker()->progress());
      }
      else
      {
        setStatus(IModelComponent::Updated);
      }
    }
  }
}

void TimeSeriesProviderComponent::finish()
{
  if(isPrepared())
  {
    setStatus(IModelComponent::Finishing , "TimeSeriesProviderComponent with id " + id() + " is being disposed" , 100);

    initializeFailureCleanUp();

    setPrepared(false);
    setInitialized(false);

    setStatus(IModelComponent::Finished , "TimeSeriesProviderComponent with id " + id() + " has been disposed" , 100);
    setStatus(IModelComponent::Created , "TimeSeriesProviderComponent with id " + id() + " ran successfully and has been re-created" , 100);
  }
}

ICloneableModelComponent *TimeSeriesProviderComponent::parent() const
{
  return m_parent;
}

ICloneableModelComponent *TimeSeriesProviderComponent::clone()
{
  if(isInitialized())
  {
    TimeSeriesProviderComponent *cloneComponent = dynamic_cast<TimeSeriesProviderComponent*>(componentInfo()->createComponentInstance());
    cloneComponent->setReferenceDirectory(referenceDirectory());

    IdBasedArgumentString *identifierArg = identifierArgument();
    IdBasedArgumentString *cloneIndentifierArg = cloneComponent->identifierArgument();

    (*cloneIndentifierArg)["Id"] = QString((*identifierArg)["Id"]);
    (*cloneIndentifierArg)["Caption"] = QString((*identifierArg)["Caption"]);
    (*cloneIndentifierArg)["Description"] = QString((*identifierArg)["Description"]);

    QString appendName = "_clone_" + QString::number(m_clones.size()) + "_" + QUuid::createUuid().toString().replace("{","").replace("}","");


    QString inputFilePath = QString((*m_inputFilesArgument)["Input File"]);
    QFileInfo inputFile = getAbsoluteFilePath(inputFilePath);

    if(inputFile.absoluteDir().exists())
    {
      QString suffix = "." + inputFile.completeSuffix();
      inputFilePath = inputFile.absoluteFilePath().replace(suffix,"") + appendName + suffix;
      QFile::copy(inputFile.absoluteFilePath(), inputFilePath);
      (*cloneComponent->m_inputFilesArgument)["Input File"] = inputFilePath;
    }


    cloneComponent->m_parent = this;
    m_clones.append(cloneComponent);

    emit propertyChanged("Clones");

    cloneComponent->initialize();

    return cloneComponent;
  }

  return nullptr;
}

QList<ICloneableModelComponent*> TimeSeriesProviderComponent::clones() const
{
  return m_clones;
}

double TimeSeriesProviderComponent::startDateTime() const
{
  return m_beginDateTime;
}

double TimeSeriesProviderComponent::endDateTime() const
{
  return m_endDateTime;
}

bool TimeSeriesProviderComponent::removeClone(TimeSeriesProviderComponent *component)
{
  int removed;

#ifdef USE_OPENMP
#pragma omp critical
#endif
  {
    removed = m_clones.removeAll(component);
  }


  if(removed)
  {
    component->m_parent = nullptr;
    emit propertyChanged("Clones");
  }

  return removed;
}

void TimeSeriesProviderComponent::initializeFailureCleanUp()
{
  for(TimeSeriesProvider *provider : m_timeSeriesProviders)
    delete provider;

  m_timeSeriesProviders.clear();
}

void TimeSeriesProviderComponent::createArguments()
{
  createInputFileArguments();
}

void TimeSeriesProviderComponent::createInputFileArguments()
{
  QStringList fidentifiers;
  fidentifiers.append("Input File");

  Quantity *fquantity = Quantity::unitLessValues("InputFilesQuantity", QVariant::String, this);
  fquantity->setDefaultValue("");
  fquantity->setMissingValue("");

  Dimension *dimension = new Dimension("IdDimension","Dimension for identifiers",this);

  m_inputFilesArgument = new IdBasedArgumentString("InputFiles", fidentifiers, dimension, fquantity, this);
  m_inputFilesArgument->setCaption("Model Input Files");
  m_inputFilesArgument->addFileFilter("Input File (*.inp)");
  m_inputFilesArgument->setMatchIdentifiersWhenReading(true);

  addArgument(m_inputFilesArgument);
}

bool TimeSeriesProviderComponent::initializeArguments(QString &message)
{
  message = "";

  bool initialized = initializeInputFilesArguments(message);

  return initialized;
}

bool TimeSeriesProviderComponent::initializeInputFilesArguments(QString &message)
{
  message = "";

  QString inputFilePath = (*m_inputFilesArgument)["Input File"];
  QFileInfo inputFile = getAbsoluteFilePath(inputFilePath);

  initializeFailureCleanUp();

  if(inputFile.isFile() && inputFile.exists())
  {
    QFile file(inputFile.absoluteFilePath());

    if(file.open(QIODevice::ReadOnly))
    {
      QTextStream streamReader(&file);
      int currentFlag = -1;
      QRegExp delimiters = QRegExp("(\\,|\\t|\\;|\\s+)");

      int lineCount = 0;

      while (!streamReader.atEnd())
      {
        QString line = streamReader.readLine().trimmed();
        lineCount++;

        if (!line.isEmpty() && !line.isNull())
        {
          bool readSuccess = true;
          QString error = "";

          auto it = m_inputFileFlags.find(line.toStdString());

          if (it != m_inputFileFlags.cend())
          {
            currentFlag = it->second;
          }
          else if (!QStringRef::compare(QStringRef(&line, 0, 2), ";;"))
          {
            //commment do nothing
          }
          else
          {
            switch (currentFlag)
            {
              case 1:
                {
                  QStringList cols = line.split(delimiters, QString::SkipEmptyParts);

                  if(cols.size() == 3)
                  {
                    QDateTime dateTime;

                    if(cols[0] == "START_DATETIME" &&
                       SDKTemporal::DateTime::tryParse(cols[1] + " " + cols[2], dateTime))
                    {
                      m_beginDateTime = SDKTemporal::DateTime::toJulianDays(dateTime);
                    }
                    else if( cols[0] == "END_DATETIME" &&
                             SDKTemporal::DateTime::tryParse(cols[1] + " " + cols[2], dateTime))
                    {
                      m_endDateTime = SDKTemporal::DateTime::toJulianDays(dateTime);
                    }
                    else
                    {
                      message = "Error reading date time";
                      return false;
                    }
                  }
                }
                break;
              case 2:
                {
                  QStringList cols = line.split(delimiters, QString::SkipEmptyParts);

                  if(cols.size() == 5)
                  {
                    QFileInfo tsFile = getAbsoluteFilePath(cols[1]);
                    QFileInfo geomFile = getAbsoluteFilePath(cols[2]);

                    if(tsFile.exists() && geomFile.exists())
                    {
                      TimeSeries *timeSeriesObj = nullptr;

                      if((timeSeriesObj = TimeSeries::createTimeSeries(cols[0],tsFile, this)))
                      {
                        TimeSeriesProvider *timeSeriesProvider = new TimeSeriesProvider(cols[0], this);
                        timeSeriesProvider->setTimeSeries(timeSeriesObj);

                        QList<HCGeometry*> geometries;

                        Envelope envp;

                        if(!GeometryFactory::readGeometryFromFile(geomFile.absoluteFilePath(), geometries, envp, error))
                        {
                          return false;
                        }

                        QList<QSharedPointer<HCGeometry>> sharedGeoms;

                        for(HCGeometry *geometry : geometries)
                        {
                          sharedGeoms.push_back(QSharedPointer<HCGeometry>(geometry));
                        }

                        timeSeriesProvider->setGeometries(sharedGeoms);

                        bool multOk = false;
                        double mult = cols[3].toDouble(&multOk);

                        if(multOk)
                          timeSeriesProvider->setMultiplier(mult);

                        auto it = m_geomMultiplierFlags.find(cols[4].toStdString());

                        if(it != m_geomMultiplierFlags.end())
                        {
                          int type = it->second;

                          switch(type)
                          {
                            case 2:
                              timeSeriesProvider->setGeometryMultiplierAttribute(TimeSeriesProvider::Length);
                              break;
                            case 3:
                              timeSeriesProvider->setGeometryMultiplierAttribute(TimeSeriesProvider::Area);
                              break;
                            default:
                              timeSeriesProvider->setGeometryMultiplierAttribute(TimeSeriesProvider::None);
                              break;
                          }
                        }

                        m_timeSeriesProviders.push_back(timeSeriesProvider);
                      }
                      else
                      {
                        message = "Unable to read ts file";
                        return false;
                      }

                    }
                    else
                    {
                      message = "Time series file/geometry file does not exist";
                      return false;
                    }
                  }
                }
                break;
            }
          }

          if (!readSuccess)
          {
            message = "Line " + QString::number(lineCount) + " : " + error;
            file.close();
            return false;
            break;
          }
        }
      }

      file.close();
    }
  }
  else
  {
    message = "Input file does not exist: " + inputFile.absoluteFilePath();
    return false;
  }

  return true;
}

void TimeSeriesProviderComponent::createInputs()
{
   for(size_t i = 0 ; i < m_timeSeriesProviders.size(); i++)
   {
     TimeSeriesProvider *timeSeriesProvider  = m_timeSeriesProviders[i];
     QSharedPointer<HCGeometry> geometry = timeSeriesProvider->geometries()[0];
     Quantity *unitless = Quantity::unitLessValues("Unitless", QVariant::Double, this);
     TimeSeriesMultiplierInput *timeSeriesMultiplierInput = new TimeSeriesMultiplierInput(timeSeriesProvider, m_geometryDimension, geometry->geometryType(), unitless, this);
     timeSeriesMultiplierInput->setCaption(timeSeriesProvider->id());
     addInput(timeSeriesMultiplierInput);
   }
}

void TimeSeriesProviderComponent::createOutputs()
{
  for(size_t i = 0 ; i < m_timeSeriesProviders.size(); i++)
  {
    TimeSeriesProvider *timeSeriesProvider  = m_timeSeriesProviders[i];
    QSharedPointer<HCGeometry> geometry = timeSeriesProvider->geometries()[0];
    Quantity *unitless = Quantity::unitLessValues("Unitless", QVariant::Double, this);
    TimeSeriesOutput *timeSeriesOutput = new TimeSeriesOutput(timeSeriesProvider,
                                                              m_geometryDimension,
                                                              geometry->geometryType(),
                                                              m_timeDimension,
                                                              unitless,
                                                              this);

    timeSeriesOutput->setCaption(timeSeriesProvider->id());
    addOutput(timeSeriesOutput);
  }
}

double TimeSeriesProviderComponent::getMinDateTime()
{
  return 0;
}

const unordered_map<string, int> TimeSeriesProviderComponent::m_inputFileFlags({
                                                                                 {"[OPTIONS]", 1},
                                                                                 {"[SOURCES]", 2},
                                                                               });

const unordered_map<string, int> TimeSeriesProviderComponent::m_optionsFlags({
                                                                               {"START_DATETIME", 1},
                                                                               {"END_DATETIME", 2},
                                                                             });

const unordered_map<string, int> TimeSeriesProviderComponent::m_geomMultiplierFlags({
                                                                                      {"NONE", 1},
                                                                                      {"LENGTH", 2},
                                                                                      {"AREA", 3},
                                                                                    });
