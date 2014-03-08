#ifndef _OSMAND_CORE_QUAD_TREE_H_
#define _OSMAND_CORE_QUAD_TREE_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>
#include <utility>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    template<typename ELEMENT_TYPE, typename COORD_TYPE>
    class QuadTree
    {
    public:
        typedef Area<COORD_TYPE> AreaT;
        typedef OOBB<COORD_TYPE> OOBBT;
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
                if(that.type == BBoxType::AABB)
                {
                    new(&asAABB) AreaT();
                    asAABB = that.asAABB;
                }
                else /* if(that.type == BBoxType::OOBB) */
                {
                    new(&asOOBB) OOBBT();
                    asOOBB = that.asOOBB;
                }
            }

            inline BBox& operator=(const BBox& that)
            {
                if(this != &that)
                {
                    if(that.type == BBoxType::AABB)
                    {
                        new(&asAABB) AreaT();
                        asAABB = that.asAABB;
                    }
                    else /* if(that.type == BBoxType::OOBB) */
                    {
                        new(&asOOBB) OOBBT();
                        asOOBB = that.asOOBB;
                    }
                    type = that.type;
                }
                return *this;
            }

            uint8_t data[DataSize];
            AreaT& asAABB;
            OOBBT& asOOBB;
            BBoxType type;
        };

        typedef std::function<bool (const ELEMENT_TYPE& element, const BBox& bbox)> Acceptor;
    private:
        Q_DISABLE_COPY(QuadTree)
    protected:
        struct Node
        {
            Node(const AreaT& area_)
                : area(area_)
            {
            }

            const AreaT area;
            std::unique_ptr< Node > subnodes[4];
            typedef std::pair<BBox, ELEMENT_TYPE> EntryPair;
            QList< EntryPair > entries;

            template<typename BBOX_TYPE>
            bool insert(const ELEMENT_TYPE& element, const BBOX_TYPE& bbox_, const uintmax_t allowedDepthRemaining)
            {
                // Check if this node can hold entire element
                if(!area.contains(bbox_))
                    return false;

                insertNoCheck(element, bbox_, allowedDepthRemaining);
                return true;
            }

            inline bool insert(const ELEMENT_TYPE& element, const BBox& bbox_, const uintmax_t allowedDepthRemaining)
            {
                if(bbox_.type == BBoxType::AABB)
                    return insert(element, bbox_.asAABB, allowedDepthRemaining);
                else /* if(bbox_.type == BBoxType::OOBB) */
                    return insert(element, bbox_.asOOBB, allowedDepthRemaining);
            }

            template<typename BBOX_TYPE>
            void insertNoCheck(const ELEMENT_TYPE& element, const BBOX_TYPE& bbox_, const uintmax_t allowedDepthRemaining)
            {
                // If depth limit is reached, add to this
                if(allowedDepthRemaining == 0u)
                {
                    entries.push_back(qMove(EntryPair(BBox(bbox_), element)));
                    return;
                }

                // Try to fit into subnodes
                for(auto idx = 0u; idx < 4; idx++)
                {
                    if(!subnodes[idx])
                    {
                        const auto subArea = area.getQuadrant(static_cast<typename AreaT::Quadrant>(idx));
                        if(!subArea.contains(bbox_))
                            continue;
                        subnodes[idx].reset(new Node(subArea));
                        subnodes[idx]->insertNoCheck(element, bbox_, allowedDepthRemaining - 1);
                        return;
                    }

                    if(subnodes[idx]->insert(element, bbox_, allowedDepthRemaining - 1))
                        return;
                }

                // Otherwise, add to current node
                entries.push_back(qMove(EntryPair(BBox(bbox_), element)));
            }

            inline void insertNoCheck(const ELEMENT_TYPE& element, const BBox& bbox_, const uintmax_t allowedDepthRemaining)
            {
                if(bbox_.type == BBoxType::AABB)
                    insertNoCheck(element, bbox_.asAABB, allowedDepthRemaining);
                else /* if(bbox_.type == BBoxType::OOBB) */
                    insertNoCheck(element, bbox_.asOOBB, allowedDepthRemaining);
            }

            template<typename BBOX_TYPE>
            void query(const BBOX_TYPE& bbox_, QList<ELEMENT_TYPE>& outResults, const bool strict, const Acceptor acceptor) const
            {
                // If this node can not contain the bbox and bbox doesn't intersect node, the node can not have anything that will give positive result
                if(!area.contains(bbox_))
                {
                    if(strict)
                        return;
                    if(!bbox_.intersects(area))
                        return;
                }

                for(const auto& entry : constOf(entries))
                {
                    if(contains(bbox_, entry.first) || (!strict && intersects(bbox_, entry.first)))
                    {
                        if(!acceptor || acceptor(entry.second, entry.first))
                            outResults.push_back(entry.second);
                    }
                }

                for(auto idx = 0u; idx < 4; idx++)
                {
                    if(!subnodes[idx])
                        continue;

                    subnodes[idx]->query(bbox_, outResults, strict, acceptor);
                }
            }

            inline void query(const BBox& bbox_, QList<ELEMENT_TYPE>& outResults, const bool strict, const Acceptor acceptor) const
            {
                if(bbox_.type == BBoxType::AABB)
                    query(bbox_.asAABB, outResults, strict, acceptor);
                else /* if(bbox_.type == BBoxType::OOBB) */
                    query(bbox_.asOOBB, outResults, strict, acceptor);
            }

            template<typename BBOX_TYPE>
            bool test(const BBOX_TYPE& bbox_, const bool strict, const Acceptor acceptor) const
            {
                // If this node can not contain the bbox and bbox doesn't intersect node, the node can not have anything that will give positive result
                if(!area.contains(bbox_))
                {
                    if(strict)
                        return false;
                    if(!bbox_.intersects(area))
                        return false;
                }

                for(const auto& entry : constOf(entries))
                {
                    if(contains(bbox_, entry.first) || (!strict && intersects(bbox_, entry.first)))
                    {
                        if(!acceptor || acceptor(entry.second, entry.first))
                            return true;
                    }
                }

                for(auto idx = 0u; idx < 4; idx++)
                {
                    if(!subnodes[idx])
                        continue;

                    if(subnodes[idx]->test(bbox_, strict, acceptor))
                        return true;
                }

                return false;
            }

            inline bool test(const BBox& bbox_, const bool strict, const Acceptor acceptor) const
            {
                if(bbox_.type == BBoxType::AABB)
                    test(bbox_.asAABB, strict, acceptor);
                else /* if(bbox_.type == BBoxType::OOBB) */
                    test(bbox_.asOOBB, strict, acceptor);
            }

            template<typename BBOX_TYPE>
            static bool contains(const BBOX_TYPE& which, const BBox& what)
            {
                if(what.type == BBoxType::AABB)
                    return which.contains(what.asAABB);
                else /* if(bbox_.type == BBoxType::OOBB) */
                    return which.contains(what.asOOBB);
            }

            template<typename BBOX_TYPE>
            static bool intersects(const BBOX_TYPE& which, const BBox& what)
            {
                if(what.type == BBoxType::AABB)
                    return which.intersects(what.asAABB);
                else /* if(bbox_.type == BBoxType::OOBB) */
                    return which.intersects(what.asOOBB);
            }
        };

        std::unique_ptr< Node > _root;
    public:
        QuadTree(const AreaT& rootArea = AreaT::largest(), const uintmax_t maxDepth_ = std::numeric_limits<uintmax_t>::max())
            : _root(new Node(rootArea))
            , maxDepth(std::max(maxDepth_, static_cast<uintmax_t>(1u)))
        {
        }

        virtual ~QuadTree()
        {
        }

        const uintmax_t maxDepth;

        inline bool insert(const ELEMENT_TYPE& entry, const BBox& bbox, const bool strict = false)
        {
            if(bbox.type == BBoxType::AABB)
                return insert(entry, bbox.asAABB, strict);
            else /* if(bbox.type == BBoxType::OOBB) */
                return insert(entry, bbox.asOOBB, strict);
        }

        inline void query(const BBox& bbox, QList<ELEMENT_TYPE>& outResults, const bool strict = false, const Acceptor acceptor = nullptr) const
        {
            if(bbox.type == BBoxType::AABB)
                query(bbox.asAABB, outResults, strict, acceptor);
            else /* if(bbox.type == BBoxType::OOBB) */
                query(bbox.asOOBB, outResults, strict, acceptor);
        }

        inline bool test(const BBox& bbox, const bool strict = false, const Acceptor acceptor = nullptr) const
        {
            if(bbox.type == BBoxType::AABB)
                return test(bbox.asAABB, strict, acceptor);
            else /* if(bbox.type == BBoxType::OOBB) */
                return test(bbox.asOOBB, strict, acceptor);
        }

        inline bool insert(const ELEMENT_TYPE& entry, const AreaT& bbox, const bool strict = false)
        {
            // Check if this root can hold entire element
            if(!_root->area.contains(bbox))
            {
                if(strict)
                    return false;
                if(!bbox.intersects(_root->area))
                    return false;
            }

            _root->insertNoCheck(entry, bbox, maxDepth - 1);
            return true;
        }

        inline void query(const AreaT& bbox, QList<ELEMENT_TYPE>& outResults, const bool strict = false, const Acceptor acceptor = nullptr) const
        {
            _root->query(bbox, outResults, strict, acceptor);
        }

        inline bool test(const AreaT& bbox, const bool strict = false, const Acceptor acceptor = nullptr) const
        {
            return _root->test(bbox, strict, acceptor);
        }

        inline bool insert(const ELEMENT_TYPE& entry, const OOBBT& bbox, const bool strict = false)
        {
            // Check if this root can hold entire element
            if(!_root->area.contains(bbox))
            {
                if(strict)
                    return false;
                if(!bbox.intersects(_root->area))
                    return false;
            }

            _root->insertNoCheck(entry, bbox, maxDepth - 1);
            return true;
        }

        inline void query(const OOBBT& bbox, QList<ELEMENT_TYPE>& outResults, const bool strict = false, const Acceptor acceptor = nullptr) const
        {
            _root->query(bbox, outResults, strict, acceptor);
        }

        inline bool test(const OOBBT& bbox, const bool strict = false, const Acceptor acceptor = nullptr) const
        {
            return _root->test(bbox, strict, acceptor);
        }
    };
}

#endif // !defined(_OSMAND_CORE_QUAD_TREE_H_)
