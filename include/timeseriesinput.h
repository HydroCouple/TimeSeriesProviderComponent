#ifndef TIMESERIESINPUT_H
#define TIMESERIESINPUT_H

#include "timeseriesprovidercomponent_global.h"
#include "spatial/geometryexchangeitems.h"

#include <unordered_map>

class Quantity;
class TimeSeriesProviderComponent;
class TimeSeriesProvider;

class TIMESERIESPROVIDERCOMPONENT_EXPORT TimeSeriesMultiplierInput: public GeometryInputDouble
{
    Q_OBJECT

  public:

    TimeSeriesMultiplierInput(TimeSeriesProvider *timeSeriesProvider,
                              Dimension *geometryDimension,
                              HydroCouple::Spatial::IGeometry::GeometryType geometryType,
                              Quantity *valueDefinition,
                              TimeSeriesProviderComponent *component);

    virtual ~TimeSeriesMultiplierInput() override;

    bool setProvider(HydroCouple::IOutput *provider) override;

    bool canConsume(HydroCouple::IOutput *provider, QString &message) const override;

    void retrieveValuesFromProvider() override;

    void applyData() override;

  private:

    static bool equalsGeometry(HydroCouple::Spatial::IGeometry *geom1, HydroCouple::Spatial::IGeometry *geom2, double epsilon = 0.00001);

  private:

    std::unordered_map<int,int> m_geometryMapping;
    TimeSeriesProvider *m_timeSeriesProvider;
};

#endif // TIMESERIESINPUT_H
