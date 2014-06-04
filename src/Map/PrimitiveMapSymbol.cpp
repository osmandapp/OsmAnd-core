#include "PrimitiveMapSymbol.h"

OsmAnd::PrimitiveMapSymbol::PrimitiveMapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_,
    const bool isShareable_,
    const int order_,
    const IntersectionModeFlags intersectionModeFlags_)
    : MapSymbol(group_, isShareable_, order_, intersectionModeFlags_)
    , vertices(nullptr)
    , verticesCount(0)
    , indices(nullptr)
    , indicesCount(0)
    , primitiveType(PrimitiveType::Invalid)
{
}

OsmAnd::PrimitiveMapSymbol::~PrimitiveMapSymbol()
{
    releaseVerticesAndIndices();
}

OsmAnd::PointI OsmAnd::PrimitiveMapSymbol::getPosition31() const
{
    return position31;
}

void OsmAnd::PrimitiveMapSymbol::setPosition31(const PointI position)
{
    position31 = position;
}

void OsmAnd::PrimitiveMapSymbol::releaseVerticesAndIndices()
{
    if (vertices != nullptr)
    {
        delete[] vertices;
        vertices = nullptr;
    }
    verticesCount = 0;

    if (indices != nullptr)
    {
        delete[] indices;
        indices = nullptr;
    }
    indicesCount = 0;
}
