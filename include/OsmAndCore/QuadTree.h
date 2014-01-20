#ifndef _OSMAND_CORE_QUAD_TREE_H_
#define _OSMAND_CORE_QUAD_TREE_H_

#include <OsmAndCore/stdlib_common.h>
#include <utility>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    template<typename ENTRY, typename COORD_TYPE>
    class QuadTree
    {
    public:
        typedef Area<COORD_TYPE> AreaT;

    private:
        Q_DISABLE_COPY(QuadTree)
    protected:
        template<typename ENTRY>
        struct Node
        {
            Node(const AreaT& area_)
                : area(area_)
            {
            }

            const AreaT area;
            std::unique_ptr< Node<ENTRY> > subnodes[4];
            typedef std::pair<AreaT, ENTRY> EntryPair;
            QList< EntryPair > entries;

            bool insert(const ENTRY& entry, const AreaT& area_, const uintmax_t allowedDepthRemaining)
            {
                // Check if this node can hold entire entry
                if(!area.contains(area_))
                    return false;

                // If depth limit is reached, add to this
                if(allowedDepthRemaining == 0u)
                {
                    entries.push_back(qMove(EntryPair(area_, entry)));
                    return true;
                }

                // Try to fit into subnodes
                for(auto idx = 0u; idx < 4; idx++)
                {
                    if(!subnodes[idx])
                        subnodes[idx].reset(new Node<ENTRY>(area.getQuadrant(static_cast<AreaT::Quadrant>(idx))));

                    if(subnodes[idx]->insert(entry, area_, allowedDepthRemaining - 1))
                        return true;
                }

                // Otherwise, add to current node
                entries.push_back(qMove(EntryPair(area_, entry)));
                return true;
            }

            void query(const AreaT& area_, QList<ENTRY>& outResults, const bool strict) const
            {
                if(!(area_->contains(area) || (!strict && area_->intersects(area))))
                    return;

                for(auto itEntry = entries.cbegin(); itEntry != entries.cend(); ++itEntry)
                {
                    if(area_->contains(itEntry->first) || (!strict && area_->intersects(itEntry->first)))
                        outResults.push_back(itEntry->second);
                }

                for(auto idx = 0u; idx < 4; idx++)
                {
                    if(!subnodes[idx])
                        continue;

                    subnodes[idx]->query(area_, outResults, strict);
                }
            }

            bool test(const AreaT& area_, const bool strict) const
            {
                if(!(area_->contains(area) || (!strict && area_->intersects(area))))
                    return false;

                for(auto itEntry = entries.cbegin(); itEntry != entries.cend(); ++itEntry)
                {
                    if(area_->contains(itEntry->first) || (!strict && area_->intersects(itEntry->first)))
                        return true;
                }

                for(auto idx = 0u; idx < 4; idx++)
                {
                    if(!subnodes[idx])
                        continue;

                    if(subnodes[idx]->test(area_, strict))
                        return true;
                }

                return false;
            }
        };

        std::unique_ptr< Node<ENTRY> > _root;
    public:
        QuadTree(const uintmax_t maxDepth_ = std::numeric_limits<uintmax_t>::max())
            : _root(new Node(AreaT::largest()))
            , maxDepth(std::max(maxDepth_, 1))
        {
        }

        virtual ~QuadTree()
        {
        }

        const uintmax_t maxDepth;

        bool insert(const ENTRY& entry, const AreaT& area)
        {
            return _root->insert(entry, area, maxDepth-1);
        }

        void query(const AreaT& area, QList<ENTRY>& outResults, const bool strict = false) const
        {
            _root->query(area, outResults, strict);
        }

        bool test(const AreaT& area, const bool strict = false) const
        {
            return _root->test(area, strict);
        }
    };
}

#endif // _OSMAND_CORE_QUAD_TREE_H_
