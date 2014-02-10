#include "TurnInfo.h"

QString OsmAnd::TurnInfo::toString() const {
    switch (type) {
    case OsmAnd::TurnType::C:
        return "straight";
    case OsmAnd::TurnType::KL:
        return "keep left";
    case OsmAnd::TurnType::KR:
        return "keep right";
    case OsmAnd::TurnType::TL:
        return "turn left";
    case OsmAnd::TurnType::TR:
        return "turn right";
    case OsmAnd::TurnType::TSLL:
        return "turn slightly left";
    case OsmAnd::TurnType::TSLR:
        return "turn slightly right";
    case OsmAnd::TurnType::TSHL:
        return "turn sharply left";
    case OsmAnd::TurnType::TSHR:
        return "turn sharply right";
    case OsmAnd::TurnType::TU:
    case OsmAnd::TurnType::TRU:
        return "u-turn";
    case OsmAnd::TurnType::RD_LEFT:
    case OsmAnd::TurnType::RD_RIGHT:
        return "roundabout (exit " + QString::number(exitOut)  +") ";
    default:
        break;
    }
    return "uknown";
}
