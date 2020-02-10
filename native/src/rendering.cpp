#include <math.h>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <set>
#include <cmath>
#include <time.h>

#include <SkTypes.h>
#include <SkBitmap.h>
#include <SkCanvas.h>
#include <SkColorFilter.h>
#include <SkShader.h>
#include <SkBitmapProcShader.h>
#include <SkPathEffect.h>
#include <SkBlurMaskFilter.h>
#include <SkBlurDrawLooper.h>
#include <SkDashPathEffect.h>
#include <SkPaint.h>
#include <SkPath.h>
#include <SkFilterQuality.h>

#include "CommonCollections.h"
#include "commonOsmAndCore.h"
#include "commonRendering.h"
#include "renderRules.h"
#include "binaryRead.h"
#include "textdraw.cpp"
#include "rendering.h"
#include "Logging.h"

const int MAX_V = 10;
const int MAX_V_AREA = 2000;
const int DEFAULT_POLYGON_MAX = 11;
const int DEFAULT_LINE_MAX = 100;
const int DEFAULT_POINTS_MAX = 200;
const int POI_ZOOM_SHIFT = 7;
#if defined(WIN32)
#undef min
#endif

struct MapDataObjectPrimitive {
	MapDataObject* obj;
	int typeInd;
	double order;
	int objectType;
	double area;
	bool pointAdded;
};


void calcPoint(std::pair<int, int>  c, RenderingContext* rc)
{	
    rc->pointCount++;

	double tx = c.first/ (rc->tileDivisor);
	double ty = c.second / (rc->tileDivisor);

    float dTileX = tx - rc->getLeft();
    float dTileY = ty - rc->getTop();
    rc->calcX = rc->cosRotateTileSize * dTileX - rc->sinRotateTileSize * dTileY;
    rc->calcY = rc->sinRotateTileSize * dTileX + rc->cosRotateTileSize * dTileY;
    if (rc->calcX >= 0 && rc->calcX < rc->getWidth()&& rc->calcY >= 0 && rc->calcY < rc->getHeight())
        rc->pointInsideCount++;
}

UNORDERED(map)<std::string, sk_sp<SkPathEffect>> pathEffects;
sk_sp<SkPathEffect> getDashEffect(RenderingContext* rc, std::string input)
{
    const char* chars = input.c_str();
    int i = 0;
    char fval[10];
    int flength = 0;
    vector<float> primFloats;
    bool afterColon = false;
    std::string hash = "";
    for(;;i++) {
        if(chars[i] != '_' && chars[i] != 0 && chars[i] != ':') {
           // suppose it is a character
           fval[flength++] = chars[i];
        } else {
            fval[flength] = 0;
			float parsed = 0;
			if(flength > 0) {	
			   parsed = atof(fval);
			}

			if(afterColon) {
				primFloats[primFloats.size() - 1] += parsed;
			} else {
				parsed = rc->getDensityValue(parsed);
				primFloats.push_back(parsed); 
			}
			hash += (parsed * 10);
			flength = 0;
			
			if(chars[i] == ':') {
				afterColon = true;
			} else {
				afterColon = false;
			}
            if(chars[i] == 0) break;                
        }
    }

if(pathEffects.find(hash) != pathEffects.end())
        return pathEffects[hash];

    sk_sp<SkPathEffect> r = SkDashPathEffect::Make(&primFloats[0], primFloats.size(), 0);
    pathEffects[hash] = r;
    return r;
}

int updatePaint(RenderingRuleSearchRequest* req, SkPaint* paint, int ind, int area, RenderingContext* rc)
{
    RenderingRuleProperty* rColor;
    RenderingRuleProperty* rStrokeW;
    RenderingRuleProperty* rCap;
    RenderingRuleProperty* rPathEff;
    if (ind == 0)
    {
        rColor = req->props()->R_COLOR;
        rStrokeW = req->props()->R_STROKE_WIDTH;
        rCap = req->props()->R_CAP;
        rPathEff = req->props()->R_PATH_EFFECT;
    }
    else if (ind == 1)
    {
        rColor = req->props()->R_COLOR_2;
        rStrokeW = req->props()->R_STROKE_WIDTH_2;
        rCap = req->props()->R_CAP_2;
        rPathEff = req->props()->R_PATH_EFFECT_2;
    }
    else if (ind == -1)
    {
        rColor = req->props()->R_COLOR_0;
        rStrokeW = req->props()->R_STROKE_WIDTH_0;
        rCap = req->props()->R_CAP_0;
        rPathEff = req->props()->R_PATH_EFFECT_0;
    }
    else if (ind == -2)
    {
        rColor = req->props()->R_COLOR__1;
        rStrokeW = req->props()->R_STROKE_WIDTH__1;
        rCap = req->props()->R_CAP__1;
        rPathEff = req->props()->R_PATH_EFFECT__1;
    }
    else if (ind == 2)
    {
        rColor = req->props()->R_COLOR_3;
        rStrokeW = req->props()->R_STROKE_WIDTH_3;
        rCap = req->props()->R_CAP_3;
        rPathEff = req->props()->R_PATH_EFFECT_3;
    }
    else if (ind == -3)
    {
        rColor = req->props()->R_COLOR__2;
        rStrokeW = req->props()->R_STROKE_WIDTH__2;
        rCap = req->props()->R_CAP__2;
        rPathEff = req->props()->R_PATH_EFFECT__2;
    }
    else if (ind == 3)
    {
        rColor = req->props()->R_COLOR_4;
        rStrokeW = req->props()->R_STROKE_WIDTH_4;
        rCap = req->props()->R_CAP_4;
        rPathEff = req->props()->R_PATH_EFFECT_4;
    } else 
    {
    	rColor = req->props()->R_COLOR_5;
        rStrokeW = req->props()->R_STROKE_WIDTH_5;
        rCap = req->props()->R_CAP_5;
        rPathEff = req->props()->R_PATH_EFFECT_5;
    }

    if (area)
    {
    	paint->setColorFilter(NULL);
    	paint->setShader(NULL);
    	paint->setLooper(NULL);
        paint->setStyle(SkPaint::kStrokeAndFill_Style);
        paint->setStrokeWidth(0);
    }
    else
    {
        float stroke = getDensityValue(rc, req, rStrokeW);
        if (!(stroke > 0))
            return 0;
        paint->setColorFilter(NULL);
        paint->setShader(NULL);
        paint->setLooper(NULL);

        paint->setStyle(SkPaint::kStroke_Style);
        paint->setStrokeWidth(stroke);
        std::string cap = req->getStringPropertyValue(rCap);
        std::string pathEff = req->getStringPropertyValue(rPathEff);

        if (cap == "BUTT" || cap == "")
            paint->setStrokeCap(SkPaint::kButt_Cap);
        else if (cap == "ROUND")
            paint->setStrokeCap(SkPaint::kRound_Cap);
        else if (cap == "SQUARE")
            paint->setStrokeCap(SkPaint::kSquare_Cap);
        else
            paint->setStrokeCap(SkPaint::kButt_Cap);

        if (pathEff.size() > 0)
        {
            sk_sp<SkPathEffect> p = getDashEffect(rc, pathEff);
            paint->setPathEffect(p);
        }
        else
        {
            paint->setPathEffect(NULL);
        }
    }

    int color = req->getIntPropertyValue(rColor);
    paint->setColor(color);

    if (ind == 0)
    {
        std::string shader = req->getStringPropertyValue(req->props()->R_SHADER);
        if (shader.size() > 0)
        {
            SkBitmap* bmp = getCachedBitmap(rc, shader);
            if (bmp != NULL) {
            	paint->setShader(
                	SkShader::MakeBitmapShader(*bmp, SkShader::kRepeat_TileMode, SkShader::kRepeat_TileMode));
                if(color == 0) {
					paint->setColor(0xffffffff);                	
                }
            }
        }
    }

    // do not check shadow color here
    if (rc->getShadowRenderingMode() == 1 && ind == 0)
    {
        int shadowColor = req->getIntPropertyValue(req->props()->R_SHADOW_COLOR);
        int shadowLayer = getDensityValue(rc, req, req->props()->R_SHADOW_RADIUS);
        if (shadowColor == 0) {
			shadowColor = rc->getShadowRenderingColor();
		}
        if (shadowColor == 0)
            shadowLayer = 0;

        if (shadowLayer > 0)
            paint->setLooper(SkBlurDrawLooper::Make(shadowColor, SkBlurMaskFilter::ConvertRadiusToSigma(shadowLayer), 0, 0));
    }
    return 1;
}

