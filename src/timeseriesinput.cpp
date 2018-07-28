#include "stdafx.h"
#include "timeseriesinput.h"
#include "spatial/geometryexchangeitems.h"
#include "timeseriesprovider.h"
#include "core/valuedefinition.h"
#include "timeseriesprovidercomponent.h"

using namespace HydroCouple;
using namespace HydroCouple::Spatial;


TimeSeriesMultiplierInput::TimeSeriesMultiplierInput(TimeSeriesProvider *timeSeriesProvider,
                                                     Dimension *geometryDimension,
                                                     IGeometry::GeometryType geometryType,
                                                     Quantity *valueDefinition,
                                                     TimeSeriesProviderComponent *component):
  GeometryInputDouble(timeSeriesProvider->id(), geometryType, geometryDimension, valueDefinition, component),
  m_timeSeriesProvider(timeSeriesProvider)
{
  addGeometries(timeSeriesProvider->geometries());
}

TimeSeriesMultiplierInput::~TimeSeriesMultiplierInput()
{

}

bool TimeSeriesMultiplierInput::setProvider(IOutput *provider)
{
  m_geometryMapping.clear();

  if(AbstractInput::setProvider(provider) && provider)
  {
    IGeometryComponentDataItem *geometryDataItem = dynamic_cast<IGeometryComponentDataItem*>(provider);

    if(geometryDataItem->geometryCount())
    {
      for(int i = 0; i < geometryCount() ; i++)
      {
        IGeometry *myGeometry = geometry(i);

        for(int j = 0; j < geometryDataItem->geometryCount() ; j++)
        {
          IGeometry *providerGeometry = geometryDataItem->geometry(j);

          if(equalsGeometry(myGeometry, providerGeometry))
          {
            m_geometryMapping[i] = j;
            break;
          }
        }
      }
    }

    return true;
  }

  return false;
}

bool TimeSeriesMultiplierInput::canConsume(IOutput *provider, QString &message) const
{
  IGeometryComponentDataItem *geometryDataItem = nullptr;

  if((geometryDataItem = dynamic_cast<IGeometryComponentDataItem*>(provider)) &&
     (geometryDataItem->geometryType() == IGeometry::LineString ||
      geometryDataItem->geometryType() == IGeometry::LineStringZ ||
      geometryDataItem->geometryType() == IGeometry::LineStringZM )
     &&
     (provider->valueDefinition()->type() == QVariant::Double ||
      provider->valueDefinition()->type() == QVariant::Int))
  {
    return true;
  }

  message = "Provider must be a LineString";
  return false;
}

void TimeSeriesMultiplierInput::retrieveValuesFromProvider()
{
  provider()->updateValues(this);
}

void TimeSeriesMultiplierInput::applyData()
{
  IGeometryComponentDataItem *geometryDataItem = nullptr;

  if((geometryDataItem = dynamic_cast<IGeometryComponentDataItem*>(provider())))
  {
    for(auto it : m_geometryMapping)
    {
      double value = 0;
      geometryDataItem->getValue(it.second, & value);
      setValue( it.first, &value);

      if(value)
      {
        m_timeSeriesProvider->setMultiplier(value);
      }
    }
  }
}

bool TimeSeriesMultiplierInput::equalsGeometry(IGeometry *geom1, IGeometry *geom2, double epsilon)
{
  if(geom1->geometryType() == geom2->geometryType())
  {
    switch (geom1->geometryType())
    {
      case IGeometry::LineString:
      case IGeometry::LineStringM:
      case IGeometry::LineStringZ:
      case IGeometry::LineStringZM:
        {
          ILineString *lineString1 = dynamic_cast<ILineString*>(geom1);
          ILineString *lineString2 = dynamic_cast<ILineString*>(geom2);

          if(lineString1 && lineString2 && lineString1->pointCount() == lineString2->pointCount())
          {
            for(int i = 0 ; i < lineString1->pointCount(); i++)
            {
              IPoint *p1 = lineString1->point(i);
              IPoint *p2 = lineString2->point(i);

              double dx = p1->x() - p2->x();
              double dy = p1->y() - p2->y();

              double dist = sqrt(dx * dx + dy * dy);

              if(dist < epsilon)
              {
                return true;
              }
            }
          }
        }
        break;
      default:
        {
          return geom1->equals(geom2);
        }
        break;
    }
  }

  return false;
}
