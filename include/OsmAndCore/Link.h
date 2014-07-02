#ifndef _OSMAND_CORE_LINK_H_
#define _OSMAND_CORE_LINK_H_

#include <OsmAndCore/stdlib_common.h>
#include <type_traits>

#include <OsmAndCore/QtExtensions.h>
#include <QReadWriteLock>

namespace OsmAnd
{
    template<typename ENTITY>
    class Link : public std::enable_shared_from_this< Link<ENTITY> > Q_DECL_FINAL
    {
        Q_DISABLE_COPY(Link);

    public:
        typedef Link<ENTITY> LinkT;

        template<typename ENTITY>
        class LinkLocker Q_DECL_FINAL
        {
            Q_DISABLE_COPY(LinkLocker);

        public:
            typedef Link<ENTITY> LinkT;

        private:
            const std::shared_ptr<LinkT> _link;
        protected:
            LinkLocker(const LinkT& link)
                : _link(link.shared_from_this())
            {
                _link->lock();
            }
        public:
            ~LinkLocker()
            {
                _link->unlock();
            }

            operator const ENTITY&() const
            {
                return _link->_linkedEntity;
            }

        friend class LinkT;
        };
        typedef LinkLocker<ENTITY> LinkLockerT;

    private:
        //mutable QReadWriteLock _lock;
        const ENTITY _linkedEntity;
    protected:
        void lock() const
        {
            //_lock.lockForRead();
        }

        void unlock() const
        {
            //_lock.unlock();
        }
    public:
        Link(const ENTITY linkedEntity)
            : _linkedEntity(linkedEntity)
        {
        }

        ~Link()
        {
        }
        
    friend class LinkLockerT;
    };
}

#endif // !defined(_OSMAND_CORE_LINK_H_)