void renderText(MapDataObject* obj, RenderingRuleSearchRequest* req, RenderingContext* rc, std::string tag,
		std::string value, float xText, float yText, SkPath* path, SHARED_PTR<IconDrawInfo> ico) {
	std::vector<std::string>::iterator it = obj->namesOrder.begin();
	uint k = 0;
	while (it != obj->namesOrder.end()) {
		k++;
		auto pos = (*it).rfind(":") ;
		if(pos != std::string::npos && pos + 4 >= it->length()) {
			it++;
			continue;
		}
		std::string tagName = (*it) == "name" ? "" : (*it);
		std::string name = obj->objectNames[*it];
		bool missingName = rc->getPreferredLocale() != "";
		//if(nameTag) {
			std::string tagNameLocale = (*it)+ ":"+rc->getPreferredLocale();
			if (rc -> getPreferredLocale() != "" && obj->objectNames.find(tagNameLocale)  != 
					obj->objectNames.end() ) {
				std::string sname  = obj->objectNames[tagNameLocale];
				if (sname.length() > 0) {
					name = sname;
					missingName = false;
				}
			}
		//}
		if (name.length() > 0) {
			if(missingName) {						
				name = rc->getTranslatedString(name);
			}
			//name = rc->getReshapedString(name);
			req->setInitialTagValueZoom(tag, value, rc->getZoom(), obj);
			req->setIntFilter(req->props()->R_TEXT_LENGTH, name.length());
			req->setStringFilter(req->props()->R_NAME_TAG, tagName);
			if (req->searchRule(RenderingRulesStorage::TEXT_RULES)
					&& req->isSpecified(req->props()->R_TEXT_SIZE)) {
				SHARED_PTR<TextDrawInfo> info(new TextDrawInfo(name,obj));
				info->icon = ico;
				std::string tagName2 = req->getStringPropertyValue(req->props()->R_NAME_TAG2);
				if(tagName2 != "") {
					std::string tv = obj->objectNames[tagName2];
					if(tv != "") {
						info->text = name + " (" + tv+ ")";
					}
				}
				info->drawOnPath = (path != NULL) && (req->getIntPropertyValue(req->props()->R_TEXT_ON_PATH, 0) > 0);
				if (path != NULL)
					info->path = new SkPath(*path);

				fillTextProperties(rc, info, req, xText, yText);
				info->secondOrder = ((obj->id %10000) << 8) + k;
				rc->textToDraw.push_back(info);
			}
		}
		it++;
	}

}

void drawPolylineShadow(SkCanvas* cv, SkPaint* paint, RenderingContext* rc, SkPath* path, int shadowColor, int shadowRadius)
{
    // blurred shadows
    if (rc->getShadowRenderingMode() == 2 && shadowRadius > 0) {
        // simply draw shadow? difference from option 3 ?
        // paint->setColor(0xffffffff);
        paint->setLooper(SkBlurDrawLooper::Make(shadowColor, SkBlurMaskFilter::ConvertRadiusToSigma(shadowRadius), 0, 0));
        PROFILE_NATIVE_OPERATION(rc, cv->drawPath(*path, *paint));
    }

    // option shadow = 3 with solid border
    if (rc->getShadowRenderingMode() == 3 && shadowRadius > 0) {
        paint->setLooper(NULL);
        paint->setStrokeWidth(paint->getStrokeWidth() + shadowRadius * 2);
        //		paint->setColor(0xffbababa);
        paint->setColorFilter(SkColorFilter::MakeModeFilter(shadowColor, SkBlendMode::kSrcIn));
        //		paint->setColor(shadowColor);
        PROFILE_NATIVE_OPERATION(rc, cv->drawPath(*path, *paint));
    }
}


