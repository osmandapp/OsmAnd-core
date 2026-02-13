#ifndef _OSMAND_CORE_MAP_MARKER_P_H_
#define _OSMAND_CORE_MAP_MARKER_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QReadWriteLock>
#include <QHash>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "CommonTypes.h"
#include "MapSymbolsGroup.h"
#include "IUpdatableMapSymbolsGroup.h"
#include "OnSurfaceRasterMapSymbol.h"
#include "OnSurfaceVectorMapSymbol.h"
#include "Model3DMapSymbol.h"
#include "MapMarker.h"

namespace OsmAnd
{
    class MapMarkersCollection;
    class MapMarkersCollection_P;

    class MapMarker;
    class MapMarker_P : public std::enable_shared_from_this<MapMarker_P>
    {
        Q_DISABLE_COPY_AND_MOVE(MapMarker_P);

    public:
        typedef MapMarker::PinIconVerticalAlignment PinIconVerticalAlignment;
        typedef MapMarker::PinIconHorisontalAlignment PinIconHorisontalAlignment;

    private:
        std::shared_ptr<const TextRasterizer> textRasterizer;
    protected:
        MapMarker_P(MapMarker* const owner);

        mutable QReadWriteLock _lock;
        bool _ownerIsLost;
        bool _hasUnappliedChanges;
        bool _hasUnappliedPrimitiveChanges;

        bool _isHidden;

        bool _isAccuracyCircleVisible;
        double _additionalPositionParameter;
        PositionType _positionType;
        PointI _position;

        float _height;

        float _elevationScaleFactor;

        bool _adjustElevationToVectorObject;

        QHash< MapMarker::OnSurfaceIconKey, float > _directions;

        int _model3DMaxSizeInPixels;
        float _model3DDirection;

        ColorARGB _pinIconModulationColor;
        ColorARGB _onSurfaceIconModulationColor;
        
        QVector<PointI64> _linePoints;
        int _offsetFromLine = 0;

        bool _updateAfterCreated = false;

        QString _caption;
        TextRasterizer::Style _captionStyle;

        class KeyedOnSurfaceRasterMapSymbol : public OnSurfaceRasterMapSymbol
        {
        private:
        protected:
            KeyedOnSurfaceRasterMapSymbol(
                const MapMarker::OnSurfaceIconKey key,
                const std::shared_ptr<MapSymbolsGroup>& group);
        public:
            virtual ~KeyedOnSurfaceRasterMapSymbol();

            const MapMarker::OnSurfaceIconKey key;

        friend class OsmAnd::MapMarker_P;
        };

        bool applyChanges();

        std::shared_ptr<MapMarker::SymbolsGroup> inflateSymbolsGroup(int subsection) const;
        mutable QReadWriteLock _symbolsGroupsRegistryLock;
        mutable QHash< MapSymbolsGroup*, std::weak_ptr< MapSymbolsGroup > > _symbolsGroupsRegistry;
        void registerSymbolsGroup(const std::shared_ptr<MapSymbolsGroup>& symbolsGroup) const;
        void unregisterSymbolsGroup(MapSymbolsGroup* const symbolsGroup) const;
    public:
        virtual ~MapMarker_P();

        ImplementationInterface<MapMarker> owner;

        bool isHidden() const;
        void setIsHidden(const bool hidden);

        bool isAccuracyCircleVisible() const;
        void setIsAccuracyCircleVisible(const bool visible);

        PointI getPosition() const;
        void setPosition(const PointI position);

        PositionType getPositionType() const;
        void setPositionType(const PositionType positionType);

        double getAdditionalPosition() const;
        void setAdditionalPosition(const double additionalPosition);

        float getHeight() const;
        void setHeight(const float height);

        float getElevationScaleFactor() const;
        void setElevationScaleFactor(const float scaleFactor);

        bool getAdjustElevationToVectorObject() const;
        void setAdjustElevationToVectorObject(const bool adjust);

        float getOnMapSurfaceIconDirection(const MapMarker::OnSurfaceIconKey key) const;
        void setOnMapSurfaceIconDirection(const MapMarker::OnSurfaceIconKey key, const float direction);

        int getModel3DMaxSizeInPixels() const;
        void setModel3DMaxSizeInPixels(const int maxSizeInPixels);
        float getModel3DDirection() const;
        void setModel3DDirection(const float direction);

        ColorARGB getPinIconModulationColor() const;
        void setPinIconModulationColor(const ColorARGB colorValue);
        
        ColorARGB getOnSurfaceIconModulationColor() const;
        void setOnSurfaceIconModulationColor(const ColorARGB colorValue);
        
        void attachToVectorLine(const QVector<PointI64>& segmentPoints);
        void attachToVectorLine(QVector<PointI64>&& points);
        
        void setOffsetFromLine(int offset);

        void setUpdateAfterCreated(bool updateAfterCreated);

        void setCaption(const QString& caption);
        void setCaptionStyle(const TextRasterizer::Style& captionStyle);

        void setOwnerIsLost();
        bool hasUnappliedChanges() const;
        bool hasUnappliedPrimitiveChanges() const;
        
        std::shared_ptr<MapMarker::SymbolsGroup> createSymbolsGroup(int subsection) const;

    friend class OsmAnd::MapMarker;
    friend class OsmAnd::MapMarkersCollection;
    friend class OsmAnd::MapMarkersCollection_P;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_MARKER_P_H_)
