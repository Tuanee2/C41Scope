#include "scope_controller.h"

#include <QMetaObject>
#include <QVariantMap>

#include <algorithm>

#include "store/frame_store.h"
#include "store/history_store.h"
#include "tools/command/stats_command_tool.h"
#include "tools/view/callback_view_tool.h"

namespace c41scope {

ScopeController::ScopeController(QObject* parent)
    : QObject(parent)
    , m_store(std::make_shared<HistoryStore>())
    , m_dataMode(DataMode::History)
    , m_dataDirty(false)
    , m_dataEpoch(0)
    , m_hasData(false)
    , m_tMin(0.0)
    , m_tMax(0.0)
{
    auto invokeOnTarget = [this](const char* methodName) {
        QObject* target = m_viewToolTarget.data();
        if (target == nullptr) {
            return;
        }

        QMetaObject::invokeMethod(target, methodName, Qt::DirectConnection);
    };

    registerViewTool(std::make_shared<CallbackViewTool>("zoom_in", "Zoom +", [invokeOnTarget]() {
        invokeOnTarget("zoomIn");
    }));
    registerViewTool(std::make_shared<CallbackViewTool>("zoom_out", "Zoom -", [invokeOnTarget]() {
        invokeOnTarget("zoomOut");
    }));
    registerViewTool(std::make_shared<CallbackViewTool>("fit", "Fit", [invokeOnTarget]() {
        invokeOnTarget("fitView");
    }));
    registerViewTool(std::make_shared<CallbackViewTool>("pan", "Pan", [invokeOnTarget]() {
        invokeOnTarget("pan");
    }));
    registerViewTool(std::make_shared<CallbackViewTool>("split", "Split", [this]() {
        emit splitToolRequested();
    }));

    registerCommandTool(std::make_shared<StatsCommandTool>([this](const QString& title, const QString& body) {
        emit commandToolResultReady(title, body);
    }));
}

void ScopeController::setDataMode(DataMode mode)
{
    if (mode == m_dataMode.load(std::memory_order_acquire)) {
        return;
    }

    std::shared_ptr<IScopeDataStore> newStore;
    if (mode == DataMode::History) {
        newStore = std::make_shared<HistoryStore>();
    } else {
        newStore = std::make_shared<FrameStore>();
    }

    {
        std::lock_guard<std::mutex> lock(m_storeMutex);
        m_store = std::move(newStore);
    }

    m_dataMode.store(mode, std::memory_order_release);
    resetTimeRange();
    clearChannelIds();
    m_dataDirty.store(true, std::memory_order_release);
    m_dataEpoch.fetch_add(1, std::memory_order_acq_rel);
    emit dataModeChanged();
}

DataMode ScopeController::dataMode() const
{
    return m_dataMode.load(std::memory_order_acquire);
}

int ScopeController::dataModeValue() const
{
    return static_cast<int>(dataMode());
}

void ScopeController::setDataModeValue(int mode)
{
    const DataMode normalized = (mode == static_cast<int>(DataMode::Frame))
        ? DataMode::Frame
        : DataMode::History;

    setDataMode(normalized);
}

void ScopeController::pushSamples(int channelId,
                                  const float* data,
                                  std::size_t count,
                                  double t0,
                                  double dt)
{
    std::shared_ptr<IScopeDataStore> store = currentStore();
    if (!store) {
        return;
    }

    store->pushSamples(channelId, data, count, t0, dt);
    updateTimeRange(t0, count, dt);
    registerChannelId(channelId);
    m_dataDirty.store(true, std::memory_order_release);
    m_dataEpoch.fetch_add(1, std::memory_order_acq_rel);
}

void ScopeController::pushFrame(int channelId,
                                const float* data,
                                std::size_t count,
                                double t0,
                                double dt)
{
    std::shared_ptr<IScopeDataStore> store = currentStore();
    if (!store) {
        return;
    }

    store->pushFrame(channelId, data, count, t0, dt);
    updateTimeRange(t0, count, dt);
    registerChannelId(channelId);
    m_dataDirty.store(true, std::memory_order_release);
    m_dataEpoch.fetch_add(1, std::memory_order_acq_rel);
}

ScopeSnapshot ScopeController::snapshot(const SnapshotRequest& req)
{
    std::shared_ptr<IScopeDataStore> store = currentStore();
    if (!store) {
        return ScopeSnapshot{};
    }

    return store->buildSnapshot(req);
}

ToolContext ScopeController::context() const
{
    ToolContext ctx;
    ctx.dataMode = m_dataMode.load(std::memory_order_acquire);
    ctx.hasSelection = false;
    ctx.allChannels = true;

    if (m_hasData.load(std::memory_order_acquire)) {
        ctx.tMax = m_tMax.load(std::memory_order_acquire);

        double timeWindow = ctx.tMax - m_tMin.load(std::memory_order_acquire);
        QObject* activeTarget = m_viewToolTarget.data();
        if (activeTarget != nullptr) {
            bool ok = false;
            const double targetWindow = activeTarget->property("timeWindow").toDouble(&ok);
            if (ok) {
                timeWindow = targetWindow;
            }

            const QVariantList channelList = activeTarget->property("channelIds").toList();
            if (!channelList.empty()) {
                ctx.allChannels = false;
                ctx.channelIds.reserve(static_cast<std::size_t>(channelList.size()));
                for (const QVariant& value : channelList) {
                    bool channelOk = false;
                    const int channelId = value.toInt(&channelOk);
                    if (!channelOk) {
                        continue;
                    }

                    if (std::find(ctx.channelIds.begin(), ctx.channelIds.end(), channelId) == ctx.channelIds.end()) {
                        ctx.channelIds.push_back(channelId);
                    }
                }
            }
        }

        ctx.tMin = std::max(0.0, ctx.tMax - std::max(0.05, timeWindow));
    }

    return ctx;
}

bool ScopeController::consumeDataDirty()
{
    return m_dataDirty.exchange(false, std::memory_order_acq_rel);
}

std::uint64_t ScopeController::dataEpoch() const
{
    return m_dataEpoch.load(std::memory_order_acquire);
}

bool ScopeController::hasData() const
{
    return m_hasData.load(std::memory_order_acquire);
}

double ScopeController::earliestTime() const
{
    return m_tMin.load(std::memory_order_acquire);
}

double ScopeController::latestTime() const
{
    return m_tMax.load(std::memory_order_acquire);
}

void ScopeController::clear()
{
    std::shared_ptr<IScopeDataStore> store = currentStore();
    if (store) {
        store->clear();
    }

    resetTimeRange();
    clearChannelIds();
    m_dataDirty.store(true, std::memory_order_release);
    m_dataEpoch.fetch_add(1, std::memory_order_acq_rel);
}

void ScopeController::registerViewTool(const std::shared_ptr<IViewTool>& tool)
{
    if (!tool) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_toolMutex);
        m_viewTools.push_back(tool);
    }

    emit viewToolsChanged();
}

