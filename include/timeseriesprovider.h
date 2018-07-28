#ifndef TIMESERIESPROVIDER_H
#define TIMESERIESPROVIDER_H

#include "timeseriesprovidercomponent_global.h"
#include "timeseries.h"

class HCGeometry;

class TIMESERIESPROVIDERCOMPONENT_EXPORT TimeSeriesProvider : public QObject
{
    Q_OBJECT

  public:

    enum GeometryMultiplierAttribute
    {
      None,
      Length,
      Area,
    };

    TimeSeriesProvider(const QString &id, QObject *parent);

    virtual ~TimeSeriesProvider();

    QString id() const;

    double multiplier() const;

    void setMultiplier(double multiplier);

    GeometryMultiplierAttribute geometryMultiplierAttribute() const;

    void setGeometryMultiplierAttribute(GeometryMultiplierAttribute geometryMultiplierAttribute);

    TimeSeries *timeSeries() const;

    void setTimeSeries(TimeSeries *timeSeries);

    QList<QSharedPointer<HCGeometry>> geometries() const;

    void setGeometries(const QList<QSharedPointer<HCGeometry>> &geometries);

  private:
    QString m_id;
    GeometryMultiplierAttribute m_geometryMultiplierAttribute;
    QList<QSharedPointer<HCGeometry>> m_geometries;
    double m_multiplier;
    TimeSeries *m_timeSeries;

};

#endif // TIMESERIESPROVIDER_H
