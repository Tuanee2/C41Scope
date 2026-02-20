#pragma once

#include <QString>

#include <functional>

#include "../../core/tools_interfaces.h"

namespace c41scope {

class CallbackViewTool final : public IViewTool {
public:
    CallbackViewTool(QString toolId, QString toolName, std::function<void()> triggerFn);

    QString id() const override;
    QString name() const override;
    void trigger() override;

private:
    QString m_id;
    QString m_name;
    std::function<void()> m_triggerFn;
};

} // namespace c41scope

