#include "RingPrimitiveMapSymbol.h"

OsmAnd::RingPrimitiveMapSymbol::RingPrimitiveMapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_,
    const bool isShareable_,
    const int order_,
    const IntersectionModeFlags intersectionModeFlags_)
    : PrimitiveMapSymbol(group_, isShareable_, order_, intersectionModeFlags_)
{
}

OsmAnd::RingPrimitiveMapSymbol::~RingPrimitiveMapSymbol()
{
}

void OsmAnd::RingPrimitiveMapSymbol::generateAsLine(
    const FColorARGB color /*= FColorARGB(1.0f, 1.0f, 1.0f, 1.0f)*/,
    const unsigned int pointsCount /*= 360*/,
    float radius /*= 1.0f*/)
{
    releaseVerticesAndIndices();

    primitiveType = PrimitiveType::LineLoop;

    // Ring as line-loop has no reusable vertices, because it's rendered as triangle-fan,
    // so there's no indices
    indices = nullptr;
    indicesCount = 0;

    // Allocate space for pointsCount vertices
    vertices = new Vertex[pointsCount];
    auto pVertex = vertices;

    // Generate each vertex
    const auto step = M_2_PI / pointsCount;
    for (auto pointIdx = 0u; pointIdx < pointsCount; pointIdx++)
    {
        const auto angle = step * pointIdx;
        pVertex->positionXY[0] = radius * qCos(angle);
        pVertex->positionXY[1] = radius * qSin(angle);
        pVertex->color = color;

        pVertex += 1;
    }
}
