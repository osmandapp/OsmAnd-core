#include "commonRendering.h"

#include <SkCodec.h>
#include <SkImageInfo.h>
#include <SkPath.h>

#include "Logging.h"
#include "SkImageGenerator.h"

RenderingContextResults::RenderingContextResults(RenderingContext* rc) {
	this->zoom = rc->zoom;
	this->density = rc->density;
	this->screenDensityRatio = rc->screenDensityRatio;
	this->textScale = rc->textScale;
	this->leftX = rc->leftX;
	this->topY = rc->topY;
	this->width = rc->width;
	this->height = rc->height;
	this->textIntersect = rc->textIntersect;
	this->iconsIntersect = rc->iconsIntersect;
}

TextDrawInfo::TextDrawInfo(std::string itext, MapDataObject* mo)
	: text(itext), icon(NULL), visible(false), combined(false), drawOnPath(false), path(NULL), pathRotate(0),
	  intersectionMargin(0), intersectionSizeFactor(1) {
	object = *mo;
}

bool GetResourceAsBitmap(const char* resourcePath, SkBitmap* dst) {
	sk_sp<SkData> resourceData = SkData::MakeFromFileName(resourcePath);
	if (resourceData == nullptr) {
		return false;
	}
	std::unique_ptr<SkImageGenerator> gen(SkImageGenerator::MakeFromEncoded(resourceData));
	if (!gen) {
		return false;
	}
	SkPMColor ctStorage[256];
	sk_sp<SkColorTable> ctable(new SkColorTable(ctStorage, 256));
	int count = ctable->count();
	return dst->tryAllocPixels(gen->getInfo(), nullptr, ctable.get()) &&
		   gen->getPixels(gen->getInfo().makeColorSpace(nullptr), dst->getPixels(), dst->rowBytes(),
						  const_cast<SkPMColor*>(ctable->readColors()), &count);
}

SkStreamAsset* GetResourceAsStream(const char* resourcePath) {
	std::unique_ptr<SkFILEStream> stream(new SkFILEStream(resourcePath));
	if (!stream->isValid()) {
		return nullptr;
	}
	return stream.release();
}

sk_sp<SkData> GetResourceAsData(const char* resourcePath) {
	return SkData::MakeFromFileName(resourcePath);
}

sk_sp<SkTypeface> MakeResourceAsTypeface(const char* resourcePath) {
	std::unique_ptr<SkStreamAsset> stream(GetResourceAsStream(resourcePath));
	if (!stream) {
		return nullptr;
	}
	return SkTypeface::MakeFromStream(stream.release());
}

TextDrawInfo::~TextDrawInfo() {
	if (path) delete path;
}

IconDrawInfo::IconDrawInfo(MapDataObject* obj)
	: bmp_1(NULL), bmp(NULL), bmp2(NULL), bmp3(NULL), bmp4(NULL), bmp5(NULL), shield(NULL), visible(false),
	  intersectionMargin(0), intersectionSizeFactor(1) {
	object = *obj;
}

RenderingContext::~RenderingContext() {
	textToDraw.clear();
	iconsToDraw.clear();
}

bool RenderingContext::interrupted() {
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
			SkBitmap* bm = new SkBitmap();
			if (!GetResourceAsBitmap(fl.c_str(), bm)) {
				OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Unable to decode '%s'", fl.c_str());
				delete bm;
				return NULL;
			}
			return bm;
		}
	}
	return NULL;
}

string prepareIconValue(MapDataObject& object, string qval) {
	if (qval.find('?') == std::string::npos) {
		return qval;
	}
	int st = qval.find('?');
	int en = qval.find_last_of('?');
	string tagVal = qval.substr(0, st);
	string tagName = qval.substr(0, en).substr(st + 1);
	for (int i = 0; i < object.additionalTypes.size(); i++) {
		if (object.additionalTypes[i].first == tagName) {
			tagVal += object.additionalTypes[i].second;
			break;
		}
	}
	tagVal += qval.substr(en + 1, qval.length());
	return tagVal;
}

UNORDERED(map)<std::string, SkBitmap*> cachedBitmaps;
SkBitmap* getCachedBitmap(RenderingContext* rc, const std::string& bitmapResource) {
	if (bitmapResource.size() == 0) return NULL;

	// Try to find previously cached
	UNORDERED(map)<std::string, SkBitmap*>::iterator itPreviouslyCachedBitmap = cachedBitmaps.find(bitmapResource);
	if (itPreviouslyCachedBitmap != cachedBitmaps.end()) return itPreviouslyCachedBitmap->second;

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
