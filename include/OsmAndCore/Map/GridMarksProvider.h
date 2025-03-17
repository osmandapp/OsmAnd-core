#ifndef _OSMAND_CORE_GRID_MARKS_PROVIDER_H_
#define _OSMAND_CORE_GRID_MARKS_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapKeyedSymbolsProvider.h>
#include <OsmAndCore/TextRasterizer.h>

namespace OsmAnd
{
    class GridMarksProvider_P;
    class OSMAND_CORE_API GridMarksProvider
        : public std::enable_shared_from_this<GridMarksProvider>
        , public IMapKeyedSymbolsProvider
    {
        Q_DISABLE_COPY_AND_MOVE(GridMarksProvider);

    private:
        PrivateImplementation<GridMarksProvider_P> _p;
    protected:
    public:
        GridMarksProvider();
        virtual ~GridMarksProvider();

        void setPrimaryStyle(
            const TextRasterizer::Style& style, const float offset, bool center = false);
        void setSecondaryStyle(
            const TextRasterizer::Style& style, const float offset, bool center = false);

        void setPrimary(const bool withValues, const QString& northernSuffix,
            const QString& southernSuffix, const QString& easternSuffix, const QString& westernSuffix);
        void setSecondary(const bool withValues, const QString& northernSuffix,
            const QString& southernSuffix, const QString& easternSuffix, const QString& westernSuffix);
   
        virtual void applyMapChanges(IMapRenderer* renderer) Q_DECL_OVERRIDE;

        virtual QList<IMapKeyedSymbolsProvider::Key> getProvidedDataKeys() const Q_DECL_OVERRIDE;

        virtual bool supportsNaturalObtainData() const Q_DECL_OVERRIDE;
        virtual bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric = nullptr) Q_DECL_OVERRIDE;

        virtual bool supportsNaturalObtainDataAsync() const Q_DECL_OVERRIDE;
        virtual void obtainDataAsync(
            const IMapDataProvider::Request& request,
            const IMapDataProvider::ObtainDataAsyncCallback callback,
            const bool collectMetric = false) Q_DECL_OVERRIDE;
        
        virtual ZoomLevel getMinZoom() const Q_DECL_OVERRIDE;
        virtual ZoomLevel getMaxZoom() const Q_DECL_OVERRIDE;

        virtual int64_t getPriority() const Q_DECL_OVERRIDE;
    };
}

#endif // !defined(_OSMAND_CORE_GRID_MARKS_PROVIDER_H_)