SkPaint* oneWayPaint(){
    SkPaint* oneWay = new SkPaint;
    oneWay->setStyle(SkPaint::kStroke_Style);
    oneWay->setColor(0xff3a3e9c);
    oneWay->setAntiAlias(true);
    return oneWay;
}
void drawOneWayPaints(RenderingContext* rc, SkCanvas* cv, SkPath* p, int oneway, int color) {
	float rmin = rc->getDensityValue(1);
	if(rmin > 1) {
		rmin = rmin * 2 / 3;
	}
	if (rc->oneWayPaints.size() == 0) {
        const float intervals_oneway[4][4] = {
            {0, 12, 10 * rmin, 152},
            {0, 12,  9 * rmin, 152 + rmin},
            {0, 12 + 6 * rmin, 2 * rmin , 152 + 2 * rmin},
            {0, 12 + 6 * rmin, 1 * rmin, 152 + 3 * rmin}
        };
		sk_sp<SkPathEffect> arrowDashEffect1 = SkDashPathEffect::Make(intervals_oneway[0], 4, 0);
		sk_sp<SkPathEffect> arrowDashEffect2 = SkDashPathEffect::Make(intervals_oneway[1], 4, 1);
		sk_sp<SkPathEffect> arrowDashEffect3 = SkDashPathEffect::Make(intervals_oneway[2], 4, 1);
		sk_sp<SkPathEffect> arrowDashEffect4 = SkDashPathEffect::Make(intervals_oneway[3], 4, 1);

		SkPaint* p = oneWayPaint();
		p->setStrokeWidth(rmin * 2);
		p->setPathEffect(arrowDashEffect1);
		rc->oneWayPaints.push_back(*p);
		delete p;

		p = oneWayPaint();
		p->setStrokeWidth(rmin * 4);
		p->setPathEffect(arrowDashEffect2);
		rc->oneWayPaints.push_back(*p);
		delete p;

		p = oneWayPaint();
		p->setStrokeWidth(rmin * 6);
		p->setPathEffect(arrowDashEffect3);
		rc->oneWayPaints.push_back(*p);
		delete p;

		p = oneWayPaint();
		p->setStrokeWidth(rmin * 8);
		p->setPathEffect(arrowDashEffect4);
		rc->oneWayPaints.push_back(*p);
		delete p;
	}
	
	if (rc->reverseWayPaints.size() == 0) {
            const float intervals_reverse[4][4] = {
                {0, 12, 10 * rmin, 152},
                {0, 12 + 1 * rmin, 9 * rmin, 152},
                {0, 12 + 2 * rmin, 2 * rmin, 152 + 6 * rmin},
                {0, 12 + 3 * rmin, 1 * rmin, 152 + 6 * rmin}
            };            
		sk_sp<SkPathEffect> arrowDashEffect1 = SkDashPathEffect::Make(intervals_reverse[0], 4, 0);
		sk_sp<SkPathEffect> arrowDashEffect2 = SkDashPathEffect::Make(intervals_reverse[1], 4, 1);
		sk_sp<SkPathEffect> arrowDashEffect3 = SkDashPathEffect::Make(intervals_reverse[2], 4, 1);
		sk_sp<SkPathEffect> arrowDashEffect4 = SkDashPathEffect::Make(intervals_reverse[3], 4, 1);
		SkPaint* p = oneWayPaint();
		p->setStrokeWidth(rmin * 2);
		p->setPathEffect(arrowDashEffect1);
		rc->reverseWayPaints.push_back(*p);
		delete p;

		p = oneWayPaint();
		p->setStrokeWidth(rmin * 4);
		p->setPathEffect(arrowDashEffect2);
		rc->reverseWayPaints.push_back(*p);
		delete p;

		p = oneWayPaint();
		p->setStrokeWidth(rmin * 6);
		p->setPathEffect(arrowDashEffect3);
		rc->reverseWayPaints.push_back(*p);
		delete p;

		p = oneWayPaint();
		p->setStrokeWidth(rmin * 8);
		p->setPathEffect(arrowDashEffect4);
		rc->reverseWayPaints.push_back(*p);
		delete p;
	}
	if (oneway > 0) {
		for (size_t i = 0; i < rc->oneWayPaints.size(); i++) {
			rc->oneWayPaints.at(i).setColor(color);
			PROFILE_NATIVE_OPERATION(rc, cv->drawPath(*p, rc->oneWayPaints.at(i)));
		}
	} else {
		for (size_t i = 0; i < rc->reverseWayPaints.size(); i++) {
			rc->oneWayPaints.at(i).setColor(color);
			PROFILE_NATIVE_OPERATION(rc, cv->drawPath(*p, rc->reverseWayPaints.at(i)));
		}
	}
}

int assignOnewayColor(MapDataObject* mObj, RenderingRuleSearchRequest* req, RenderingContext* rc, 
	std::string tag, std::string value, int& onewayColor) {
	int oneway = 0;
	if (rc->getZoom() >= 16 && tag == "highway" && rc->getNoHighwayOnewayArrows() < 1) {
		if (mObj->containsAdditional("oneway", "yes")) {
			oneway = 1;
		} else if (mObj->containsAdditional("oneway", "-1")) {
			oneway = -1;
		}
	}

	if(rc->getZoom() >= 15 && tag == "route" && value == "ferry") {
		if (mObj->containsAdditional("oneway", "yes")) {
			oneway = 1;
		} else if (mObj->containsAdditional("oneway", "-1")) {
			oneway = -1;
		}
	}

	if(tag == "waterway" && rc->getWaterwayArrows() > 0 && (
		(rc->getZoom() >= 15 && value == "stream") ||
		(rc->getZoom() >= 12 && value == "river") ||
		(rc->getZoom() >= 14 && value == "canal")
		)) {
			oneway = 1;
			onewayColor = 0xff6286ff;
	}
	if(tag == "seamark:type" && rc->getWaterwayArrows() > 0 && (
		(rc->getZoom() >= 9 && value == "separation_lane") ||
		(rc->getZoom() >= 9 && value == "separation_line")
		)) {
			oneway = 1;
			onewayColor = 0xff6286ff;
	}
	if(tag == "piste:type" && rc->getZoom() >= 14) {
		if (!mObj->containsAdditional("oneway", "no") && (
			mObj->containsAdditional("piste:oneway", "yes") || mObj->containsAdditional("oneway", "yes") ||
			value == "downhill" || value == "sled")) {
			oneway = 1;
			onewayColor = 0xff000000;
		}
	}
	if(tag == "aerialway" && rc->getZoom() >= 14 && (
		value == "chair_lift" ||
		value == "t-bar" ||
		value == "j-bar" ||
		value == "platter" ||
		value == "magic_carpet" ||
		value == "rope_tow" ||
		value == "zip_line" ||
		value == "drag_lift"
		)) {
		if (!(mObj->containsAdditional("oneway", "no"))) {
			oneway = 1;
			onewayColor = 0xff5959ff;
		}
	}
	if(tag == "aerialway" && rc->getZoom() >= 14 && (
		value == "gondola" ||
		value == "cable_car" ||
		value == "mixed_lift"
		)) {
		if (mObj->containsAdditional("oneway", "yes")) {
			oneway = 1;
			onewayColor = 0xff5959ff;
		}
	}
	if(tag == "highway" && value == "via_ferrata" && rc->getZoom() >= 15) {
		if (mObj->containsAdditional("oneway", "yes")) {
			oneway = 1;	
			onewayColor = 0xff5959ff;
		}
	}
	return oneway;
}


