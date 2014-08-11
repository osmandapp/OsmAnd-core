#ifndef _OSMAND_CORE_BUILDING_H_
#define _OSMAND_CORE_BUILDING_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/MemoryCommon.h>

namespace OsmAnd
{
    namespace Model
    {
        class OSMAND_CORE_API Building
        {
            Q_DISABLE_COPY_AND_MOVE(Building);
        public:
            enum Interpolation 
            {
                Invalid = 0,
                All = -1,
                Even = -2,
                Odd = -3,
                Alphabetic = -4
            };

        private:
        protected:
        public:
            Building();
            virtual ~Building();

            int64_t _id;
            QString _name;
            QString _latinName;
            QString _name2; // WTF?
            QString _latinName2; // WTF?
            QString _postcode;
            uint32_t _xTile24;
            uint32_t _yTile24;
            uint32_t _x2Tile24;
            uint32_t _y2Tile24;
            Interpolation _interpolation;
            uint32_t _interpolationInterval;
            uint32_t _offset;
        };

    } // namespace Model

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_BUILDING_H_)
