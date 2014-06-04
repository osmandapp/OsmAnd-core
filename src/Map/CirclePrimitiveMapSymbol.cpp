#include "CirclePrimitiveMapSymbol.h"

OsmAnd::CirclePrimitiveMapSymbol::CirclePrimitiveMapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_,
    const bool isShareable_,
    const int order_,
    const IntersectionModeFlags intersectionModeFlags_)
    : PrimitiveMapSymbol(group_, isShareable_, order_, intersectionModeFlags_)
{
    primitiveType = PrimitiveType::TriangleFan;
}

OsmAnd::CirclePrimitiveMapSymbol::~CirclePrimitiveMapSymbol()
{
}

void OsmAnd::CirclePrimitiveMapSymbol::generate(
    const FColorARGB color /*= FColorARGB(1.0f, 1.0f, 1.0f, 1.0f)*/,
    const unsigned int pointsCount /*= 360*/,
    float radius /*= 1.0f*/)
{
    releaseVerticesAndIndices();

    // Circle has no reusable vertices, because it's rendered as triangle-fan,
    // so there's no indices
    indices = nullptr;
    indicesCount = 0;

    // Allocate space for pointsCount+2 vertices
    vertices = new Vertex[pointsCount + 2];
    auto pVertex = vertices;

    // First vertex is the center
    pVertex->positionXY[0] = 0.0f;
    pVertex->positionXY[1] = 0.0f;
    pVertex->color = color;
    pVertex += 1;
    
    // Generate each next vertex except the last one
    const auto step = M_2_PI / pointsCount;
    for (auto pointIdx = 0u; pointIdx < pointsCount; pointIdx++)
    {
        const auto angle = step * pointIdx;
        pVertex->positionXY[0] = radius * qCos(angle);
        pVertex->positionXY[1] = radius * qSin(angle);
        pVertex->color = color;

        pVertex += 1;
    }

    // Close the fan
    pVertex->positionXY[0] = vertices[1].positionXY[0];
    pVertex->positionXY[1] = vertices[1].positionXY[0];
    pVertex->color = color;
}
