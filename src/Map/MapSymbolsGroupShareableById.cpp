#include "MapSymbolsGroupShareableById.h"

OsmAnd::MapSymbolsGroupShareableById::MapSymbolsGroupShareableById(const uint64_t id_)
    : id(id_)
{
}

OsmAnd::MapSymbolsGroupShareableById::~MapSymbolsGroupShareableById()
{
}

QString OsmAnd::MapSymbolsGroupShareableById::getDebugTitle() const
{
    return QString(QLatin1String("%1")).arg(id);
}
