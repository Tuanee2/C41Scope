#include "scope_view.h"

#include <QColor>
#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "scope_controller.h"

namespace c41scope {

namespace {

struct ChannelNodeBundle {
    int channelId = -1;
    QSGGeometryNode* node = nullptr;
    QSGGeometry* geometry = nullptr;
    QSGFlatColorMaterial* material = nullptr;
    int capacity = 0;
};

struct ScopeRootNode : public QSGNode {
    std::vector<ChannelNodeBundle> channels;
};

constexpr std::array<QColor, 6> kChannelPalette = {
    QColor(0x0f, 0x9d, 0x58),
    QColor(0xdb, 0x44, 0x37),
    QColor(0x42, 0x85, 0xf4),
    QColor(0xf4, 0xb4, 0x00),
    QColor(0x00, 0xac, 0xc1),
    QColor(0x8e, 0x24, 0xaa),
};

constexpr double kZoomInFactor = 0.8;
constexpr double kZoomOutFactor = 1.25;
constexpr double kMaxTimeWindow = 3600.0;
constexpr double kDefaultTimeWindow = 5.0;

} // namespace

ScopeView::ScopeView(QQuickItem* parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);

    m_frameTimer.setInterval(16);
    m_frameTimer.setTimerType(Qt::PreciseTimer);
    QObject::connect(&m_frameTimer, &QTimer::timeout, this, [this]() {
        onFrameTick();
    });
    m_frameTimer.start();
}

ScopeView::~ScopeView()
{
    if (m_controllerDestroyedConnection) {
        QObject::disconnect(m_controllerDestroyedConnection);
    }
}

QObject* ScopeView::controller() const
{
    return m_controller;
}

void ScopeView::setController(QObject* ctrl)
{
    ScopeController* newController = qobject_cast<ScopeController*>(ctrl);
    if (ctrl != nullptr && newController == nullptr) {
        return;
    }

    if (m_controller == newController) {
        return;
    }

    if (m_controllerDestroyedConnection) {
        QObject::disconnect(m_controllerDestroyedConnection);
    }

    m_controller = newController;
    m_lastDataEpoch = (m_controller != nullptr) ? m_controller->dataEpoch() : 0;

    if (m_controller != nullptr) {
        m_controllerDestroyedConnection = QObject::connect(
            m_controller,
            &QObject::destroyed,
            this,
            [this]() {
                m_controller = nullptr;
                markViewDirty();
                emit controllerChanged();
            });
    }

    markViewDirty();
    emit controllerChanged();
}

double ScopeView::timeWindow() const
{
    return m_timeWindow;
}

void ScopeView::setTimeWindow(double seconds)
{
    const double normalized = std::max(0.05, seconds);
    if (std::abs(normalized - m_timeWindow) < 1e-9) {
        return;
    }

    m_timeWindow = normalized;
    markViewDirty();
    emit timeWindowChanged();
}

bool ScopeView::paused() const
{
    return m_paused;
}

void ScopeView::setPaused(bool paused)
{
    if (m_paused == paused) {
        return;
    }

    m_paused = paused;
    if (m_controller != nullptr) {
        m_frozenTMax = m_controller->latestTime();
    }

    markViewDirty();
    emit pausedChanged();
}

bool ScopeView::liveMode() const
{
    return m_liveMode;
}

void ScopeView::setLiveMode(bool enabled)
{
    if (m_liveMode == enabled) {
        return;
    }

    m_liveMode = enabled;
    if (m_controller != nullptr) {
        m_frozenTMax = m_controller->latestTime();
    }

    markViewDirty();
    emit liveModeChanged();
}

bool ScopeView::showGrid() const
{
    return m_showGrid;
}

void ScopeView::setShowGrid(bool enabled)
{
    if (m_showGrid == enabled) {
        return;
    }

    m_showGrid = enabled;
    markViewDirty();
    emit showGridChanged();
}

QVariantList ScopeView::channelIds() const
{
    return m_channelIds;
}

