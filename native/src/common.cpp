#include "Common.h"
#include "common2.h"

#include <SkPath.h>
#include <SkBitmap.h>
#include <SkImageDecoder.h>
#include "Logging.h"

#if defined(_WIN32)
//#	include <windows.h>
//#	include <mmsystem.h>
#define isnan _isnan
#define isinf !_finite
#elif defined(__APPLE__)
#	include <mach/mach_time.h>|
#else
#	include <time.h>
#endif

TextDrawInfo::TextDrawInfo(std::string itext)
	: text(itext)
	, drawOnPath(false)
	, path(NULL)
	, pathRotate(0)
{
}

TextDrawInfo::~TextDrawInfo()
{
	if (path)
		delete path;
}

IconDrawInfo::IconDrawInfo()
	: bmp_1(NULL), bmp(NULL), bmp2(NULL), bmp3(NULL), bmp4(NULL), bmp5(NULL),
	  shield(NULL)
{

}

RenderingContext::~RenderingContext()
{
	std::vector<TextDrawInfo*>::iterator itTextToDraw;
	for(itTextToDraw = textToDraw.begin(); itTextToDraw != textToDraw.end(); ++itTextToDraw)
		delete (*itTextToDraw);
}

bool RenderingContext::interrupted()
{
	return false;
}

SkBitmap* RenderingContext::getCachedBitmap(const std::string& bitmapResource) {
	if (defaultIconsDir.size() > 0 && bitmapResource.length() > 0) {
		string fl = string(defaultIconsDir + "h_" + bitmapResource + ".png");
		FILE* f = fopen(fl.c_str(), "r");
		if (f == NULL) {
			fl = string(defaultIconsDir + "g_" + bitmapResource + ".png");
			f = fopen(fl.c_str(), "r");
		}
		if (f != NULL) {
			fclose(f);
			OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "Open file %s", fl.c_str());
			SkBitmap* bmp = new SkBitmap();
			if (!SkImageDecoder::DecodeFile(fl.c_str(), bmp)) {
				delete bmp;
				return NULL;
			}
			return bmp;
		}
	}
	return NULL;
}


UNORDERED(map)<std::string, SkBitmap*> cachedBitmaps;
SkBitmap* getCachedBitmap(RenderingContext* rc, const std::string& bitmapResource)
{

	if(bitmapResource.size() == 0)
		return NULL;

	// Try to find previously cached
	UNORDERED(map)<std::string, SkBitmap*>::iterator itPreviouslyCachedBitmap = cachedBitmaps.find(bitmapResource);
	if (itPreviouslyCachedBitmap != cachedBitmaps.end())
		return itPreviouslyCachedBitmap->second;
	
	rc->nativeOperations.Pause();
	SkBitmap* iconBitmap = rc->getCachedBitmap(bitmapResource);
	cachedBitmaps[bitmapResource] = iconBitmap;
	rc->nativeOperations.Start();

	return iconBitmap;
}

void purgeCachedBitmaps() {
	UNORDERED(map)<std::string, SkBitmap*>::iterator it = cachedBitmaps.begin();
	for (; it != cachedBitmaps.end(); it++) {
		delete it->second;
	}
}

std::string RenderingContext::getTranslatedString(const std::string& src) {
	return src;
}

std::string RenderingContext::getReshapedString(const std::string& src) {
	return src;
}


double getPowZoom(float zoom){
	if(zoom >= 0 && zoom - floor(zoom) < 0.05f){
		return 1 << ((int)zoom);
	} else {
		return pow(2, zoom);
	}
}

double convert31YToMeters(int y1, int y2) {
	// translate into meters
	return (y1 - y2) * 0.01863f;
}

double convert31XToMeters(int x1, int x2) {
	// translate into meters
	return (x1 - x2) * 0.011f;
}


