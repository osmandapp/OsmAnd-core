%native (copyPixels) jboolean copyPixels(SkBitmap sourceBitmap, jobject targetBitmap);
%{

extern "C" {

#include <android/bitmap.h>

// Copy pixels from SkBitmap to android.graphics.Bitmap
SWIGEXPORT jboolean JNICALL
Java_net_osmand_core_jni_OsmAndCoreJNI_copyPixels(JNIEnv *jenv, jclass jCls,
                                                  jlong jSourceBitmapPtr,
                                                  jobject jTargetBitmap)
{
	(void)jCls;

	SkBitmap* pSourceBitmap = *(SkBitmap**) &jSourceBitmapPtr;
	if (!pSourceBitmap)
	{
		SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "sourceBitmap is null");
		return (jboolean) 0;
	}

	if (!jTargetBitmap)
	{
		SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "targetBitmap is null");
		return (jboolean) 0;
	}

	AndroidBitmapInfo androidBitmapInfo;
	if (AndroidBitmap_getInfo(jenv, jTargetBitmap, &androidBitmapInfo) != ANDROID_BITMAP_RESULT_SUCCESS)
	{
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Failed to get bitmap info");
		return (jboolean) 0;
	}

	if (androidBitmapInfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888)
	{
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Expected RGBA_8888 format");	
		return false;
	}
	if (androidBitmapInfo.width != pSourceBitmap->width() || androidBitmapInfo.height != pSourceBitmap->height())
	{
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Expected android bitmap %dx%d pixels", pSourceBitmap->width(), pSourceBitmap->height());	
		return false;
	}

	void* addr = NULL;
	if (AndroidBitmap_lockPixels(jenv, jTargetBitmap, &addr) != ANDROID_BITMAP_RESULT_SUCCESS || !addr)
	{
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Failed to lock bitmap pixels");
		return (jboolean) 0;
	}

	std::memcpy(addr, pSourceBitmap->getPixels(), androidBitmapInfo.width * androidBitmapInfo.height * 4);

	int unlockResult = AndroidBitmap_unlockPixels(jenv, jTargetBitmap);
	if (unlockResult != ANDROID_BITMAP_RESULT_SUCCESS)
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Failed to unlock bitmap pixels");

	return (jboolean) 1;
}

}

%}