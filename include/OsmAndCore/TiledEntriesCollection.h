#ifndef _OSMAND_CORE_TILED_ENTRIES_COLLECTION_H_
#define _OSMAND_CORE_TILED_ENTRIES_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <array>
#include <functional>
#include <type_traits>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/QtCommon.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QHash>
#include <QReadWriteLock>
#include <QAtomicInt>
#include <QThread>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Logging.h>

//#define OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE_SUPPORT 1
#if !defined(OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE_SUPPORT)
#   define OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE_SUPPORT 0
#endif // !defined(OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE_SUPPORT)

namespace OsmAnd
{
    template<typename ENTRY>
    class TiledEntriesCollectionEntry;

    template<typename ENTRY>
    class TiledEntriesCollection
    {
        Q_DISABLE_COPY_AND_MOVE(TiledEntriesCollection);

    public:
        typedef TiledEntriesCollection<ENTRY> Collection;

        class Link : public std::enable_shared_from_this < Link >
        {
            Q_DISABLE_COPY_AND_MOVE(Link);

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

        typedef std::array< QHash< TileId, std::shared_ptr<ENTRY> >, ZoomLevelsCount > Storage;

    private:
    protected:
        Storage _storage;
        mutable QReadWriteLock _collectionLock;

        const std::shared_ptr< Link > _link;

        virtual void onCollectionModified() const
        {
        }

        virtual void onEntryModified(const TileId tileId, const ZoomLevel zoomLevel) const
        {
            Q_UNUSED(tileId);
            Q_UNUSED(zoomLevel);
        }
    public:
        TiledEntriesCollection()
            : _link(new Link(*this))
        {
        }
        virtual ~TiledEntriesCollection()
        {
        }

        virtual bool hasEntries(const ZoomLevel zoom) const
        {
            QReadLocker scopedLocker(&_collectionLock);

            const auto& storage = _storage[zoom];

            return !storage.isEmpty();
        }

        virtual bool tryObtainEntry(std::shared_ptr<ENTRY>& outEntry, const TileId tileId, const ZoomLevel zoom) const
        {
            if (!_collectionLock.tryLockForRead())
                return false;

            const auto& storage = _storage[zoom];
            const auto& itEntry = storage.constFind(tileId);
            if (itEntry != storage.cend())
            {
                outEntry = *itEntry;

                _collectionLock.unlock();
                return true;
            }

            _collectionLock.unlock();
            return false;
        }

        virtual bool obtainEntry(std::shared_ptr<ENTRY>& outEntry, const TileId tileId, const ZoomLevel zoom) const
        {
            QReadLocker scopedLocker(&_collectionLock);

            const auto& storage = _storage[zoom];
            const auto& itEntry = storage.constFind(tileId);
            if (itEntry != storage.cend())
            {
                outEntry = *itEntry;

                return true;
            }

            return false;
        }

        virtual void obtainOrAllocateEntry(std::shared_ptr<ENTRY>& outEntry, const TileId tileId, const ZoomLevel zoom, std::function<ENTRY* (const Collection&, const TileId, const ZoomLevel)> allocator)
        {
            assert(allocator != nullptr);

            QWriteLocker scopedLocker(&_collectionLock);

            auto& storage = _storage[zoom];
            auto itEntry = storage.constFind(tileId);
            if (itEntry != storage.cend())
            {
                outEntry = *itEntry;
                return;
            }

            auto newEntry = allocator(*this, tileId, zoom);
            outEntry.reset(newEntry);
            itEntry = storage.insert(tileId, outEntry);

            onCollectionModified();
        }

        virtual void obtainEntries(QList< std::shared_ptr<ENTRY> >* outList, std::function<bool(const std::shared_ptr<ENTRY>& entry, bool& cancel)> filter = nullptr) const
        {
            QReadLocker scopedLocker(&_collectionLock);

            bool doCancel = false;
            for (const auto& storage : constOf(_storage))
            {
                for (const auto& entry : constOf(storage))
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
        }

        virtual void removeAllEntries()
        {
            QWriteLocker scopedLocker(&_collectionLock);

            auto modified = false;
            for (auto& storage : _storage)
            {
                for (const auto& entry : constOf(storage))
                    entry->unlink();

                if (!modified && !storage.isEmpty())
                    modified = true;
                storage.clear();
            }

            if (modified)
                onCollectionModified();
        }

        virtual void removeEntry(const TileId tileId, const ZoomLevel zoom)
        {
            QWriteLocker scopedLocker(&_collectionLock);

            auto& storage = _storage[zoom];
            const auto& itEntry = storage.find(tileId);
            if (itEntry == storage.end())
                return;

            itEntry.value()->unlink();
            storage.erase(itEntry);

            onCollectionModified();
        }

        virtual void removeEntries(std::function<bool(const std::shared_ptr<ENTRY>& entry, bool& cancel)> filter = nullptr)
        {
            QWriteLocker scopedLocker(&_collectionLock);

            auto modified = false;
            bool doCancel = false;
            for (auto& storage : _storage)
            {
                auto itEntryPair = mutableIteratorOf(storage);
                while (itEntryPair.hasNext())
                {
                    const auto& value = itEntryPair.next().value();

                    const auto doRemove = (filter == nullptr) || filter(value, doCancel);
                    if (doRemove)
                    {
                        value->unlink();
                        itEntryPair.remove();

                        modified = true;
                    }

                    if (doCancel)
                        break;
                }

                if (doCancel)
                    break;
            }

            if (modified)
                onCollectionModified();
        }

        virtual void forAllExecute(std::function<void(const std::shared_ptr<ENTRY>& entry, bool& cancel)> action) const
        {
            QReadLocker scopedLocker(&_collectionLock);

            bool doCancel = false;
            for (const auto& storage : constOf(_storage))
            {
                for (const auto& entry : constOf(storage))
                {
                    action(entry, doCancel);

                    if (doCancel)
                        return;
                }
            }
        }

        virtual unsigned int getEntriesCount() const
        {
            QReadLocker scopedLocker(&_collectionLock);

            unsigned int count = 0;
            for (const auto& storage : constOf(_storage))
                count += storage.size();

            return count;
        }

    friend class OsmAnd::TiledEntriesCollectionEntry < ENTRY > ;
    };