void drawPolyline(MapDataObject* mObj, RenderingRuleSearchRequest* req, SkCanvas* cv, SkPaint* paint,
	RenderingContext* rc, tag_value pair, int layer, int drawOnlyShadow) {
	size_t length = mObj->points.size();
	if (length < 2) {
		return;
	}
	std::string tag = pair.first;
	std::string value = pair.second;

	req->setInitialTagValueZoom(tag, value, rc->getZoom(), mObj);
	req->setIntFilter(req->props()->R_LAYER, layer);
	bool rendered = req->searchRule(2);
	if (!rendered || !updatePaint(req, paint, 0, 0, rc)) {
		return;
	}
	int shadowColor = req->getIntPropertyValue(req->props()->R_SHADOW_COLOR);
	int shadowRadius = getDensityValue(rc, req, req->props()->R_SHADOW_RADIUS);
	if(drawOnlyShadow && shadowRadius == 0) {
		return;
	}
	if(shadowColor == 0) {
		shadowColor = rc->getShadowRenderingColor();
	}
	rc->visible++;
	SkPath path;
	SkPoint middlePoint;
	bool middleSet = false;
	bool intersect = false;
	uint prevCross = 15, pprevCross = 15;
	int x, y, px, py;
	bool startPoint = true;
	for (uint i = 0; i <= length; i++) {
		uint cross = 0;
		if(i == length) {
			cross = 15;
		} else {
			calcPoint(mObj->points.at(i), rc);
			cross |= (rc->calcX < 0 ? 1 : 0);
			cross |= (rc->calcX > rc->getWidth() ? 2 : 0);
			cross |= (rc->calcY < 0 ? 4 : 0);
			cross |= (rc->calcY > rc->getHeight() ? 8 : 0);
		}
		if (prevCross == 0 && cross == 0) {
			middlePoint.set(px, py);
			middleSet = true;
		} else if(!middleSet && cross == 0) {
			middlePoint.set(rc->calcX, rc->calcY);
		}
		// calculate if previous point is outside on same as side as point before and next point (skip it)
		if( (pprevCross & prevCross & cross) == 0 && i > 0) {
			// we could speed up rendering for far away points to calculate projection
			if(prevCross != 0) {
				int targetx = x, targety = y;
				if((pprevCross & prevCross) != 0) {
					// point before is on the same side outside so we can move point along the line to next one
					targetx = rc->calcX;
					targetx = rc->calcY;
				} else if((cross & prevCross) != 0) {
					// point next is on the same side outside so we can move point along the line to prev one
					targetx = px;
					targety = py;
				}
				//// we need to check that average point is not inside rectangle
				int nx = x, ny = y;
				int crs = prevCross;
				while (crs == prevCross && abs(targetx - nx) > 50 && abs(targety - ny) > 50) {
					x = nx;
					nx = (x + targetx) / 2;
					y = ny;
					ny = (y + targety) / 2;
					crs = (nx < 0 ? 1 : 0) + (nx > rc->getWidth() ? 2 : 0) + 
							(ny < 0 ? 4 : 0) + (ny > rc->getHeight() ? 8 : 0);
				}
			}
			if (startPoint) {
				path.moveTo(x, y);
				startPoint = false;
			} else {
				path.lineTo(x, y);
			}
		} else {
			// all 3 points are on same side outside of visible area
			startPoint = true;
		}
		if (!intersect && (prevCross & cross) == 0) {
			intersect = true;
		}

		pprevCross = prevCross;
		prevCross = cross;

		px = x;
		py = y;
		x = rc->calcX;
		y = rc->calcY;
	}

	if (!intersect) {
		return;
	}

	if (drawOnlyShadow) {
		drawPolylineShadow(cv, paint, rc, &path, shadowColor, shadowRadius);
	} else {
		if (updatePaint(req, paint, -3, 0, rc)) {
			PROFILE_NATIVE_OPERATION(rc, cv->drawPath(path, *paint));
		}
		if (updatePaint(req, paint, -2, 0, rc)) {
			PROFILE_NATIVE_OPERATION(rc, cv->drawPath(path, *paint));
		}
		if (updatePaint(req, paint, -1, 0, rc)) {
			PROFILE_NATIVE_OPERATION(rc, cv->drawPath(path, *paint));
		}
		if (updatePaint(req, paint, 0, 0, rc)) {
			PROFILE_NATIVE_OPERATION(rc, cv->drawPath(path, *paint));
		}
		// looks like double drawing (check if there are any issues)
		// PROFILE_NATIVE_OPERATION(rc, cv->drawPath(path, *paint));
		if (updatePaint(req, paint, 1, 0, rc)) {
			PROFILE_NATIVE_OPERATION(rc, cv->drawPath(path, *paint));
		}
		if (updatePaint(req, paint, 2, 0, rc)) {
			PROFILE_NATIVE_OPERATION(rc, cv->drawPath(path, *paint));
		}
		if (updatePaint(req, paint, 3, 0, rc)) {
			PROFILE_NATIVE_OPERATION(rc, cv->drawPath(path, *paint));
		}
		if (updatePaint(req, paint, 4, 0, rc)) {
			PROFILE_NATIVE_OPERATION(rc, cv->drawPath(path, *paint));
		}
		int onewayColor = 0xff3a3e9c;
		int oneway = assignOnewayColor(mObj, req, rc, tag, value, onewayColor);
		if (oneway != 0 && !drawOnlyShadow) {
			if(req->getIntPropertyValue(req->props()->R_ONEWAY_ARROWS_COLOR) != 0) {
				onewayColor = req->getIntPropertyValue(req->props()->R_ONEWAY_ARROWS_COLOR);
			}
			drawOneWayPaints(rc, cv, &path, oneway, onewayColor);
		}
		if (!drawOnlyShadow) {
			renderText(mObj, req, rc, pair.first, pair.second, middlePoint.fX, middlePoint.fY, &path, NULL);
		}
	}
}
#define I_MIN_VALUE 0x8000

