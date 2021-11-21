#ifndef _OSMAND_CORE_QUAD_TREE_H_
#define _OSMAND_CORE_QUAD_TREE_H_

#include <OsmAndCore/stdlib_common.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <functional>
#include <utility>
#include <array>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/QtCommon.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QList>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Ref.h>

namespace OsmAnd
{
    template<typename ELEMENT_TYPE, typename COORD_TYPE>
    class QuadTree
    {
    public:
        typedef COORD_TYPE CoordType;
        typedef QuadTree<ELEMENT_TYPE, COORD_TYPE> QuadTreeT;
        typedef Area<COORD_TYPE> AreaT;
        typedef OOBB<COORD_TYPE> OOBBT;
        typedef Point<COORD_TYPE> PointT;
        enum class BBoxType
        {
            Invalid,
            AABB,
            OOBB,
        };

        struct BBox
        {
            enum {
                DataSize = (sizeof(AreaT) > sizeof(OOBBT)) ? sizeof(AreaT) : sizeof(OOBBT)
            };

            inline BBox()
                : asAABB(*reinterpret_cast<AreaT*>(&data))
                , asOOBB(*reinterpret_cast<OOBBT*>(&data))
                , type(BBoxType::Invalid)
            {
            }

            inline BBox(const AreaT& aabb)
                : asAABB(*reinterpret_cast<AreaT*>(&data))
                , asOOBB(*reinterpret_cast<OOBBT*>(&data))
                , type(BBoxType::AABB)
            {
                new(&asAABB) AreaT();
                asAABB = aabb;
            }

            inline BBox(const OOBBT& oobb)
                : asAABB(*reinterpret_cast<AreaT*>(&data))
                , asOOBB(*reinterpret_cast<OOBBT*>(&data))
                , type(BBoxType::OOBB)
            {
                new(&asOOBB) OOBBT();
                asOOBB = oobb;
            }

            inline BBox(const BBox& that)
                : asAABB(*reinterpret_cast<AreaT*>(&data))
                , asOOBB(*reinterpret_cast<OOBBT*>(&data))
                , type(that.type)
            {
                if (that.type == BBoxType::AABB)
                {
                    new(&asAABB) AreaT();
                    asAABB = that.asAABB;
                }
                else /* if (that.type == BBoxType::OOBB) */
                {
                    new(&asOOBB) OOBBT();
                    asOOBB = that.asOOBB;
                }
            }

            inline BBox& operator=(const BBox& that)
            {
                if (this != &that)
                {
                    if (that.type == BBoxType::AABB)
                    {
                        new(&asAABB) AreaT();
                        asAABB = that.asAABB;
                    }
                    else /* if (that.type == BBoxType::OOBB) */
                    {
                        new(&asOOBB) OOBBT();
                        asOOBB = that.asOOBB;
                    }
                    type = that.type;
                }
                return *this;
            }

            inline BBox& operator=(const AreaT& that)
            {
                new(&asAABB) AreaT();
                asAABB = that;
                type = BBoxType::AABB;

                return *this;
            }

            inline BBox& operator=(const OOBBT& that)
            {
                new(&asOOBB) OOBBT();
                asOOBB = that;
                type = BBoxType::OOBB;

                return *this;
            }

            inline BBox getEnlargedBy(const PointT& delta) const
            {
                if (type == BBoxType::AABB)
                    return BBox(asAABB.getEnlargedBy(delta));
                else /* if (type == BBoxType::OOBB) */
                    return BBox(asOOBB.getEnlargedBy(delta));
            }

            inline BBox getEnlargedBy(const COORD_TYPE& delta) const
            {
                if (type == BBoxType::AABB)
                    return BBox(asAABB.getEnlargedBy(delta));
                else /* if (type == BBoxType::OOBB) */
                    return BBox(asOOBB.getEnlargedBy(delta));
            }

            inline BBox getEnlargedBy(
                const COORD_TYPE& dt,
                const COORD_TYPE& dl,
                const COORD_TYPE& db,
                const COORD_TYPE& dr) const
            {
                if (type == BBoxType::AABB)
                    return BBox(asAABB.getEnlargedBy(dt, dl, db, dr));
                else /* if (type == BBoxType::OOBB) */
                    return BBox(asOOBB.getEnlargedBy(dt, dl, db, dr));
            }

