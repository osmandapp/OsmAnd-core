#ifndef _OSMAND_INTERNAL_H_
#define _OSMAND_INTERNAL_H_

#ifdef OSMAND_NATIVE_PROFILING
#   define PROFILE_NATIVE_OPERATION(rc, op) do { rc->nativeOperations.pause(); op; rc->nativeOperations.start() } while(0)
#else
#   define PROFILE_NATIVE_OPERATION(rc, op) do { op; } while(0)
#endif

#endif // _OSMAND_INTERNAL_H_