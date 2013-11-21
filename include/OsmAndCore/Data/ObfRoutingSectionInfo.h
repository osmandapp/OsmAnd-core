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

#ifndef _OSMAND_CORE_OBF_ROUTING_SECTION_INFO_H_
#define _OSMAND_CORE_OBF_ROUTING_SECTION_INFO_H_

#include <cstdint>
#include <memory>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QVector>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/ObfSectionInfo.h>

namespace OsmAnd {

    class ObfRoutingSectionReader_P;
    class ObfReader_P;
    class ObfRoutingSubsectionInfo;
    namespace Model {
        class Road;
    } // namespace Model
    class RoutePlanner;
    class RoutePlannerContext;
    class RoutingRulesetContext;

    class ObfRoutingSectionInfo_P;
    class OSMAND_CORE_API ObfRoutingSectionInfo : public ObfSectionInfo
    {
        Q_DISABLE_COPY(ObfRoutingSectionInfo)
    private:
        const std::unique_ptr<ObfRoutingSectionInfo_P> _d;
    protected:
        ObfRoutingSectionInfo(const std::weak_ptr<ObfInfo>& owner);

        QList< std::shared_ptr<ObfRoutingSubsectionInfo> > _subsections;
        QList< std::shared_ptr<ObfRoutingSubsectionInfo> > _baseSubsections;
    public:
        virtual ~ObfRoutingSectionInfo();

        const QList< std::shared_ptr<ObfRoutingSubsectionInfo> >& subsections;
        const QList< std::shared_ptr<ObfRoutingSubsectionInfo> >& baseSubsections;

    friend class OsmAnd::ObfRoutingSectionReader_P;
    friend class OsmAnd::ObfReader_P;
    friend class OsmAnd::Model::Road;
    friend class OsmAnd::RoutePlanner;
    friend class OsmAnd::RoutePlannerContext;
    friend class OsmAnd::RoutingRulesetContext;
    };

    class OSMAND_CORE_API ObfRoutingSubsectionInfo : public ObfSectionInfo
    {
    private:
    protected:
        ObfRoutingSubsectionInfo(const std::shared_ptr<ObfRoutingSubsectionInfo>& parent);
        ObfRoutingSubsectionInfo(const std::shared_ptr<ObfRoutingSectionInfo>& section);

        AreaI _area31;

        uint32_t _dataOffset;

        uint32_t _subsectionsOffset;
        QList< std::shared_ptr<ObfRoutingSubsectionInfo> > _subsections;
    public:
        virtual ~ObfRoutingSubsectionInfo();

        bool containsData() const;
        const AreaI& area31;

        const std::shared_ptr<ObfRoutingSubsectionInfo> parent;
        const std::shared_ptr<ObfRoutingSectionInfo> section;

    friend class OsmAnd::ObfRoutingSectionReader_P;
    friend class OsmAnd::ObfReader_P;
    };

    class OSMAND_CORE_API ObfRoutingBorderLineHeader
    {
    private:
    protected:
        ObfRoutingBorderLineHeader();

        uint32_t _x;
        uint32_t _y;
        uint32_t _x2;
        bool _x2present;
        uint32_t _y2;
        bool _y2present;
        uint32_t _offset;
    public:
        virtual ~ObfRoutingBorderLineHeader();

        uint32_t& x;
        uint32_t& y;
        uint32_t& x2;
        bool& x2present;
        uint32_t& y2;
        bool& y2present;
        uint32_t& offset;

        friend class OsmAnd::ObfRoutingSectionReader_P;
        friend class OsmAnd::ObfReader_P;
    };
    
    class OSMAND_CORE_API ObfRoutingBorderLinePoint
    {
    private:
    protected:
        ObfRoutingBorderLinePoint();

        uint64_t _id;
        PointI _location;
        QVector<uint32_t> _types;
        float _distanceToStartPoint;
        float _distanceToEndPoint;
    public:
        virtual ~ObfRoutingBorderLinePoint();

        const uint64_t& id;
        const PointI& location;
        const QVector<uint32_t>& types;
        float& distanceToStartPoint;
        float& distanceToEndPoint;

        std::shared_ptr<ObfRoutingBorderLinePoint> bboxedClone(uint32_t x31) const;

    friend class OsmAnd::ObfRoutingSectionReader_P;
    friend class OsmAnd::ObfReader_P;
    };

} // namespace OsmAnd

#endif // _OSMAND_CORE_OBF_ROUTING_SECTION_INFO_H_