            inline bool isContainedBy(const AreaT& that) const
            {
                if (type == BBoxType::AABB)
                    return that.contains(asAABB);
                else /* if (type == BBoxType::OOBB) */
                    return that.contains(asOOBB);
            }

            inline bool isContainedBy(const OOBBT& that) const
            {
                if (type == BBoxType::AABB)
                    return that.contains(asAABB);
                else /* if (type == BBoxType::OOBB) */
                    return that.contains(asOOBB);
            }

            inline bool isIntersectedBy(const AreaT& that) const
            {
                if (type == BBoxType::AABB)
                    return that.intersects(asAABB);
                else /* if (type == BBoxType::OOBB) */
                    return that.intersects(asOOBB);
            }

            inline bool isIntersectedBy(const OOBBT& that) const
            {
                if (type == BBoxType::AABB)
                    return that.intersects(asAABB);
                else /* if (type == BBoxType::OOBB) */
                    return that.intersects(asOOBB);
            }

            inline bool contains(const AreaT& that) const
            {
                if (type == BBoxType::AABB)
                    return asAABB.contains(that);
                else /* if (type == BBoxType::OOBB) */
                    return asOOBB.contains(that);
            }

            inline bool contains(const OOBBT& that) const
            {
                if (type == BBoxType::AABB)
                    return asAABB.contains(that);
                else /* if (type == BBoxType::OOBB) */
                    return asOOBB.contains(that);
            }

            inline bool intersects(const AreaT& that) const
            {
                if (type == BBoxType::AABB)
                    return asAABB.intersects(that);
                else /* if (type == BBoxType::OOBB) */
                    return asOOBB.intersects(that);
            }

            inline bool intersects(const OOBBT& that) const
            {
                if (type == BBoxType::AABB)
                    return asAABB.intersects(that);
                else /* if (type == BBoxType::OOBB) */
                    return asOOBB.intersects(that);
            }

            uint8_t data[DataSize];
            AreaT& asAABB;
            OOBBT& asOOBB;
            BBoxType type;
        };
        typedef std::pair<BBox, ELEMENT_TYPE> EntryT;

        typedef std::function<bool (const ELEMENT_TYPE& element, const BBox& bbox)> Acceptor;
    private:
    protected:
        struct Node
        {
            Node(const AreaT& area_)
                : area(area_)
            {
            }

            Node(const Node& that)
                : area(that.area)
            {
                for (auto idx = 0u; idx < 4; idx++)
                {
                    const auto& source = that.subnodes[idx];
                    if (!source)
                        continue;
                    auto& target = subnodes[idx];

                    target = new Node(*source);
                }

                for (const auto& source : constOf(that.entries))
                    entries.push_back(source);
            }

            const AreaT area;
            std::array< Ref< Node >, 4 > subnodes;
            QList< EntryT > entries;

            template<typename BBOX_TYPE>
            bool insert(const ELEMENT_TYPE& element, const BBOX_TYPE& bbox_, const uintmax_t allowedDepthRemaining)
            {
                // Check if this node can hold entire element
                if (!area.contains(bbox_))
                    return false;

                insertNoCheck(element, bbox_, allowedDepthRemaining);
                return true;
            }

            inline bool insert(const ELEMENT_TYPE& element, const BBox& bbox_, const uintmax_t allowedDepthRemaining)
            {
                if (bbox_.type == BBoxType::AABB)
                    return insert(element, bbox_.asAABB, allowedDepthRemaining);
                else /* if (bbox_.type == BBoxType::OOBB) */
                    return insert(element, bbox_.asOOBB, allowedDepthRemaining);
            }

