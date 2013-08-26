/**
* @file
*
* @section LICENSE
*
* OsmAnd - Android navigation software based on OSM maps.
* Copyright (C) 2010-2013  OsmAnd Authors listed in AUTHORS file
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __OBF_MAP_SECTION_READER_P_H_
#define __OBF_MAP_SECTION_READER_P_H_

#include <stdint.h>
#include <memory>
#include <functional>

#include <QHash>
#include <QMap>
#include <QSet>

#include <OsmAndCore.h>
#include <CommonTypes.h>
#include <ObfMapSectionInfo_P.h>
#include <MapTypes.h>

namespace OsmAnd {

    class ObfReader_P;
    class ObfMapSectionInfo;
    class ObfMapSectionLevel;
    namespace Model {
        class MapObject;
    } // namespace Model
    class IQueryController;

    class ObfMapSectionReader;
    class OSMAND_CORE_API ObfMapSectionReader_P
    {
    private:
        ObfMapSectionReader_P();
        ~ObfMapSectionReader_P();

    protected:
        struct LevelTreeNode
        {
            LevelTreeNode();

            bool _isOcean;
            uint32_t _offset;
            uint32_t _length;
            uint32_t _childrenInnerOffset;
            uint32_t _dataOffset;
            MapFoundationType _foundation;
            AreaI _area31;
        };

        static void read(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<ObfMapSectionInfo>& section);

        static void readMapLevelHeader(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfMapSectionInfo>& section,
            const std::shared_ptr<ObfMapSectionLevel>& level);

        static void readRules(const std::unique_ptr<ObfReader_P>& reader,
            const std::shared_ptr<ObfMapSectionInfo_P::Rules>& rules);
        static void readRule(const std::unique_ptr<ObfReader_P>& reader,
            uint32_t defaultId, const std::shared_ptr<ObfMapSectionInfo_P::Rules>& rules);
        static void createRule(const std::shared_ptr<ObfMapSectionInfo_P::Rules>& rules, uint32_t type, uint32_t id, const QString& tag, const QString& val);

        static void readMapLevelTreeNodes(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfMapSectionInfo>& section,
            const std::shared_ptr<const ObfMapSectionLevel>& level, QList< std::shared_ptr<LevelTreeNode> >& nodes);
        static void readTreeNode(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfMapSectionInfo>& section,
            const AreaI& parentArea, const std::shared_ptr<LevelTreeNode>& treeNode);
        static void readTreeNodeChildren(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfMapSectionInfo>& section,
            const std::shared_ptr<LevelTreeNode>& treeNode,
            QList< std::shared_ptr<LevelTreeNode> >* nodesWithData,
            const AreaI* bbox31,
            IQueryController* controller);

        static void readMapObjectsBlock(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfMapSectionInfo>& section,
            LevelTreeNode* treeNode,
            QList< std::shared_ptr<const OsmAnd::Model::MapObject> >* resultOut,
            const AreaI* bbox31,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::MapObject>&)> visitor,
            IQueryController* controller);

        static void readMapObject(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfMapSectionInfo>& section,
            LevelTreeNode* treeNode,
            uint64_t baseId,
            std::shared_ptr<OsmAnd::Model::MapObject>& mapObjectOut,
            const AreaI* bbox31);

        enum {
            ShiftCoordinates = 5,
            MaskToRead = ~((1u << ShiftCoordinates) - 1),
        };

        static void loadMapObjects(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfMapSectionInfo>& section,
            ZoomLevel zoom, const AreaI* bbox31 = nullptr,
            QList< std::shared_ptr<const OsmAnd::Model::MapObject> >* resultOut = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::MapObject>&)> visitor = nullptr,
            IQueryController* controller = nullptr);

    friend class OsmAnd::ObfMapSectionReader;
    friend class OsmAnd::ObfReader_P;
    };

} // namespace OsmAnd

#endif // __OBF_MAP_SECTION_READER_P_H_