    template<typename ENTRY>
    class TiledEntriesCollectionEntry
    {
        Q_DISABLE_COPY_AND_MOVE(TiledEntriesCollectionEntry);

    public:
        typedef TiledEntriesCollection<ENTRY> Collection;
        typedef typename Collection::Link Link;

    private:
        std::weak_ptr<Link> _link;

        void unlink()
        {
            detach();

            _link.reset();
        }
    protected:
        TiledEntriesCollectionEntry(const Collection& collection, const TileId tileId_, const ZoomLevel zoom_)
            : _link(collection._link)
            , link(_link)
            , tileId(tileId_)
            , zoom(zoom_)
        {
        }

        virtual void detach()
        {
        }

        void safeUnlink()
        {
            if (_link.expired())
                return;

            unlink();
            return;
        }

        virtual void onEntryModified() const
        {
            if (const auto link = _link.lock())
                link->collection.onEntryModified(tileId, zoom);
        }
    public:
        virtual ~TiledEntriesCollectionEntry()
        {
            assert(_link.expired());
        }

        const std::weak_ptr<Link>& link;

        const TileId tileId;
        const ZoomLevel zoom;

    friend class OsmAnd::TiledEntriesCollection < ENTRY > ;
    };

    template<typename ENTRY, typename STATE_ENUM, STATE_ENUM UNDEFINED_STATE_VALUE
#if OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE_SUPPORT
        , bool LOG_TRACE = false
#endif // OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE_SUPPORT
    >
    class TiledEntriesCollectionEntryWithState : public TiledEntriesCollectionEntry < ENTRY >
    {
    public:
        typedef typename TiledEntriesCollectionEntry<ENTRY>::Collection Collection;

    private:
        typedef TiledEntriesCollectionEntry < ENTRY > super;
    protected:
#if OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE_SUPPORT
        mutable QMutex _stateLock;
        volatile int _stateValue;
#else
        QAtomicInt _stateValue;
#endif // OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE_SUPPORT
    public:
        TiledEntriesCollectionEntryWithState(const Collection& collection, const TileId tileId, const ZoomLevel zoom, const STATE_ENUM state = UNDEFINED_STATE_VALUE)
            : TiledEntriesCollectionEntry<ENTRY>(collection, tileId, zoom)
            , _stateValue(static_cast<int>(state))
        {
        }
        virtual ~TiledEntriesCollectionEntryWithState()
        {
        }

        inline STATE_ENUM getState() const
        {
#if OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE_SUPPORT
            return static_cast<STATE_ENUM>(_stateValue);
#else
            return static_cast<STATE_ENUM>(_stateValue.loadAcquire());
#endif // OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE_SUPPORT
        }

        inline void setState(const STATE_ENUM newState)
        {
#if OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE_SUPPORT
            QMutexLocker scopedLocker(&_stateLock);
            if (LOG_TRACE
#if OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE_SUPPORT > 1
                || true
#endif
                )
            {
                LogPrintf(LogSeverityLevel::Debug,
                    "%s %dx%d@%d state '%d'=>'%d' (thread %p)",
                    typeid(ENTRY).name(),
                    tileId.x,
                    tileId.y,
                    static_cast<int>(zoom),
                    _stateValue,
                    static_cast<int>(newState),
                    QThread::currentThreadId());
            }
            _stateValue = static_cast<int>(newState);
#else
            _stateValue.fetchAndStoreOrdered(static_cast<int>(newState));
#endif // OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE_SUPPORT

            super::onEntryModified();
        }

        inline bool setStateIf(const STATE_ENUM testState, const STATE_ENUM newState)
        {
#if OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE_SUPPORT
            QMutexLocker scopedLocker(&_stateLock);
            if (_stateValue != static_cast<int>(testState))
            {
                if (LOG_TRACE
#if OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE_SUPPORT > 1
                    || true
#endif
                    )
                {
                    LogPrintf(LogSeverityLevel::Debug,
                        "%s %dx%d@%d state '%d'=>'%d' failed, since not '%d' (thread %p)",
                        typeid(ENTRY).name(),
                        tileId.x,
                        tileId.y,
                        static_cast<int>(zoom),
                        _stateValue,
                        static_cast<int>(newState),
                        static_cast<int>(testState),
                        QThread::currentThreadId());
                }
                return false;
            }
            if (LOG_TRACE
#if OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE_SUPPORT > 1
                || true
#endif
                )
            {
                LogPrintf(LogSeverityLevel::Debug,
                    "%s %dx%d@%d state '%d'=>'%d' (thread %p)",
                    typeid(ENTRY).name(),
                    tileId.x,
                    tileId.y,
                    static_cast<int>(zoom),
                    _stateValue,
                    static_cast<int>(newState),
                    QThread::currentThreadId());
            }
            _stateValue = static_cast<int>(newState);
            super::onEntryModified();
            return true;
#else
            const bool modified = _stateValue.testAndSetOrdered(static_cast<int>(testState), static_cast<int>(newState));
            if (modified)
                super::onEntryModified();
            return modified;
#endif // OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE_SUPPORT
        }
    };
}

#endif // !defined(_OSMAND_CORE_TILED_ENTRIES_COLLECTION_H_)
