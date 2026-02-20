#include "stats_command_tool.h"

#include <QTextStream>

#include <algorithm>
#include <limits>

namespace c41scope {

StatsCommandTool::StatsCommandTool(ResultCallback callback)
    : m_callback(std::move(callback))
{
}

QString StatsCommandTool::id() const
{
    return QStringLiteral("stats_mean_max_min");
}

QString StatsCommandTool::name() const
{
    return QStringLiteral("Mean/Max/Min");
}

QString StatsCommandTool::category() const
{
    return QStringLiteral("Analysis");
}

bool StatsCommandTool::isApplicable(const ToolContext& ctx) const
{
    Q_UNUSED(ctx);
    return true;
}

void StatsCommandTool::run(IScopeDataAPI* api)
{
    if (api == nullptr || !m_callback) {
        return;
    }

    const ToolContext ctx = api->context();
    if (ctx.tMax <= ctx.tMin) {
        m_callback(QStringLiteral("Mean/Max/Min"), QStringLiteral("No data in current range."));
        return;
    }

    SnapshotRequest req;
    req.tMin = ctx.tMin;
    req.tMax = ctx.tMax;
    req.channels.all = ctx.allChannels;
    if (!req.channels.all) {
        req.channels.channelIds = ctx.channelIds;
    }

    const ScopeSnapshot snapshot = api->snapshot(req);
    if (snapshot.channels.empty()) {
        m_callback(QStringLiteral("Mean/Max/Min"), QStringLiteral("No channel data available."));
        return;
    }

    QString output;
    QTextStream stream(&output);
    stream.setRealNumberNotation(QTextStream::FixedNotation);
    stream.setRealNumberPrecision(6);

    stream << "Range: [" << snapshot.tMin << ", " << snapshot.tMax << "]\\n";

    bool anyValue = false;
    for (const ChannelSnapshot& channel : snapshot.channels) {
        double sum = 0.0;
        std::size_t count = 0;
        float minValue = std::numeric_limits<float>::infinity();
        float maxValue = -std::numeric_limits<float>::infinity();

        for (const ScopeSegment& segment : channel.segments) {
            if (segment.data == nullptr || segment.count == 0) {
                continue;
            }

            for (std::size_t i = 0; i < segment.count; ++i) {
                const float value = segment.data[i];
                sum += static_cast<double>(value);
                minValue = std::min(minValue, value);
                maxValue = std::max(maxValue, value);
                ++count;
            }
        }

        if (count == 0) {
            continue;
        }

        anyValue = true;
        const double mean = sum / static_cast<double>(count);

        stream << "CH " << channel.channelId
               << " | mean=" << mean
               << " | max=" << maxValue
               << " | min=" << minValue
               << " | n=" << static_cast<qulonglong>(count)
               << "\\n";
    }

    if (!anyValue) {
        m_callback(QStringLiteral("Mean/Max/Min"), QStringLiteral("No samples in selected range."));
        return;
    }

    m_callback(QStringLiteral("Mean/Max/Min"), output.trimmed());
}

} // namespace c41scope
