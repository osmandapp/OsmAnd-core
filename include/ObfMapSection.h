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
#include <list>
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
        struct OSMAND_CORE_API MapTree
        {
            MapTree();

            off_t _offset;
            size_t _length;
            long _mapDataBlock;
            bool _isOcean;
            int _left;
            int _right;
            int _top;
            int _bottom;
        };

        struct OSMAND_CORE_API MapRoot : public MapTree
        {
            int _minZoom;
            int _maxZoom;

            //! ???
            std::list< std::shared_ptr<MapTree> > _trees;
        };

        ObfMapSection();
        virtual ~ObfMapSection();

        //! ???
        std::list< std::shared_ptr<MapRoot> > _levels;

        //! ???
        std::map<std::string, std::map<std::string, int> > _encodingRules;
            
        //! ???
        std::unordered_map<int, std::tuple<std::string, std::string, int> > _decodingRules;

        //! ???
        int _nameEncodingType;
            
        //! ???
        int _refEncodingType;
            
        //! ???
        int _coastlineEncodingType;
            
        //! ???
        int _coastlineBrokenEncodingType;
            
        //! ???
        int _landEncodingType;
            
        //! ???
        int _onewayAttribute ;
            
        //! ???
        int _onewayReverseAttribute ;

        //! ???
        std::unordered_set<int> _positiveLayers;
            
        //! ???
        std::unordered_set<int> _negativeLayers;

        //! ???
        bool getRule(std::string t, std::string v, int& outRule);

        //! ???
        std::tuple<std::string, std::string, int> decodeType(int type);
            
        //! ???
        void finishInitializingTags();
            
        //! ???
        void initMapEncodingRule(int type, int id, std::string tag, std::string val);
            
        //! ???
        bool isBaseMap();

    protected:
        //! ???
        static void read(gpb::io::CodedInputStream* cis, ObfMapSection* section, bool onlyInitEncodingRules);

    private:
        //! ???
        static void readMapEncodingRule(gpb::io::CodedInputStream* cis, ObfMapSection* section, int id);

        //! ???
        static void readMapTreeBounds(gpb::io::CodedInputStream* cis, MapTree* tree, int left, int right, int top, int bottom);

        //! ???
        static MapRoot* readMapLevel(gpb::io::CodedInputStream* cis, MapRoot* root, bool initSubtrees);

    friend class ObfReader;
    };

} // namespace OsmAnd

#endif // __OBF_MAP_SECTION_H_