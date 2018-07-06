#include "stdafx.h"
#include "timeseriesprovider.h"
#include "spatial/geometry.h"

TimeSeriesProvider::TimeSeriesProvider(const QString &id, QObject *parent)
  : QObject(parent),
    m_id(id),
    m_geometryMultiplierAttribute(GeometryMultiplierAttribute::None),
    m_multiplier(1.0),
    m_timeSeries(nullptr)
{

}

TimeSeriesProvider::~TimeSeriesProvider()
{
  if(m_timeSeries)
  {
    delete m_timeSeries;
  }
}

QString TimeSeriesProvider::id() const
{
  return m_id;
}

double TimeSeriesProvider::multiplier() const
{
  return m_multiplier;
}

void TimeSeriesProvider::setMultiplier(double multiplier)
{
  m_multiplier = multiplier;
}

TimeSeriesProvider::GeometryMultiplierAttribute TimeSeriesProvider::geometryMultiplierAttribute() const
{
  return m_geometryMultiplierAttribute;
}

void TimeSeriesProvider::setGeometryMultiplierAttribute(GeometryMultiplierAttribute geometryMultiplierAttribute)
{
  m_geometryMultiplierAttribute = geometryMultiplierAttribute;
}

TimeSeries *TimeSeriesProvider::timeSeries() const
{
  return m_timeSeries;
}

void TimeSeriesProvider::setTimeSeries(TimeSeries *timeSeries)
{
  m_timeSeries = timeSeries;
}

QList<QSharedPointer<HCGeometry>> TimeSeriesProvider::geometries() const
{
  return m_geometries;
}

void TimeSeriesProvider::setGeometries(const QList<QSharedPointer<HCGeometry> > &geometries)
{
  m_geometries = geometries;
}
