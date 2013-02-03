
#include "stdint.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "jni.h"

// define BOOT_JAR
// define BOOTSTRAP_SO "-Davian.bootstrap=librouting_test_jar.so"

#ifdef __ANDROID__
#include <android/log.h>
void print( const char* format, ...)
{
  va_list args;
  va_start(args, format);
  __android_log_vprint(ANDROID_LOG_WARN, "net.osmand:native", format, args);
  va_end(args);
}
#else 
void print( const char* format, ...)
{
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  printf("\n");
  va_end(args); 
}
#endif

#if (defined __MINGW32__) || (defined _MSC_VER)
#  define EXPORT __declspec(dllexport)
#else
#  define EXPORT __attribute__ ((visibility("default")))
#endif

#ifdef BOOT_JAR 
  #if (! defined __x86_64__) && ((defined __MINGW32__) || (defined _MSC_VER))
  #  define SYMBOL(x) binary_boot_jar_##x
  #else
  #  define SYMBOL(x) _binary_boot_jar_##x
  #endif
    
  extern "C" {
    
    extern const uint8_t SYMBOL(start)[];
    extern const uint8_t SYMBOL(end)[];
  
    EXPORT const uint8_t*
    bootJar(unsigned* size)
    {
      *size = SYMBOL(end) - SYMBOL(start);
     return SYMBOL(start);
    }
    
  } // extern "C"
#else
  #if (! defined __x86_64__) && ((defined __MINGW32__) || (defined _MSC_VER))
  #  define SYMBOL(x) binary_resources_jar_##x
  #else
  #  define SYMBOL(x) _binary_resources_jar_##x
  #endif
    
  extern "C" {
    
    extern const uint8_t SYMBOL(start)[];
    extern const uint8_t SYMBOL(end)[];
  
    EXPORT const uint8_t*
    binaryResourcesJar(unsigned* size)
    {
      *size = SYMBOL(end) - SYMBOL(start);
     return SYMBOL(start);
    }
    
  } // extern "C"

  #if (! defined __x86_64__) && ((defined __MINGW32__) || (defined _MSC_VER))
  #  define BOOTIMAGE_BIN(x) binary_bootimage_bin_##x
  #  define CODEIMAGE_BIN(x) binary_codeimage_bin_##x
  #else
  #  define BOOTIMAGE_BIN(x) _binary_bootimage_bin_##x
  #  define CODEIMAGE_BIN(x) _binary_codeimage_bin_##x
  #endif

 extern "C" {

  extern const uint8_t BOOTIMAGE_BIN(start)[];
  extern const uint8_t BOOTIMAGE_BIN(end)[];

  EXPORT const uint8_t*
  bootimageBin(unsigned* size)
  {
    *size = BOOTIMAGE_BIN(end) - BOOTIMAGE_BIN(start);
    return BOOTIMAGE_BIN(start);
  }

  extern const uint8_t CODEIMAGE_BIN(start)[];
  extern const uint8_t CODEIMAGE_BIN(end)[];

  EXPORT const uint8_t*
  codeimageBin(unsigned* size)
  {
    *size = CODEIMAGE_BIN(end) - CODEIMAGE_BIN(start);
    return CODEIMAGE_BIN(start);
  }

 } // extern "C"
#endif

JNIEnv* globalE = NULL;
JavaVM* vm;
JNIEnv* createJVM(){
  if(globalE) {
    return globalE;
  }
  int exitCode = 0;
  int optionsCount = 0;

  JavaVMOption options[5];

  #ifdef BOOT_JAR
    options[optionsCount++].optionString = const_cast<char*>("-Xbootclasspath:[bootJar]");
  #else
    options[optionsCount++].optionString
       = const_cast<char*>("-Davian.bootimage=bootimageBin");
    options[optionsCount++].optionString
       = const_cast<char*>("-Davian.codeimage=codeimageBin");
    options[optionsCount++].optionString
       = const_cast<char*>("-Xbootclasspath:[binaryResourcesJar]");
  #endif
  #ifdef BOOTSTRAP_SO
    options[optionsCount++].optionString =
     const_cast<char*>(BOOTSTRAP_SO);
  #endif

  void* env;
  JavaVMInitArgs vmArgs;
  vmArgs.version = JNI_VERSION_1_2;
  vmArgs.ignoreUnrecognized = JNI_TRUE;
  vmArgs.nOptions = optionsCount;
  vmArgs.options = options;
  JNI_CreateJavaVM(&vm, &env, &vmArgs);
  globalE = static_cast<JNIEnv*>(env);
  return globalE;
}


bool checkException(JNIEnv* e){
  if (e->ExceptionCheck()) {  e->ExceptionDescribe(); return true;  }
  return false;
}

jstring createString(JNIEnv* e, const char* s) {
  size_t length = strlen(s);
  jchar tempChars[length]; 
  for (size_t i = 0; i < length; i++) { 
    tempChars[i] = s[i]; 
  } 
  jstring str = e->NewString(tempChars, length);
  return str;
}

// Java_net_osmand_plus_render_NativeOsmandLibrary_testRoutingPing
extern "C" JNIEXPORT jint JNICALL Java_net_osmand_NativeLibrary_testRoutingPing(JNIEnv* ienv,
    jobject obj) {
  print("PONG");
}


jint nativeRouting(const char* obfPath,
    jdouble slat, jdouble slon, jdouble elat, jdouble elon) {
  JNIEnv* e = createJVM();
  print("JVM started %d",  e->GetVersion());

  jclass objclass = e->FindClass("java/lang/Object");  
  if(checkException(e)) return -1;
  print("Object class found");


  jclass c = e->FindClass("net/osmand/router/TestRouting");
  if(checkException(e)) return -1;

  jmethodID m = e->GetStaticMethodID(c, "calculateRoute", "(Ljava/lang/String;DDDD)V");
  if(checkException(e)) return -1;

  jstring obfstr = createString(e, obfPath);
  e->CallStaticVoidMethod(c, m, obfstr, slat, slon, elat, elon);
  if(checkException(e)) return -1;
  // vm->DestroyJavaVM();
  // globalE = NULL;
  // vm = NULL;
  return 0;

}

extern "C" JNIEXPORT jint JNICALL Java_net_osmand_NativeLibrary_testNativeRouting(JNIEnv* ienv,
    jobject obj, jstring obfPath,
    jdouble slat, jdouble slon, jdouble elat, jdouble elon) {
  const char *nativeString = ienv->GetStringUTFChars(obfPath, 0);
  print("About to call routing with %s", nativeString);
  nativeRouting(nativeString, slat,slon, elat, elon) ;
  ienv->ReleaseStringUTFChars(obfPath, nativeString);
  return 0;
}

int main(){
  print("Prepare");
  nativeRouting("/home/victor/projects/OsmAnd/data/osm-gen", 52.29363, 4.850542, 51.9369, 4.41623 );
}