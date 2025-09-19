#include "AtlasMapRendererSymbolsStageModel3D.h"

#include <QPair>

#include "AtlasMapRenderer.h"
#include "AtlasMapRenderer_Metrics.h"
#include "AtlasMapRendererDebugStage.h"

// Should be even: currently there is no use of height in the middle of bbox edge
#define INTERMEDIATE_HEIGHTS 2

OsmAnd::AtlasMapRendererSymbolsStageModel3D::AtlasMapRendererSymbolsStageModel3D(
    AtlasMapRendererSymbolsStage* const symbolsStage_)
    : symbolsStage(symbolsStage_)
    , gpuAPI(symbolsStage_->gpuAPI)
    , currentState(symbolsStage_->currentState)
{
}

OsmAnd::AtlasMapRendererSymbolsStageModel3D::~AtlasMapRendererSymbolsStageModel3D()
{
}

OsmAnd::AtlasMapRenderer* OsmAnd::AtlasMapRendererSymbolsStageModel3D::getRenderer() const
{
    return symbolsStage->getRenderer();
}

glm::mat4 OsmAnd::AtlasMapRendererSymbolsStageModel3D::calculateModelMatrix(
    const Model3D::BBox& bbox,
    const glm::vec3& positionInWorld,
    const float direction,
    const float scale,
    bool& outElevated) const
{
    const auto mScale = glm::scale(glm::vec3(scale));
    const auto directionAngle = static_cast<float>(Utilities::normalizedAngleDegrees(90.0f - direction));
    const auto mDirectionInWorld = glm::rotate(qDegreesToRadians(directionAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    const auto mTranslateToPositionInWorld = glm::translate(positionInWorld);
    const auto mPositionInWorld = mTranslateToPositionInWorld * mDirectionInWorld * mScale;

    const auto center = (mPositionInWorld * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)).xz();
    const auto leftFar = (mPositionInWorld * glm::vec4(bbox.minX, 0.0f, bbox.minZ, 1.0f)).xz();
    const auto rightFar = (mPositionInWorld * glm::vec4(bbox.maxX, 0.0f, bbox.minZ, 1.0f)).xz();
    const auto leftNear = (mPositionInWorld * glm::vec4(bbox.minX, 0.0f, bbox.maxZ, 1.0f)).xz();
    const auto rightNear = (mPositionInWorld * glm::vec4(bbox.maxX, 0.0f, bbox.maxZ, 1.0f)).xz();

    const auto centerElevation = getPointInWorldHeight(center);
    const auto farLeftElevation = getPointInWorldHeight(leftFar);
    const auto farRightElevation = getPointInWorldHeight(rightFar);
    const auto nearLeftElevation = getPointInWorldHeight(leftNear);
    const auto nearRightElevation = getPointInWorldHeight(rightNear);

    const auto leftHeights = getHeightOfPointsOnSegment(leftFar, leftNear, farLeftElevation, nearLeftElevation);
    const auto rightHeights = getHeightOfPointsOnSegment(rightFar, rightNear, farRightElevation, nearRightElevation);
    const auto farHeights = getHeightOfPointsOnSegment(leftFar, rightFar, farLeftElevation, farRightElevation);
    const auto nearHeights = getHeightOfPointsOnSegment(leftNear, rightNear, nearLeftElevation, nearRightElevation);

    const auto lengthX = scale * bbox.lengthX();
    const auto lengthZ = scale * bbox.lengthZ();

    // Replace with sum of points on lines from above 
    const auto avgHeight = (farLeftElevation + farRightElevation + nearLeftElevation + nearRightElevation) / 4.0f;
    const auto height = centerElevation > avgHeight ? (avgHeight + centerElevation) / 2.0f : avgHeight;
    const auto mElevationTranslate = glm::translate(glm::vec3(0.0f, height, 0.0f));

    const auto leftHeightsHalfSize = leftHeights.size() / 2;
    const auto leftHeightsFarHalf = leftHeights.mid(0, leftHeightsHalfSize);
    const auto leftHeightsNearHalf = leftHeights.mid(leftHeights.size() - leftHeightsHalfSize, -1);
    const auto leftHeightsFarHalfSum = std::accumulate(leftHeightsFarHalf.begin(), leftHeightsFarHalf.end(), 0.0f);
    const auto leftHeightsNearHalfSum = std::accumulate(leftHeightsNearHalf.begin(), leftHeightsNearHalf.end(), 0.0f);
    const auto leftHeightsAvgDiff = (leftHeightsFarHalfSum - leftHeightsNearHalfSum) / leftHeightsHalfSize;

    const auto rightHeightsHalfSize = rightHeights.size() / 2;
    const auto rightHeightsFarHalf = rightHeights.mid(0, rightHeightsHalfSize);
    const auto rightHeightsNearHalf = rightHeights.mid(rightHeights.size() - rightHeightsHalfSize, -1);
    const auto rightHeightsFarHalfSum = std::accumulate(rightHeightsFarHalf.begin(), rightHeightsFarHalf.end(), 0.0f);
    const auto rightHeightsNearHalfSum = std::accumulate(rightHeightsNearHalf.begin(), rightHeightsNearHalf.end(), 0.0f);
    const auto rightHeightsAvgDiff = (rightHeightsFarHalfSum - rightHeightsNearHalfSum) / rightHeightsHalfSize;

    const auto avgElevationDiffZ = (leftHeightsAvgDiff + rightHeightsAvgDiff) / 2.0f;
    const auto rotationAngleX = static_cast<float>(qAtan(avgElevationDiffZ / (lengthZ / 2.0f)));
    const auto mRotationX = glm::rotate(rotationAngleX, glm::vec3(1.0f, 0.0f, 0.0f));

    const auto farHeightsHalfSize = farHeights.size() / 2;
    const auto farHeightsLeftHalf = farHeights.mid(0, farHeightsHalfSize);
    const auto farHeightsRightHalf = farHeights.mid(farHeights.size() - farHeightsHalfSize, -1);
    const auto farHeightsLeftHalfSum = std::accumulate(farHeightsLeftHalf.begin(), farHeightsLeftHalf.end(), 0.0f);
    const auto farHeightsRightHalfSum = std::accumulate(farHeightsRightHalf.begin(), farHeightsRightHalf.end(), 0.0f);
    const auto farHeightsAvgDiff = (farHeightsRightHalfSum - farHeightsLeftHalfSum) / farHeightsHalfSize;

    const auto nearHeightsHalfSize = nearHeights.size() / 2;
    const auto nearHeightsLeftHalf = nearHeights.mid(0, nearHeightsHalfSize);
    const auto nearHeightsRightHalf = nearHeights.mid(nearHeights.size() - nearHeightsHalfSize, -1);
    const auto nearHeightsLeftHalfSum = std::accumulate(nearHeightsLeftHalf.begin(), nearHeightsLeftHalf.end(), 0.0f);
    const auto nearHeightsRightHalfSum = std::accumulate(nearHeightsRightHalf.begin(), nearHeightsRightHalf.end(), 0.0f);
    const auto nearHeightsAvgDiff = (nearHeightsRightHalfSum - nearHeightsLeftHalfSum) / nearHeightsHalfSize;

    const auto avgElevationDiffX = (farHeightsAvgDiff + nearHeightsAvgDiff) / 2.0f;
    const auto rotationAngleZ = static_cast<float>(qAtan(avgElevationDiffX / (lengthX / 2.0f)));
    const auto mRotationZ = glm::rotate(rotationAngleZ, glm::vec3(0.0f, 0.0f, 1.0f));

    outElevated = !qFuzzyIsNull(height);

    const auto mRotateOnRelief = mRotationX * mRotationZ;
    return mElevationTranslate * mTranslateToPositionInWorld * mDirectionInWorld * mRotateOnRelief * mScale;
}

QVector<float> OsmAnd::AtlasMapRendererSymbolsStageModel3D::getHeightOfPointsOnSegment(
    const glm::vec2& startInWorld,
    const glm::vec2& endInWorld,
    const float startHeight,
    const float endHeight) const
{
    const auto delta = endInWorld - startInWorld;
    
    QVector<float> heights;
    heights.push_back(startHeight);

    const int segmentsCount = INTERMEDIATE_HEIGHTS + 1;
    for (auto intermediatePoint = 0; intermediatePoint < INTERMEDIATE_HEIGHTS; intermediatePoint++)
    {
        const auto percent = static_cast<float>(intermediatePoint + 1) / segmentsCount;
        const auto postitionInWorld = startInWorld + delta * percent;
        const auto height = getPointInWorldHeight(postitionInWorld);
        heights.push_back(height);
    }

    heights.push_back(endHeight);

    return heights;
}

float OsmAnd::AtlasMapRendererSymbolsStageModel3D::getPointInWorldHeight(const glm::vec2& pointInWorld) const
{
    const auto point31 = getLocation31FromPointInWorld(pointInWorld);
    return getRenderer()->getHeightOfLocation(currentState, point31);
}

OsmAnd::PointI OsmAnd::AtlasMapRendererSymbolsStageModel3D::getLocation31FromPointInWorld(const glm::vec2& pointInWorld_) const
{
    const PointF pointInWorld(pointInWorld_);
    const auto tileSize31 = Utilities::getPowZoom(ZoomLevel::MaxZoomLevel - currentState.zoomLevel);
    const auto offsetFromTarget31 = pointInWorld * tileSize31 / AtlasMapRenderer::TileSize3D;
    return currentState.target31 + static_cast<PointI>(offsetFromTarget31);
}

void OsmAnd::AtlasMapRendererSymbolsStageModel3D::obtainRenderables(
    const std::shared_ptr<const MapSymbolsGroup>& mapSymbolGroup,
    const std::shared_ptr<const Model3DMapSymbol>& model3DMapSymbol,
    const MapRenderer::MapSymbolReferenceOrigins& referenceOrigins,
    std::shared_ptr<AtlasMapRendererSymbolsStage::RenderableSymbol>& outRenderableSymbol,
    const bool allowFastCheckByFrustum /*= true*/,
    AtlasMapRenderer_Metrics::Metric_renderFrame* metric /*= nullptr*/)
{
    const auto& internalState = symbolsStage->getInternalState();

    const auto& originalBBox = model3DMapSymbol->bbox;
    const auto& position31 = model3DMapSymbol->getPosition31();
    const auto& direction = model3DMapSymbol->getDirection();
    
    float scale;
    if (model3DMapSymbol->maxSizeInPixels == 0)
        scale = 1 / currentState.visualZoom;
    else
    {
        scale = model3DMapSymbol->maxSizeInPixels * internalState.pixelInWorldProjectionScale;
        const float preHeight = getRenderer()->getHeightOfLocation(currentState, position31);
        const float cameraHeight = internalState.distanceFromCameraToGround;
        if (cameraHeight > preHeight && !qFuzzyIsNull(cameraHeight))
            scale *= (cameraHeight - preHeight) / cameraHeight;
    }

    const auto offsetFromTarget31 = Utilities::shortestVector31(currentState.target31, position31);
    const auto offsetFromTarget = Utilities::convert31toFloat(offsetFromTarget31, currentState.zoomLevel);
    const auto positionInWord = offsetFromTarget * AtlasMapRenderer::TileSize3D;

    bool modelElevated;
    const auto mModel = calculateModelMatrix(
        originalBBox,
        glm::vec3(positionInWord.x, 0.0f, positionInWord.y),
        direction,
        scale,
        modelElevated);

    // Test against visible frustum area (if allowed)
    if (!symbolsStage->debugSettings->disableSymbolsFastCheckByFrustum
            && allowFastCheckByFrustum
            && model3DMapSymbol->allowFastCheckByFrustum)
    {
        QVector<PointI> modelPoints31(8);
        modelPoints31[0] = getLocation31FromPointInWorld((mModel * glm::vec4(originalBBox.minX, originalBBox.minY, originalBBox.minZ, 1.0f)).xz());
        modelPoints31[1] = getLocation31FromPointInWorld((mModel * glm::vec4(originalBBox.minX, originalBBox.minY, originalBBox.maxZ, 1.0f)).xz());
        modelPoints31[2] = getLocation31FromPointInWorld((mModel * glm::vec4(originalBBox.maxX, originalBBox.minY, originalBBox.maxZ, 1.0f)).xz());
        modelPoints31[3] = getLocation31FromPointInWorld((mModel * glm::vec4(originalBBox.maxX, originalBBox.minY, originalBBox.minZ, 1.0f)).xz());
        modelPoints31[4] = getLocation31FromPointInWorld((mModel * glm::vec4(originalBBox.minX, originalBBox.maxY, originalBBox.minZ, 1.0f)).xz());
        modelPoints31[5] = getLocation31FromPointInWorld((mModel * glm::vec4(originalBBox.minX, originalBBox.maxY, originalBBox.maxZ, 1.0f)).xz());
        modelPoints31[6] = getLocation31FromPointInWorld((mModel * glm::vec4(originalBBox.maxX, originalBBox.maxY, originalBBox.maxZ, 1.0f)).xz());
        modelPoints31[7] = getLocation31FromPointInWorld((mModel * glm::vec4(originalBBox.maxX, originalBBox.maxY, originalBBox.minZ, 1.0f)).xz());

        if (modelElevated)
        {
            PointI elevatedModelPoint31;
            for (auto& modelPoint31 : modelPoints31)
            {
                const auto modelPointHeight = getRenderer()->getHeightOfLocation(currentState, modelPoint31);
                if (getRenderer()->getProjectedLocation(
                    internalState, currentState, modelPoint31, modelPointHeight, elevatedModelPoint31))
                {
                    modelPoint31 = elevatedModelPoint31;
                }
            }
        }

        typedef QPair<PointI, PointI> Edge;
        QVector<Edge> bboxEdges31(12);
        auto pBBoxEdges31 = bboxEdges31.data();
        for (int i = 0; i < 4; i++)
        {
            *(pBBoxEdges31++) = Edge(modelPoints31[i], modelPoints31[i + 4]);
            *(pBBoxEdges31++) = Edge(modelPoints31[i], modelPoints31[(i + 1) % 4]);
            *(pBBoxEdges31++) = Edge(modelPoints31[i + 4], modelPoints31[(i + 1) % 4 + 4]);
        }

        bool tested = false;
        for (const auto& edge31 : bboxEdges31)
        {
            const auto& p1 = edge31.first;
            const auto& p2 = edge31.second;
            if (internalState.globalFrustum2D31.test(p1, p2))
            {
                tested = true;
                break;
            }
        }

        if (!tested)
        {
            if (metric)
                metric->onSurfaceSymbolsRejectedByFrustum++;
            return;
        }
    }

    // Get GPU resource
    const auto gpuResource = symbolsStage->captureGpuResource(referenceOrigins, model3DMapSymbol);
    if (!gpuResource)
        return;

    if (const auto& gpuMeshResource = std::dynamic_pointer_cast<const GPUAPI::MeshInGPU>(gpuResource))
    {
        // Position31 must not be set, as renderable position is mutable
        assert(gpuMeshResource->position31 == nullptr);
    }

    // Don't render fully transparent symbols
    const auto opacityFactor = symbolsStage->getSubsectionOpacityFactor(model3DMapSymbol);
    if (opacityFactor > 0.0f)
    {
        std::shared_ptr<RenderableModel3DSymbol> renderable(new RenderableModel3DSymbol());
        renderable->mapSymbolGroup = mapSymbolGroup;
        renderable->mapSymbol = model3DMapSymbol;
        renderable->referenceOrigins = const_cast<MapRenderer::MapSymbolReferenceOrigins*>(&referenceOrigins);
        renderable->gpuResource = gpuResource;
        renderable->opacityFactor = opacityFactor;
        renderable->mModel = mModel;
        outRenderableSymbol = renderable;
    }
}

bool OsmAnd::AtlasMapRendererSymbolsStageModel3D::plotSymbol(
    const std::shared_ptr<RenderableModel3DSymbol>& renderable,
    ScreenQuadTree& intersections,
    const bool applyFiltering /*= true*/,
    AtlasMapRenderer_Metrics::Metric_renderFrame* metric /*= nullptr*/)
{
    return true;
}