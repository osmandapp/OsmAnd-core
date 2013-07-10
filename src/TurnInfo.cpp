/**
* @file
*
* @section LICENSE
*
* OsmAnd - Android navigation software based on OSM maps.
* Copyright (C) 2010-2013  OsmAnd Authors listed in AUTHORS file
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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
