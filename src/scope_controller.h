#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QVariantList>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

#include "core/iscope_data_api.h"
#include "core/tools_interfaces.h"

namespace c41scope {

class IScopeDataStore;

class ScopeController : public QObject, public IScopeDataAPI {
    Q_OBJECT
    Q_PROPERTY(int dataMode READ dataModeValue WRITE setDataModeValue NOTIFY dataModeChanged)
    Q_PROPERTY(QVariantList viewToolItems READ viewToolItems NOTIFY viewToolsChanged)
    Q_PROPERTY(QObject* viewToolTarget READ viewToolTarget WRITE setViewToolTarget NOTIFY viewToolTargetChanged)
    Q_PROPERTY(QVariantList commandToolItems READ commandToolItems NOTIFY commandToolsChanged)
    Q_PROPERTY(QVariantList channelIds READ channelIds NOTIFY channelIdsChanged)

public:
    explicit ScopeController(QObject* parent = nullptr);
    ~ScopeController() override = default;

    void setDataMode(DataMode mode);
    DataMode dataMode() const;

    int dataModeValue() const;
    void setDataModeValue(int mode);

    void pushSamples(int channelId,
                     const float* data,
                     std::size_t count,
                     double t0,
                     double dt);

    void pushFrame(int channelId,
                   const float* data,
                   std::size_t count,
                   double t0,
                   double dt);

    ScopeSnapshot snapshot(const SnapshotRequest& req) override;
    ToolContext context() const override;

    bool consumeDataDirty();
    std::uint64_t dataEpoch() const;
    bool hasData() const;
    double earliestTime() const;
    double latestTime() const;

    void clear();

    void registerViewTool(const std::shared_ptr<IViewTool>& tool);
    void registerInteractiveTool(const std::shared_ptr<IInteractiveTool>& tool);
    void registerCommandTool(const std::shared_ptr<ICommandTool>& tool);

    std::vector<std::shared_ptr<IViewTool>> viewTools() const;
    std::vector<std::shared_ptr<IInteractiveTool>> interactiveTools() const;
    std::vector<std::shared_ptr<ICommandTool>> commandTools() const;

    QVariantList viewToolItems() const;
    QVariantList commandToolItems() const;
    QVariantList channelIds() const;

    QObject* viewToolTarget() const;
    void setViewToolTarget(QObject* target);

    Q_INVOKABLE bool triggerViewTool(const QString& toolId);
    Q_INVOKABLE bool triggerCommandTool(const QString& toolId);

signals:
    void dataModeChanged();
    void viewToolsChanged();
    void commandToolsChanged();
    void viewToolTargetChanged();
    void channelIdsChanged();
    void splitToolRequested();
    void commandToolResultReady(const QString& title, const QString& body);

private:
    std::shared_ptr<IScopeDataStore> currentStore() const;
    void updateTimeRange(double t0, std::size_t count, double dt);
    void resetTimeRange();
    bool registerChannelId(int channelId);
    void clearChannelIds();

    mutable std::mutex m_storeMutex;
    std::shared_ptr<IScopeDataStore> m_store;

    mutable std::mutex m_toolMutex;
    std::vector<std::shared_ptr<IViewTool>> m_viewTools;
    std::vector<std::shared_ptr<IInteractiveTool>> m_interactiveTools;
    std::vector<std::shared_ptr<ICommandTool>> m_commandTools;
    QPointer<QObject> m_viewToolTarget;
    mutable std::mutex m_channelMutex;
    std::vector<int> m_channelIds;

    std::atomic<DataMode> m_dataMode;
    std::atomic<bool> m_dataDirty;
    std::atomic<std::uint64_t> m_dataEpoch;
    std::atomic<bool> m_hasData;
    std::atomic<double> m_tMin;
    std::atomic<double> m_tMax;
};

} // namespace c41scope
