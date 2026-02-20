#pragma once

#include <cstddef>
#include <vector>

namespace c41scope {

enum class DataMode {
    History = 0,
    Frame = 1,
};

struct ChannelSelection {
    bool all = true;
    std::vector<int> channelIds;
};

struct SnapshotRequest {
    double tMin = 0.0;
    double tMax = 0.0;
    ChannelSelection channels;
    int resolutionHint = 0;
};

struct ScopeSegment {
    const float* data = nullptr;
    std::size_t count = 0;
    double t0 = 0.0;
    double dt = 0.0;
};

struct ChannelSnapshot {
    int channelId = -1;
    std::vector<ScopeSegment> segments;
};

struct ScopeSnapshot {
    double tMin = 0.0;
    double tMax = 0.0;
    std::vector<ChannelSnapshot> channels;
};

struct ToolContext {
    DataMode dataMode = DataMode::History;
    bool hasSelection = false;
    double tMin = 0.0;
    double tMax = 0.0;
    bool allChannels = true;
    std::vector<int> channelIds;
};

} // namespace c41scope
