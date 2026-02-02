#ifndef _OSMAND_CORE_VECTOR_LINE_ARROWS_PROVIDER_P_H_
#define _OSMAND_CORE_VECTOR_LINE_ARROWS_PROVIDER_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include <QHash>
#include <QList>
#include <QReadWriteLock>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "CommonTypes.h"
#include "IMapKeyedSymbolsProvider.h"
#include "VectorLinesCollection.h"
#include "MapMarkersCollection.h"
#include "MapMarker.h"
#include "MapMarkerBuilder.h"

namespace OsmAnd
{
    class VectorLineArrowsProvider;
    class VectorLineArrowsProvider_P Q_DECL_FINAL
		: public std::enable_shared_from_this<VectorLineArrowsProvider_P>
    {
        Q_DISABLE_COPY_AND_MOVE(VectorLineArrowsProvider_P);

    private:
        std::shared_ptr<VectorLinesCollection> _linesCollection;

        mutable QReadWriteLock _markersLock;
        std::shared_ptr<MapMarkersCollection> _markersCollection;

        QList<std::shared_ptr<MapMarker>> getLineMarkers(int lineId) const;

        void rebuildArrows();
        
    protected:
        VectorLineArrowsProvider_P(
            VectorLineArrowsProvider* const owner,
            const std::shared_ptr<VectorLinesCollection>& collection);

        void init();

    public:
        virtual ~VectorLineArrowsProvider_P();

        ImplementationInterface<VectorLineArrowsProvider> owner;

        QList<IMapKeyedSymbolsProvider::Key> getProvidedDataKeys() const;
        bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData);

        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom() const;

    friend class OsmAnd::VectorLineArrowsProvider;
    };
}

#endif // !defined(_OSMAND_CORE_VECTOR_LINE_ARROWS_PROVIDER_P_H_)
