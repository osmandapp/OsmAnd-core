#include "TransportStopExit.h"
#include "Utilities.h"

OsmAnd::TransportStopExit::TransportStopExit()
    : x31(0)
    , y31(0)
    , ref(QString())
{
}

OsmAnd::TransportStopExit::TransportStopExit(uint32_t x31, uint32_t y31, QString ref)
    : x31(x31)
    , y31(y31)
    , ref(ref)
{
}

OsmAnd::TransportStopExit::~TransportStopExit()
{
}

void OsmAnd::TransportStopExit::setLocation(int zoom, uint32_t dx, uint32_t dy)
{
    x31 = dx << (31 - zoom);
    y31 = dy << (31 - zoom);
    location = LatLon(Utilities::getLatitudeFromTile(zoom, dy), Utilities::getLongitudeFromTile(zoom, dx));
}

bool OsmAnd::TransportStopExit::compareExit(std::shared_ptr<TransportStopExit>& thatObj) {
    return x31 == thatObj->x31 && y31 == thatObj->y31 && ref == thatObj->ref;
}
