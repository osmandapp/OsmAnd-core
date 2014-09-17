#ifndef _OSMAND_CORE_MAP_STYLE_VALUE_DEFINITION_H_
#define _OSMAND_CORE_MAP_STYLE_VALUE_DEFINITION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/Map/MapCommonTypes.h>

namespace OsmAnd
{
    class OSMAND_CORE_API MapStyleValueDefinition
    {
        Q_DISABLE_COPY_AND_MOVE(MapStyleValueDefinition);
    public:
        enum class Class
        {
            Input,
            Output,
        };

    private:
    protected:
    public:
        MapStyleValueDefinition(
            const MapStyleValueDefinition::Class valueClass,
            const MapStyleValueDataType dataType,
            const QString& name,
            const bool isComplex);
        virtual ~MapStyleValueDefinition();

        const MapStyleValueDefinition::Class valueClass;
        const MapStyleValueDataType dataType;
        const QString name;
        const bool isComplex;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLE_VALUE_DEFINITION_H_)
