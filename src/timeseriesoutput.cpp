#include "stdafx.h"
#include "timeseriesoutput.h"
#include "timeseriesprovider.h"
#include "timeseriesprovidercomponent.h"
#include "temporal/timedata.h"
#include "core/dimension.h"
#include "core/valuedefinition.h"

using namespace HydroCouple;
using namespace HydroCouple::Spatial;
using namespace SDKTemporal;

TimeSeriesOutput::TimeSeriesOutput(TimeSeriesProvider *provider,
                                   Dimension *geometryDimension,
                                   IGeometry::GeometryType geometryType,
                                   Dimension *timeDimension,
                                   Quantity *valueDefinition,
                                   TimeSeriesProviderComponent *component) :
  TimeGeometryOutputDouble(provider->id(),
                           geometryType,
                           timeDimension,
                           geometryDimension,
                           valueDefinition,
                           component),
  m_timeSeriesProvider(provider),
  m_modelComponent(component)
{
  addGeometries(provider->geometries());

  m_currentDateTime = m_modelComponent->endDateTime() + 10;

  for(int i = 0 ; i < m_timeSeriesProvider->timeSeries()->numRows() - 1 ; i++)
  {
    double dateTime1 = m_timeSeriesProvider->timeSeries()->dateTime(i);
    double dateTime2 = m_timeSeriesProvider->timeSeries()->dateTime(i + 1);

    if(m_modelComponent->startDateTime() >= dateTime1 && m_modelComponent->startDateTime() <= dateTime2)
    {
      m_currentIndex = i + 1;
      m_currentDateTime = dateTime2;

      addTime(new SDKTemporal::DateTime(dateTime1 ,this));
      addTime(new SDKTemporal::DateTime(dateTime2 ,this));

      if(geometryCount() == m_timeSeriesProvider->timeSeries()->numColumns())
      {
        if(m_timeSeriesProvider->geometryMultiplierAttribute() ==  TimeSeriesProvider::Length &&
           (this->geometryType() == IGeometry::LineString ||
            this->geometryType() == IGeometry::LineStringM ||
            this->geometryType() == IGeometry::LineStringZ ||
            this->geometryType() == IGeometry::LineStringZM))
        {
          for(int j = 0 ; j < geometryCount() ; j++)
          {
            ILineString *lineString = dynamic_cast<ILineString*>(geometry(j));

            double value1 = m_timeSeriesProvider->timeSeries()->value(i, j)     *  m_timeSeriesProvider->multiplier() * lineString->length();
            double value2 = m_timeSeriesProvider->timeSeries()->value(i + 1, j) *  m_timeSeriesProvider->multiplier() * lineString->length();

            setValue(0, j, &value1);
            setValue(1, j, &value2);
          }
        }
        else
        {
          for(int j = 0 ; j < geometryCount() ; j++)
          {
            double value1 = m_timeSeriesProvider->timeSeries()->value(i, j)     *  m_timeSeriesProvider->multiplier();
            double value2 = m_timeSeriesProvider->timeSeries()->value(i + 1, j) *  m_timeSeriesProvider->multiplier();

            setValue(0, j, &value1);
            setValue(1, j, &value2);
          }
        }
      }
      else
      {
        if(m_timeSeriesProvider->geometryMultiplierAttribute() ==  TimeSeriesProvider::Length &&
           (this->geometryType() == IGeometry::LineString ||
            this->geometryType() == IGeometry::LineStringM ||
            this->geometryType() == IGeometry::LineStringZ ||
            this->geometryType() == IGeometry::LineStringZM))
        {
          for(int j = 0 ; j < geometryCount() ; j++)
          {
            ILineString *lineString = dynamic_cast<ILineString*>(geometry(j));

            double value1 = m_timeSeriesProvider->timeSeries()->value(i)     *  m_timeSeriesProvider->multiplier() * lineString->length();
            double value2 = m_timeSeriesProvider->timeSeries()->value(i + 1) *  m_timeSeriesProvider->multiplier() * lineString->length();

            setValue(0, j, &value1);
            setValue(1, j, &value2);
          }
        }
        else
        {
          for(int j = 0 ; j < geometryCount() ; j++)
          {
            double value1 = m_timeSeriesProvider->timeSeries()->value(i)     *  m_timeSeriesProvider->multiplier();
            double value2 = m_timeSeriesProvider->timeSeries()->value(i + 1) *  m_timeSeriesProvider->multiplier();

            setValue(0, j, &value1);
            setValue(1, j, &value2);
          }
        }
      }

      break;
    }
  }
}