int ray_intersect_xo(int prevX, int prevY, int x, int y, int middleY) {
	// prev node above line
	// x,y node below line
	if (prevY > y) {
		int tx = x;
		int ty = y;
		x = prevX;
		y = prevY;
		prevX = tx;
		prevY = ty;
	}
	if (y == middleY || prevY == middleY) {
			middleY -= 1;
	}
	if (prevY > middleY || y < middleY) {
			return I_MIN_VALUE;
	} else {
		if (y == prevY) {
			// the node on the boundary !!!
			return x;
		}
		// that tested on all cases (left/right)
		double rx = x + ((double) middleY - y) * ((double) x - prevX) / (((double) y - prevY));
		return (int) rx;
	}
}
// COPIED from MapAlgorithms
bool ray_intersect_x(int prevX, int prevY, int nx, int ny, int x, int y) {
	int t = ray_intersect_xo(prevX, prevY, nx, ny, y);
	if(t == I_MIN_VALUE){
		return false;
	}
	if(t < x){
		return true;
	}
	return false;
}

int countIntersections(vector<pair<int,int> >& points, int x, int y) {
	int intersections = 0;
	if(points.size() == 0) {
		return 0;
	}
	for (uint i = 0; i < points.size() - 1; i++) {
		if (ray_intersect_x(points[i].first, points[i].second,
				points[i + 1].first, points[i + 1].second, x, y)) {
			intersections++;
		}
	}
	// special handling, also count first and last, might not be closed, but
	// we want this!
	if (ray_intersect_x(points[0].first, points[0].second,
				points[points.size()-1].first, points[points.size()-1].second, x, y)) {
		intersections++;
	}
	return intersections;
}

bool contains(vector<pair<int,int> >& points, int x, int y) {
	return countIntersections(points, x, y) % 2 == 1;
}

std::pair<int,int> fixZoomPOI(std::pair<int,int> p) {
	p.first = (p.first >> POI_ZOOM_SHIFT) << POI_ZOOM_SHIFT;
	p.second = (p.second >> POI_ZOOM_SHIFT) << POI_ZOOM_SHIFT;
	return p;
}

void drawPolygon(MapDataObject* mObj, RenderingRuleSearchRequest* req, SkCanvas* cv, SkPaint* paint,
	RenderingContext* rc, tag_value pair, const MapDataObjectPrimitive& prim) {
	size_t length = mObj->points.size();
	if (length <= 2) {
		return;
	}
	std::string tag = pair.first;
	std::string value = pair.second;

	req->setInitialTagValueZoom(tag, value, rc->getZoom(), mObj);
	bool rendered = req->searchRule(3);

	float xText = 0;
	float yText = 0;
	if (!rendered || !updatePaint(req, paint, 0, 1, rc)) {
		return;
	}
	bool ignoreTextInCenter = false;			
	rc->visible++;
	SkPath path;
	uint i = 0;
	bool containsPoint = false;
	std::vector< std::pair<int,int > > ps;
	uint prevCross = 0;
	
	for (; i < length; i++) {
		calcPoint(mObj->points.at(i), rc);
		if (i == 0) {
			path.moveTo(rc->calcX, rc->calcY);
		} else {
			path.lineTo(rc->calcX, rc->calcY);
		}
		float tx = rc->calcX;
		if (tx < 0) {
			tx = 0;
		}
		if (tx > rc->getWidth()) {
			tx = rc->getWidth();
		}
		float ty = rc->calcY;
		if (ty < 0) {
			ty = 0;
		}
		if (ty > rc->getHeight()) {
			ty = rc->getHeight();
		}
		xText += tx;
		yText += ty;
		if (!containsPoint) {
			if (rc->calcX >= 0 && rc->calcY >= 0 && rc->calcX < rc->getWidth() && rc->calcY < rc->getHeight()) {
				containsPoint = true;
			} else {
				ps.push_back(std::pair<int, int>(rc->calcX, rc->calcY));
			}
			uint cross = 0;
			cross |= (rc->calcX < 0 ? 1 : 0);
			cross |= (rc->calcX > rc->getWidth() ? 2 : 0);
			cross |= (rc->calcY < 0 ? 4 : 0);
			cross |= (rc->calcY > rc->getHeight() ? 8 : 0);
			if(i > 0) {
				if((prevCross & cross) == 0) {
					containsPoint = true;
				}
			}
			prevCross = cross;
		}
	}

	xText /= length;
	yText /= length;

	if(!containsPoint){
		if(contains(ps, rc->getWidth() / 2, rc->getHeight() / 2)) {
			ignoreTextInCenter = true;
			xText = rc->getWidth() / 2;
			yText = rc->getHeight() / 2;
		} else {
			return;
		}		
	}

	if (mObj->isLabelSpecified()) {
		calcPoint(fixZoomPOI(std::pair<int, int>(mObj->getLabelX(), mObj->getLabelY())), rc);
		xText = rc->calcX;
		yText = rc->calcY;
	} 
	
	std::vector<coordinates> polygonInnerCoordinates = mObj->polygonInnerCoordinates;
	if (polygonInnerCoordinates.size() > 0) {
		path.setFillType(SkPath::kEvenOdd_FillType);
		for (uint j = 0; j < polygonInnerCoordinates.size(); j++) {
			coordinates cs = polygonInnerCoordinates.at(j);
			for (uint i = 0; i < cs.size(); i++) {
				calcPoint(cs[i], rc);
				if (i == 0) {
					path.moveTo(rc->calcX, rc->calcY);
				} else {
					path.lineTo(rc->calcX, rc->calcY);
				}
			}
		}
	}
	if (updatePaint(req, paint, -1, 1, rc)) {
		PROFILE_NATIVE_OPERATION(rc, cv->drawPath(path, *paint));
		updatePaint(req, paint, 0, 1, rc);
	}
	PROFILE_NATIVE_OPERATION(rc, cv->drawPath(path, *paint));
	if (updatePaint(req, paint, 1, 0, rc)) {
		PROFILE_NATIVE_OPERATION(rc, cv->drawPath(path, *paint));
	}
	bool addTextForSmallAreas = req->getBoolPropertyValue(req->props()->R_IGNORE_POLYGON_AS_POINT_AREA);
	// ignorePointArea = false;
	if(!prim.pointAdded &&  (prim.area > MAX_V_AREA || addTextForSmallAreas) && !ignoreTextInCenter) {
		renderText(mObj, req, rc, pair.first, pair.second, xText, yText, NULL, NULL);
	}
	
}

