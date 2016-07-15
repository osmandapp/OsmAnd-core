#ifndef _OSMAND_CORE_MAP_STYLE_BUILTIN_VALUE_DEFINITIONS_H_
#define _OSMAND_CORE_MAP_STYLE_BUILTIN_VALUE_DEFINITIONS_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QAtomicInt>
#include <QMutex>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/Map/MapCommonTypes.h>
#include <OsmAndCore/Map/MapStyleValueDefinition.h>

namespace OsmAnd
{
    class OSMAND_CORE_API MapStyleBuiltinValueDefinitions
    {
        Q_DISABLE_COPY_AND_MOVE(MapStyleBuiltinValueDefinitions)
    public:
        class OSMAND_CORE_API MapStyleBuiltinValueDefinition : public MapStyleValueDefinition
        {
        private:
        protected:
            MapStyleBuiltinValueDefinition(
                const int id,
                const MapStyleValueDefinition::Class valueClass,
                const MapStyleValueDataType dataType,
                const QString& name,
                const bool isComplex);
        public:
            virtual ~MapStyleBuiltinValueDefinition();

            const int id;

        friend class OsmAnd::MapStyleBuiltinValueDefinitions;
        };

    private:
    protected:
        MapStyleBuiltinValueDefinitions();

        int _runtimeGeneratedId;

        static QAtomicInt s_instanceFlag;
        static QMutex s_instanceMutex;
        static std::shared_ptr<const MapStyleBuiltinValueDefinitions> s_instance;
    public:
        virtual ~MapStyleBuiltinValueDefinitions();

        static std::shared_ptr<const MapStyleBuiltinValueDefinitions> get();

        // Definitions
#       define DECLARE_BUILTIN_VALUEDEF(varname, valueClass, dataType, name, isComplex) \
            const std::shared_ptr<const MapStyleBuiltinValueDefinition> varname;
#       include <OsmAndCore/Map/MapStyleBuiltinValueDefinitions_Set.h>
#       undef DECLARE_BUILTIN_VALUEDEF

        // Identifiers of definitions above
#       define DECLARE_BUILTIN_VALUEDEF(varname, valueClass, dataType, name, isComplex) \
            const int id_##varname;
#       include <OsmAndCore/Map/MapStyleBuiltinValueDefinitions_Set.h>
#       undef DECLARE_BUILTIN_VALUEDEF

        const int lastBuiltinValueDefinitionId;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLE_BUILTIN_VALUE_DEFINITIONS_H_)
