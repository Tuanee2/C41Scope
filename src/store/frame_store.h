#pragma once

#include <cstddef>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "../core/iscope_data_store.h"

namespace c41scope {

class FrameStore final : public IScopeDataStore {
public:
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
    struct FrameData {
        std::vector<float> front;
        std::vector<float> back;
        std::size_t count = 0;
        double t0 = 0.0;
        double dt = 0.0;
    };

    static bool isChannelSelected(const ChannelSelection& selection, int channelId);

    std::mutex m_mutex;
    std::unordered_map<int, FrameData> m_channels;
    std::unordered_map<int, std::vector<float>> m_snapshotBuffers;
    ScopeSnapshot m_lastSnapshot;
};

} // namespace c41scope