            template<typename BBOX_TYPE>
            void insertNoCheck(const ELEMENT_TYPE& element, const BBOX_TYPE& bbox_, const uintmax_t allowedDepthRemaining)
            {
                // If depth limit is reached, add to this
                if (allowedDepthRemaining == 0u)
                {
                    entries.push_back(qMove(EntryT(BBox(bbox_), element)));
                    return;
                }

                // Try to fit into subnodes
                for(auto idx = 0u; idx < 4; idx++)
                {
                    if (!subnodes[idx])
                    {
                        const auto subArea = area.getQuadrant(static_cast<Quadrant>(idx));
                        if (!subArea.contains(bbox_))
                            continue;
                        subnodes[idx] = new Node(subArea);
                        subnodes[idx]->insertNoCheck(element, bbox_, allowedDepthRemaining - 1);
                        return;
                    }

                    if (subnodes[idx]->insert(element, bbox_, allowedDepthRemaining - 1))
                        return;
                }

                // Otherwise, add to current node
                entries.push_back(qMove(EntryT(BBox(bbox_), element)));
            }

            inline void insertNoCheck(const ELEMENT_TYPE& element, const BBox& bbox_, const uintmax_t allowedDepthRemaining)
            {
                if (bbox_.type == BBoxType::AABB)
                    insertNoCheck(element, bbox_.asAABB, allowedDepthRemaining);
                else /* if (bbox_.type == BBoxType::OOBB) */
                    insertNoCheck(element, bbox_.asOOBB, allowedDepthRemaining);
            }

            void get(QList<ELEMENT_TYPE>& outResults, const Acceptor acceptor) const
            {
                for (const auto& entry : constOf(entries))
                {
                    if (!acceptor || acceptor(entry.second, entry.first))
                        outResults.push_back(entry.second);
                }

                for (auto idx = 0u; idx < 4; idx++)
                {
                    if (!subnodes[idx])
                        continue;

                    subnodes[idx]->get(outResults, acceptor);
                }
            }

            void get(QList<EntryT>& outResults, const Acceptor acceptor) const
            {
                for (const auto& entry : constOf(entries))
                {
                    if (!acceptor || acceptor(entry.second, entry.first))
                        outResults.push_back(entry);
                }

                for (auto idx = 0u; idx < 4; idx++)
                {
                    if (!subnodes[idx])
                        continue;

                    subnodes[idx]->get(outResults, acceptor);
                }
            }

            template<typename BBOX_TYPE>
            void query(const BBOX_TYPE& bbox_, QList<ELEMENT_TYPE>& outResults, const bool strict, const Acceptor acceptor) const
            {
                // If this node can not contain the bbox and bbox doesn't intersect node,
                // the node can not have anything that will give positive result
                if (!area.contains(bbox_))
                {
                    if (strict)
                        return;
                    if (!bbox_.intersects(area))
                        return;
                }

                for(const auto& entry : constOf(entries))
                {
                    if (contains(bbox_, entry.first) || (!strict && intersects(bbox_, entry.first)))
                    {
                        if (!acceptor || acceptor(entry.second, entry.first))
                            outResults.push_back(entry.second);
                    }
                }

                for(auto idx = 0u; idx < 4; idx++)
                {
                    if (!subnodes[idx])
                        continue;

                    subnodes[idx]->query(bbox_, outResults, strict, acceptor);
                }
            }

            inline void query(
                const BBox& bbox_,
                QList<ELEMENT_TYPE>& outResults,
                const bool strict,
                const Acceptor acceptor) const
            {
                if (bbox_.type == BBoxType::AABB)
                    query(bbox_.asAABB, outResults, strict, acceptor);
                else /* if (bbox_.type == BBoxType::OOBB) */
                    query(bbox_.asOOBB, outResults, strict, acceptor);
            }

            template<typename BBOX_TYPE>
            bool test(const BBOX_TYPE& bbox_, const bool strict, const Acceptor acceptor) const
            {
                // If this node can not contain the bbox and bbox doesn't intersect node,
                // the node can not have anything that will give positive result
                if (!area.contains(bbox_))
                {
                    if (strict)
                        return false;
                    if (!bbox_.intersects(area))
                        return false;
                }

                for(const auto& entry : constOf(entries))
                {
                    if (contains(bbox_, entry.first) || (!strict && intersects(bbox_, entry.first)))
                    {
                        if (!acceptor || acceptor(entry.second, entry.first))
                            return true;
                    }
                }

                for(auto idx = 0u; idx < 4; idx++)
                {
                    if (!subnodes[idx])
                        continue;

                    if (subnodes[idx]->test(bbox_, strict, acceptor))
                        return true;
                }

                return false;
            }

