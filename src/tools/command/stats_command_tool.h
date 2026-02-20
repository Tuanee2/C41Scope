#pragma once

#include <QString>

#include <functional>

#include "../../core/tools_interfaces.h"

namespace c41scope {

class StatsCommandTool final : public ICommandTool {
public:
    using ResultCallback = std::function<void(const QString& title, const QString& body)>;

    explicit StatsCommandTool(ResultCallback callback);

    QString id() const override;
    QString name() const override;
    QString category() const override;
    bool isApplicable(const ToolContext& ctx) const override;
    void run(IScopeDataAPI* api) override;

private:
    ResultCallback m_callback;
};

} // namespace c41scope