void ScopeController::registerInteractiveTool(const std::shared_ptr<IInteractiveTool>& tool)
{
    if (!tool) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_toolMutex);
    m_interactiveTools.push_back(tool);
}

void ScopeController::registerCommandTool(const std::shared_ptr<ICommandTool>& tool)
{
    if (!tool) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_toolMutex);
        m_commandTools.push_back(tool);
    }

    emit commandToolsChanged();
}

std::vector<std::shared_ptr<IViewTool>> ScopeController::viewTools() const
{
    std::lock_guard<std::mutex> lock(m_toolMutex);
    return m_viewTools;
}

std::vector<std::shared_ptr<IInteractiveTool>> ScopeController::interactiveTools() const
{
    std::lock_guard<std::mutex> lock(m_toolMutex);
    return m_interactiveTools;
}

std::vector<std::shared_ptr<ICommandTool>> ScopeController::commandTools() const
{
    std::lock_guard<std::mutex> lock(m_toolMutex);
    return m_commandTools;
}

QVariantList ScopeController::viewToolItems() const
{
    QVariantList items;

    std::lock_guard<std::mutex> lock(m_toolMutex);
    items.reserve(static_cast<int>(m_viewTools.size()));
    for (const std::shared_ptr<IViewTool>& tool : m_viewTools) {
        if (!tool) {
            continue;
        }

        QVariantMap item;
        item.insert("toolId", tool->id());
        item.insert("label", tool->name());
        items.push_back(item);
    }

    return items;
}

