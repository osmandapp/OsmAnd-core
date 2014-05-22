#ifndef _OSMAND_CORE_KEYED_ENTRIES_COLLECTION_H_
#define _OSMAND_CORE_KEYED_ENTRIES_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>
#include <cassert>
#include <functional>
#include <type_traits>

#include <OsmAndCore/QtExtensions.h>
#include <QHash>
#include <QReadWriteLock>
#include <QAtomicInt>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Logging.h>

#if !defined(OSMAND_TRACE_KEYED_ENTRIES_COLLECTION_STATE)
#   define OSMAND_TRACE_KEYED_ENTRIES_COLLECTION_STATE 0
#endif // !defined(OSMAND_TRACE_KEYED_ENTRIES_COLLECTION_STATE)

namespace OsmAnd
{
    template<typename KEY, typename ENTRY>
    class KeyedEntriesCollectionEntry;

    template<typename KEY, typename ENTRY>
    class KeyedEntriesCollection
    {
    public:
        typedef typename KeyedEntriesCollection<KEY, ENTRY> Collection;

        class Link : public std::enable_shared_from_this < Link >
        {
        private:
        protected:
        public:
            Link(Collection& collection_)
                : collection(collection_)
            {
            }

            virtual ~Link()
            {
            }

            Collection& collection;
        };

    private:
        QHash< KEY, std::shared_ptr<ENTRY> > _collection;
        mutable QReadWriteLock _collectionLock;
    protected:
        const std::shared_ptr< Link > _link;
    public:
        KeyedEntriesCollection()
            : _link(new Link(*this))
        {
        }
        virtual ~KeyedEntriesCollection()
        {
        }

        virtual bool tryObtainEntry(std::shared_ptr<ENTRY>& outEntry, const KEY key) const
        {
            if (!_collectionLock.tryLockForRead())
                return false;

            const auto& itEntry = _collection.constFind(key);
            if (itEntry != _collection.cend())
            {
                outEntry = *itEntry;

                _collectionLock.unlock();
                return true;
            }

            _collectionLock.unlock();
            return false;
        }

        virtual bool obtainEntry(std::shared_ptr<ENTRY>& outEntry, const KEY key) const
        {
            QReadLocker scopedLocker(&_collectionLock);

            const auto& itEntry = _collection.constFind(key);
            if (itEntry != _collection.cend())
            {
                outEntry = *itEntry;

                return true;
            }

            return false;
        }

        virtual void obtainOrAllocateEntry(std::shared_ptr<ENTRY>& outEntry, const KEY key, std::function<ENTRY* (const Collection&, const KEY)> allocator)
        {
            assert(allocator != nullptr);

            QWriteLocker scopedLocker(&_collectionLock);

            auto itEntry = _collection.constFind(key);
            if (itEntry != _collection.cend())
            {
                outEntry = *itEntry;
                return;
            }

            auto newEntry = allocator(*this, key);
            outEntry.reset(newEntry);
            itEntry = _collection.insert(key, outEntry);
        }

        virtual void obtainEntries(QList< std::shared_ptr<ENTRY> >* outList, std::function<bool(const std::shared_ptr<ENTRY>& entry, bool& cancel)> filter = nullptr) const
        {
            QReadLocker scopedLocker(&_collectionLock);

            bool doCancel = false;
            for (const auto& entry : constOf(_collection))
            {
                if (!filter || (filter && filter(entry, doCancel)))
                {
                    if (outList)
                        outList->push_back(entry);
                }

                if (doCancel)
                    return;
            }
        }

        virtual void removeAllEntries()
        {
            QWriteLocker scopedLocker(&_collectionLock);

            for (const auto& entry : constOf(_collection))
                entry->unlink();
            _collection.clear();
        }

        virtual void removeEntry(const KEY key)
        {
            QWriteLocker scopedLock(&_collectionLock);

            const auto& itEntry = _collection.find(key);
            if (itEntry == _collection.end())
                return;

            itEntry.value()->unlink();
            _collection.erase(itEntry);
        }

        virtual void removeEntries(std::function<bool(const std::shared_ptr<ENTRY>& entry, bool& cancel)> filter = nullptr)
        {
            QWriteLocker scopedLock(&_collectionLock);

            bool doCancel = false;
            QMutableHashIterator< KEY, std::shared_ptr<ENTRY> > itEntryPair(_collection);
            while (itEntryPair.hasNext())
            {
                const auto& value = itEntryPair.next().value();

                const auto doRemove = (filter == nullptr) || filter(value, doCancel);
                if (doRemove)
                {
                    value->unlink();
                    itEntryPair.remove();
                }

                if (doCancel)
                    return;
            }
        }

        virtual void forAllExecute(std::function<void(const std::shared_ptr<ENTRY>& entry, bool& cancel)> action) const
        {
            QReadLocker scopedLock(&_collectionLock);

            bool doCancel = false;
            for (const auto& entry : constOf(_collection))
            {
                action(entry, doCancel);

                if (doCancel)
                    return;
            }
        }

        virtual unsigned int getEntriesCount() const
        {
            QReadLocker scopedLocker(&_collectionLock);

            return _collection.size();
        }

    friend class OsmAnd::KeyedEntriesCollectionEntry< KEY, ENTRY >;
    };

