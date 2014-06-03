#include "MapSymbolsGroup.h"

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

OsmAnd::IUpdatableMapSymbolsGroup::IUpdatableMapSymbolsGroup()
{
}

OsmAnd::IUpdatableMapSymbolsGroup::~IUpdatableMapSymbolsGroup()
{
}