void drawPoint(MapDataObject* mObj,	RenderingRuleSearchRequest* req, SkCanvas* cv, SkPaint* paint,
	RenderingContext* rc, std::pair<std::string, std::string>  pair, uint ord)
{
	bool renderTxt = ord == 0;
	std::string tag = pair.first;
	std::string value = pair.second;

	req->setInitialTagValueZoom(tag, value, rc->getZoom(), mObj);
	req->setIntFilter(req->props()->R_TEXT_LENGTH, mObj->objectNames["name"].length());
	req->searchRule(1);
	std::string resId = req->getStringPropertyValue(req-> props()-> R_ICON);
	std::string shieldId = req->getStringPropertyValue(req-> props()-> R_SHIELD);

	SkBitmap* bmp = getCachedBitmap(rc, resId);	
	
	if (bmp == NULL && !renderTxt)
		return;
	SkBitmap* shield = getCachedBitmap(rc, shieldId);
	
	size_t length = mObj->points.size();
	rc->visible++;
	float px = 0;
	float py = 0;
	if (mObj->isLabelSpecified()) {
		calcPoint(fixZoomPOI(std::pair<int, int>(mObj->getLabelX(), mObj->getLabelY())), rc);
		px = rc->calcX;
		py = rc->calcY;
	} else {
		uint i = 0;
		for (; i < length; i++) {
			calcPoint(fixZoomPOI(mObj->points.at(i)), rc);
			px += rc->calcX;
			py += rc->calcY;
		}
		if (length > 1) {
			px /= length;
			py /= length;
		}
	}
	
	SHARED_PTR<IconDrawInfo> ico;
	if (bmp != NULL) {
		ico.reset(new IconDrawInfo(mObj));
		ico->x = px;
		ico->y = py;
		ico->bmp_1 = getCachedBitmap(rc, req->getStringPropertyValue(req-> props()-> R_ICON_1));	
		ico->bmp = bmp;
		ico->bmpId = resId;
		ico->bmp2 = getCachedBitmap(rc, req->getStringPropertyValue(req-> props()-> R_ICON2));	
		ico->bmp3 = getCachedBitmap(rc, req->getStringPropertyValue(req-> props()-> R_ICON3));	
		ico->bmp4 = getCachedBitmap(rc, req->getStringPropertyValue(req-> props()-> R_ICON4));	
		ico->bmp5 = getCachedBitmap(rc, req->getStringPropertyValue(req-> props()-> R_ICON5));	
		ico->shield = shield;
		ico->shiftPy = req->getFloatPropertyValue(req-> props()-> R_ICON_SHIFT_PY, 0);
		ico->shiftPx = req->getFloatPropertyValue(req-> props()-> R_ICON_SHIFT_PX, 0);
		ico->iconSize = getDensityValue(rc, req, req->props()->R_ICON_VISIBLE_SIZE, -1);
		ico->order = req->getIntPropertyValue(req-> props()-> R_ICON_ORDER, 100);
		ico->intersectionSizeFactor = req->getFloatPropertyValue(req->props()-> R_INTERSECTION_SIZE_FACTOR, 1);
		ico->intersectionMargin = getDensityValue(rc, req, req->props()->R_INTERSECTION_MARGIN);
		ico->secondOrder = ((mObj->id %10000) << 8) + ord;
		if(ico->order >= 0) 
			rc->iconsToDraw.push_back(ico);
	}
	if (renderTxt) {
		renderText(mObj, req, rc, pair.first, pair.second, px, py, NULL, ico);
	}

}

void drawObject(RenderingContext* rc,  SkCanvas* cv, RenderingRuleSearchRequest* req,
	SkPaint* paint, vector<MapDataObjectPrimitive>& array, int objOrder) {
	//double polygonLimit = 100;
	//float orderToSwitch = 0;
	for (uint i = 0; i < array.size(); i++) {
		rc->allObjects++;
		MapDataObject* mObj = array[i].obj;
		tag_value pair = mObj->types.at(array[i].typeInd);
		if (array[i].objectType == 3) {
			// polygon
			drawPolygon(mObj, req, cv, paint, rc, pair, array[i]);
		} else if (array[i].objectType == 2) {
			drawPolyline(mObj, req, cv, paint, rc, pair, mObj->getSimpleLayer(), objOrder == 1);
		} else if (array[i].objectType == 1) {
			drawPoint(mObj, req, cv, paint, rc, pair, array[i].typeInd);
		}
		if (i % 25 == 0 && rc->interrupted()) {
			return;
		}
	}
}
bool iconOrder(SHARED_PTR<IconDrawInfo>& text1, SHARED_PTR<IconDrawInfo>& text2) {
	if(text1->order == text2->order)  
		return text1->secondOrder < text2->secondOrder;
	return text1->order < text2->order;
}

SkRect makeRect(RenderingContext* rc,  SHARED_PTR<IconDrawInfo>& icon, SkBitmap* ico, SkRect* rm) {
	float coef = rc->getDensityValue(rc->getScreenDensityRatio() * rc->getTextScale());
	float cx = icon->x;
	float cy = icon->y;
	if(rm != NULL) {
		cx = rm -> centerX();
		cy = rm -> centerY();
	} else {
		cx +=  icon->shiftPx *  ico->width() / 2 * coef;
		cy +=  icon->shiftPy *  ico->height() / 2 * coef;
	}
	float left = cx -  ico->width() / 2 * coef;
	float top = cy - ico->height() / 2 * coef; 
	SkRect r = SkRect::MakeXYWH(left, top, ico->width() * coef,
			ico->height() * coef );
	return r;
}

