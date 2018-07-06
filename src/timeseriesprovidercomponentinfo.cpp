#include "stdafx.h"
#include "timeseriesprovidercomponentinfo.h"
#include "spatial/geometryfactory.h"
#include "timeseriesprovidercomponent.h"

using namespace HydroCouple;

TimeSeriesProviderComponentInfo::TimeSeriesProviderComponentInfo(QObject *parent)
  :AbstractModelComponentInfo(parent)
{
  GeometryFactory::registerGDAL();

  setId("A TimeSeries Provider Component 1.0.0");
  setCaption("TimeSeries Provider Component");
  setIconFilePath("./../../resources/images/hydrocouplecomposer.png");
  setDescription("A one-dimensional channel heat and solute transport model.");
  setCategory("Data\\Spatiotemporal");
  setCopyright("");
  setVendor("");
  setUrl("www.hydrocouple.org");
  setEmail("caleb.buahin@gmail.com");
  setVersion("1.0.0");

  QStringList documentation;
  documentation << "Several sources";
  setDocumentation(documentation);
}

TimeSeriesProviderComponentInfo::~TimeSeriesProviderComponentInfo()
{
}

IModelComponent *TimeSeriesProviderComponentInfo::createComponentInstance()
{
  QString id =  QUuid::createUuid().toString();
  TimeSeriesProviderComponent *component = new TimeSeriesProviderComponent(id, this);
  component->setDescription("Calibration Model Instance");
  return component;
}
