#include "MapSymbolsGroupWithId.h"

OsmAnd::MapSymbolsGroupWithId::MapSymbolsGroupWithId(const uint64_t id_, const bool sharableById_)
    : id(id_)
    , sharableById(sharableById_)
{
}

OsmAnd::MapSymbolsGroupWithId::~MapSymbolsGroupWithId()
{
}

QString OsmAnd::MapSymbolsGroupWithId::getDebugTitle() const
{
    return QString(QLatin1String("%1")).arg(id);
}
