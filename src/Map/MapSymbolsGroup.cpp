#include "MapSymbolsGroup.h"

#include "MapSymbol.h"
#include "QKeyValueIterator.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::MapSymbolsGroup::MapSymbolsGroup()
    : additionalInstancesDiscardOriginal(false)
{
}

OsmAnd::MapSymbolsGroup::~MapSymbolsGroup()
{
}

bool OsmAnd::MapSymbolsGroup::obtainSharingKey(SharingKey& outKey) const
{
    return false;
}

bool OsmAnd::MapSymbolsGroup::obtainSortingKey(SortingKey& outKey) const
{
    return false;
}

QString OsmAnd::MapSymbolsGroup::toString() const
{
    return QString::asprintf("(@%p)", this);
}

std::shared_ptr<OsmAnd::MapSymbol> OsmAnd::MapSymbolsGroup::getFirstSymbolWithContentClass(const MapSymbol::ContentClass contentClass) const
{
    for (const auto& symbol : constOf(symbols))
    {
        if (symbol->contentClass == contentClass)
            return symbol;
    }

    return nullptr;
}

unsigned int OsmAnd::MapSymbolsGroup::numberOfSymbolsWithContentClass(const MapSymbol::ContentClass contentClass) const
{
    unsigned int count = 0;

    for (const auto& symbol : constOf(symbols))
    {
        if (symbol->contentClass != contentClass)
            continue;

        count++;
    }

    return count;
}

OsmAnd::MapSymbolsGroup::AdditionalInstance::AdditionalInstance(const std::shared_ptr<MapSymbolsGroup>& originalGroup_)
    : originalGroup(originalGroup_)
{
}

OsmAnd::MapSymbolsGroup::AdditionalInstance::~AdditionalInstance()
{
}

std::shared_ptr<OsmAnd::MapSymbol> OsmAnd::MapSymbolsGroup::AdditionalInstance::getFirstSymbolWithContentClass(const MapSymbol::ContentClass contentClass) const
{
    for(const auto& symbol : rangeOf(constOf(symbols)))
    {
        if (symbol.key()->contentClass == contentClass)
            return symbol.key();
    }

    return nullptr;
}

unsigned int OsmAnd::MapSymbolsGroup::AdditionalInstance::numberOfSymbolsWithContentClass(const MapSymbol::ContentClass contentClass) const
{
    unsigned int count = 0;

    for (const auto& symbol : rangeOf(constOf(symbols)))
    {
        if (symbol.key()->contentClass != contentClass)
            continue;

        count++;
    }

    return count;
}

OsmAnd::MapSymbolsGroup::AdditionalSymbolInstanceParameters::AdditionalSymbolInstanceParameters(const AdditionalInstance* const groupInstancePtr_)
    : groupInstancePtr(groupInstancePtr_)
{
}

OsmAnd::MapSymbolsGroup::AdditionalSymbolInstanceParameters::~AdditionalSymbolInstanceParameters()
{
}

OsmAnd::MapSymbolsGroup::AdditionalBillboardSymbolInstanceParameters::AdditionalBillboardSymbolInstanceParameters(const AdditionalInstance* const groupInstancePtr_)
    : AdditionalSymbolInstanceParameters(groupInstancePtr_)
    , overridesPosition31(false)
    , overridesOffset(false)
{
}

OsmAnd::MapSymbolsGroup::AdditionalBillboardSymbolInstanceParameters::~AdditionalBillboardSymbolInstanceParameters()
{
}

OsmAnd::MapSymbolsGroup::AdditionalOnSurfaceSymbolInstanceParameters::AdditionalOnSurfaceSymbolInstanceParameters(const AdditionalInstance* const groupInstancePtr_)
    : AdditionalSymbolInstanceParameters(groupInstancePtr_)
    , overridesPosition31(false)
    , overridesDirection(false)
{
}

OsmAnd::MapSymbolsGroup::AdditionalOnSurfaceSymbolInstanceParameters::~AdditionalOnSurfaceSymbolInstanceParameters()
{
}

OsmAnd::MapSymbolsGroup::AdditionalOnPathSymbolInstanceParameters::AdditionalOnPathSymbolInstanceParameters(const AdditionalInstance* const groupInstancePtr_)
    : AdditionalSymbolInstanceParameters(groupInstancePtr_)
    , overridesPinPointOnPath(false)
{
}

OsmAnd::MapSymbolsGroup::AdditionalOnPathSymbolInstanceParameters::~AdditionalOnPathSymbolInstanceParameters()
{
}

OsmAnd::MapSymbolsGroup::Comparator::Comparator()
{
}

bool OsmAnd::MapSymbolsGroup::Comparator::operator()(
    const std::shared_ptr<const MapSymbolsGroup>& l,
    const std::shared_ptr<const MapSymbolsGroup>& r) const
{
    MapSymbolsGroup::SortingKey lKey;
    const auto lHasKey = l->obtainSortingKey(lKey);

    MapSymbolsGroup::SortingKey rKey;
    const auto rHasKey = r->obtainSortingKey(rKey);

    if (lHasKey && rHasKey)
    {
        if (lKey != rKey)
            return (lKey < rKey);

        return (l < r);
    }

    if (!lHasKey && !rHasKey)
        return (l < r);

    if (lHasKey && !rHasKey)
        return true;

    if (!lHasKey && rHasKey)
        return false;

    return (l < r);
}