void ScopeView::setChannelIds(const QVariantList& channelIds)
{
    std::vector<int> normalizedIds;
    normalizedIds.reserve(static_cast<std::size_t>(channelIds.size()));
    for (const QVariant& value : channelIds) {
        bool ok = false;
        const int channelId = value.toInt(&ok);
        if (!ok) {
            continue;
        }

        if (std::find(normalizedIds.begin(), normalizedIds.end(), channelId) == normalizedIds.end()) {
            normalizedIds.push_back(channelId);
        }
    }

    std::sort(normalizedIds.begin(), normalizedIds.end());

    const bool sameIds = (normalizedIds == m_channelIdValues);
    if (sameIds && m_hasChannelFilter) {
        return;
    }

    m_channelIds.clear();
    m_channelIds.reserve(static_cast<qsizetype>(normalizedIds.size()));
    for (const int channelId : normalizedIds) {
        m_channelIds.push_back(channelId);
    }
    m_channelIdValues = std::move(normalizedIds);
    m_hasChannelFilter = true;

    std::vector<int> filteredHiddenIds;
    filteredHiddenIds.reserve(m_hiddenChannelIdValues.size());
    for (const int hiddenId : m_hiddenChannelIdValues) {
        if (std::find(m_channelIdValues.begin(), m_channelIdValues.end(), hiddenId) != m_channelIdValues.end()) {
            filteredHiddenIds.push_back(hiddenId);
        }
    }

    if (filteredHiddenIds != m_hiddenChannelIdValues) {
        m_hiddenChannelIdValues = std::move(filteredHiddenIds);
        m_hiddenChannelIds.clear();
        m_hiddenChannelIds.reserve(static_cast<qsizetype>(m_hiddenChannelIdValues.size()));
        for (const int hiddenId : m_hiddenChannelIdValues) {
            m_hiddenChannelIds.push_back(hiddenId);
        }
        emit hiddenChannelIdsChanged();
    }

    markViewDirty();
    emit channelIdsChanged();
}

QVariantList ScopeView::hiddenChannelIds() const
{
    return m_hiddenChannelIds;
}

void ScopeView::setHiddenChannelIds(const QVariantList& channelIds)
{
    std::vector<int> normalizedIds;
    normalizedIds.reserve(static_cast<std::size_t>(channelIds.size()));
    for (const QVariant& value : channelIds) {
        bool ok = false;
        const int channelId = value.toInt(&ok);
        if (!ok) {
            continue;
        }

        if (std::find(normalizedIds.begin(), normalizedIds.end(), channelId) == normalizedIds.end()) {
            normalizedIds.push_back(channelId);
        }
    }

    std::sort(normalizedIds.begin(), normalizedIds.end());

    if (m_hasChannelFilter) {
        normalizedIds.erase(
            std::remove_if(
                normalizedIds.begin(),
                normalizedIds.end(),
                [this](int channelId) {
                    return std::find(m_channelIdValues.begin(), m_channelIdValues.end(), channelId) == m_channelIdValues.end();
                }),
            normalizedIds.end());
    }

    if (normalizedIds == m_hiddenChannelIdValues) {
        return;
    }

    m_hiddenChannelIdValues = std::move(normalizedIds);
    m_hiddenChannelIds.clear();
    m_hiddenChannelIds.reserve(static_cast<qsizetype>(m_hiddenChannelIdValues.size()));
    for (const int channelId : m_hiddenChannelIdValues) {
        m_hiddenChannelIds.push_back(channelId);
    }

    markViewDirty();
    emit hiddenChannelIdsChanged();
}

void ScopeView::zoomIn()
{
    setTimeWindow(m_timeWindow * kZoomInFactor);
}

void ScopeView::zoomOut()
{
    setTimeWindow(std::min(kMaxTimeWindow, m_timeWindow * kZoomOutFactor));
}

void ScopeView::fitView()
{
    if (m_controller != nullptr && m_controller->hasData()) {
        m_frozenTMax = m_controller->latestTime();
        const double span = std::max(0.05, m_controller->latestTime() - m_controller->earliestTime());
        setTimeWindow(std::min(kMaxTimeWindow, span));
        return;
    }

    setTimeWindow(kDefaultTimeWindow);
}

void ScopeView::pan()
{
    if (m_controller == nullptr || !m_controller->hasData()) {
        return;
    }

    if (m_liveMode) {
        setLiveMode(false);
    }

    const double latest = m_controller->latestTime();
    if (m_frozenTMax <= 0.0 || m_frozenTMax > latest) {
        m_frozenTMax = latest;
    }

    markViewDirty();
}

void ScopeView::panByPixels(double deltaPixels, double viewWidthPixels)
{
    if (m_controller == nullptr || !m_controller->hasData() || viewWidthPixels <= 1.0) {
        return;
    }

    if (std::abs(deltaPixels) < 0.01) {
        return;
    }

    if (m_liveMode) {
        setLiveMode(false);
    }

    const double earliest = m_controller->earliestTime();
    const double latest = m_controller->latestTime();
    const double clampedWindow = std::max(0.05, m_timeWindow);
    const double secondsPerPixel = clampedWindow / viewWidthPixels;
    const double deltaTime = deltaPixels * secondsPerPixel;

    if (m_frozenTMax <= 0.0 || m_frozenTMax > latest) {
        m_frozenTMax = latest;
    }

    const double minTMax = std::min(latest, earliest + clampedWindow);
    const double maxTMax = latest;
    m_frozenTMax = std::clamp(m_frozenTMax - deltaTime, minTMax, maxTMax);
    markViewDirty();
}

