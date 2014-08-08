#ifndef _OSMAND_CORE_MAP_RENDERER_DEBUG_SETTINGS_H_
#define _OSMAND_CORE_MAP_RENDERER_DEBUG_SETTINGS_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    struct OSMAND_CORE_API MapRendererDebugSettings
    {
        MapRendererDebugSettings();
        virtual ~MapRendererDebugSettings();

        bool debugStageEnabled;
        bool skipSymbolsIntersectionCheck;
        bool showSymbolsBBoxesRejectedByIntersectionCheck;
        bool showSymbolsBBoxesAcceptedByIntersectionCheck;
        
        virtual void copyTo(MapRendererDebugSettings& other) const;
        virtual std::shared_ptr<MapRendererDebugSettings> createCopy() const;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_DEBUG_SETTINGS_H_)
