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

#ifndef TURNTYPE_H
#define TURNTYPE_H

#include <stdint.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <QVector>

namespace OsmAnd {
enum TurnType {
    UKNOWN,
    C,
    TL,
    TSLL, //slightly
    TSHL, //sharply
    TR,
    TSLR, //slightly
    TSHR, //sharply
    KL, // keep left
    KR,
    TU, // U-turn
    TRU, // right u-turn
    RD_RIGHT, // roundabout right hand
    RD_LEFT // roundabout left hand
};

class OSMAND_CORE_API TurnInfo
{
protected:
    friend class RoutePlannerAnalyzer;
    QString value;
    TurnType type;
    int exitOut;
    // calculated clockwise head rotation if previous direction to NORTH
    float turnAngle;
    bool skipToSpeak;
    QVector<int> lanes;
public:
    TurnInfo(TurnType type=UKNOWN, int exitOut=0):
        type(type), exitOut(exitOut) {
    }

    TurnType getType() const{return type; }
    int getExitOut() const{return exitOut; }
    float getTurnAngle() const{return turnAngle; }
    void setTurnAngle(float turnAngle) {this->turnAngle = turnAngle; }
    bool isSkipToSpeak() const {return skipToSpeak;}
    void setSkipToSpeak(bool skipToSpeak) {this->skipToSpeak = skipToSpeak; }
    const QVector<int>& getLanes() const { return lanes; }
    void setLanes(const QVector<int>& ls) { this->lanes = ls; }
    QString toString() const;

    static TurnInfo  straight() {
        return TurnInfo (TurnType::C);
    }

    static TurnInfo getExitTurn(int out, float angle, bool leftSide) {
        TurnInfo  rTurnType((leftSide?TurnType::RD_LEFT:TurnType::RD_RIGHT), out); //$NON-NLS-1$
        rTurnType.setTurnAngle(angle);
        return rTurnType;
    }



//    static TurnInfo  valueOf(QString s, bool leftSide) {
//        for (String v : predefinedTypes) {
//            if (v.equals(s)) {
//                if (leftSide && TU.equals(v)) {
//                    v = TRU;
//                }
//                return new TurnType(v);
//            }
//        }
//        if (s.startsWith("EXIT")) { //$NON-NLS-1$
//            return getExitTurn(Integer.parseInt(s.substring(4)), 0, leftSide);
//        }
//        return null;
//    }

};
}
#endif // TURNTYPE_H
