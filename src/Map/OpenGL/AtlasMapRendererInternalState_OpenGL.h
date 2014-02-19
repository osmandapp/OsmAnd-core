#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_INTERNAL_STATE_OPENGL_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_INTERNAL_STATE_OPENGL_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QList>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "AtlasMapRendererInternalState.h"
#include <glm/glm.hpp>
#include "Frustum2D.h"
#include "Frustum2D31.h"

namespace OsmAnd
{
    struct AtlasMapRendererInternalState_OpenGL : public AtlasMapRendererInternalState
    {
        virtual ~AtlasMapRendererInternalState_OpenGL();

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
        float zSkyplane;
        float zFar;
        float projectionPlaneHalfHeight;
        float projectionPlaneHalfWidth;
        float aspectRatio;
        float fovInRadians;
        float screenTileSize;
        float nearDistanceFromCameraToTarget;
        float baseDistanceFromCameraToTarget;
        float farDistanceFromCameraToTarget;
        float distanceFromCameraToTarget;
        float groundDistanceFromCameraToTarget;
        float tileScaleFactor;
        float scaleToRetainProjectedSize;
        PointF skyplaneSize;
        float correctedFogDistance;
        Frustum2D frustum2D;
        Frustum2D31 frustum2D31;
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_INTERNAL_STATE_OPENGL_H_)
