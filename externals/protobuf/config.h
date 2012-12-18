#ifndef _OSMAND_PROTOBUF_CONFIG_
#define _OSMAND_PROTOBUF_CONFIG_

// For WindowsPhone target, InitializeCriticalSection is not supported
#if defined(_WIN32) && (WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP)
#   define InitializeCriticalSection(cs) InitializeCriticalSectionEx((cs), 0, CRITICAL_SECTION_NO_DEBUG_INFO)
#endif

#if defined(_WIN32) && (WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP)
    /* the namespace of hash_map/hash_set */
	#define HASH_NAMESPACE std

	/* the name of <hash_set> */
	#define HASH_MAP_CLASS hash_map

	/* the location of <hash_map> */
	#define HASH_MAP_H <hash_map>

	/* the name of <hash_set> */
	#define HASH_SET_CLASS hash_set

	/* the location of <hash_set> */
	#define HASH_SET_H <hash_set>

	/* define if the compiler has hash_map */
	#define HAVE_HASH_MAP 1

	/* define if the compiler has hash_set */
	#define HAVE_HASH_SET 1
	
	/* Define if you have POSIX threads libraries and header files. */
	#undef HAVE_PTHREAD
#elif defined(ANDROID)
	/* the namespace of hash_map/hash_set */
	#define HASH_NAMESPACE std

	/* the name of <hash_set> */
	#define HASH_MAP_CLASS unordered_map

	/* the location of <hash_map> */
	#define HASH_MAP_H <unordered_map>

	/* the name of <hash_set> */
	#define HASH_SET_CLASS unordered_set

	/* the location of <hash_set> */
	#define HASH_SET_H <unordered_set>

	/* define if the compiler has hash_map */
	#define HAVE_HASH_MAP 1

	/* define if the compiler has hash_set */
	#define HAVE_HASH_SET 1
	
	/* Define if you have POSIX threads libraries and header files. */
	#define HAVE_PTHREAD 1
	
	/* Enable GNU extensions on systems that have them.  */
	#ifndef _GNU_SOURCE
	# define _GNU_SOURCE 1
	#endif
	
	/* Define to necessary symbol if this constant uses a non-standard name on
	   your system. */
	/* #undef PTHREAD_CREATE_JOINABLE */
#endif

/* Enable classes using zlib compression. */
#define HAVE_ZLIB 1

#endif //  _OSMAND_PROTOBUF_CONFIG_
