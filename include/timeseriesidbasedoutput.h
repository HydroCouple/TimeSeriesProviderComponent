#ifndef TIMESERIESIDBASEDOUTPUT_H
#define TIMESERIESIDBASEDOUTPUT_H

#include "hydrocoupletemporal.h"
#include "timeseriesprovidercomponent_global.h"
#include "temporal/timeseriesidbasedexchangeitem.h"
#include "spatiotemporal/timegeometryoutput.h"

class TimeSeriesProvider;
class Quantity;
class TimeSeriesProviderComponent;
class Dimension;

namespace SDKTemporal {
  class DateTime;
  class TimeSpan;
}

class TIMESERIESPROVIDERCOMPONENT_EXPORT TimeSeriesIdBasedOutput: public TimeSeriesIdBasedOutputDouble
{
    Q_OBJECT

  public:

    TimeSeriesIdBasedOutput(TimeSeriesProvider *provider,
                            Dimension* identifierDimension,
                            Dimension *timeDimension,
                            Quantity *valueDefinition,
                            TimeSeriesProviderComponent *component);

    virtual ~TimeSeriesIdBasedOutput();

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


#endif // TIMESERIESIDBASEDOUTPUT_H
