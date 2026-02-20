#pragma once

#include <cstddef>
#include <deque>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "../core/iscope_data_store.h"

namespace c41scope {

class HistoryStore final : public IScopeDataStore {
public:
    explicit HistoryStore(std::size_t chunkSize = 2048);

    void pushSamples(int channelId,
                     const float* data,
                     std::size_t count,
                     double t0,
                     double dt) override;

    void pushFrame(int channelId,
                   const float* data,
                   std::size_t count,
                   double t0,
                   double dt) override;

    ScopeSnapshot buildSnapshot(const SnapshotRequest& req) override;

    void clear() override;

private:
    struct Chunk {
        std::vector<float> values;
        std::size_t count = 0;
        double t0 = 0.0;
        double dt = 0.0;
    };

    struct ChannelData {
        std::deque<Chunk> chunks;
    };

    void appendSamplesLocked(int channelId,
                             const float* data,
                             std::size_t count,
                             double t0,
                             double dt);

    static bool isChannelSelected(const ChannelSelection& selection, int channelId);

    std::size_t m_chunkSize;
    std::mutex m_mutex;
    std::unordered_map<int, ChannelData> m_channels;
    ScopeSnapshot m_lastSnapshot;
};

} // namespace c41scope

