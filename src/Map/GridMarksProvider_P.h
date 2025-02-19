#ifndef _OSMAND_CORE_GRID_MARKS_PROVIDER_P_H_
#define _OSMAND_CORE_GRID_MARKS_PROVIDER_P_H_

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
#include "MapMarkersCollection.h"
#include "MapMarker.h"
#include "MapMarkerBuilder.h"
#include "MapRendererState.h"
#include "TextRasterizer.h"

namespace OsmAnd
{
    class GridMarksProvider;
    class GridMarksProvider_P Q_DECL_FINAL
		: public std::enable_shared_from_this<GridMarksProvider_P>
    {
        Q_DISABLE_COPY_AND_MOVE(GridMarksProvider_P);

    private:
        mutable QReadWriteLock _markersLock;
        std::shared_ptr<MapMarkersCollection> _markersCollection;

        GridConfiguration _gridConfiguration;
        ZoomLevel _mapZoomLevel;
        AreaI _visibleBBoxShifted;
        int _primaryZone;
        int _secondaryZone;

        TextRasterizer::Style primaryStyle;
        TextRasterizer::Style secondaryStyle;

        bool withPrimaryValues;
        float primaryMarksOffset;
        QString primaryNorthernHemisphereSuffix;
        QString primarySouthernHemisphereSuffix;
        QString primaryEasternHemisphereSuffix;
        QString primaryWesternHemisphereSuffix;

        bool withSecondaryValues;
        float secondaryMarksOffset;
        QString secondaryNorthernHemisphereSuffix;
        QString secondarySouthernHemisphereSuffix;
        QString secondaryEasternHemisphereSuffix;
        QString secondaryWesternHemisphereSuffix;

        void calculateGridMarks(const PointI& target31, const bool isPrimary,
            const double gap, const double refLon, QHash<int, PointD>& marksX, QHash<int, PointD>& marksY);

        void addGridMarks(const PointI& target31, const int zone, const bool isPrimary, const bool isAxisY,
            const float offset, QHash<int, PointD>& marks, QSet<int>& availableIds);
    
        void setPrimaryStyle(const TextRasterizer::Style& style, const float offset);
        void setSecondaryStyle(const TextRasterizer::Style& style, const float offset);
    
        void setPrimary(const bool withValues, const QString& northernSuffix,
            const QString& southernSuffix, const QString& easternSuffix, const QString& westernSuffix);

        void setSecondary(const bool withValues, const QString& northernSuffix,
            const QString& southernSuffix, const QString& easternSuffix, const QString& westernSuffix);
    
        void applyMapChanges(IMapRenderer* renderer);

        protected:
        GridMarksProvider_P(GridMarksProvider* const owner);

    public:
        virtual ~GridMarksProvider_P();

        ImplementationInterface<GridMarksProvider> owner;

        QList<IMapKeyedSymbolsProvider::Key> getProvidedDataKeys() const;
        bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData);

        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom() const;

    friend class OsmAnd::GridMarksProvider;
    };
}

#endif // !defined(_OSMAND_CORE_GRID_MARKS_PROVIDER_P_H_)
