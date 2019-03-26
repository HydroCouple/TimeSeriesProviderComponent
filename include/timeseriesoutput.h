#ifndef TIMESERIESOUTPUT_H
#define TIMESERIESOUTPUT_H

#include "hydrocoupletemporal.h"
#include "timeseriesprovidercomponent_global.h"
#include "spatiotemporal/timegeometryoutput.h"

class TimeSeriesProvider;
class Quantity;
class TimeSeriesProviderComponent;

class TIMESERIESPROVIDERCOMPONENT_EXPORT TimeSeriesOutput : public TimeGeometryOutputDouble
{
    Q_OBJECT

  public:

    TimeSeriesOutput(TimeSeriesProvider *provider,
                     Dimension *geometryDimension,
                     HydroCouple::Spatial::IGeometry::GeometryType geometryType,
                     Dimension *timeDimension,
                     Quantity *valueDefinition,
                     TimeSeriesProviderComponent *component);

    virtual ~TimeSeriesOutput();

    double currentDateTime() const;

    void updateValues(HydroCouple::IInput *querySpecifier) override;

    void updateValues() override;

    TimeSeriesProvider *timeSeriesProvider() const;

  private:
    int m_currentIndex = -1;
    double m_currentDateTime;
    TimeSeriesProvider *m_timeSeriesProvider;
    TimeSeriesProviderComponent *m_modelComponent;
};


#endif // TIMESERIESOUTPUT_H
