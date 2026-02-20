#include "history_store.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace c41scope {

HistoryStore::HistoryStore(std::size_t chunkSize)
    : m_chunkSize(chunkSize == 0 ? 2048 : chunkSize)
{
}

void HistoryStore::pushSamples(int channelId,
                               const float* data,
                               std::size_t count,
                               double t0,
                               double dt)
{
    if (data == nullptr || count == 0 || dt <= 0.0) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    appendSamplesLocked(channelId, data, count, t0, dt);
}

void HistoryStore::pushFrame(int channelId,
                             const float* data,
                             std::size_t count,
                             double t0,
                             double dt)
{
    // History mode keeps appending data even when the caller pushes a full frame.
    pushSamples(channelId, data, count, t0, dt);
}

ScopeSnapshot HistoryStore::buildSnapshot(const SnapshotRequest& req)
{
    std::unique_lock<std::mutex> lock(m_mutex, std::try_to_lock);
    if (!lock.owns_lock()) {
        return m_lastSnapshot;
    }

    ScopeSnapshot snapshot;
    snapshot.tMin = req.tMin;
    snapshot.tMax = req.tMax;

    for (const auto& [channelId, channelData] : m_channels) {
        if (!isChannelSelected(req.channels, channelId)) {
            continue;
        }

        ChannelSnapshot channelSnapshot;
        channelSnapshot.channelId = channelId;

        for (const Chunk& chunk : channelData.chunks) {
            if (chunk.count == 0) {
                continue;
            }

            const double chunkStart = chunk.t0;
            const double chunkEnd = chunk.t0 + chunk.dt * static_cast<double>(chunk.count - 1);
            if (chunkEnd < req.tMin || chunkStart > req.tMax) {
                continue;
            }

            channelSnapshot.segments.push_back(
                ScopeSegment{chunk.values.data(), chunk.count, chunk.t0, chunk.dt});
        }

        if (!channelSnapshot.segments.empty()) {
            snapshot.channels.push_back(std::move(channelSnapshot));
        }
    }

    std::sort(snapshot.channels.begin(), snapshot.channels.end(), [](const ChannelSnapshot& lhs, const ChannelSnapshot& rhs) {
        return lhs.channelId < rhs.channelId;
    });

    m_lastSnapshot = snapshot;
    return snapshot;
}

void HistoryStore::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_channels.clear();
    m_lastSnapshot = ScopeSnapshot{};
}

void HistoryStore::appendSamplesLocked(int channelId,
                                       const float* data,
                                       std::size_t count,
                                       double t0,
                                       double dt)
{
    ChannelData& channelData = m_channels[channelId];

    std::size_t sourceIndex = 0;
    while (sourceIndex < count) {
        const bool needNewChunk = channelData.chunks.empty()
            || channelData.chunks.back().count >= m_chunkSize
            || std::abs(channelData.chunks.back().dt - dt) > 1e-9
            || std::abs((channelData.chunks.back().t0
                         + channelData.chunks.back().dt * static_cast<double>(channelData.chunks.back().count))
                        - (t0 + dt * static_cast<double>(sourceIndex)))
                > (dt * 0.25);

        if (needNewChunk) {
            Chunk chunk;
            chunk.values.resize(m_chunkSize);
            chunk.count = 0;
            chunk.t0 = t0 + dt * static_cast<double>(sourceIndex);
            chunk.dt = dt;
            channelData.chunks.push_back(std::move(chunk));
        }

        Chunk& chunk = channelData.chunks.back();
        const std::size_t space = m_chunkSize - chunk.count;
        const std::size_t toCopy = std::min(space, count - sourceIndex);

        std::memcpy(chunk.values.data() + chunk.count,
                    data + sourceIndex,
                    toCopy * sizeof(float));

        chunk.count += toCopy;
        sourceIndex += toCopy;
    }
}

bool HistoryStore::isChannelSelected(const ChannelSelection& selection, int channelId)
{
    if (selection.all) {
        return true;
    }

    return std::find(selection.channelIds.begin(), selection.channelIds.end(), channelId)
        != selection.channelIds.end();
}

} // namespace c41scope

