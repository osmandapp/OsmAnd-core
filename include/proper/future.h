#ifndef _OSMAND_CORE_PROPER_FUTURE_H_
#define _OSMAND_CORE_PROPER_FUTURE_H_

#if defined(OSMAND_TARGET_OS_android)

#include <boost/thread/future.hpp>
namespace OsmAnd
{
    namespace proper
    {
        using namespace ::boost;

        template<class E>
        ::boost::exception_ptr make_exception_ptr(E e)
        {
            return ::boost::copy_exception<E>(e);
        }
    }
}

#else

#include <future>
namespace OsmAnd
{
    namespace proper = ::std;
}

#endif

#endif // !defined(_OSMAND_CORE_PROPER_FUTURE_H_)
