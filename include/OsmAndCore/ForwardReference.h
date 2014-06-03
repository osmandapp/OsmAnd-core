#ifndef _OSMAND_CORE_FORWARD_REFERENCE_H_
#define _OSMAND_CORE_FORWARD_REFERENCE_H_

#include <memory>

namespace OsmAnd
{
    template<class CLASS>
    struct Fwd
    {
    public:
        typedef CLASS* Ptr;
        typedef const CLASS* PtrC;
        typedef std::shared_ptr< CLASS > Ref;
        typedef std::shared_ptr< const CLASS > RefC;
        typedef std::weak_ptr< CLASS > WeakRef;
        typedef std::weak_ptr< const CLASS > WeakRefC;
        typedef const std::shared_ptr< CLASS >& NCRef;
        typedef const std::shared_ptr< const CLASS >& NCRefC;
        typedef std::shared_ptr< CLASS >& OutRef;
        typedef std::shared_ptr< const CLASS >& OutRefC;
    };

    template<class CLASS, typename... ARGS>
    typename Fwd<CLASS>::Ref RefNew(const ARGS&... args)
    {
        return Fwd<CLASS>::Ref(new CLASS(args...));
    }
}

#endif // !defined(_OSMAND_CORE_FORWARD_REFERENCE_H_)
