#ifndef _OSMAND_CORE_STDLIB__COMMON_H_
#define _OSMAND_CORE_STDLIB__COMMON_H_

#include <cstdint>

#if defined(__GXX_EXPERIMENTAL_CXX0X__) && (__cplusplus < 201103L)
#include <tr1/cinttypes>
#else
#include <cinttypes>
#endif

#include <memory>
#include <chrono>
#if defined(__GXX_EXPERIMENTAL_CXX0X__) && (__cplusplus < 201103L)
#define steady_clock monotonic_clock
#endif

#endif // _OSMAND_CORE_STDLIB__COMMON_H_
