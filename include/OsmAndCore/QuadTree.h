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
        typedef std::function<bool(const ELEMENT_TYPE& element, const AreaT& area)> Acceptor;
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
            typedef std::pair<AreaT, ELEMENT_TYPE> EntryPair;
            QList< EntryPair > entries;

            bool insert(const ELEMENT_TYPE& element, const AreaT& area_, const bool strict, const uintmax_t allowedDepthRemaining)
            {
                // Check if this node can hold entire element
                if(!(area.contains(area_) || (!strict && area.intersects(area_))))
                    return false;

                insertNoCheck(element, area_, allowedDepthRemaining);
                return true;
            }

            void insertNoCheck(const ELEMENT_TYPE& element, const AreaT& area_, const uintmax_t allowedDepthRemaining)
            {
                // If depth limit is reached, add to this
                if(allowedDepthRemaining == 0u)
                {
                    entries.push_back(qMove(EntryPair(area_, element)));
                    return;
                }

                // Try to fit into subnodes
                for(auto idx = 0u; idx < 4; idx++)
                {
                    if(!subnodes[idx])
                    {
                        const auto subArea = area.getQuadrant(static_cast<typename AreaT::Quadrant>(idx));
                        if(!subArea.contains(area_))
                            continue;
                        subnodes[idx].reset(new Node(subArea));
                        subnodes[idx]->insertNoCheck(element, area_, allowedDepthRemaining - 1);
                        return;
                    }

                    if(subnodes[idx]->insert(element, area_, true, allowedDepthRemaining - 1))
                        return;
                }

                // Otherwise, add to current node
                entries.push_back(qMove(EntryPair(area_, element)));
                return;
            }

            void query(const AreaT& area_, QList<ELEMENT_TYPE>& outResults, const bool strict, const Acceptor acceptor) const
            {
                if(!(area_.contains(area) || (!strict && area_.intersects(area))))
                    return;

                for(auto itEntry = entries.cbegin(); itEntry != entries.cend(); ++itEntry)
                {
                    if(area_.contains(itEntry->first) || (!strict && area_.intersects(itEntry->first)))
                    {
                        if(!acceptor || acceptor(itEntry->second, itEntry->first))
                            outResults.push_back(itEntry->second);
                    }
                }

                for(auto idx = 0u; idx < 4; idx++)
                {
                    if(!subnodes[idx])
                        continue;

                    subnodes[idx]->query(area_, outResults, strict, acceptor);
                }
            }

            bool test(const AreaT& area_, const bool strict, const Acceptor acceptor) const
            {
                if(!(area_.contains(area) || (!strict && area_.intersects(area))))
                    return false;

                for(auto itEntry = entries.cbegin(); itEntry != entries.cend(); ++itEntry)
                {
                    if(area_.contains(itEntry->first) || (!strict && area_.intersects(itEntry->first)))
                    {
                        if(!acceptor || acceptor(itEntry->second, itEntry->first))
                            return true;
                    }
                }

                for(auto idx = 0u; idx < 4; idx++)
                {
                    if(!subnodes[idx])
                        continue;

                    if(subnodes[idx]->test(area_, strict, acceptor))
                        return true;
                }

                return false;
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

        bool insert(const ELEMENT_TYPE& entry, const AreaT& area, const bool strict = false)
        {
            return _root->insert(entry, area, strict, maxDepth-1);
        }

        void query(const AreaT& area, QList<ELEMENT_TYPE>& outResults, const bool strict = false, const Acceptor acceptor = nullptr) const
        {
            _root->query(area, outResults, strict, acceptor);
        }

        bool test(const AreaT& area, const bool strict = false, const Acceptor acceptor = nullptr) const
        {
            return _root->test(area, strict, acceptor);
        }
    };
}

#endif // !defined(_OSMAND_CORE_QUAD_TREE_H_)