QVariantList ScopeController::commandToolItems() const
{
    QVariantList items;
    const ToolContext ctx = context();

    std::lock_guard<std::mutex> lock(m_toolMutex);
    items.reserve(static_cast<int>(m_commandTools.size()));
    for (const std::shared_ptr<ICommandTool>& tool : m_commandTools) {
        if (!tool || !tool->isApplicable(ctx)) {
            continue;
        }

        QVariantMap item;
        item.insert("toolId", tool->id());
        item.insert("label", tool->name());
        item.insert("category", tool->category());
        items.push_back(item);
    }

    return items;
}

QVariantList ScopeController::channelIds() const
{
    QVariantList ids;
    std::lock_guard<std::mutex> lock(m_channelMutex);
    ids.reserve(static_cast<int>(m_channelIds.size()));
    for (const int channelId : m_channelIds) {
        ids.push_back(channelId);
    }

    return ids;
}

QObject* ScopeController::viewToolTarget() const
{
    return m_viewToolTarget.data();
}

void ScopeController::setViewToolTarget(QObject* target)
{
    if (m_viewToolTarget.data() == target) {
        return;
    }

    m_viewToolTarget = target;
    emit viewToolTargetChanged();
}

bool ScopeController::triggerViewTool(const QString& toolId)
{
    std::shared_ptr<IViewTool> selectedTool;
    {
        std::lock_guard<std::mutex> lock(m_toolMutex);
        for (const std::shared_ptr<IViewTool>& tool : m_viewTools) {
            if (tool && tool->id() == toolId) {
                selectedTool = tool;
                break;
            }
        }
    }

    if (!selectedTool) {
        return false;
    }

    selectedTool->trigger();
    return true;
}

bool ScopeController::triggerCommandTool(const QString& toolId)
{
    std::shared_ptr<ICommandTool> selectedTool;
    const ToolContext ctx = context();
    {
        std::lock_guard<std::mutex> lock(m_toolMutex);
        for (const std::shared_ptr<ICommandTool>& tool : m_commandTools) {
            if (tool && tool->id() == toolId && tool->isApplicable(ctx)) {
                selectedTool = tool;
                break;
            }
        }
    }

    if (!selectedTool) {
        return false;
    }

    selectedTool->run(this);
    return true;
}

bool ScopeController::registerChannelId(int channelId)
{
    bool inserted = false;
    {
        std::lock_guard<std::mutex> lock(m_channelMutex);
        const auto it = std::find(m_channelIds.begin(), m_channelIds.end(), channelId);
        if (it == m_channelIds.end()) {
            m_channelIds.push_back(channelId);
            std::sort(m_channelIds.begin(), m_channelIds.end());
            inserted = true;
        }
    }

    if (inserted) {
        QMetaObject::invokeMethod(this, [this]() {
            emit channelIdsChanged();
        }, Qt::QueuedConnection);
    }

    return inserted;
}

void ScopeController::clearChannelIds()
{
    bool changed = false;
    {
        std::lock_guard<std::mutex> lock(m_channelMutex);
        if (!m_channelIds.empty()) {
            m_channelIds.clear();
            changed = true;
        }
    }

    if (changed) {
        emit channelIdsChanged();
    }
}

std::shared_ptr<IScopeDataStore> ScopeController::currentStore() const
{
    std::lock_guard<std::mutex> lock(m_storeMutex);
    return m_store;
}

void ScopeController::updateTimeRange(double t0, std::size_t count, double dt)
{
    if (count == 0 || dt <= 0.0) {
        return;
    }

    const double tMaxSample = t0 + dt * static_cast<double>(count - 1);
    const bool hadData = m_hasData.exchange(true, std::memory_order_acq_rel);

    if (!hadData) {
        m_tMin.store(t0, std::memory_order_release);
        m_tMax.store(tMaxSample, std::memory_order_release);
        return;
    }

    double currentMin = m_tMin.load(std::memory_order_acquire);
    while (t0 < currentMin
           && !m_tMin.compare_exchange_weak(currentMin, t0, std::memory_order_acq_rel)) {
    }

    double currentMax = m_tMax.load(std::memory_order_acquire);
    while (tMaxSample > currentMax
           && !m_tMax.compare_exchange_weak(currentMax, tMaxSample, std::memory_order_acq_rel)) {
    }
}

void ScopeController::resetTimeRange()
{
    m_hasData.store(false, std::memory_order_release);
    m_tMin.store(0.0, std::memory_order_release);
    m_tMax.store(0.0, std::memory_order_release);
}

} // namespace c41scope
