#include "IMapElevationDataProvider.h"

OsmAnd::IMapElevationDataProvider::IMapElevationDataProvider()
{
}

OsmAnd::IMapElevationDataProvider::~IMapElevationDataProvider()
{
}

OsmAnd::IMapElevationDataProvider::Data::Data(
    const TileId tileId_,
    const ZoomLevel zoom_,
    const size_t rowLength_,
    const uint32_t size_,
    const float* const pRawData_)
    : IMapTiledDataProvider::Data(tileId_, zoom_)
    , rowLength(rowLength_)
    , size(size_)
    , pRawData(pRawData_)
{
}

OsmAnd::IMapElevationDataProvider::Data::~Data()
{
    delete[] pRawData;

    release();
}
