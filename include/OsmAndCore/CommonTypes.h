#ifndef _OSMAND_CORE_COMMON_TYPES_H_
#define _OSMAND_CORE_COMMON_TYPES_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <QtGlobal>
#include <QtMath>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/QuickAccessors.h>

namespace OsmAnd
{
    union TileId
    {
        uint64_t id;
#if !defined(SWIG)
        struct {
            int32_t x;
            int32_t y;
        };
#else
        // Fake unwrap for SWIG
        int32_t x, y;
#endif // !defined(SWIG)

#if !defined(SWIG)
        inline operator uint64_t() const
        {
            return id;
        }

        inline TileId& operator=(const uint64_t& that)
        {
            id = that;
            return *this;
        }

        inline bool operator==(const TileId& that)
        {
            return this->id == that.id;
        }

        inline bool operator!=(const TileId& that)
        {
            return this->id != that.id;
        }

        inline bool operator==(const uint64_t& that)
        {
            return this->id == that;
        }

        inline bool operator!=(const uint64_t& that)
        {
            return this->id != that;
        }
#endif // !defined(SWIG)

        static inline TileId fromXY(const int32_t x, const int32_t y)
        {
            TileId tileId;
            tileId.x = x;
            tileId.y = y;
            return tileId;
        }

        static inline TileId zero()
        {
            TileId tileId;
            tileId.x = 0;
            tileId.y = 0;
            return tileId;
        }
    };

#if !defined(SWIG)
    static_assert(sizeof(TileId) == 8, "TileId must be 8 bytes in size");
#endif // !defined(SWIG)

    enum ZoomLevel : int32_t
    {
        ZoomLevel0 = 0,
        ZoomLevel1,
        ZoomLevel2,
        ZoomLevel3,
        ZoomLevel4,
        ZoomLevel5,
        ZoomLevel6,
        ZoomLevel7,
        ZoomLevel8,
        ZoomLevel9,
        ZoomLevel10,
        ZoomLevel11,
        ZoomLevel12,
        ZoomLevel13,
        ZoomLevel14,
        ZoomLevel15,
        ZoomLevel16,
        ZoomLevel17,
        ZoomLevel18,
        ZoomLevel19,
        ZoomLevel20,
        ZoomLevel21,
        ZoomLevel22,
        ZoomLevel23,
        ZoomLevel24,
        ZoomLevel25,
        ZoomLevel26,
        ZoomLevel27,
        ZoomLevel28,
        ZoomLevel29,
        ZoomLevel30,
        ZoomLevel31 = 31,

        InvalidZoomLevel = -1,
        MinZoomLevel = ZoomLevel0,
        MaxZoomLevel = ZoomLevel31,
    };
    enum {
        ZoomLevelsCount = static_cast<unsigned>(ZoomLevel::MaxZoomLevel) + 1u
    };

    typedef std::function<bool(const TileId tileId, const ZoomLevel zoomLevel)> TileAcceptorFunction;

    enum class LanguageId
    {
        Invariant = -1,

        Localized,
        Native
    };
    
    enum class StringMatcherMode
    {
        CHECK_ONLY_STARTS_WITH,
        CHECK_STARTS_FROM_SPACE,
        CHECK_STARTS_FROM_SPACE_NOT_BEGINNING,
        CHECK_EQUALS_FROM_SPACE,
        CHECK_CONTAINS,
        CHECK_EQUALS
    };

    const ZoomLevel MAX_BASEMAP_ZOOM_LEVEL = ZoomLevel11;
    const ZoomLevel MIN_DETAILED_ZOOM_LEVEL = ZoomLevel14;
}

#endif // !defined(_OSMAND_CORE_COMMON_TYPES_H_)
