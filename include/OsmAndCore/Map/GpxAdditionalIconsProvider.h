#ifndef _OSMAND_CORE_GPX_ADDITIONAL_ICONS_PROVIDER_H_
#define _OSMAND_CORE_GPX_ADDITIONAL_ICONS_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QHash>
#include <QList>
#include <OsmAndCore/restore_internal_warnings.h>

//#include <SkImage.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Nullable.h>
#include <OsmAndCore/IObfsCollection.h>
#include <OsmAndCore/Data/ObfPoiSectionReader.h>
#include <OsmAndCore/Map/IMapTiledSymbolsProvider.h>
#include <OsmAndCore/Map/MapSymbolsGroup.h>
#include <OsmAndCore/Map/IAmenityIconProvider.h>
#include <OsmAndCore/Map/CoreResourcesAmenityIconProvider.h>

namespace OsmAnd
{
    class OSMAND_CORE_API GpxAdditionalIconsProvider : public IMapTiledSymbolsProvider
    {
        Q_DISABLE_COPY_AND_MOVE(GpxAdditionalIconsProvider);
    public:
        class OSMAND_CORE_API Data : public IMapTiledSymbolsProvider::Data
        {
            Q_DISABLE_COPY_AND_MOVE(Data);
        private:
        protected:
        public:
            Data(const TileId tileId,
                 const ZoomLevel zoom,
                 const QList<std::shared_ptr<MapSymbolsGroup>>& symbolsGroups,
                 const RetainableCacheMetadata* const pRetainableCacheMetadata = nullptr);
            virtual ~Data();
        };
        
        class OSMAND_CORE_API SplitLabel Q_DECL_FINAL
        {
        public:
            SplitLabel(
                const PointI& pos31,
                const QString& text,
                const ColorARGB& gpxColor);
            virtual ~SplitLabel();
            
            PointI pos31;
            QString text;
            ColorARGB gpxColor;
        };

    private:
        QReadWriteLock _lock;
        QList<std::shared_ptr<MapSymbolsGroup>> buildMapSymbolsGroups(const AreaI &bbox31, const double metersPerPixel);
        
        ZoomLevel _cachedZoomLevel;
        QList<SplitLabel> _visibleSplitLabels;
        
        const std::shared_ptr<const TextRasterizer> _textRasterizer;
        TextRasterizer::Style _captionStyle;
            
        void buildStartFinishSymbolsGroup(
            const AreaI &bbox31,
            double metersPerPixel,
            QList<std::shared_ptr<MapSymbolsGroup>>& mapSymbolsGroups);
        void buildSplitIntervalsSymbolsGroup(
            const AreaI& bbox31,
            const QList<SplitLabel>& labels,
            QList<std::shared_ptr<MapSymbolsGroup>>& mapSymbolsGroups);
        
        void buildVisibleSplits(const double metersPerPixel, QList<SplitLabel>& visibleSplits);
        
        sk_sp<SkImage> getSplitIconForValue(const SplitLabel& label);
                                    
    protected:
    public:
        GpxAdditionalIconsProvider(
            const int baseOrder,
            const double screenScale,
            const QList<PointI>& startFinishPoints,
            const QList<SplitLabel>& splitLabels,
            const sk_sp<const SkImage>& startIcon,
            const sk_sp<const SkImage>& finishIcon,
            const sk_sp<const SkImage>& startFinishIcon);
        virtual ~GpxAdditionalIconsProvider();
        
        const int baseOrder;
        const double screenScale;

        const QList<PointI> startFinishPoints;
        const QList<SplitLabel> splitLabels;
        
        const sk_sp<const SkImage> startIcon;
        const sk_sp<const SkImage> finishIcon;
        const sk_sp<const SkImage> startFinishIcon;
        
        virtual OsmAnd::ZoomLevel getMinZoom() const Q_DECL_OVERRIDE;
        virtual OsmAnd::ZoomLevel getMaxZoom() const Q_DECL_OVERRIDE;
        
        virtual bool supportsNaturalObtainData() const Q_DECL_OVERRIDE;
        
        virtual bool obtainData(const IMapDataProvider::Request& request,
                                std::shared_ptr<IMapDataProvider::Data>& outData,
                                std::shared_ptr<Metric>* const pOutMetric = nullptr) Q_DECL_OVERRIDE;
        
        virtual bool supportsNaturalObtainDataAsync() const Q_DECL_OVERRIDE;
        virtual void obtainDataAsync(const IMapDataProvider::Request& request,
                                     const IMapDataProvider::ObtainDataAsyncCallback callback,
                                     const bool collectMetric = false) Q_DECL_OVERRIDE;
    };
}
#endif // !defined(_OSMAND_CORE_GPX_ADDITIONAL_ICONS_PROVIDER_H_)
