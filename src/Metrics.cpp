#include "Metrics.h"

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QStringList>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"

OsmAnd::Metric::Metric()
{
    reset();
}

OsmAnd::Metric::~Metric()
{
}

void OsmAnd::Metric::reset()
{
    for (auto& submetric : submetrics)
        submetric->reset();
}

void OsmAnd::Metric::addSubmetric(const std::shared_ptr<Metric>& submetric)
{
    submetrics.push_back(submetric);
}

void OsmAnd::Metric::addOrReplaceSubmetric(const std::shared_ptr<Metric>& submetric)
{
    auto barePtrSubmetric = submetric.get();
    const auto& submetricType = typeid(*barePtrSubmetric);

    for (auto& otherSubmetric : submetrics)
    {
        auto barePtrOtherSubmetric = otherSubmetric.get();
        if (submetricType == typeid(*barePtrOtherSubmetric))
        {
            otherSubmetric = submetric;
            return;
        }
    }

    submetrics.push_back(submetric);
}

std::shared_ptr<OsmAnd::Metric> OsmAnd::Metric::findSubmetricOfExactType(const std::type_info& requestedType)
{
    for (auto& submetric : submetrics)
    {
        auto barePtrSubmetric = submetric.get();
        if (typeid(*barePtrSubmetric) == requestedType)
            return submetric.shared_ptr();
    }

    return nullptr;
}

std::shared_ptr<const OsmAnd::Metric> OsmAnd::Metric::findSubmetricOfExactType(const std::type_info& requestedType) const
{
    for (auto& submetric : submetrics)
    {
        auto barePtrSubmetric = submetric.get();
        if (typeid(*barePtrSubmetric) == requestedType)
            return submetric.shared_ptr();
    }

    return nullptr;
}

QString OsmAnd::Metric::toString(const bool shortFormat /*= false*/, const QString& prefix /*= QString::null*/) const
{
    QStringList outputs;
    for (auto& submetric : submetrics)
    {
        const auto submetricString = submetric->toString(shortFormat, prefix);
        if (submetricString.isEmpty())
            continue;
        outputs.push_back(submetric->toString(shortFormat, prefix));
    }
    return outputs.join(QChar(QLatin1Char('\n')));
}
