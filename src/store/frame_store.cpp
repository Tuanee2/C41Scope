#include "frame_store.h"

#include <algorithm>
#include <cstring>

namespace c41scope {

void FrameStore::pushSamples(int channelId,
                             const float* data,
                             std::size_t count,
                             double t0,
                             double dt)
{
    // Frame mode treats incoming samples as the next frame.
    pushFrame(channelId, data, count, t0, dt);
}

void FrameStore::pushFrame(int channelId,
                           const float* data,
                           std::size_t count,
                           double t0,
                           double dt)
{
    if (data == nullptr || count == 0 || dt <= 0.0) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    FrameData& frame = m_channels[channelId];
    frame.back.resize(count);
    std::memcpy(frame.back.data(), data, count * sizeof(float));

    frame.front.swap(frame.back);
    frame.count = count;
    frame.t0 = t0;
    frame.dt = dt;
}

ScopeSnapshot FrameStore::buildSnapshot(const SnapshotRequest& req)
{
    std::unique_lock<std::mutex> lock(m_mutex, std::try_to_lock);
    if (!lock.owns_lock()) {
        return m_lastSnapshot;
    }

    ScopeSnapshot snapshot;
    snapshot.tMin = req.tMin;
    snapshot.tMax = req.tMax;
    m_snapshotBuffers.clear();

    for (const auto& [channelId, frame] : m_channels) {
        if (!isChannelSelected(req.channels, channelId) || frame.count == 0) {
            continue;
        }

        const double frameStart = frame.t0;
        const double frameEnd = frame.t0 + frame.dt * static_cast<double>(frame.count - 1);
        if (frameEnd < req.tMin || frameStart > req.tMax) {
            continue;
        }

        std::vector<float>& stableCopy = m_snapshotBuffers[channelId];
        stableCopy = frame.front;

        ChannelSnapshot channelSnapshot;
        channelSnapshot.channelId = channelId;
        channelSnapshot.segments.push_back(ScopeSegment{
            stableCopy.data(),
            stableCopy.size(),
            frame.t0,
            frame.dt,
        });
        snapshot.channels.push_back(std::move(channelSnapshot));
    }

    std::sort(snapshot.channels.begin(), snapshot.channels.end(), [](const ChannelSnapshot& lhs, const ChannelSnapshot& rhs) {
        return lhs.channelId < rhs.channelId;
    });

    m_lastSnapshot = snapshot;
    return snapshot;
}

void FrameStore::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_channels.clear();
    m_snapshotBuffers.clear();
    m_lastSnapshot = ScopeSnapshot{};
}

bool FrameStore::isChannelSelected(const ChannelSelection& selection, int channelId)
{
    if (selection.all) {
        return true;
    }

    return std::find(selection.channelIds.begin(), selection.channelIds.end(), channelId)
        != selection.channelIds.end();
}

} // namespace c41scope
