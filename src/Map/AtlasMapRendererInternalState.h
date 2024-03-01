#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_INTERNAL_STATE_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_INTERNAL_STATE_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QVector>
#include <QMap>
#include "restore_internal_warnings.h"

#include <glm/glm.hpp>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "MapRendererInternalState.h"
#include "Frustum2D.h"
#include "Frustum2D31.h"

namespace OsmAnd
{
    struct AtlasMapRendererInternalState : public MapRendererInternalState
    {
        AtlasMapRendererInternalState();
        virtual ~AtlasMapRendererInternalState();

        TileId targetTileId;
        PointF targetInTileOffsetN;
        int zoomLevelOffset;
        int visibleTilesCount;
        QMap<ZoomLevel, QVector<TileId>> visibleTiles;
        QMap<ZoomLevel, QSet<TileId>> visibleTilesSet;
        QMap<ZoomLevel, QVector<TileId>> frustumTiles;
        QMap<ZoomLevel, QVector<TileId>> uniqueTiles;
        QMap<ZoomLevel, TileId> uniqueTilesTargets;


        glm::vec4 glmViewport;
        glm::mat4 mOrthographicProjection;
        glm::mat4 mPerspectiveProjection;
        glm::mat4 mCameraView;
        glm::mat4 mDistance;
        glm::mat4 mElevation;
        glm::mat4 mAzimuth;
        glm::mat4 mPerspectiveProjectionInv;
        glm::mat4 mCameraViewInv;
        glm::mat4 mDistanceInv;
        glm::mat4 mElevationInv;
        glm::mat4 mAzimuthInv;
        glm::vec2 groundCameraPosition;
        glm::vec3 worldCameraPosition;
        glm::mat4 mPerspectiveProjectionView;
        float zNear;
        float zFar;
        float projectionPlaneHalfHeight;
        float projectionPlaneHalfWidth;
        float aspectRatio;
        float fovInRadians;
        float referenceTileSizeOnScreenInPixels;
        float distanceFromCameraToTarget;
        float groundDistanceFromCameraToTarget;
        float distanceFromCameraToGround;
        float distanceFromCameraToFog;
        float distanceFromTargetToFog;
        float fogShiftFactor;
        float skyHeightInKilometers;
        float skyShift;
        float skyLine;
        float zLowerDetail;
        double distanceFromCameraToGroundInMeters;        
        double metersPerUnit;
        LatLon cameraCoordinates;
        float tileOnScreenScaleFactor;
        float scaleToRetainProjectedSize;
        float pixelInWorldProjectionScale;
        float sizeOfPixelInWorld;
        PointF skyplaneSize;
        PointF leftMiddlePoint;
        PointF rightMiddlePoint;
        Frustum2DF frustum2D;
        Frustum2D31 frustum2D31;
        Frustum2D31 globalFrustum2D31;
        Frustum2DF extraField2D;
        Frustum2D31 extraFrustum2D31;
        glm::vec3 topVisibleEdgeN;
        glm::vec3 leftVisibleEdgeN;
        glm::vec3 bottomVisibleEdgeN;
        glm::vec3 rightVisibleEdgeN;
        glm::vec3 frontVisibleEdgeN;
        glm::vec3 backVisibleEdgeN;
        glm::vec3 frontVisibleEdgeP;
        glm::vec3 backVisibleEdgeTL;
        glm::vec3 backVisibleEdgeTR;
        glm::vec3 backVisibleEdgeBL;
        glm::vec3 backVisibleEdgeBR;
        float topVisibleEdgeD;
        float leftVisibleEdgeD;
        float bottomVisibleEdgeD;
        float rightVisibleEdgeD;
        float frontVisibleEdgeD;
        float backVisibleEdgeD;
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_INTERNAL_STATE_H_)