void drawIconsOverCanvas(RenderingContext* rc, RenderingRuleSearchRequest* req, SkCanvas* canvas)
{
	std::sort(rc->iconsToDraw.begin(), rc->iconsToDraw.end(), iconOrder);
	SkRect bounds = SkRect::MakeLTRB(0, 0, rc->getWidth(), rc->getHeight());
	bounds.inset(-bounds.width()/4, -bounds.height()/4);
	quad_tree<SHARED_PTR<IconDrawInfo>> boundsIntersect(bounds, 4, 0.6);
	
	size_t ji = 0;
	SkPaint p;
	p.setStyle(SkPaint::kStroke_Style);
	p.setFilterQuality(SkFilterQuality::kLow_SkFilterQuality);
	vector<SHARED_PTR<IconDrawInfo> > searchText;
	float coef = rc->getDensityValue(rc->getScreenDensityRatio() * rc->getTextScale());
	for(;ji< rc->iconsToDraw.size(); ji++)
	{
		SHARED_PTR<IconDrawInfo> icon = rc->iconsToDraw.at(ji);
		if (icon->y >= 0 && icon->y < rc->getHeight() && icon->x >= 0 && icon->x < rc->getWidth() && icon->bmp != NULL) 
		{
			SkBitmap* ico = icon->bmp;
			
			float vwidth = icon->iconSize >= 0 ? icon->iconSize : ico->width();
			float vheight = icon->iconSize >= 0 ? icon->iconSize : ico->height();
			float vleft = icon->x -  vwidth / 2 * coef;
			float vtop = icon->y - vheight / 2 * coef; 
						
			bool intersects = false;
			SkRect bbox = SkRect::MakeXYWH(0, 0, 0, 0);
			if(vwidth > 0 && vheight > 0)  
			{
				bbox = SkRect::MakeXYWH(vleft, vtop, vwidth * coef,
						vheight* coef );
				boundsIntersect.query_in_box(bbox, searchText);
			
				for (uint32_t i = 0; i < searchText.size(); i++) {
					if (SkRect::Intersects(searchText[i]->bbox, bbox)) {
						intersects =  true;
						break;
					}
				}
			}
			SkRect rm = makeRect(rc, icon, ico, NULL);
			if (!intersects) {
				icon->visible = true;
				if(icon->shield != NULL) {
					SkRect r = makeRect(rc, icon, icon->shield, &rm);
					PROFILE_NATIVE_OPERATION(rc, canvas->drawBitmapRect(*icon->shield, r, &p));
				}
				if(icon->bmp_1 != NULL) {
					SkRect r = makeRect(rc, icon, icon->bmp_1, &rm);
					PROFILE_NATIVE_OPERATION(rc, canvas->drawBitmapRect(*icon->bmp_1, r, &p));
				}
				PROFILE_NATIVE_OPERATION(rc, canvas->drawBitmapRect(*ico, rm, &p));
				if(icon->bmp2 != NULL) {
					SkRect r = makeRect(rc, icon, icon->bmp2, &rm);
					PROFILE_NATIVE_OPERATION(rc, canvas->drawBitmapRect(*icon->bmp2,  r, &p));
				}
				if(icon->bmp3 != NULL) {
					SkRect r = makeRect(rc, icon, icon->bmp3, &rm);
					PROFILE_NATIVE_OPERATION(rc, canvas->drawBitmapRect(*icon->bmp3,  r, &p));
				}
				if(icon->bmp4 != NULL) {
					SkRect r = makeRect(rc, icon, icon->bmp4, &rm);
					PROFILE_NATIVE_OPERATION(rc, canvas->drawBitmapRect(*icon->bmp4,  r, &p));
				}
				if(icon->bmp5 != NULL) {
					SkRect r = makeRect(rc, icon, icon->bmp5, &rm);
					PROFILE_NATIVE_OPERATION(rc, canvas->drawBitmapRect(*icon->bmp5,  r, &p));
				}
				if(bbox.width() > 0) {
					bbox.inset(-bbox.width()/4, -bbox.height()/4);
					icon->bbox = bbox;
					boundsIntersect.insert(icon, bbox);
				}
			}
		}
		if (rc->interrupted()) {
			break;
		}
	}
	for(;ji< rc->iconsToDraw.size(); ji++)
	{
		SHARED_PTR<IconDrawInfo> icon = rc->iconsToDraw.at(ji);
		if (!icon->visible && 
			icon->y >= 0 && icon->y < rc->getHeight() && icon->x >= 0 && icon->x < rc->getWidth() && icon->bmp != NULL) 
		{
			SkBitmap* ico = icon->bmp;
			float vwidth = icon->iconSize >= 0 ? icon->iconSize : ico->width();
			float vheight = icon->iconSize >= 0 ? icon->iconSize : ico->height();
			float vleft = icon->x -  vwidth / 2 * coef;
			float vtop = icon->y - vheight / 2 * coef; 
			SkRect bbox = SkRect::MakeXYWH(vleft, vtop, vwidth * coef,vheight* coef );
			boundsIntersect.insert(icon, bbox);
		}
	}
	rc->iconsIntersect = quad_tree<SHARED_PTR<IconDrawInfo>>(boundsIntersect);
	rc->iconsToDraw.clear();
}

double polygonArea(MapDataObject* obj, float mult) {
	double area = 0.;
	int j = obj->points.size() - 1;
	for (uint i = 0; i < obj->points.size(); i++) {
		int_pair p1 = obj->points[i] ;
		int_pair p2 = obj->points[j];
		area += (p2.first + p1.first) * (p2.second - ((float)p1.second));
		j = i;
	}
	return abs(area) * mult * mult * .5;
}

void filterLinesByDensity(RenderingContext* rc, std::vector<MapDataObjectPrimitive>&  linesResArray,
		std::vector<MapDataObjectPrimitive>& linesArray) {
	int roadsLimit = rc->roadsDensityLimitPerTile;
	int densityZ = rc->roadDensityZoomTile;
	if(densityZ == 0 || roadsLimit == 0) {
		linesResArray = linesArray;
		return;
	}
	linesResArray.reserve(linesArray.size());
	UNORDERED(map)<int64_t, pair<int, int> > densityMap;
	for (int i = linesArray.size() - 1; i >= 0; i--) {
		bool accept = true;
		int o = linesArray[i].order;
		MapDataObject* line = linesArray[i].obj;
		tag_value& ts = line->types[linesArray[i].typeInd];
		if (ts.first == "highway") {
			accept = false;
			int64_t prev = 0;
			for (uint k = 0; k < line->points.size(); k++) {
				int dz = rc->getZoom() + densityZ;
				int64_t x = (line->points[k].first) >> (31 - dz);
				int64_t y = (line->points[k].second) >> (31 - dz);
				int64_t tl = (x << dz) + y;
				if (prev != tl) {
					prev = tl;
					pair<int, int>& p = densityMap[tl];
					if (p.first < roadsLimit/* && p.second > o */) {
						accept = true;
						p.first++;
						p.second = o;
						densityMap[tl] = p;
					}
				}
			}
		}
		if(accept) {
			linesResArray.push_back(linesArray[i]);
		}
	}
	reverse(linesResArray.begin(), linesResArray.end());
}