QSGNode* ScopeView::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*)
{
    ScopeRootNode* root = static_cast<ScopeRootNode*>(oldNode);
    if (root == nullptr) {
        root = new ScopeRootNode();
    }

    const int widthPx = std::max(1, static_cast<int>(std::round(width())));
    const float heightPx = std::max(1.0f, static_cast<float>(height()));

    if (m_controller == nullptr) {
        while (!root->channels.empty()) {
            ChannelNodeBundle bundle = root->channels.back();
            root->removeChildNode(bundle.node);
            delete bundle.node;
            root->channels.pop_back();
        }
        return root;
    }

    double tMax = m_controller->latestTime();
    if (!m_controller->hasData()) {
        tMax = m_timeWindow;
    }

    if (m_paused) {
        tMax = m_frozenTMax;
    } else if (!m_liveMode) {
        tMax = m_frozenTMax;
    } else {
        m_frozenTMax = tMax;
    }

    const double tMin = std::max(0.0, tMax - m_timeWindow);

    SnapshotRequest req;
    req.tMin = tMin;
    req.tMax = tMax;
    req.resolutionHint = widthPx;
    req.channels.all = true;

    if (m_hasChannelFilter) {
        req.channels.all = false;
        if (m_hiddenChannelIdValues.empty()) {
            req.channels.channelIds = m_channelIdValues;
        } else {
            req.channels.channelIds.reserve(m_channelIdValues.size());
            for (const int channelId : m_channelIdValues) {
                if (!std::binary_search(m_hiddenChannelIdValues.begin(), m_hiddenChannelIdValues.end(), channelId)) {
                    req.channels.channelIds.push_back(channelId);
                }
            }
        }
    } else if (!m_hiddenChannelIdValues.empty()) {
        req.channels.all = false;
        const QVariantList allChannelIds = m_controller->channelIds();
        req.channels.channelIds.reserve(static_cast<std::size_t>(allChannelIds.size()));
        for (const QVariant& value : allChannelIds) {
            bool ok = false;
            const int channelId = value.toInt(&ok);
            if (!ok) {
                continue;
            }

            if (std::binary_search(m_hiddenChannelIdValues.begin(), m_hiddenChannelIdValues.end(), channelId)) {
                continue;
            }

            if (std::find(req.channels.channelIds.begin(), req.channels.channelIds.end(), channelId) == req.channels.channelIds.end()) {
                req.channels.channelIds.push_back(channelId);
            }
        }
        std::sort(req.channels.channelIds.begin(), req.channels.channelIds.end());
    }

    ScopeSnapshot snapshot = m_controller->snapshot(req);

    while (root->channels.size() > snapshot.channels.size()) {
        ChannelNodeBundle bundle = root->channels.back();
        root->removeChildNode(bundle.node);
        delete bundle.node;
        root->channels.pop_back();
    }

    while (root->channels.size() < snapshot.channels.size()) {
        auto* node = new QSGGeometryNode();
        auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 0);
        geometry->setDrawingMode(QSGGeometry::DrawLines);
        geometry->setLineWidth(1.0f);
        node->setGeometry(geometry);
        node->setFlag(QSGNode::OwnsGeometry, true);

        auto* material = new QSGFlatColorMaterial();
        material->setColor(kChannelPalette.front());
        node->setMaterial(material);
        node->setFlag(QSGNode::OwnsMaterial, true);

        root->appendChildNode(node);

        ChannelNodeBundle bundle;
        bundle.channelId = -1;
        bundle.node = node;
        bundle.geometry = geometry;
        bundle.material = material;
        bundle.capacity = 0;
        root->channels.push_back(bundle);
    }

    for (int index = 0; index < static_cast<int>(snapshot.channels.size()); ++index) {
        ChannelNodeBundle& bundle = root->channels[static_cast<std::size_t>(index)];
        const ChannelSnapshot& channel = snapshot.channels[static_cast<std::size_t>(index)];

        bundle.channelId = channel.channelId;

        const QColor channelColor = colorForChannel(channel.channelId);
        if (bundle.material->color() != channelColor) {
            bundle.material->setColor(channelColor);
            bundle.node->markDirty(QSGNode::DirtyMaterial);
        }

        const int requiredVertices = widthPx * 2;
        if (bundle.capacity != requiredVertices) {
            bundle.geometry->allocate(requiredVertices);
            bundle.capacity = requiredVertices;
        }

        auto* vertices = bundle.geometry->vertexDataAsPoint2D();
        buildEnvelopeVertices(channel, tMin, tMax, widthPx, heightPx, vertices);

        bundle.node->markDirty(QSGNode::DirtyGeometry);
    }

    return root;
}

