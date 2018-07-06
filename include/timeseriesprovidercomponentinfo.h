#ifndef TIMESERIESPROVIDERCOMPONENTINFO_H
#define TIMESERIESPROVIDERCOMPONENTINFO_H

#include "timeseriesprovidercomponent_global.h"
#include "core/abstractmodelcomponentinfo.h"

class TIMESERIESPROVIDERCOMPONENT_EXPORT TimeSeriesProviderComponentInfo  : public AbstractModelComponentInfo
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "TimeSeriesProviderComponentInfo")

  public:

    TimeSeriesProviderComponentInfo(QObject *parent = nullptr);

    virtual ~TimeSeriesProviderComponentInfo();

    HydroCouple::IModelComponent* createComponentInstance() override;
};

#endif // TIMESERIESPROVIDERCOMPONENTINFO_H
