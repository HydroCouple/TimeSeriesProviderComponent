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
#include "timeseriesidbasedoutput.h"

#include <QTextStream>
#include <QDebug>

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

  while (m_clones.size())
  {
    TimeSeriesProviderComponent *clone =  dynamic_cast<TimeSeriesProviderComponent*>(m_clones.first());
    removeClone(clone);
    delete clone;
  }

  if(m_parent)
  {
    m_parent->removeClone(this);
    m_parent = nullptr;
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

    m_currentDateTime += m_stepSize;

    applyInputValues();

    updateOutputValues(requiredOutputs);

    currentDateTimeInternal()->setJulianDay(m_currentDateTime);

    if(m_currentDateTime >=  m_endDateTime)
    {
      setStatus(IModelComponent::Done , "Simulation finished successfully", 100);
    }
    else
    {
      if(progressChecker()->performStep(m_currentDateTime))
      {
        setStatus(IModelComponent::Updated , "Simulation performed time-step | DateTime: " + QString::number(m_currentDateTime, 'f') , progressChecker()->progress());
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

double TimeSeriesProviderComponent::nextDateTime() const
{
  return m_currentDateTime;
}

bool TimeSeriesProviderComponent::removeClone(TimeSeriesProviderComponent *component)
{
  int removed;

#ifdef USE_OPENMP
#pragma omp critical (TimeSeriesProviderComponent)
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

  m_timeSeriesDesc.clear();

  initializeFailureCleanUp();

  if(inputFile.isFile() && inputFile.exists() && !inputFile.isDir())
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

                    if(cols[0] == "START_DATETIME" && SDKTemporal::DateTime::tryParse(cols[1] + " " + cols[2], dateTime))
                    {
                      m_beginDateTime = SDKTemporal::DateTime::toJulianDays(dateTime);
                    }
                    else if(cols[0] == "END_DATETIME" && SDKTemporal::DateTime::tryParse(cols[1] + " " + cols[2], dateTime))
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
                  QStringList cols = TimeSeries::splitLine(line, "\\,|\\t|\\;|\\s");

                  if(cols.size() >= 6)
                  {
                    if(!QString::compare(cols[1], "SPATIAL", Qt::CaseInsensitive))
                    {

                      QFileInfo tsFile = getAbsoluteFilePath(cols[2]);
                      QFileInfo geomFile = getAbsoluteFilePath(cols[3]);

                      if(tsFile.exists() && geomFile.exists())
                      {
                        TimeSeries *timeSeriesObj = nullptr;

                        if((timeSeriesObj = TimeSeries::createTimeSeries(cols[0],tsFile, nullptr)))
                        {
                          TimeSeriesProvider *timeSeriesProvider = new TimeSeriesProvider(cols[0], nullptr);
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
                          double mult = cols[4].toDouble(&multOk);

                          if(multOk)
                            timeSeriesProvider->setMultiplier(mult);

                          auto it = m_geomMultiplierFlags.find(cols[5].toStdString());

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

                          if(cols.size() >= 7)
                          {
                            m_timeSeriesDesc.push_back(cols[6].toStdString());
                          }
                          else
                          {
                            m_timeSeriesDesc.push_back(cols[0].toStdString());
                          }
                        }
                        else
                        {
                          message = "Unable to read ts file: "+ tsFile.filePath();
                          return false;
                        }

                      }
                      else
                      {
                        message = "Time series file/geometry file does not exist: "+ inputFile.filePath();
                        return false;
                      }
                    }
                    else
                    {
                      message = "Timeseries type specified is incorrect: "+ cols[1];
                      return false;
                    }
                  }
                  else if(cols.size() >= 4)
                  {
                    if(!QString::compare(cols[1], "ID", Qt::CaseInsensitive))
                    {

                      QFileInfo tsFile = getAbsoluteFilePath(cols[2]);

                      if(tsFile.exists())
                      {
                        TimeSeries *timeSeriesObj = nullptr;

                        if((timeSeriesObj = TimeSeries::createTimeSeries(cols[0],tsFile, nullptr)))
                        {
                          TimeSeriesProvider *timeSeriesProvider = new TimeSeriesProvider(cols[0], nullptr);
                          timeSeriesProvider->setTimeSeries(timeSeriesObj);
                          timeSeriesProvider->setTimeSeriesType(TimeSeriesProvider::Id);

                          bool multOk = false;
                          double mult = cols[3].toDouble(&multOk);

                          if(multOk)
                          {
                            timeSeriesProvider->setMultiplier(mult);
                          }

                          m_timeSeriesProviders.push_back(timeSeriesProvider);

                          if(cols.size() >= 5)
                          {
                            m_timeSeriesDesc.push_back(cols[4].toStdString());
                          }
                          else
                          {
                            m_timeSeriesDesc.push_back(cols[0].toStdString());
                          }
                        }
                        else
                        {
                          message = "Unable to read ts file: "+ tsFile.filePath();
                          return false;
                        }
                      }
                      else
                      {
                        message = "Time series file/geometry file does not exist: "+ inputFile.filePath();
                        return false;
                      }
                    }
                    else
                    {
                      message = "Timeseries type specified is incorrect: "+ cols[1];
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

      currentDateTimeInternal()->setJulianDay(m_beginDateTime);
      timeHorizonInternal()->setJulianDay(m_beginDateTime);
      timeHorizonInternal()->setDuration(m_endDateTime - m_beginDateTime);

      m_currentDateTime = m_beginDateTime;
      initializeTimeVariables();

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

    QList<QSharedPointer<HCGeometry>> geometries = timeSeriesProvider->geometries();


    if(geometries.length())
    {
      QSharedPointer<HCGeometry> geometry = geometries[0];
      Quantity *unitless = Quantity::unitLessValues("Unitless", QVariant::Double, this);
      TimeSeriesMultiplierInput *timeSeriesMultiplierInput = new TimeSeriesMultiplierInput(timeSeriesProvider, m_geometryDimension, geometry->geometryType(), unitless, this);
      timeSeriesMultiplierInput->setCaption(timeSeriesProvider->id() + " Multiplier");
      //     timeSeriesMultiplierInput->setDescription(QString::fromStdString(m_timeSeriesDesc[i]));
      addInput(timeSeriesMultiplierInput);
    }
  }
}

void TimeSeriesProviderComponent::createOutputs()
{
  //  m_timeSeriesOutputs.clear();

  for(size_t i = 0 ; i < m_timeSeriesProviders.size(); i++)
  {
    TimeSeriesProvider *timeSeriesProvider  = m_timeSeriesProviders[i];

    if(timeSeriesProvider->timeSeriesType() == TimeSeriesProvider::Spatial)
    {
      QSharedPointer<HCGeometry> geometry = timeSeriesProvider->geometries()[0];
      Quantity *unitless = Quantity::unitLessValues("Unitless", QVariant::Double, this);
      TimeSeriesOutput *timeSeriesOutput = new TimeSeriesOutput(timeSeriesProvider,
                                                                m_geometryDimension,
                                                                geometry->geometryType(),
                                                                m_timeDimension,
                                                                unitless,
                                                                this);

      timeSeriesOutput->setCaption(QString::fromStdString(m_timeSeriesDesc[i]));
      timeSeriesOutput->setDescription(QString::fromStdString(m_timeSeriesDesc[i]));
      addOutput(timeSeriesOutput);
    }
    else
    {
      Quantity *unitless = Quantity::unitLessValues("Unitless", QVariant::Double, this);

      Dimension *identifierDimension = new Dimension("Identifiers", this);

      TimeSeriesIdBasedOutput *timeSeriesOutput = new TimeSeriesIdBasedOutput(timeSeriesProvider,
                                                                              identifierDimension,
                                                                              m_timeDimension,
                                                                              unitless,
                                                                              this);

      timeSeriesOutput->setCaption(QString::fromStdString(m_timeSeriesDesc[i]));
      timeSeriesOutput->setDescription(QString::fromStdString(m_timeSeriesDesc[i]));

      addOutput(timeSeriesOutput);
    }
  }
}

void TimeSeriesProviderComponent::initializeTimeVariables()
{
  m_stepSize = timeHorizonInternal()->duration();

  for(size_t i = 0; i < m_timeSeriesProviders.size(); i++)
  {
    TimeSeries *timeSeries = m_timeSeriesProviders[i]->timeSeries();

    for(int j = 1; j < timeSeries->numRows(); j++)
    {
      m_stepSize = std::min(m_stepSize,  timeSeries->dateTime(j) - timeSeries->dateTime(j-1));
    }
  }

  m_stepSize /= 2.0;
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
