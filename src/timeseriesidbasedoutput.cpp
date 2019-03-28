#include "stdafx.h"
#include "timeseriesidbasedoutput.h"
#include "timeseriesprovider.h"
#include "core/dimension.h"
#include "temporal/timedata.h"
#include "core/valuedefinition.h"
#include "timeseriesprovidercomponent.h"

using namespace HydroCouple;
using namespace SDKTemporal;

TimeSeriesIdBasedOutput::TimeSeriesIdBasedOutput(TimeSeriesProvider *provider,
                                                 Dimension *identifierDimension,
                                                 Dimension *timeDimension,
                                                 Quantity *valueDefinition,
                                                 TimeSeriesProviderComponent *component):
  TimeSeriesIdBasedOutputDouble(provider->id(),
                                identifierDimension,
                                timeDimension,
                                valueDefinition,
                                component),
  m_timeSeriesProvider(provider),
  m_modelComponent(component)
{
  int numRows = m_timeSeriesProvider->timeSeries()->numRows();

  DateTime *dt1 = new DateTime(0, this);
  addTime(dt1);

  DateTime *dt2 = new DateTime(0.1, this);
  addTime(dt2);

  if(numRows > 0)
  {
    m_currentDateTime = m_timeSeriesProvider->timeSeries()->dateTime(0);

    timeInternal(0)->setJulianDay(m_currentDateTime - 0.000000000001);
    timeInternal(1)->setJulianDay(m_currentDateTime);
  }

  QStringList columnNames;

  for(int i = 0; i < m_timeSeriesProvider->timeSeries()->numColumns(); i++)
  {
    columnNames.push_back(m_timeSeriesProvider->timeSeries()->getColumnName(i).trimmed());
  }

  addIdentifiers(columnNames);
}

TimeSeriesIdBasedOutput::~TimeSeriesIdBasedOutput()
{
}

double TimeSeriesIdBasedOutput::currentDateTime() const
{
  return m_currentDateTime;
}

void TimeSeriesIdBasedOutput::updateValues(IInput *querySpecifier)
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

void TimeSeriesIdBasedOutput::updateValues()
{
  int lastDateTimeIndex = timeCount() - 1;
  DateTime *lastDateTime = timeInternal(lastDateTimeIndex);

  if(lastDateTime->julianDay() < m_modelComponent->nextDateTime())
  {
    moveDataToPrevTime();

    m_currentIndex = m_timeSeriesProvider->timeSeries()->findDateTimeIndex(m_modelComponent->nextDateTime(), m_currentIndex) + 1;

    if(m_currentIndex >= 0 && m_currentIndex < m_timeSeriesProvider->timeSeries()->numRows())
    {
      m_currentDateTime = m_timeSeriesProvider->timeSeries()->dateTime(m_currentIndex);

      DateTime *lastDateTime = timeInternal(lastDateTimeIndex);
      lastDateTime->setJulianDay(m_currentDateTime);

      resetTimeSpan();

      if(m_currentDateTime <= m_modelComponent->endDateTime())
      {
        for(int j = 0 ; j < m_timeSeriesProvider->timeSeries()->numColumns() ; j++)
        {
          double value = m_timeSeriesProvider->timeSeries()->value(m_currentIndex, j) * m_timeSeriesProvider->multiplier();
          setValue(timeCount() - 1, j, &value);
        }
      }
    }
  }
}

TimeSeriesProvider *TimeSeriesIdBasedOutput::timeSeriesProvider() const
{
  return m_timeSeriesProvider;
}