            inline bool test(const BBox& bbox_, const bool strict, const Acceptor acceptor) const
            {
                if (bbox_.type == BBoxType::AABB)
                    return test(bbox_.asAABB, strict, acceptor);
                else /* if (bbox_.type == BBoxType::OOBB) */
                    return test(bbox_.asOOBB, strict, acceptor);
            }

            inline void select(const PointT& point, QList<ELEMENT_TYPE>& outResults, const Acceptor acceptor) const
            {
                // If this node can not contain the point, the node can not have anything that will give positive result
                if (!area.contains(point))
                    return;

                for (const auto& entry : constOf(entries))
                {
                    if (contains(entry.first, point))
                    {
                        if (!acceptor || acceptor(entry.second, entry.first))
                            outResults.push_back(entry.second);
                    }
                }

                for (auto idx = 0u; idx < 4; idx++)
                {
                    if (!subnodes[idx])
                        continue;

                    subnodes[idx]->select(point, outResults, acceptor);
                }
            }

            inline bool removeOne(const ELEMENT_TYPE& element, const BBox& bbox_)
            {
                if (!contains(area, bbox_) && !intersects(bbox_, area))
                    return false;

                auto itEntry = mutableIteratorOf(entries);
                while (itEntry.hasNext())
                {
                    const auto& entry = itEntry.next();

                    if (entry.second != element)
                        continue;

                    itEntry.remove();
                    return true;
                }

                for (auto idx = 0u; idx < 4; idx++)
                {
                    if (!subnodes[idx])
                        continue;

                    if (subnodes[idx]->removeOne(element, bbox_))
                        return true;
                }

                return false;
            }

            inline unsigned int removeAll(const ELEMENT_TYPE& element, const BBox& bbox_)
            {
                if (!contains(area, bbox_) && !intersects(bbox_, area))
                    return false;

                unsigned int removedCount = 0;

                auto itEntry = mutableIteratorOf(entries);
                while (itEntry.hasNext())
                {
                    const auto& entry = itEntry.next();

                    if (entry.second != element)
                        continue;

                    itEntry.remove();
                    removedCount++;
                }

                for (auto idx = 0u; idx < 4; idx++)
                {
                    if (!subnodes[idx])
                        continue;

                    removedCount += subnodes[idx]->removeAll(element, bbox_);
                }

                return removedCount;
            }

            inline bool removeOneSlow(const ELEMENT_TYPE& element)
            {
                auto itEntry = mutableIteratorOf(entries);
                while (itEntry.hasNext())
                {
                    const auto& entry = itEntry.next();

                    if (entry.second != element)
                        continue;

                    itEntry.remove();
                    return true;
                }

                for (auto idx = 0u; idx < 4; idx++)
                {
                    if (!subnodes[idx])
                        continue;

                    if (subnodes[idx]->removeOneSlow(element))
                        return true;
                }

                return false;
            }

            inline unsigned int removeAllSlow(const ELEMENT_TYPE& element)
            {
                unsigned int removedCount = 0;

                auto itEntry = mutableIteratorOf(entries);
                while (itEntry.hasNext())
                {
                    const auto& entry = itEntry.next();

                    if (entry.second != element)
                        continue;

                    itEntry.remove();
                    removedCount++;
                }

                for (auto idx = 0u; idx < 4; idx++)
                {
                    if (!subnodes[idx])
                        continue;

                    removedCount += subnodes[idx]->removeAllSlow(element);
                }

                return removedCount;
            }

            inline unsigned int removeSlow(const Acceptor acceptor)
            {
                unsigned int removedCount = 0;

                auto itEntry = mutableIteratorOf(entries);
                while (itEntry.hasNext())
                {
                    const auto& entry = itEntry.next();

                    if (!acceptor(entry.second, entry.first))
                        continue;

                    itEntry.remove();
                    removedCount++;
                }

                for (auto idx = 0u; idx < 4; idx++)
                {
                    if (!subnodes[idx])
                        continue;

                    removedCount += subnodes[idx]->removeSlow(acceptor);
                }

                return removedCount;
            }