void ScopeView::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size()) {
        markViewDirty();
    }
}

void ScopeView::onFrameTick()
{
    bool shouldUpdate = m_viewDirty;

    if (m_controller != nullptr) {
        const std::uint64_t epoch = m_controller->dataEpoch();
        const bool hasFreshData = (epoch != m_lastDataEpoch);
        if (hasFreshData && !m_paused) {
            shouldUpdate = true;
        }

        if (hasFreshData) {
            m_lastDataEpoch = epoch;
        }
    }

    if (shouldUpdate) {
        update();
        m_viewDirty = false;
    }
}

void ScopeView::markViewDirty()
{
    m_viewDirty = true;
}

void ScopeView::ensureColumnBuffers(int widthPx)
{
    if (widthPx <= 0) {
        return;
    }

    const std::size_t required = static_cast<std::size_t>(widthPx);
    if (m_colMin.size() < required) {
        m_colMin.resize(required);
        m_colMax.resize(required);
        m_colHas.resize(required);
    }
}

void ScopeView::buildEnvelopeVertices(const ChannelSnapshot& channel,
                                      double tMin,
                                      double tMax,
                                      int widthPx,
                                      float heightPx,
                                      QSGGeometry::Point2D* outVertices)
{
    ensureColumnBuffers(widthPx);
    std::fill_n(m_colHas.data(), widthPx, 0U);

    const double timeRange = std::max(1e-9, tMax - tMin);
    const double invRange = 1.0 / timeRange;

    float globalMin = std::numeric_limits<float>::infinity();
    float globalMax = -std::numeric_limits<float>::infinity();
    bool hasAnySample = false;

    for (const ScopeSegment& segment : channel.segments) {
        if (segment.data == nullptr || segment.count == 0 || segment.dt <= 0.0) {
            continue;
        }

        for (std::size_t i = 0; i < segment.count; ++i) {
            const double t = segment.t0 + segment.dt * static_cast<double>(i);
            if (t < tMin || t > tMax) {
                continue;
            }

            int x = static_cast<int>((t - tMin) * invRange * static_cast<double>(widthPx - 1));
            x = std::clamp(x, 0, widthPx - 1);

            const float v = segment.data[i];
            if (!m_colHas[static_cast<std::size_t>(x)]) {
                m_colHas[static_cast<std::size_t>(x)] = 1U;
                m_colMin[static_cast<std::size_t>(x)] = v;
                m_colMax[static_cast<std::size_t>(x)] = v;
            } else {
                m_colMin[static_cast<std::size_t>(x)] = std::min(m_colMin[static_cast<std::size_t>(x)], v);
                m_colMax[static_cast<std::size_t>(x)] = std::max(m_colMax[static_cast<std::size_t>(x)], v);
            }

            globalMin = std::min(globalMin, v);
            globalMax = std::max(globalMax, v);
            hasAnySample = true;
        }
    }

    if (!hasAnySample) {
        const float y = heightPx * 0.5f;
        for (int x = 0; x < widthPx; ++x) {
            const float xf = static_cast<float>(x);
            outVertices[2 * x].set(xf, y);
            outVertices[(2 * x) + 1].set(xf, y);
        }
        return;
    }

    float dataRange = globalMax - globalMin;
    if (std::abs(dataRange) < 1e-6f) {
        globalMin -= 1.0f;
        globalMax += 1.0f;
        dataRange = globalMax - globalMin;
    }

    float carryMin = globalMin;
    float carryMax = globalMax;

    for (int x = 0; x < widthPx; ++x) {
        const std::size_t idx = static_cast<std::size_t>(x);
        if (m_colHas[idx]) {
            carryMin = m_colMin[idx];
            carryMax = m_colMax[idx];
        }

        const float normalizedMax = (carryMax - globalMin) / dataRange;
        const float normalizedMin = (carryMin - globalMin) / dataRange;

        const float yTop = (1.0f - normalizedMax) * heightPx;
        const float yBottom = (1.0f - normalizedMin) * heightPx;

        const float xf = static_cast<float>(x);
        outVertices[2 * x].set(xf, yTop);
        outVertices[(2 * x) + 1].set(xf, yBottom);
    }
}

QColor ScopeView::colorForChannel(int channelId) const
{
    const int paletteIndex = std::abs(channelId) % static_cast<int>(kChannelPalette.size());
    return kChannelPalette[static_cast<std::size_t>(paletteIndex)];
}

} // namespace c41scope
