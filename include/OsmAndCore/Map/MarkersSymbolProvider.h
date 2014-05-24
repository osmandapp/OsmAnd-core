#ifndef _OSMAND_CORE_MARKERS_SYMBOL_PROVIDER_H_
#define _OSMAND_CORE_MARKERS_SYMBOL_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QMultiHash>
#include <QList>

#include <SkBitmap.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapSymbolKeyedProvider.h>

namespace OsmAnd
{
    class MarkersSymbolProvider_P;
    class OSMAND_CORE_API MarkersSymbolProvider : public IMapSymbolKeyedProvider
    {
        Q_DISABLE_COPY(MarkersSymbolProvider);

    public:
        class OSMAND_CORE_API Marker
        {
        private:
        protected:
            Marker();
        public:
            virtual ~Marker();

            //TODO: caption lines + font settings
            //TODO: pin icon
            //TODO: area icon
            //TODO: location
            //TODO: heading

            // Area of the marker (radius is measured in meters):
            //double areaRadius;
            //SkColor areaBaseColor;

            //TODO: apply();
        };

    private:
        PrivateImplementation<MarkersSymbolProvider_P> _p;
    protected:
    public:
        MarkersSymbolProvider();
        virtual ~MarkersSymbolProvider();

        std::shared_ptr<Marker> createMarker(
            const QString& name,
            const PointI location31,
            const float azimuth = 0.0f,
            const double areaRadius = 0.0,
            const SkColor areaBaseColor = SK_ColorTRANSPARENT,
            const std::shared_ptr<const SkBitmap>& pinIcon = nullptr,
            const std::shared_ptr<const SkBitmap>& surfaceIcon = nullptr
            //TODO: add caption text with font settings for rasterizer
            );

        QList< std::shared_ptr<Marker> > getMarkersWithName(const QString& name) const;
        QMultiHash< QString, std::shared_ptr<Marker> > getMarkers() const;
        
        bool removeMarker(const std::shared_ptr<Marker>& marker);
        int removeMarkersWithName(const QString& name);
        int removeAllMarkers();

        virtual QSet<Key> getKeys() const;

        virtual bool obtainSymbols(
            QList< std::shared_ptr<const MapSymbolsGroup> >& outSymbolGroups,
            const FilterCallback filterCallback = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_MARKERS_SYMBOL_PROVIDER_H_)