            inline void clear()
            {
                entries.clear();
                for (auto idx = 0u; idx < 4; idx++)
                {
                    if (!subnodes[idx])
                        continue;

                    subnodes[idx] = nullptr;
                }
            }

            template<typename BBOX_TYPE>
            static bool contains(const BBOX_TYPE& which, const BBox& what)
            {
                if (what.type == BBoxType::AABB)
                    return which.contains(what.asAABB);
                else /* if (what.type == BBoxType::OOBB) */
                    return which.contains(what.asOOBB);
            }

            static bool contains(const BBox& which, const PointT& what)
            {
                if (which.type == BBoxType::AABB)
                    return which.asAABB.contains(what);
                else /* if (which.type == BBoxType::OOBB) */
                    return which.asOOBB.contains(what);
            }

            static bool intersects(const BBox& which, const BBox& what)
            {
                if (which.type == BBoxType::AABB)
                    return intersects(which.asAABB, what);
                else /* if (which.type == BBoxType::OOBB) */
                    return intersects(which.asOOBB, what);
            }

            template<typename BBOX_TYPE>
            static bool intersects(const BBOX_TYPE& which, const BBox& what)
            {
                if (what.type == BBoxType::AABB)
                    return which.intersects(what.asAABB);
                else /* if (what.type == BBoxType::OOBB) */
                    return which.intersects(what.asOOBB);
            }

        private:
            Q_DISABLE_MOVE(Node);
            Q_DISABLE_ASSIGN(Node);
        };

        Ref< Node > _rootNode;
    public:
        inline QuadTree(
            const AreaT& rootArea = AreaT::largest(),
            const uintmax_t maxDepth_ = std::numeric_limits<uintmax_t>::max())
            : _rootNode(new Node(rootArea))
            , maxDepth(std::max(maxDepth_, static_cast<uintmax_t>(1u)))
        {
        }

        inline QuadTree(const QuadTreeT& that)
            : _rootNode(new Node(*that._rootNode))
            , maxDepth(that.maxDepth)
        {
        }

#ifdef Q_COMPILER_RVALUE_REFS
        inline QuadTree(QuadTreeT&& that)
            : _rootNode(that._rootNode)
            , maxDepth(that.maxDepth)
        {
            that._rootNode = new Node(_rootNode->area);
        }
#endif // Q_COMPILER_RVALUE_REFS

        virtual ~QuadTree()
        {
        }

        inline QuadTreeT& operator=(const QuadTreeT& that)
        {
            if (this != &that)
            {
                maxDepth = that.maxDepth;
                _rootNode = new Node(*that._rootNode);
            }
            return *this;
        }

#ifdef Q_COMPILER_RVALUE_REFS
        inline QuadTreeT& operator=(QuadTreeT&& that)
        {
            if (this != &that)
            {
                maxDepth = that.maxDepth;
                _rootNode = that._rootNode;

                that._rootNode = new Node(_rootNode->area);
            }
            return *this;
        }
#endif // Q_COMPILER_RVALUE_REFS

        uintmax_t maxDepth;

        inline AreaT getRootArea() const
        {
            return _rootNode->area;
        }

        inline const AreaT& rootArea() const
        {
            return _rootNode->area;
        }

        inline bool insert(const ELEMENT_TYPE& entry, const BBox& bbox, const bool strict = false)
        {
            if (bbox.type == BBoxType::AABB)
                return insert(entry, bbox.asAABB, strict);
            else /* if (bbox.type == BBoxType::OOBB) */
                return insert(entry, bbox.asOOBB, strict);
        }

        template<class ITERATOR_TYPE>
        inline int insertFrom(
            const ITERATOR_TYPE& itBegin,
            const ITERATOR_TYPE& itEnd,
            const std::function<bool(const ELEMENT_TYPE& item, BBox& outBbox)> obtainBBox,
            const bool strict = false)
        {
            int insertedCount = 0;

            for (auto itItem = itBegin; itItem != itEnd; ++itItem)
            {
                const auto& item = *itItem;
                BBox bbox;
                if (!obtainBBox(item, bbox))
                    continue;

                if (insert(item, bbox, strict))
                    insertedCount++;
            }

            return insertedCount;
        }

