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
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <string>
#include <google/protobuf/io/coded_stream.h>
#include <ObfSection.h>

namespace OsmAnd {

    namespace gpb = google::protobuf;

    /**
    'Map' section of OsmAnd Binary File
    */
    struct OSMAND_CORE_API ObfMapSection : public ObfSection
    {
        ObfMapSection(class ObfReader* owner);
        virtual ~ObfMapSection();

        struct LevelTree
        {
            int32_t _mapDataBlock;
            bool _isOcean;
            int32_t _left;
            int32_t _right;
            int32_t _top;
            int32_t _bottom;
        };

        struct MapLevel
        {
            int32_t _offset;
            int32_t _length;
            uint32_t _minZoom;
            uint32_t _maxZoom;
            int32_t _left;
            int32_t _right;
            int32_t _top;
            int32_t _bottom;
            QList< std::shared_ptr<LevelTree> > _trees;
        };

        QList< std::shared_ptr<MapLevel> > _levels;

        //! ???
        //std::map<std::string, std::map<std::string, int> > _encodingRules;
            
        //! ???
        //std::unordered_map<int, std::tuple<std::string, std::string, int> > _decodingRules;

        //! ???
        //int _nameEncodingType;
            
        //! ???
        //int _refEncodingType;
            
        //! ???
        //int _coastlineEncodingType;
            
        //! ???
        //int _coastlineBrokenEncodingType;
            
        //! ???
        //int _landEncodingType;
            
        //! ???
        //int _onewayAttribute ;
            
        //! ???
        //int _onewayReverseAttribute ;

        //! ???
        //std::unordered_set<int> _positiveLayers;
            
        //! ???
        //std::unordered_set<int> _negativeLayers;

        //! ???
        //bool getRule(std::string t, std::string v, int& outRule);

        //! ???
        //std::tuple<std::string, std::string, int> decodeType(int type);
            
        //! ???
        //void finishInitializingTags();
            
        //! ???
        //void initMapEncodingRule(int type, int id, std::string tag, std::string val);
            
        bool isBaseMap();

        static void queryMapObjects(ObfReader* reader, ObfMapSection* section/*, QList< std::shared_ptr<> >* resultOut*/);
    protected:
        static void read(ObfReader* reader, ObfMapSection* section);
        static void readMapLevel(ObfReader* reader, ObfMapSection* section, MapLevel* level);

    private:
        //! ???
        //static void readMapEncodingRule(ObfReader* reader, ObfMapSection* section, int id);

        //! ???
        //static void readMapTreeBounds(ObfReader* reader, MapTree* tree, int left, int right, int top, int bottom);

    friend class ObfReader;
    };

} // namespace OsmAnd

#endif // __OBF_MAP_SECTION_H_