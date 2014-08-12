#include "MapSymbolsGroup.h"

#include "MapSymbol.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::MapSymbolsGroup::MapSymbolsGroup()
{
}

OsmAnd::MapSymbolsGroup::~MapSymbolsGroup()
{
}

QString OsmAnd::MapSymbolsGroup::getDebugTitle() const
{
    static QString noDebugTitle(QLatin1String("?"));

    return noDebugTitle;
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
        if (symbol->contentClass == contentClass)
            count++;
    }

    return count;
}

OsmAnd::IUpdatableMapSymbolsGroup::IUpdatableMapSymbolsGroup()
{
}

OsmAnd::IUpdatableMapSymbolsGroup::~IUpdatableMapSymbolsGroup()
{
}
