#ifndef _OSMAND_CORE_MAP_TILED_COLLECTION_POINT_H_
#define _OSMAND_CORE_MAP_TILED_COLLECTION_POINT_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QList>
#include <OsmAndCore/restore_internal_warnings.h>

#include <SkImage.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/CommonSWIG.h>

namespace OsmAnd
{
    class OSMAND_CORE_API MapTiledCollectionPoint
    {
        Q_DISABLE_COPY_AND_MOVE(MapTiledCollectionPoint);
        
    protected:
        MapTiledCollectionPoint();
    public:
        virtual ~MapTiledCollectionPoint();
        
        virtual OsmAnd::PointI getPoint31() const = 0;
        virtual sk_sp<const SkImage> getImageBitmap(bool isFullSize = true) const = 0;
        virtual QString getCaption() const = 0;
    };
    
    SWIG_EMIT_DIRECTOR_BEGIN(MapTiledCollectionPoint);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            OsmAnd::PointI,
            getPoint31);
        SWIG_EMIT_DIRECTOR_CONST_METHOD(
            sk_sp<const SkImage>,
            getImageBitmap,
            bool isFullSize);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            QString,
            getCaption);
    SWIG_EMIT_DIRECTOR_END(MapTiledCollectionPoint);
}

#endif // !defined(_OSMAND_CORE_MAP_TILED_COLLECTION_POINT_H_)
