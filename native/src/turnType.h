#ifndef _OSMAND_TURN_TYPE_H
#define _OSMAND_TURN_TYPE_H
#include "CommonCollections.h"
#include "commonOsmAndCore.h"
#include <set>

struct TurnType {
    const static int C = 1;//"C"; // continue (go straight) //$NON-NLS-1$
    const static int TL = 2; // turn left //$NON-NLS-1$
    const static int TSLL = 3; // turn slightly left //$NON-NLS-1$
    const static int TSHL = 4; // turn sharply left //$NON-NLS-1$
    const static int TR = 5; // turn right //$NON-NLS-1$
    const static int TSLR = 6; // turn slightly right //$NON-NLS-1$
    const static int TSHR = 7; // turn sharply right //$NON-NLS-1$
    const static int KL = 8; // keep left //$NON-NLS-1$
    const static int KR = 9; // keep right//$NON-NLS-1$
    const static int TU = 10; // U-turn //$NON-NLS-1$
    const static int TRU = 11; // Right U-turn //$NON-NLS-1$
    const static int OFFR = 12; // Off route //$NON-NLS-1$
    const static int RNDB = 13; // Roundabout
    const static int RNLB = 14; // Roundabout left
    
private:
    int value;
    int exitOut;
    // calculated clockwise head rotation if previous direction to NORTH
    float turnAngle;
    bool skipToSpeak;
    vector<int> lanes;
    bool possiblyLeftTurn;
    bool possiblyRightTurn;
    
public:
    TurnType(int vl) : value(vl) {
    }
    
    static TurnType valueOf(int vs, bool leftSide);
    static TurnType straight();
    static SHARED_PTR<TurnType> ptrValueOf(int vs, bool leftSide);
    static SHARED_PTR<TurnType> ptrStraight();

    int getActiveCommonLaneTurn();
    string toXmlString();
    static TurnType fromString(string s, bool leftSide);
    static string toString(vector<int>& lns);

    string toString();

    static void collectTurnTypes(int lane, set<int>& set);
    static int orderFromLeftToRight(int type);
    static int convertType(string lane);

    // calculated Clockwise head rotation if previous direction to NORTH
    inline float getTurnAngle() {
        return turnAngle;
    }
    
    inline bool isLeftSide() {
        return value == RNLB || value == TRU;
    }
    
    inline void setTurnAngle(float turnAngle) {
        this->turnAngle = turnAngle;
    }
    
    inline int getValue() {
        return value;
    }
    
    inline int getExitOut() {
        return exitOut;
    }
    
    inline bool isRoundAbout() {
        return value == RNDB || value == RNLB;
    }
    
    // lanes encoded as array of int
    // 0 bit - 0/1 - to use or not
    // 1-5 bits - additional turn info
    // 6-10 bits - secondary turn
    // 11-15 bits - tertiary turn
    inline void setLanes(const vector<int>& lanes) {
        this->lanes = lanes;
    }
    
    static TurnType getExitTurn(int out, float angle, bool leftSide) {
        TurnType r = valueOf(RNDB, leftSide);
        r.exitOut = out;
        r.setTurnAngle(angle);
        return r;
    }

    static SHARED_PTR<TurnType> getPtrExitTurn(int out, float angle, bool leftSide) {
        auto r = ptrValueOf(RNDB, leftSide);
        r->exitOut = out;
        r->setTurnAngle(angle);
        return r;
    }
    
    // Note that the primary turn will be the one displayed on the map.
    static void setPrimaryTurnAndReset(vector<int>& lanes, int lane, int turnType) {
        lanes[lane] = (turnType << 1);
    }
    
    static int getPrimaryTurn(int laneValue) {
        // Get the primary turn modifier for the lane
        return (laneValue >> 1) & ((1 << 4) - 1);
    }
    
    static void setSecondaryTurn(vector<int>& lanes, int lane, int turnType) {
        lanes[lane] &= ~(15 << 5);
        lanes[lane] |= (turnType << 5);
    }
    
    static void setPrimaryTurn(vector<int>& lanes, int lane, int turnType) {
        lanes[lane] &= ~(15 << 1);
        lanes[lane] |= (turnType << 1);
    }
    
    static int getSecondaryTurn(int laneValue) {
        // Get the secondary turn modifier for the lane
        return (laneValue >> 5) & ((1 << 5) - 1);
    }
    
    static void setPrimaryTurnShiftOthers(vector<int>& lanes, int lane, int turnType) {
        int pt = getPrimaryTurn(lanes[lane]);
        int st = getSecondaryTurn(lanes[lane]);
        //int tt = getTertiaryTurn(lanes[lane]);
        setPrimaryTurnAndReset(lanes, lane, turnType);
        setSecondaryTurn(lanes, lane, pt);
        setTertiaryTurn(lanes, lane, st);
    }
    
    static void setSecondaryTurnShiftOthers(vector<int>& lanes, int lane, int turnType) {
        int st = getSecondaryTurn(lanes[lane]);
        //int tt = getTertiaryTurn(lanes[lane]);
        setSecondaryTurn(lanes, lane, turnType);
        setTertiaryTurn(lanes, lane, st);
    }
    
    static void setTertiaryTurn(vector<int>& lanes, int lane, int turnType) {
        lanes[lane] &= ~(15 << 10);
        lanes[lane] |= (turnType << 10);
    }
    
    static int getTertiaryTurn(int laneValue) {
        // Get the tertiary turn modifier for the lane
        return (laneValue >> 10);
    }
    
    inline vector<int>& getLanes() {
        return lanes;
    }
    
    inline void setPossibleLeftTurn(bool possiblyLeftTurn) {
        this->possiblyLeftTurn = possiblyLeftTurn;
    }
    
    inline void setPossibleRightTurn(bool possiblyRightTurn) {
        this->possiblyRightTurn = possiblyRightTurn;
    }
    
    inline bool isPossibleLeftTurn() {
        return possiblyLeftTurn;
    }
    
    inline bool isPossibleRightTurn() {
        return possiblyRightTurn;
    }
    
    inline bool keepLeft() {
        return value == KL;
    }
    
    inline bool keepRight() {
        return value == KR;
    }
    
    inline bool goAhead() {
        return value == C;
    }
    
    inline bool isSkipToSpeak() {
        return skipToSpeak;
    }
    
    inline void setSkipToSpeak(bool skipToSpeak) {
        this->skipToSpeak = skipToSpeak;
    }
    
    static bool isLeftTurn(int type) {
        return type == TL || type == TSHL || type == TSLL || type == TU || type == KL;
    }
    
    static bool isLeftTurnNoUTurn(int type) {
        return type == TL || type == TSHL || type == TSLL || type == KL;
    }
    
    static bool isRightTurn(int type) {
        return type == TR || type == TSHR || type == TSLR || type == TRU || type == KR;
    }
    
    static bool isRightTurnNoUTurn(int type) {
        return type == TR || type == TSHR || type == TSLR || type == KR;
    }
    
    static bool isSlightTurn(int type) {
        return type == TSLL || type == TSLR || type == C || type == KL || type == KR;
    }
    
    static bool hasAnySlightTurnLane(int type) {
        return isSlightTurn(getPrimaryTurn(type)) || isSlightTurn(getSecondaryTurn(type)) || isSlightTurn(getTertiaryTurn(type));
    }
    
    static bool hasAnyTurnLane(int type, int turn) {
        return getPrimaryTurn(type) == turn || getSecondaryTurn(type) == turn || getTertiaryTurn(type) == turn;
    }
};

#endif /*_OSMAND_TURN_TYPE_H*/
