#ifndef _OSMAND_CORE_BASE_OBJECT_H_
#define _OSMAND_CORE_BASE_OBJECT_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore.h>

#define OSMAND_OBJECT(name)                                                                                         \
    Q_DISABLE_COPY(name)                                                                                            \
    public:                                                                                                         \
        typedef name* Ptr;                                                                                          \
        typedef const name* PtrC;                                                                                   \
        typedef std::shared_ptr< name > Ref;                                                                        \
        typedef std::shared_ptr< const name > RefC;                                                                 \
        typedef std::weak_ptr< name > WeakRef;                                                                      \
        typedef std::weak_ptr< const name > WeakRefC;                                                               \
        typedef const std::shared_ptr< name >& NCRef;                                                               \
        typedef const std::shared_ptr< const name >& NCRefC;                                                        \
        typedef std::shared_ptr< name >& OutRef;                                                                    \
        typedef std::shared_ptr< const name >& OutRefC;                                                             \
                                                                                                                    \
        template<typename... ARGS>                                                                                  \
        static Ref NewRef(const ARGS&... args)                                                                      \
        {                                                                                                           \
            return Ref(name(args...));                                                                              \
        }                                                                                                           \
                                                                                                                    \
        inline Ref getRef()                                                                                         \
        {                                                                                                           \
            return std::static_pointer_cast<name>(shared_from_this());                                              \
        }                                                                                                           \
        inline RefC getRefC() const                                                                                 \
        {                                                                                                           \
            return std::static_pointer_cast<const name>(shared_from_this());                                        \
        }                                                                                                           \
    private:

namespace OsmAnd
{
    class OSMAND_CORE_API BaseObject : public std::enable_shared_from_this < BaseObject >
    {
        OSMAND_OBJECT(BaseObject)
    private:
    protected:
    public:
        BaseObject();
        virtual ~BaseObject();
    };
}

#endif // !defined(_OSMAND_CORE_BASE_OBJECT_H_)