bool sortByOrder(const MapDataObjectPrimitive& i,const MapDataObjectPrimitive& j) 
{
	if( i.order == j.order) {
		if(i.typeInd == j.typeInd) {
			return i.obj->points.size() < j.obj->points.size() ;
		} 
		// polygon
		if(i.objectType == 3) 
		{
			return i.typeInd > j.typeInd;	
		}
		return i.typeInd < j.typeInd;
	}
	return (i.order < j.order); 
}
bool sortPolygonsOrder(const MapDataObjectPrimitive& i,const MapDataObjectPrimitive& j) 
{
	if( i.order == j.order) return i.typeInd < j.typeInd;
	return (i.order>j.order); 
}

void sortObjectsByProperOrder(std::vector <MapDataObject* > mapDataObjects,
	RenderingRuleSearchRequest* req, RenderingContext* rc,
		std::vector<MapDataObjectPrimitive>&  polygonsArray, std::vector<MapDataObjectPrimitive>&  pointsArray,
		std::vector<MapDataObjectPrimitive>&  linesResArray) {
	if (req != NULL) {
		std::vector<MapDataObjectPrimitive>  linesArray;
		req->clearState();
		const uint size = mapDataObjects.size();
		float mult = 1. / getPowZoom(max(31 - (rc->getZoom() + 8), 0));
		double minPolygonSize = rc->polygonMinSizeToDisplay;
		uint i = 0;
		for (; i < size; i++) {
			MapDataObject* mobj = mapDataObjects[i];
			size_t sizeTypes = mobj->types.size();
			size_t j = 0;
			for (; j < sizeTypes; j++) {
				int layer = mobj->getSimpleLayer();
				tag_value pair = mobj->types[j];
				req->setTagValueZoomLayer(pair.first, pair.second, rc->getZoom(), layer, mobj);
				req->setIntFilter(req->props()->R_AREA, mobj->area);
				req->setIntFilter(req->props()->R_POINT, mobj->points.size() == 1);
				req->setIntFilter(req->props()->R_CYCLE, mobj->cycle());

				if (req->searchRule(RenderingRulesStorage::ORDER_RULES)) {					
					int objectType = req->getIntPropertyValue(req->props()->R_OBJECT_TYPE);
					int order = req->getIntPropertyValue(req->props()->R_ORDER);
					bool addTextForSmallAreas = req->getBoolPropertyValue(req->props()->R_IGNORE_POLYGON_AS_POINT_AREA);
					bool addPoint = req->getBoolPropertyValue(req->props()->R_ADD_POINT);
					if(order >= 0) {
						// int l = req->getIntPropertyValue(req->props()->R_LAYER);
						MapDataObjectPrimitive mapObj;
						mapObj.objectType = objectType;
						mapObj.order = order;
						mapObj.area = 0;
						mapObj.pointAdded = false;
						mapObj.typeInd = j;
						mapObj.obj = mobj;
						// polygon
						if(objectType == 3) {
							mapObj.pointAdded = addPoint;
							double area = polygonArea(mobj, mult);
							mapObj.area = area;

							MapDataObjectPrimitive pointObj = mapObj;
							pointObj.objectType = 1;
							
							
							if(area > MAX_V && area > minPolygonSize) { 
								mapObj.order = mapObj.order + (1. / area);
								if(mapObj.order < DEFAULT_POLYGON_MAX) {
									polygonsArray.push_back(mapObj);
								} else {
									linesArray.push_back(mapObj);	
								}
								if(addPoint && (area > MAX_V_AREA || addTextForSmallAreas)) {
									pointsArray.push_back(pointObj); 
								}
							} 
						} else if(objectType == 1) {
							pointsArray.push_back(mapObj);
						} else {
							if(mapObj.order < DEFAULT_POLYGON_MAX) {
								polygonsArray.push_back(mapObj);	
							} else {
								linesArray.push_back(mapObj);
							}
						}
						if (req->getIntPropertyValue(req->props()->R_SHADOW_LEVEL) > 0) {
							rc->shadowLevelMin = std::min(rc->shadowLevelMin, order);
							rc->shadowLevelMax = std::max(rc->shadowLevelMax, order);
							req->clearIntvalue(req->props()->R_SHADOW_LEVEL);
						}
					}
				}

			}
		}
		sort(polygonsArray.begin(), polygonsArray.end(), sortByOrder);
		sort(pointsArray.begin(), pointsArray.end(), sortByOrder);
		sort(linesArray.begin(), linesArray.end(), sortByOrder);
		filterLinesByDensity(rc, linesResArray, linesArray);
	}
}

void doRendering(std::vector <MapDataObject* > mapDataObjects, SkCanvas* canvas,
		RenderingRuleSearchRequest* req,
		RenderingContext* rc) {
	rc->nativeOperations.Start();
	SkPaint* paint = new SkPaint;
	paint->setAntiAlias(true);

	std::vector<MapDataObjectPrimitive>  polygonsArray;
	std::vector<MapDataObjectPrimitive>  pointsArray;
	std::vector<MapDataObjectPrimitive>  linesArray;
	sortObjectsByProperOrder(mapDataObjects, req, rc, polygonsArray, pointsArray, linesArray);
	rc->lastRenderedKey = 0;
	drawObject(rc, canvas, req, paint, polygonsArray, 0);
	rc->lastRenderedKey = DEFAULT_POLYGON_MAX;
	if (rc->getShadowRenderingMode() > 1) {
		drawObject(rc, canvas, req, paint, linesArray, 1);
	}
	rc->lastRenderedKey = (DEFAULT_POLYGON_MAX + DEFAULT_LINE_MAX) / 2;
	drawObject(rc, canvas, req, paint, linesArray, 2);
	rc->lastRenderedKey = DEFAULT_LINE_MAX;

	drawObject(rc, canvas, req, paint, pointsArray, 3);
	rc->lastRenderedKey = DEFAULT_POINTS_MAX;

	drawIconsOverCanvas(rc, req,canvas);

	rc->textRendering.Start();
	drawTextOverCanvas(rc, req, canvas);
	rc->textRendering.Pause();

	delete paint;
	rc->nativeOperations.Pause();
	OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info,  "Native ok (rendering %d, text %d ms) \n (%d points, %d points inside, %d of %d objects visible)\n",
				(int)rc->nativeOperations.GetElapsedMs(),	(int)rc->textRendering.GetElapsedMs(),
				rc->pointCount, rc->pointInsideCount, rc->visible, rc->allObjects);
}
