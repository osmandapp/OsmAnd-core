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

#include <OsmAndCore.h>
#include <memory>
#include <QList>
#include <QMap>
#include <QSet>
#include <tuple>
#include <google/protobuf/io/coded_stream.h>
#include <ObfSection.h>
#include <IQueryFilter.h>
#include <IQueryCallback.h>

namespace OsmAnd {

    namespace gpb = google::protobuf;

    /**
    'Map' section of OsmAnd Binary File
    */
    struct OSMAND_CORE_API ObfMapSection : public ObfSection
    {
        ObfMapSection(class ObfReader* owner);
        virtual ~ObfMapSection();

        struct MapObject
        {
            uint64_t _id;
            bool _isArea;
            QList<uint32_t> _types;
            QList<uint32_t> _extraTypes;
            QMap<uint32_t, uint32_t> _names;
        };

        struct LevelTree
        {
            int32_t _offset;
            int32_t _length;
            int32_t _mapDataBlock;
            bool _isOcean;
            uint32_t _left31;
            uint32_t _right31;
            uint32_t _top31;
            uint32_t _bottom31;
        };

        struct MapLevel
        {
            int32_t _offset;
            int32_t _length;
            uint32_t _minZoom;
            uint32_t _maxZoom;
            uint32_t _left31;
            uint32_t _right31;
            uint32_t _top31;
            uint32_t _bottom31;
            QList< std::shared_ptr<LevelTree> > _trees;
        };

        QList< std::shared_ptr<MapLevel> > _levels;

        QMap< QString, QMap<QString, uint32_t> > _encodingRules;
        typedef std::tuple<QString, QString, uint32_t> DecodingRule;
        QMap< uint32_t, DecodingRule > _decodingRules;
        uint32_t _nameEncodingType;
        uint32_t _refEncodingType;
        uint32_t _coastlineEncodingType;
        uint32_t _coastlineBrokenEncodingType;
        uint32_t _landEncodingType;
        uint32_t _onewayAttribute ;
        uint32_t _onewayReverseAttribute ;
        QSet<uint32_t> _positiveLayers;
        QSet<uint32_t> _negativeLayers;

        //! ???
        //bool getRule(std::string t, std::string v, int& outRule);

        //! ???
        //std::tuple<std::string, std::string, int> decodeType(int type);
        bool isBaseMap();
        void loadRules(ObfReader* reader);
        static void queryMapObjects(ObfReader* reader, ObfMapSection* section, QList< std::shared_ptr<OsmAnd::ObfMapSection::MapObject> >* resultOut = nullptr, IQueryFilter* filter = nullptr, IQueryCallback* callback = nullptr);
    protected:
        static void read(ObfReader* reader, ObfMapSection* section);
        static void readMapLevel(ObfReader* reader, ObfMapSection* section, MapLevel* level);
        static void readEncodingRules(ObfReader* reader,
            QMap< QString, QMap<QString, uint32_t> >& encodingRules,
            QMap< uint32_t, DecodingRule >& decodingRules,
            uint32_t& nameEncodingType,
            uint32_t& coastlineEncodingType,
            uint32_t& landEncodingType,
            uint32_t& onewayAttribute,
            uint32_t& onewayReverseAttribute,
            uint32_t& refEncodingType,
            uint32_t& coastlineBrokenEncodingType,
            QSet<uint32_t>& negativeLayers,
            QSet<uint32_t>& positiveLayers);
        static void readEncodingRule(ObfReader* reader, uint32_t defaultId,
            QMap< QString, QMap<QString, uint32_t> >& encodingRules,
            QMap< uint32_t, DecodingRule >& decodingRules,
            uint32_t& nameEncodingType,
            uint32_t& coastlineEncodingType,
            uint32_t& landEncodingType,
            uint32_t& onewayAttribute,
            uint32_t& onewayReverseAttribute,
            uint32_t& refEncodingType,
            QSet<uint32_t>& negativeLayers,
            QSet<uint32_t>& positiveLayers);
        static void readLevelTrees(ObfReader* reader, ObfMapSection* section, MapLevel* level, QList< std::shared_ptr<LevelTree> >& trees);
        static void readLevelTree(ObfReader* reader, ObfMapSection* section, MapLevel* level, LevelTree* tree);
        static void queryMapObjects(ObfReader* reader, ObfMapSection* section, MapLevel* level, LevelTree* tree, QList< std::shared_ptr<LevelTree> >& subtrees, IQueryFilter* filter, IQueryCallback* callback);
        static void readMapObjects(ObfReader* reader, ObfMapSection* section, LevelTree* tree, QList< std::shared_ptr<OsmAnd::ObfMapSection::MapObject> >* resultOut, IQueryFilter* filter, IQueryCallback* callback);
        static void readMapObject(ObfReader* reader, ObfMapSection* section, LevelTree* tree, std::shared_ptr<OsmAnd::ObfMapSection::MapObject>& mapObjectOut, IQueryFilter* filter);
        enum {
            ShiftCoordinates = 5,
            MaskToRead = ~((1 << ShiftCoordinates) - 1),
        };
    private:
    friend class ObfReader;
    };

} // namespace OsmAnd

#endif // __OBF_MAP_SECTION_H_