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

#ifndef __OBF_MAP_SECTION_H_
#define __OBF_MAP_SECTION_H_

#include <memory>
#include <tuple>
#include <functional>

#include <google/protobuf/io/coded_stream.h>

#include <QList>
#include <QHash>
#include <QMap>
#include <QSet>
#include <QVector>

#include <OsmAndCore.h>
#include <ObfSection.h>
#include <QueryFilter.h>
#include <IQueryController.h>
#include <Area.h>

namespace OsmAnd {

    class ObfReader;
    namespace gpb = google::protobuf;

    namespace Model {
        class MapObject;
    } // namespace Model

    /**
    'Map' section of OsmAnd Binary File
    */
    class OSMAND_CORE_API ObfMapSection : public ObfSection
    {
    protected:
        struct LevelTreeNode
        {
            LevelTreeNode();

            uint32_t _offset;
            uint32_t _length;
            uint32_t _dataOffset;
            bool _isOcean;
            AreaI _area31;
        };

        typedef std::tuple<QString, QString, uint32_t> DecodingRule;
    public:
        class OSMAND_CORE_API MapLevel
        {
        private:
        protected:
            MapLevel();

            uint32_t _offset;
            uint32_t _length;
            uint32_t _minZoom;
            uint32_t _maxZoom;
            AreaI _area31;
        public:
            virtual ~MapLevel();

            const uint32_t& minZoom;
            const uint32_t& maxZoom;
            const uint32_t& length;
            const AreaI& area31;

            friend class OsmAnd::ObfMapSection;
        };

        class OSMAND_CORE_API Rules
        {
        private:
        protected:
            Rules();

            QHash< QString, QHash<QString, uint32_t> > _encodingRules;
            QMap< uint32_t, DecodingRule > _decodingRules;
            uint32_t _nameEncodingType;
            uint32_t _refEncodingType;
            uint32_t _coastlineEncodingType;
            uint32_t _coastlineBrokenEncodingType;
            uint32_t _landEncodingType;
            uint32_t _onewayAttribute;
            uint32_t _onewayReverseAttribute;
            QSet<uint32_t> _positiveLayers;
            QSet<uint32_t> _negativeLayers;
        public:
            virtual ~Rules();

            const QMap< uint32_t, DecodingRule >& decodingRules;
            bool obtainTagValueId(const QString& tag, const QString& value, uint32_t& outId) const;

        friend class OsmAnd::ObfMapSection;
        };
    private:
    protected:
        ObfMapSection(ObfReader* owner);

        bool _isBaseMap;

        QList< std::shared_ptr<MapLevel> > _mapLevels;
        std::shared_ptr< Rules > _rules;

        static void read(ObfReader* reader, ObfMapSection* section);
        static void readMapLevelHeader(ObfReader* reader, ObfMapSection* section, MapLevel* level);
        static void readRules(ObfReader* reader, Rules* rules);
        static void readRule(ObfReader* reader, uint32_t defaultId, Rules* rules);
        static void createRule(Rules* rules, uint32_t type, uint32_t id, const QString& tag, const QString& val);
        static void readMapLevelTreeNodes(ObfReader* reader, ObfMapSection* section, MapLevel* level, QList< std::shared_ptr<LevelTreeNode> >& nodes);
        static void readTreeNode(ObfReader* reader, ObfMapSection* section, const AreaI& parentArea, LevelTreeNode* treeNode);
        static void readTreeNodeChildren(ObfReader* reader, ObfMapSection* section,
            LevelTreeNode* treeNode,
            QList< std::shared_ptr<LevelTreeNode> >* nodesWithData,
            QueryFilter* filter,
            IQueryController* controller);
        static void readMapObjectsBlock(ObfReader* reader, ObfMapSection* section,
            LevelTreeNode* treeNode,
            QList< std::shared_ptr<OsmAnd::Model::MapObject> >* resultOut,
            QueryFilter* filter,
            std::function<bool (const std::shared_ptr<OsmAnd::Model::MapObject>&)> visitor,
            IQueryController* controller);
        static void readMapObject(ObfReader* reader, ObfMapSection* section,
            LevelTreeNode* treeNode,
            uint64_t baseId,
            std::shared_ptr<OsmAnd::Model::MapObject>& mapObjectOut,
            QueryFilter* filter);
        enum {
            ShiftCoordinates = 5,
            MaskToRead = ~((1 << ShiftCoordinates) - 1),
        };
    public:
        virtual ~ObfMapSection();

        const bool& isBaseMap;
        const QList< std::shared_ptr<MapLevel> >& mapLevels;
        const std::shared_ptr< Rules >& rules;
        
        static void loadMapObjects(ObfReader* reader, ObfMapSection* section,
            QList< std::shared_ptr<OsmAnd::Model::MapObject> >* resultOut = nullptr,
            QueryFilter* filter = nullptr,
            std::function<bool (const std::shared_ptr<OsmAnd::Model::MapObject>&)> visitor = nullptr,
            IQueryController* controller = nullptr);

    friend class OsmAnd::ObfReader;
    };

} // namespace OsmAnd

#endif // __OBF_MAP_SECTION_H_
