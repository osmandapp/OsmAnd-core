#ifndef _OSMAND_CORE_MARKERS_SYMBOL_PROVIDER_P_H_
#define _OSMAND_CORE_MARKERS_SYMBOL_PROVIDER_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include <QMultiHash>
#include <QList>
#include <QReadWriteLock>

#include <SkBitmap.h>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "CommonTypes.h"
#include "IMapSymbolCollectionProvider.h"
#include "MarkersSymbolProvider.h"

namespace OsmAnd
{
    class MarkersSymbolProvider;
    class MarkersSymbolProvider_P
    {
        Q_DISABLE_COPY(MarkersSymbolProvider_P);

    public:
        typedef MarkersSymbolProvider::Marker Marker;
        typedef MarkersSymbolProvider::FilterCallback FilterCallback;

    private:
    protected:
        MarkersSymbolProvider_P(MarkersSymbolProvider* const owner);

        mutable QReadWriteLock _markersLock;
        QMultiHash< QString, std::shared_ptr<Marker> > _markers;
    public:
        virtual ~MarkersSymbolProvider_P();

        ImplementationInterface<MarkersSymbolProvider> owner;

        std::shared_ptr<Marker> createMarker(
            const QString& name,
            const PointI location31,
            const float azimuth,
            const double areaRadius,
            const SkColor areaBaseColor,
            const std::shared_ptr<const SkBitmap>& pinIcon,
            const std::shared_ptr<const SkBitmap>& surfaceIcon
            //TODO: add caption text with font settings for rasterizer
            );

        QList< std::shared_ptr<Marker> > getMarkersWithName(const QString& name) const;
        QMultiHash< QString, std::shared_ptr<Marker> > getMarkers() const;

        bool removeMarker(const std::shared_ptr<Marker>& marker);
        int removeMarkersWithName(const QString& name);
        int removeAllMarkers();

        virtual bool obtainSymbols(
            QList< std::shared_ptr<const MapSymbolsGroup> >& outSymbolGroups,
            const FilterCallback filterCallback);

    friend class OsmAnd::MarkersSymbolProvider;
    };
}

#endif // !defined(_OSMAND_CORE_MARKERS_SYMBOL_PROVIDER_P_H_)