        template<class CONTAINER_TYPE>
        inline int insertFrom(
            const CONTAINER_TYPE& container,
            const std::function<bool(const ELEMENT_TYPE& item, BBox& outBbox)> obtainBBox,
            const bool strict = false)
        {
            return insertFrom(std::begin(container), std::end(container), obtainBBox, strict);
        }

        inline void query(
            const BBox& bbox,
            QList<ELEMENT_TYPE>& outResults,
            const bool strict = false,
            const Acceptor acceptor = nullptr) const
        {
            if (bbox.type == BBoxType::AABB)
                query(bbox.asAABB, outResults, strict, acceptor);
            else /* if (bbox.type == BBoxType::OOBB) */
                query(bbox.asOOBB, outResults, strict, acceptor);
        }

        inline bool test(const BBox& bbox, const bool strict = false, const Acceptor acceptor = nullptr) const
        {
            if (bbox.type == BBoxType::AABB)
                return test(bbox.asAABB, strict, acceptor);
            else /* if (bbox.type == BBoxType::OOBB) */
                return test(bbox.asOOBB, strict, acceptor);
        }

        inline bool insert(const ELEMENT_TYPE& entry, const AreaT& bbox, const bool strict = false)
        {
            // Check if this root can hold entire element
            if (!_rootNode->area.contains(bbox))
            {
                if (strict)
                    return false;
                if (!bbox.intersects(_rootNode->area))
                    return false;
            }

            _rootNode->insertNoCheck(entry, bbox, maxDepth - 1);
            return true;
        }

        inline void query(
            const AreaT& bbox,
            QList<ELEMENT_TYPE>& outResults,
            const bool strict = false,
            const Acceptor acceptor = nullptr) const
        {
            _rootNode->query(bbox, outResults, strict, acceptor);
        }

        inline bool test(const AreaT& bbox, const bool strict = false, const Acceptor acceptor = nullptr) const
        {
            return _rootNode->test(bbox, strict, acceptor);
        }

        inline bool insert(const ELEMENT_TYPE& entry, const OOBBT& bbox, const bool strict = false)
        {
            // Check if this root can hold entire element
            if (!_rootNode->area.contains(bbox))
            {
                if (strict)
                    return false;
                if (!bbox.intersects(_rootNode->area))
                    return false;
            }

            _rootNode->insertNoCheck(entry, bbox, maxDepth - 1);
            return true;
        }

        inline void get(QList<ELEMENT_TYPE>& outResults, const Acceptor acceptor = nullptr) const
        {
            _rootNode->get(outResults, acceptor);
        }

        inline void get(QList<EntryT>& outResults, const Acceptor acceptor = nullptr) const
        {
            _rootNode->get(outResults, acceptor);
        }

        inline void query(
            const OOBBT& bbox,
            QList<ELEMENT_TYPE>& outResults,
            const bool strict = false,
            const Acceptor acceptor = nullptr) const
        {
            _rootNode->query(bbox, outResults, strict, acceptor);
        }

        inline bool test(const OOBBT& bbox, const bool strict = false, const Acceptor acceptor = nullptr) const
        {
            return _rootNode->test(bbox, strict, acceptor);
        }

        inline void select(const PointT& point, QList<ELEMENT_TYPE>& outResults, const Acceptor acceptor = nullptr) const
        {
            _rootNode->select(point, outResults, acceptor);
        }

        inline bool removeOne(const ELEMENT_TYPE& entry, const BBox& bbox)
        {
            return _rootNode->removeOne(entry, bbox);
        }

        inline unsigned int removeAll(const ELEMENT_TYPE& entry, const BBox& bbox)
        {
            return _rootNode->removeAll(entry, bbox);
        }

        inline bool removeOneSlow(const ELEMENT_TYPE& entry)
        {
            return _rootNode->removeOneSlow(entry);
        }

        inline unsigned int removeAllSlow(const ELEMENT_TYPE& entry)
        {
            return _rootNode->removeAllSlow(entry);
        }

        inline unsigned int removeSlow(const Acceptor acceptor)
        {
            return _rootNode->removeAllSlow(acceptor);
        }

        inline void clear()
        {
            return _rootNode->clear();
        }
    };
}

#endif // !defined(_OSMAND_CORE_QUAD_TREE_H_)
