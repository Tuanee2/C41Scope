#pragma once

#include <QMetaObject>
#include <QQuickItem>
#include <QSGGeometry>
#include <QTimer>
#include <QVariantList>

#include <cstdint>
#include <vector>

#include "core/scope_types.h"

namespace c41scope {

class ScopeController;

class ScopeView : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QObject* controller READ controller WRITE setController NOTIFY controllerChanged)
    Q_PROPERTY(double timeWindow READ timeWindow WRITE setTimeWindow NOTIFY timeWindowChanged)
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(bool liveMode READ liveMode WRITE setLiveMode NOTIFY liveModeChanged)
    Q_PROPERTY(bool showGrid READ showGrid WRITE setShowGrid NOTIFY showGridChanged)
    Q_PROPERTY(QVariantList channelIds READ channelIds WRITE setChannelIds NOTIFY channelIdsChanged)

public:
    explicit ScopeView(QQuickItem* parent = nullptr);
    ~ScopeView() override;

    QObject* controller() const;
    void setController(QObject* ctrl);

    double timeWindow() const;
    void setTimeWindow(double seconds);

    bool paused() const;
    void setPaused(bool paused);

    bool liveMode() const;
    void setLiveMode(bool enabled);

    bool showGrid() const;
    void setShowGrid(bool enabled);

    QVariantList channelIds() const;
    void setChannelIds(const QVariantList& channelIds);

    Q_INVOKABLE void zoomIn();
    Q_INVOKABLE void zoomOut();
    Q_INVOKABLE void fitView();
    Q_INVOKABLE void pan();
    Q_INVOKABLE void panByPixels(double deltaPixels, double viewWidthPixels);

signals:
    void controllerChanged();
    void timeWindowChanged();
    void pausedChanged();
    void liveModeChanged();
    void showGridChanged();
    void channelIdsChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

private:
    void onFrameTick();

    void markViewDirty();
    void ensureColumnBuffers(int widthPx);
    void buildEnvelopeVertices(const ChannelSnapshot& channel,
                               double tMin,
                               double tMax,
                               int widthPx,
                               float heightPx,
                               QSGGeometry::Point2D* outVertices);

    QColor colorForChannel(int channelId) const;

    ScopeController* m_controller = nullptr;
    QMetaObject::Connection m_controllerDestroyedConnection;
    QTimer m_frameTimer;

    double m_timeWindow = 5.0;
    bool m_paused = false;
    bool m_liveMode = true;
    bool m_showGrid = true;
    double m_frozenTMax = 0.0;
    bool m_viewDirty = true;
    std::uint64_t m_lastDataEpoch = 0;
    bool m_hasChannelFilter = false;
    QVariantList m_channelIds;
    std::vector<int> m_channelIdValues;

    std::vector<float> m_colMin;
    std::vector<float> m_colMax;
    std::vector<unsigned char> m_colHas;
};

} // namespace c41scope
