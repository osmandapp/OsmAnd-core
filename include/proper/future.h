#ifndef _OSMAND_CORE_PROPER_FUTURE_H_
#define _OSMAND_CORE_PROPER_FUTURE_H_

#if defined(OSMAND_TARGET_OS_android)

#include <boost/thread/future.hpp>
namespace OsmAnd
{
    namespace proper = ::boost;
}

#else

#include <future>
namespace OsmAnd
{
    namespace proper = ::std;
}

#endif

#endif // _OSMAND_CORE_PROPER_FUTURE_H_
