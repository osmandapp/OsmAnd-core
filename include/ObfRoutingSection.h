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

#ifndef __OBF_ROUTING_REGION_SECTION_H_
#define __OBF_ROUTING_REGION_SECTION_H_

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
    'Routing Region' section of OsmAnd Binary File
    */
    struct OSMAND_CORE_API ObfRoutingSection : public ObfSection
    {
        struct OSMAND_CORE_API Subregion
        {
            //! Ctor
            Subregion(ObfRoutingSection* section);

            //! Pointer to parent section
            ObfRoutingSection* const _section;

            //! ???
            int _length;
            
            //! ???
            int _offset;
            
            //! ???
            int _left;
            
            //! ???
            int _right;
            
            //! ???
            int _top;
            
            //! ???
            int _bottom;
            
            //! ???
            int _shiftToData;
            
            //! ???
            std::list< std::shared_ptr<Subregion> > _subregions;

            //! ???
            //List<RouteDataObject> dataObjects = null;
            /*
            public int getEstimatedSize(){
                int shallow = 7 * INT_SIZE + 4*3;
                if (subregions != null) {
                    shallow += 8;
                    for (RouteSubregion s : subregions) {
                        shallow += s.getEstimatedSize();
                    }
                }
                return shallow;
            }

            public int countSubregions(){
                int cnt = 1;
                if (subregions != null) {
                    for (RouteSubregion s : subregions) {
                        cnt += s.countSubregions();
                    }
                }
                return cnt;
            }
            */
        protected:
            static Subregion* read(gpb::io::CodedInputStream* cis, Subregion* current, Subregion* parent, int depth, bool readCoordinates);

        friend struct ObfRoutingSection;
        };

        struct TypeRule
        {
            enum Type : int
            {
                ACCESS = 1,
                ONEWAY = 2,
                HIGHWAY_TYPE = 3,
                MAXSPEED = 4,
                ROUNDABOUT = 5,
                TRAFFIC_SIGNALS = 6,
                RAILWAY_CROSSING = 7,
                LANES = 8,
            };

            //! Ctor
            TypeRule(std::string tag, std::string value);

            //! ???
            const std::string _tag;
            
            //! ???
            const std::string _value;

            //! ???
            int _intValue;

            //! ???
            float _floatValue;

            //! ???
            Type _type;

            //! ???
            /*
            public boolean roundabout(){
                return type == ROUNDABOUT;
            }

            public int getType() {
                return type;
            }

            public int onewayDirection(){
                if(type == ONEWAY){
                    return intValue;
                }
                return 0;
            }

            public float maxSpeed(){
                if(type == MAXSPEED){
                    return floatValue;
                }
                return -1;
            }

            public int lanes(){
                if(type == LANES){
                    return intValue;
                }
                return -1;
            }

            public String highwayRoad(){
                if(type == HIGHWAY_TYPE){
                    return v;
                }
                return null;
            }

            */
        };

        ObfRoutingSection();
        virtual ~ObfRoutingSection();

        //! ???
        int _regionsRead;
        
        //! ???
        int _borderBoxPointer;
        
        //! ???
        int _baseBorderBoxPointer;
        
        //! ???
        int _borderBoxLength;
        
        //! ???
        int _baseBorderBoxLength;

        //! ???
        std::list< std::shared_ptr<Subregion> > _subregions;
        
        //! ???
        std::list< std::shared_ptr<Subregion> > _baseSubregions;
        
        //! ???
        std::list< std::shared_ptr<TypeRule> > _routeEncodingRules;

        //! ???
        int _nameTypeRule;
        
        //! ???
        int _refTypeRule;

    private:
        //! ???
        void initRouteEncodingRule(int id, std::string tags, std::string val);
    protected:
        static void read(gpb::io::CodedInputStream* cis, ObfRoutingSection* section);

    private:
        //! ???
        static void readRouteEncodingRule(gpb::io::CodedInputStream* cis, ObfRoutingSection* section, int id);

    friend class ObfReader;
    };

} // namespace OsmAnd

#endif // __OBF_ROUTING_REGION_SECTION_H_