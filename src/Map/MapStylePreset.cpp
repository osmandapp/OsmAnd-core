#include "MapStylePreset.h"

OsmAnd::MapStylePreset::MapStylePreset(const Type type_, const QString& name_, const QString& styleName_)
    : type(type_)
    , name(name_)
    , styleName(styleName_)
{
}

OsmAnd::MapStylePreset::~MapStylePreset()
{
}
