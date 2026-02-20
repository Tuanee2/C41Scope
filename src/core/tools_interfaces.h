#pragma once

#include <QUrl>

#include <QString>

#include "iscope_data_api.h"

namespace c41scope {

class IViewTool {
public:
    virtual ~IViewTool() = default;
    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual void trigger() = 0;
};

class IInteractiveTool {
public:
    virtual ~IInteractiveTool() = default;
    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual QUrl panelQml() const = 0;
    virtual void onActivate(IScopeDataAPI* api) = 0;
    virtual void onDeactivate() = 0;
};

class ICommandTool {
public:
    virtual ~ICommandTool() = default;
    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual QString category() const = 0;
    virtual bool isApplicable(const ToolContext& ctx) const = 0;
    virtual void run(IScopeDataAPI* api) = 0;
};

} // namespace c41scope

