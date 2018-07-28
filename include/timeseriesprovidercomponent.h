#ifndef TIMESERIESPROVIDERCOMPONENT_H
#define TIMESERIESPROVIDERCOMPONENT_H

#include "timeseriesprovidercomponentinfo.h"
#include "temporal/abstracttimemodelcomponent.h"

#include <unordered_map>

class TimeSeriesProvider;
class Dimension;
class TimeSeriesOutput;

class TIMESERIESPROVIDERCOMPONENT_EXPORT TimeSeriesProviderComponent : public AbstractTimeModelComponent,
    public virtual HydroCouple::ICloneableModelComponent
{
    Q_OBJECT

    Q_INTERFACES(HydroCouple::ICloneableModelComponent)

  public:

    TimeSeriesProviderComponent(const QString &id, TimeSeriesProviderComponentInfo *modelComponentInfo = nullptr);

    virtual ~TimeSeriesProviderComponent();

    QList<QString> validate() override;

    void prepare() override;

    void update(const QList<HydroCouple::IOutput*> &requiredOutputs = QList<HydroCouple::IOutput*>()) override;

    void finish() override;

    HydroCouple::ICloneableModelComponent* parent() const override;

    HydroCouple::ICloneableModelComponent* clone() override;

    QList<HydroCouple::ICloneableModelComponent*> clones() const override;

    double startDateTime() const;

    double endDateTime() const;

  protected:

    bool removeClone(TimeSeriesProviderComponent *component);

    void initializeFailureCleanUp() override;

  private:

    void createArguments() override;

    void createInputFileArguments();

    bool initializeArguments(QString &message) override;

    bool initializeInputFilesArguments(QString &message);

    void createInputs() override;

    void createOutputs() override;

    double getMinDateTime();

  private:

    Dimension *m_timeDimension,
              *m_geometryDimension;

    IdBasedArgumentString *m_inputFilesArgument;

    std::vector<TimeSeriesProvider*> m_timeSeriesProviders;
    std::vector<TimeSeriesOutput*> m_timeSeriesOutputs;

    double m_beginDateTime,
           m_endDateTime;

    TimeSeriesProviderComponent *m_parent;
    QList<HydroCouple::ICloneableModelComponent*> m_clones;

    static const std::unordered_map<std::string,int> m_inputFileFlags;
    static const std::unordered_map<std::string,int> m_optionsFlags;
    static const std::unordered_map<std::string,int> m_geomMultiplierFlags;

};

#endif // TIMESERIESPROVIDERCOMPONENT_H