TimeSeriesOutput::~TimeSeriesOutput()
{

}

double TimeSeriesOutput::currentDateTime() const
{
  return m_currentDateTime;
}

void TimeSeriesOutput::updateValues(IInput *querySpecifier)
{
  if(!m_modelComponent->workflow())
  {
    ITimeComponentDataItem* timeExchangeItem = dynamic_cast<ITimeComponentDataItem*>(querySpecifier);
    QList<IOutput*>updateList;

    if(timeExchangeItem)
    {
      double queryTime = timeExchangeItem->time(timeExchangeItem->timeCount() - 1)->julianDay();

      while (m_currentDateTime < queryTime &&
             m_modelComponent->status() == IModelComponent::Updated)
      {
        m_modelComponent->update(updateList);
      }
    }
    else
    {
      if(m_modelComponent->status() == IModelComponent::Updated)
      {
        m_modelComponent->update(updateList);
      }
    }
  }

  refreshAdaptedOutputs();
}

void TimeSeriesOutput::updateValues()
{
  moveDataToPrevTime();

  m_currentIndex ++;

  if(m_currentIndex >= 0 && m_currentIndex < m_timeSeriesProvider->timeSeries()->numRows())
  {
    m_currentDateTime = m_timeSeriesProvider->timeSeries()->dateTime(m_currentIndex);

    int lastDateTimeIndex = timeCount() - 1;
    DateTime *lastDateTime = m_times[lastDateTimeIndex];
    lastDateTime->setJulianDay(lastDateTimeIndex);

    if(m_currentDateTime <= m_modelComponent->endDateTime())
    {
      if(geometryCount() == m_timeSeriesProvider->timeSeries()->numColumns())
      {
        if(m_timeSeriesProvider->geometryMultiplierAttribute() ==  TimeSeriesProvider::Length &&
           (this->geometryType() == IGeometry::LineString ||
            this->geometryType() == IGeometry::LineStringM ||
            this->geometryType() == IGeometry::LineStringZ ||
            this->geometryType() == IGeometry::LineStringZM))
        {
          for(int j = 0 ; j < geometryCount() ; j++)
          {
            ILineString *lineString = dynamic_cast<ILineString*>(geometry(j));
            double value = m_timeSeriesProvider->timeSeries()->value(m_currentIndex, j)  *  m_timeSeriesProvider->multiplier() * lineString->length();
            setValue(timeCount() - 1, j, &value);
          }
        }
        else
        {
          for(int j = 0 ; j < geometryCount() ; j++)
          {
            double value = m_timeSeriesProvider->timeSeries()->value(m_currentIndex, j)     *  m_timeSeriesProvider->multiplier();
            setValue(timeCount() - 1, j, &value);
          }
        }
      }
      else
      {
        if(m_timeSeriesProvider->geometryMultiplierAttribute() ==  TimeSeriesProvider::Length &&
           (this->geometryType() == IGeometry::LineString ||
            this->geometryType() == IGeometry::LineStringM ||
            this->geometryType() == IGeometry::LineStringZ ||
            this->geometryType() == IGeometry::LineStringZM))
        {
          for(int j = 0 ; j < geometryCount() ; j++)
          {
            ILineString *lineString = dynamic_cast<ILineString*>(geometry(j));
            double value = m_timeSeriesProvider->timeSeries()->value(m_currentIndex) * m_timeSeriesProvider->multiplier() * lineString->length();
            setValue(timeCount() - 1, j, &value);
          }
        }
        else
        {
          for(int j = 0 ; j < geometryCount() ; j++)
          {
            double value = m_timeSeriesProvider->timeSeries()->value(m_currentIndex)  *  m_timeSeriesProvider->multiplier();
            setValue(timeCount() - 1, j, &value);
          }
        }
      }
    }
  }
  else
  {
    m_currentDateTime = m_modelComponent->endDateTime() + 10;
  }
}
