#include "MapStyleBuiltinValueDefinitions.h"

#include "MapStyleValueDefinition.h"

OsmAnd::MapStyleBuiltinValueDefinitions::MapStyleBuiltinValueDefinitions()
    :

    // Definitions
#   define DECLARE_BUILTIN_VALUEDEF(varname, valueClass, dataType, name, isComplex) \
        varname(new OsmAnd::MapStyleValueDefinition( \
            OsmAnd::MapStyleValueClass::valueClass, \
            OsmAnd::MapStyleValueDataType::dataType, \
            QLatin1String(name), isComplex)),
#   include "MapStyleBuiltinValueDefinitions_Set.h"
#   undef DECLARE_BUILTIN_VALUEDEF

    // Identifiers of definitions above
#   define DECLARE_BUILTIN_VALUEDEF(varname, valueClass, dataType, name, isComplex) \
    id_##varname(varname->id),
#   include "MapStyleBuiltinValueDefinitions_Set.h"
#   undef DECLARE_BUILTIN_VALUEDEF

    /*, */ _footerDummy(0)
{
}

OsmAnd::MapStyleBuiltinValueDefinitions::~MapStyleBuiltinValueDefinitions()
{
}
