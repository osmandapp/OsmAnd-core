#ifndef _OSMAND_TURN_TYPE_CPP
#define _OSMAND_TURN_TYPE_CPP
#include "turnType.h"

TurnType TurnType::valueOf(int vs, bool leftSide) {
    if (vs == TU && leftSide) {
        vs = TRU;
    } else if(vs == RNDB && leftSide) {
        vs = RNLB;
    }
    
    TurnType turnType(vs);
    return turnType;
    //		if (s != null && s.startsWith("EXIT")) { //$NON-NLS-1$
    //			return getExitTurn(Integer.parseInt(s.substring(4)), 0, leftSide);
    //		}
    //		return null;
}

TurnType TurnType::straight() {
    return valueOf(C, false);
}

SHARED_PTR<TurnType> TurnType::ptrValueOf(int vs, bool leftSide) {
    if (vs == TU && leftSide) {
        vs = TRU;
    } else if(vs == RNDB && leftSide) {
        vs = RNLB;
    }
    return std::make_shared<TurnType>(vs);
}

SHARED_PTR<TurnType> TurnType::ptrStraight() {
    return ptrValueOf(C, false);
}


int TurnType::getActiveCommonLaneTurn() {
    if (lanes.empty()) {
        return C;
    }
    for(int i = 0; i < lanes.size(); i++) {
        if (lanes[i] % 2 == 1) {
            return getPrimaryTurn(lanes[i]);
        }
    }
    return C;
}

string TurnType::toXmlString() {
    switch (value) {
        case C:
            return "C";
        case TL:
            return "TL";
        case TSLL:
            return "TSLL";
        case TSHL:
            return "TSHL";
        case TR:
            return "TR";
        case TSLR:
            return "TSLR";
        case TSHR:
            return "TSHR";
        case KL:
            return "KL";
        case KR:
            return "KR";
        case TU:
            return "TU";
        case TRU:
            return "TRU";
        case OFFR:
            return "OFFR";
        case RNDB: {
            char numstr[10];
            sprintf(numstr, "%d", exitOut);
            return "RNDB" + string(numstr);
        }
        case RNLB: {
            char numstr[10];
            sprintf(numstr, "%d", exitOut);
            return "RNLB" + string(numstr);
        }
    }
    return "C";
}

TurnType TurnType::fromString(string s, bool leftSide) {
    if ("C" == s) {
        return valueOf(C, leftSide);
    } else if ("TL" == s) {
        return valueOf(TL, leftSide);
    } else if ("TSLL" == s) {
        return valueOf(TSLL, leftSide);
    } else if ("TSHL" == s) {
        return valueOf(TSHL, leftSide);
    } else if ("TR" == s) {
        return valueOf(TR, leftSide);
    } else if ("TSLR" == s) {
        return valueOf(TSLR, leftSide);
    } else if ("TSHR" == s) {
        return valueOf(TSHR, leftSide);
    } else if ("KL" == s) {
        return valueOf(KL, leftSide);
    } else if ("KR" == s) {
        return valueOf(KR, leftSide);
    } else if ("TU" == s) {
        return valueOf(TU, leftSide);
    } else if ("TRU" == s) {
        return valueOf(TRU, leftSide);
    } else if ("OFFR" == s) {
        return valueOf(OFFR, leftSide);
    } else if (s.find("EXIT") == 0 || s.find("RNDB") == 0 || s.find("RNLB") == 0) {
        int val = -1;
        if (sscanf(s.c_str(), "xxxx%d", &val) != EOF) {
            return getExitTurn(val, 0, leftSide);
        }
    }
    return straight();
}

string TurnType::toString(vector<int>& lns) {
    string s = "";
    for (int h = 0; h < lns.size(); h++) {
        if (h > 0) {
            s += "|";
        }
        if (lns[h] % 2 == 1) {
            s += "+";
        }
        int pt = getPrimaryTurn(lns[h]);
        if (pt == 0) {
            pt = 1;
        }
        s += valueOf(pt, false).toXmlString();
        int st = getSecondaryTurn(lns[h]);
        if (st != 0) {
            s += "," + valueOf(st, false).toXmlString();
        }
        int tt = getTertiaryTurn(lns[h]);
        if (tt != 0) {
            s += "," + valueOf(tt, false).toXmlString();
        }
    }
    s += "";
    return s;
}

string TurnType::toString() {
    string vl;
    if (isRoundAbout()) {
        char numstr[10];
        sprintf(numstr, "%d", exitOut);
        vl = "Take " + string(numstr) + " exit";
    } else if (value == C) {
        vl = "Go ahead";
    } else if (value == TSLL) {
        vl = "Turn slightly left";
    } else if (value == TL) {
        vl = "Turn left";
    } else if (value == TSHL) {
        vl = "Turn sharply left";
    } else if (value == TSLR) {
        vl = "Turn slightly right";
    } else if (value == TR) {
        vl = "Turn right";
    } else if (value == TSHR) {
        vl = "Turn sharply right";
    } else if (value == TU) {
        vl = "Make uturn";
    } else if (value == TRU) {
        vl = "Make uturn";
    } else if (value == KL) {
        vl = "Keep left";
    } else if (value == KR) {
        vl = "Keep right";
    } else if (value == OFFR) {
        vl = "Off route";
    }
    if (!vl.empty()) {
        if (!lanes.empty()) {
            vl += "(" + toString(lanes) +")";
        }
        return vl;
    }
    return "";
}

void TurnType::collectTurnTypes(int lane, set<int>& set) {
    int pt = getPrimaryTurn(lane);
    if (pt != 0) {
        set.insert(pt);
    }
    pt = getSecondaryTurn(lane);
    if (pt != 0) {
        set.insert(pt);
    }
    pt = getTertiaryTurn(lane);
    if (pt != 0) {
        set.insert(pt);
    }
}

int TurnType::orderFromLeftToRight(int type) {
    switch(type) {
        case TU:
            return -5;
        case TSHL:
            return -4;
        case TL:
            return -3;
        case TSLL:
            return -2;
        case KL:
            return -1;
            
        case TRU:
            return 5;
        case TSHR:
            return 4;
        case TR:
            return 3;
        case TSLR:
            return 2;
        case KR:
            return 1;
        default:
            return 0;
    }
}

int TurnType::convertType(string lane) {
    int turn;
    if (lane == "none" || lane == "through") {
        turn = C;
    } else if (lane == "slight_right" || lane == "merge_to_right") {
        turn = TSLR;
    } else if (lane == "slight_left" || lane == "merge_to_left") {
        turn = TSLL;
    } else if (lane == "right") {
        turn = TR;
    } else if (lane == "left") {
        turn = TL;
    } else if (lane == "sharp_right") {
        turn = TSHR;
    } else if (lane == "sharp_left") {
        turn = TSHL;
    } else if (lane == "reverse") {
        turn = TU;
    } else {
        // Unknown string
        turn = C;
        //			continue;
    }
    return turn;
}

#endif /*_OSMAND_TURN_TYPE_CPP*/