double calculateProjection31TileMetric(int xA, int yA, int xB, int yB, int xC, int yC) {
	// Scalar multiplication between (AB, AC)
	double multiple = convert31XToMeters(xB, xA) * convert31XToMeters(xC, xA) + convert31YToMeters(yB, yA) * convert31YToMeters(yC, yA);
	return multiple;
}
double squareDist31TileMetric(int x1, int y1, int x2, int y2) {
// translate into meters
	double dy = convert31YToMeters(y1, y2);
	double dx = convert31XToMeters(x1, x2);
	return dx * dx + dy * dy;
}


double checkLongitude(double longitude) {
	while (longitude < -180 || longitude > 180) {
		if (longitude < 0) {
			longitude += 360;
		} else {
			longitude -= 360;
		}
	}
	return longitude;
}

double checkLatitude(double latitude) {
	while (latitude < -90 || latitude > 90) {
		if (latitude < 0) {
			latitude += 180;
		} else {
			latitude -= 180;
		}
	}
	if (latitude < -85.0511) {
		return -85.0511;
	} else if (latitude > 85.0511) {
		return 85.0511;
	}
	return latitude;
}


int get31TileNumberX(double longitude){
	longitude = checkLongitude(longitude);
	long long int l =  1;
	l <<= 31;
	return (int)((longitude + 180)/360 * l);
}

int get31TileNumberY(double latitude) {
	latitude = checkLatitude(latitude);
	double eval = log(tan(toRadians(latitude)) + 1 / cos(toRadians(latitude)));
	long long int l =  1;
	l <<= 31;
	if (eval > M_PI) {
		eval = M_PI;
	}
	return (int) ((1 - eval / M_PI) / 2 * l);
}

double getLongitudeFromTile(float zoom, double x) {
	return x / getPowZoom(zoom) * 360.0 - 180.0;
}


double getLatitudeFromTile(float zoom, double y){
	int sign = y < 0 ? -1 : 1;
	double result = atan(sign*sinh(M_PI * (1 - 2 * y / getPowZoom(zoom)))) * 180. / M_PI;
	return result;
}

double get31LongitudeX(int tileX){
	return getLongitudeFromTile(21, tileX /1024.);
}

double get31LatitudeY(int tileY){
	return getLatitudeFromTile(21, tileY /1024.);

}


double getTileNumberX(float zoom, double longitude) {
	if (longitude == 180.) {
		return getPowZoom(zoom) - 1;
	}
	longitude = checkLongitude(longitude);
	return (longitude + 180.) / 360. * getPowZoom(zoom);
}


double getTileNumberY(float zoom, double latitude) {
	latitude = checkLatitude(latitude);
	double eval = log(tan(toRadians(latitude)) + 1 / cos(toRadians(latitude)));
	if (isinf(eval) || isnan(eval)) {
		latitude = latitude < 0 ? -89.9 : 89.9;
		eval = log(tan(toRadians(latitude)) + 1 / cos(toRadians(latitude)));
	}
	double result = (1 - eval / M_PI) / 2 * getPowZoom(zoom);
	return result;
}

double getDistance(double lat1, double lon1, double lat2, double lon2) {
	double R = 6371; // km
	double dLat = toRadians(lat2 - lat1);
	double dLon = toRadians(lon2 - lon1);
	double a = sin(dLat / 2) * sin(dLat / 2)
			+ cos(toRadians(lat1)) * cos(toRadians(lat2)) * sin(dLon / 2) * sin(dLon / 2);
	double c = 2 * atan2(sqrt(a), sqrt(1 - a));
	return R * c * 1000;
}

int findFirstNumberEndIndex(string value) {
	uint i = 0;
	bool valid = false;
	if (value.length() > 0 && value[0] == '-') {
		i++;
	}
	while (i < value.length() && ((value[i] >= '0' && value[i] <= '9') || value[i] == '.')) {
		i++;
		valid = true;
	}
	if (valid) {
		return i;
	} else {
		return -1;
	}
}


double alignAngleDifference(double diff) {
	while (diff > M_PI) {
		diff -= 2 * M_PI;
	}
	while (diff <= -M_PI) {
		diff += 2 * M_PI;
	}
	return diff;

}
