#ifndef _OSMAND_CORE_REFERENCE_H_
#define _OSMAND_CORE_REFERENCE_H_

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

    template<class CLASS>
    typename Fwd<CLASS>::Ref RefNew()
    {
        return (new CLASS()).getRef();
    }
}

#define OSMAND_BE_REFERENCEABLE_CLASS(name)                                                                         \
     public:                                                                                                        \
         typedef name* Ptr;                                                                                         \
         typedef const name* PtrC;                                                                                  \
         typedef std::shared_ptr< name > Ref;                                                                       \
         typedef std::shared_ptr< const name > RefC;                                                                \
         typedef std::weak_ptr< name > WeakRef;                                                                     \
         typedef std::weak_ptr< const name > WeakRefC;                                                              \
         typedef const std::shared_ptr< name >& NCRef;                                                              \
         typedef const std::shared_ptr< const name >& NCRefC;                                                       \
         typedef std::shared_ptr< name >& OutRef;                                                                   \
         typedef std::shared_ptr< const name >& OutRefC;                                                            \
                                                                                                                    \
         inline Ref getRef()                                                                                        \
         {                                                                                                          \
             return shared_from_this();                                                                             \
         }                                                                                                          \
         inline RefC getRefC() const                                                                                \
         {                                                                                                          \
             return shared_from_this();                                                                             \
         }
#define OSMAND_REFERENCEABLE_CLASS(name) name : public std::enable_shared_from_this< name >

#endif // !defined(_OSMAND_CORE_REFERENCE_H_)