    template<typename KEY, typename ENTRY>
    class KeyedEntriesCollectionEntry
    {
    public:
        typedef typename KeyedEntriesCollection<KEY, ENTRY> Collection;
        typedef typename Collection::Link Link;

    private:
        std::weak_ptr<Link> _link;

        void unlink()
        {
            detach();

            assert(checkIsSafeToUnlink());
            _link.reset();
        }
    protected:
        KeyedEntriesCollectionEntry(const Collection& collection, const KEY key_)
            : _link(collection._link)
            , link(_link)
            , key(key_)
        {
        }

        virtual void detach()
        {
        }

        virtual bool checkIsSafeToUnlink()
        {
            return true;
        }

        void safeUnlink()
        {
            if (_link.expired())
                return;

            unlink();
            return;
        }
    public:
        virtual ~KeyedEntriesCollectionEntry()
        {
            assert(_link.expired());
        }

        const std::weak_ptr<Link>& link;

        const KEY key;

    friend class OsmAnd::KeyedEntriesCollection<KEY, ENTRY>;
    };

    template<typename KEY, typename ENTRY, typename STATE_ENUM, STATE_ENUM UNDEFINED_STATE_VALUE
#if OSMAND_TRACE_KEYED_ENTRIES_COLLECTION_STATE
        , bool LOG_TRACE = false
#endif // OSMAND_TRACE_KEYED_ENTRIES_COLLECTION_STATE
    >
    class KeyedEntriesCollectionEntryWithState : public KeyedEntriesCollectionEntry<KEY, ENTRY>
    {
    protected:
#if OSMAND_TRACE_KEYED_ENTRIES_COLLECTION_STATE
        mutable QMutex _stateLock;
        volatile int _stateValue;
#else
        QAtomicInt _stateValue;
#endif // OSMAND_TRACE_KEYED_ENTRIES_COLLECTION_STATE
    public:
        KeyedEntriesCollectionEntryWithState(const Collection& collection, const KEY key, const STATE_ENUM state = UNDEFINED_STATE_VALUE)
            : KeyedEntriesCollectionEntry<KEY, ENTRY>(collection, key)
            , _stateValue(static_cast<int>(state))
        {
        }
        virtual ~KeyedEntriesCollectionEntryWithState()
        {
        }

        inline STATE_ENUM getState() const
        {
#if OSMAND_TRACE_KEYED_ENTRIES_COLLECTION_STATE
            return static_cast<STATE_ENUM>(_stateValue);
#else
            return static_cast<STATE_ENUM>(_stateValue.load());
#endif // OSMAND_TRACE_KEYED_ENTRIES_COLLECTION_STATE
        }

        inline void setState(const STATE_ENUM newState)
        {
#if OSMAND_TRACE_KEYED_ENTRIES_COLLECTION_STATE
            QMutexLocker scopedLocker(&_stateLock);
            if (LOG_TRACE
#if OSMAND_TRACE_KEYED_ENTRIES_COLLECTION_STATE > 1
                || true
#endif
                )
            {
                LogPrintf(LogSeverityLevel::Debug,
                    "%s %s state '%d'=>'%d'",
                    typeid(ENTRY).name(),
                    qPrintable(QString(QLatin1String("%1")).arg(key)),
                    _stateValue, static_cast<int>(newState));
            }
            _stateValue = static_cast<int>(newState);
#else
            _stateValue.fetchAndStoreOrdered(static_cast<int>(newState));
#endif // OSMAND_TRACE_KEYED_ENTRIES_COLLECTION_STATE
        }

        inline bool setStateIf(const STATE_ENUM testState, const STATE_ENUM newState)
        {
#if OSMAND_TRACE_KEYED_ENTRIES_COLLECTION_STATE
            QMutexLocker scopedLocker(&_stateLock);
            if (_stateValue != static_cast<int>(testState))
            {
                if (LOG_TRACE
#if OSMAND_TRACE_KEYED_ENTRIES_COLLECTION_STATE > 1
                    || true
#endif
                    )
                {
                    LogPrintf(LogSeverityLevel::Debug,
                        "%s %s state '%d'=>'%d' failed, since not '%d'",
                        typeid(ENTRY).name(),
                        qPrintable(QString(QLatin1String("%1")).arg(key)),
                        _stateValue, static_cast<int>(newState), static_cast<int>(testState));
                }
                return false;
            }
            if (LOG_TRACE
#if OSMAND_TRACE_KEYED_ENTRIES_COLLECTION_STATE > 1
                || true
#endif
                )
            {
                LogPrintf(LogSeverityLevel::Debug,
                    "%s %s state '%d'=>'%d'",
                    typeid(ENTRY).name(),
                    qPrintable(QString(QLatin1String("%1")).arg(key)),
                    _stateValue, static_cast<int>(newState));
            }
            _stateValue = static_cast<int>(newState);
            return true;
#else
            return _stateValue.testAndSetOrdered(static_cast<int>(testState), static_cast<int>(newState));
#endif // OSMAND_TRACE_KEYED_ENTRIES_COLLECTION_STATE
        }
    };
}

#endif // !defined(_OSMAND_CORE_KEYED_ENTRIES_COLLECTION_H_)
