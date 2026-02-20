#pragma once

#include <cstddef>

#include "scope_types.h"

namespace c41scope {

class IScopeDataStore {
public:
    virtual ~IScopeDataStore() = default;

    virtual void pushSamples(int channelId,
                             const float* data,
                             std::size_t count,
                             double t0,
                             double dt) = 0;

    virtual void pushFrame(int channelId,
                           const float* data,
                           std::size_t count,
                           double t0,
                           double dt) = 0;

    virtual ScopeSnapshot buildSnapshot(const SnapshotRequest& req) = 0;

    virtual void clear() = 0;
};

} // namespace c41scope

