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

QString OsmAnd::MapSymbolsGroup::getDebugTitle() const
{
    return QString().sprintf("MapSymbolsGroup %p", this);
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

OsmAnd::MapSymbolsGroup::AdditionalSymbolInstanceParameters::AdditionalSymbolInstanceParameters()
{

}

OsmAnd::MapSymbolsGroup::AdditionalSymbolInstanceParameters::~AdditionalSymbolInstanceParameters()
{

}

OsmAnd::MapSymbolsGroup::AdditionalBillboardSymbolInstanceParameters::AdditionalBillboardSymbolInstanceParameters()
    : overridesPosition31(false)
    , overridesOffset(false)
{
}

OsmAnd::MapSymbolsGroup::AdditionalBillboardSymbolInstanceParameters::~AdditionalBillboardSymbolInstanceParameters()
{
}

OsmAnd::MapSymbolsGroup::AdditionalOnSurfaceSymbolInstanceParameters::AdditionalOnSurfaceSymbolInstanceParameters()
    : overridesPosition31(false)
    , overridesDirection(false)
{
}

OsmAnd::MapSymbolsGroup::AdditionalOnSurfaceSymbolInstanceParameters::~AdditionalOnSurfaceSymbolInstanceParameters()
{
}

OsmAnd::MapSymbolsGroup::AdditionalOnPathSymbolInstanceParameters::AdditionalOnPathSymbolInstanceParameters()
    : overridesPinPointOnPath(false)
{
}

OsmAnd::MapSymbolsGroup::AdditionalOnPathSymbolInstanceParameters::~AdditionalOnPathSymbolInstanceParameters()
{
}
