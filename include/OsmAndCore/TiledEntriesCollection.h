#ifndef _OSMAND_CORE_TILED_ENTRIES_COLLECTION_H_
#define _OSMAND_CORE_TILED_ENTRIES_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>
#include <cassert>
#include <array>
#include <functional>
#include <type_traits>

#include <OsmAndCore/QtExtensions.h>
#include <QHash>
#include <QReadWriteLock>
#include <QAtomicInt>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Logging.h>

#if !defined(OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE)
#   define OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE 0
#endif // !defined(OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE)

namespace OsmAnd
{
    template<typename ENTRY>
    class TiledEntriesCollectionEntry;

    template<typename ENTRY>
    class TiledEntriesCollection
    {
    public:
        typedef TiledEntriesCollection<ENTRY> Collection;

        class Link : public std::enable_shared_from_this< Link >
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
        std::array< QHash< TileId, std::shared_ptr<ENTRY> >, ZoomLevelsCount > _zoomLevels;
        mutable QReadWriteLock _collectionLock;
    protected:
        const std::shared_ptr< Link > _link;
    public:
        TiledEntriesCollection()
            : _link(new Link(*this))
        {
        }
        virtual ~TiledEntriesCollection()
        {
        }

        virtual bool tryObtainEntry(std::shared_ptr<ENTRY>& outEntry, const TileId tileId, const ZoomLevel zoom) const
        {
            if (!_collectionLock.tryLockForRead())
                return false;

            const auto& zoomLevel = _zoomLevels[zoom];
            const auto& itEntry = zoomLevel.constFind(tileId);
            if (itEntry != zoomLevel.cend())
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

            const auto& zoomLevel = _zoomLevels[zoom];
            const auto& itEntry = zoomLevel.constFind(tileId);
            if (itEntry != zoomLevel.cend())
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

            auto& zoomLevel = _zoomLevels[zoom];
            auto itEntry = zoomLevel.constFind(tileId);
            if (itEntry != zoomLevel.cend())
            {
                outEntry = *itEntry;
                return;
            }

            auto newEntry = allocator(*this, tileId, zoom);
            outEntry.reset(newEntry);
            itEntry = zoomLevel.insert(tileId, outEntry);
        }

        virtual void obtainEntries(QList< std::shared_ptr<ENTRY> >* outList, std::function<bool (const std::shared_ptr<ENTRY>& entry, bool& cancel)> filter = nullptr) const
        {
            QReadLocker scopedLocker(&_collectionLock);

            bool doCancel = false;
            for(const auto& zoomLevel : constOf(_zoomLevels))
            {
                for(const auto& entry : constOf(zoomLevel))
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

            for(auto& zoomLevel : _zoomLevels)
            {
                for(const auto& entry : constOf(zoomLevel))
                    entry->unlink();

                zoomLevel.clear();
            }
        }

        virtual void removeEntry(const TileId tileId, const ZoomLevel zoom)
        {
            QWriteLocker scopedLock(&_collectionLock);

            auto& zoomLevel = _zoomLevels[zoom];
            const auto& itEntry = zoomLevel.find(tileId);
            if (itEntry == zoomLevel.end())
                return;

            itEntry.value()->unlink();
            zoomLevel.erase(itEntry);
        }

        virtual void removeEntries(std::function<bool (const std::shared_ptr<ENTRY>& entry, bool& cancel)> filter = nullptr)
        {
            QWriteLocker scopedLock(&_collectionLock);

            bool doCancel = false;
            for(auto& zoomLevel : _zoomLevels)
            {
                QMutableHashIterator< TileId, std::shared_ptr<ENTRY> > itEntryPair(zoomLevel);
                while(itEntryPair.hasNext())
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
        }

        virtual void forAllExecute(std::function<void (const std::shared_ptr<ENTRY>& entry, bool& cancel)> action) const
        {
            QReadLocker scopedLock(&_collectionLock);

            bool doCancel = false;
            for(const auto& zoomLevel : constOf(_zoomLevels))
            {
                for(const auto& entry : constOf(zoomLevel))
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
            for(const auto& zoomLevel : constOf(_zoomLevels))
                count += zoomLevel.size();

            return count;
        }

    friend class OsmAnd::TiledEntriesCollectionEntry<ENTRY>;
    };

    template<typename ENTRY>
    class TiledEntriesCollectionEntry
    {
    public:
        typedef TiledEntriesCollection<ENTRY> Collection;
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
        virtual ~TiledEntriesCollectionEntry()
        {
            assert(_link.expired());
        }

        const std::weak_ptr<Link>& link;

        const TileId tileId;
        const ZoomLevel zoom;

    friend class OsmAnd::TiledEntriesCollection<ENTRY>;
    };

    template<typename ENTRY, typename STATE_ENUM, STATE_ENUM UNDEFINED_STATE_VALUE
#if OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE
        , bool LOG_TRACE = false
#endif // OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE
    >
    class TiledEntriesCollectionEntryWithState : public TiledEntriesCollectionEntry<ENTRY>
    {
    public:
        typedef typename TiledEntriesCollectionEntry<ENTRY>::Collection Collection;

    protected:
#if OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE
        mutable QMutex _stateLock;
        volatile int _stateValue;
#else
        QAtomicInt _stateValue;
#endif // OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE
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
#if OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE
            return static_cast<STATE_ENUM>(_stateValue);
#else
            return static_cast<STATE_ENUM>(_stateValue.load());
#endif // OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE
        }

        inline void setState(const STATE_ENUM newState)
        {
#if OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE
            QMutexLocker scopedLocker(&_stateLock);
            if (LOG_TRACE
#if OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE > 1
                || true
#endif
                )
            {
                LogPrintf(LogSeverityLevel::Debug,
                    "%s %dx%d@%d state '%d'=>'%d'",
                    typeid(ENTRY).name(),
                    tileId.x, tileId.y, static_cast<int>(zoom),
                    _stateValue, static_cast<int>(newState));
            }
            _stateValue = static_cast<int>(newState);
#else
            _stateValue.fetchAndStoreOrdered(static_cast<int>(newState));
#endif // OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE
        }

        inline bool setStateIf(const STATE_ENUM testState, const STATE_ENUM newState)
        {
#if OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE
            QMutexLocker scopedLocker(&_stateLock);
            if (_stateValue != static_cast<int>(testState))
            {
                if (LOG_TRACE
#if OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE > 1
                    || true
#endif
                    )
                {
                    LogPrintf(LogSeverityLevel::Debug,
                        "%s %dx%d@%d state '%d'=>'%d' failed, since not '%d'",
                        typeid(ENTRY).name(),
                        tileId.x, tileId.y, static_cast<int>(zoom),
                        _stateValue, static_cast<int>(newState), static_cast<int>(testState));
                }
                return false;
            }
            if (LOG_TRACE
#if OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE > 1
                || true
#endif
                )
            {
                LogPrintf(LogSeverityLevel::Debug,
                    "%s %dx%d@%d state '%d'=>'%d'",
                    typeid(ENTRY).name(),
                    tileId.x, tileId.y, static_cast<int>(zoom),
                    _stateValue, static_cast<int>(newState));
            }
            _stateValue = static_cast<int>(newState);
            return true;
#else
            return _stateValue.testAndSetOrdered(static_cast<int>(testState), static_cast<int>(newState));
#endif // OSMAND_TRACE_TILED_ENTRIES_COLLECTION_STATE
        }
    };
}

#endif // !defined(_OSMAND_CORE_TILED_ENTRIES_COLLECTION_H_)
