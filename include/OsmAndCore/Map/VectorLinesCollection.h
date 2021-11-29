#ifndef _OSMAND_CORE_VECTOR_LINES_COLLECTION_H_
#define _OSMAND_CORE_VECTOR_LINES_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QVector>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapKeyedSymbolsProvider.h>
#include <OsmAndCore/Map/VectorLine.h>

namespace OsmAnd
{
    class VectorLineBuilder;
    class VectorLineBuilder_P;

    class VectorLinesCollection_P;
    class OSMAND_CORE_API VectorLinesCollection : public IMapKeyedSymbolsProvider
    {
        Q_DISABLE_COPY_AND_MOVE(VectorLinesCollection);

    private:
        PrivateImplementation<VectorLinesCollection_P> _p;
    protected:
    public:
        VectorLinesCollection();
        virtual ~VectorLinesCollection();

        QList< std::shared_ptr<VectorLine> > getLines() const;
        bool removeLine(const std::shared_ptr<VectorLine>& line);
        void removeAllLines();

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

    friend class OsmAnd::VectorLineBuilder;
    friend class OsmAnd::VectorLineBuilder_P;
    };
}

#endif // !defined(_OSMAND_CORE_VECTOR_LINES_COLLECTION_H_)
