#ifndef _OSMAND_CORE_MAP_STYLE_BUILTIN_VALUE_DEFINITIONS_H_
#define _OSMAND_CORE_MAP_STYLE_BUILTIN_VALUE_DEFINITIONS_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>

namespace OsmAnd
{
    class MapStyleValueDefinition;
    class MapStyle;

    class OSMAND_CORE_API MapStyleBuiltinValueDefinitions
    {
        Q_DISABLE_COPY(MapStyleBuiltinValueDefinitions);
    private:
    protected:
        MapStyleBuiltinValueDefinitions();
    public:
        virtual ~MapStyleBuiltinValueDefinitions();

        // Definitions
#       define DECLARE_BUILTIN_VALUEDEF(varname, valueClass, dataType, name, isComplex) \
            const std::shared_ptr<const MapStyleValueDefinition> varname;
#       include <OsmAndCore/Map/MapStyleBuiltinValueDefinitions_Set.h>
#       undef DECLARE_BUILTIN_VALUEDEF

        // Identifiers of definitions above
#       define DECLARE_BUILTIN_VALUEDEF(varname, valueClass, dataType, name, isComplex) \
            const int id_##varname;
#       include <OsmAndCore/Map/MapStyleBuiltinValueDefinitions_Set.h>
#       undef DECLARE_BUILTIN_VALUEDEF

    friend class OsmAnd::MapStyle;

    private:
        int _footerDummy;
    };
} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_MAP_STYLE_BUILTIN_VALUE_DEFINITIONS_H_)
