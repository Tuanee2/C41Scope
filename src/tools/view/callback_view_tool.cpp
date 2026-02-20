#include "callback_view_tool.h"

#include <utility>

namespace c41scope {

CallbackViewTool::CallbackViewTool(QString toolId, QString toolName, std::function<void()> triggerFn)
    : m_id(std::move(toolId))
    , m_name(std::move(toolName))
    , m_triggerFn(std::move(triggerFn))
{
}

QString CallbackViewTool::id() const
{
    return m_id;
}

QString CallbackViewTool::name() const
{
    return m_name;
}

void CallbackViewTool::trigger()
{
    if (m_triggerFn) {
        m_triggerFn();
    }
}

} // namespace c41scope

