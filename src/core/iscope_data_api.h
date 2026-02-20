#pragma once

#include "scope_types.h"

namespace c41scope {

class IScopeDataAPI {
public:
    virtual ~IScopeDataAPI() = default;

    virtual ScopeSnapshot snapshot(const SnapshotRequest& req) = 0;
    virtual ToolContext context() const = 0;
};

} // namespace c41scope

