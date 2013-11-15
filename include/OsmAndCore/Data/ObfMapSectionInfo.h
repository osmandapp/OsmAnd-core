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

#ifndef __OBF_MAP_SECTION_INFO_H_
#define __OBF_MAP_SECTION_INFO_H_

#include <cstdint>
#include <memory>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QHash>
#include <QMap>
#include <QSet>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/ObfSectionInfo.h>

namespace OsmAnd {

    class ObfMapSectionReader_P;
    class ObfReader_P;

    class ObfMapSectionLevel_P;
    class OSMAND_CORE_API ObfMapSectionLevel
    {
        Q_DISABLE_COPY(ObfMapSectionLevel);
    private:
        const std::unique_ptr<ObfMapSectionLevel_P> _d;
    protected:
        ObfMapSectionLevel();

        uint32_t _offset;
        uint32_t _length;
        ZoomLevel _minZoom;
        ZoomLevel _maxZoom;
        AreaI _area31;

        uint32_t _boxesInnerOffset;
    public:
        virtual ~ObfMapSectionLevel();

        const uint32_t& offset;
        const uint32_t& length;
        
        const ZoomLevel& minZoom;
        const ZoomLevel& maxZoom;
        const AreaI& area31;

    friend class OsmAnd::ObfMapSectionReader_P;
    };

    struct ObfMapSectionDecodingRule
    {
        uint32_t type;

        QString tag;
        QString value;
    };

    struct OSMAND_CORE_API ObfMapSectionDecodingEncodingRules
    {
        ObfMapSectionDecodingEncodingRules();

        QHash< QString, QHash<QString, uint32_t> > encodingRuleIds;
        QMap< uint32_t, ObfMapSectionDecodingRule > decodingRules;
        uint32_t name_encodingRuleId;
        uint32_t ref_encodingRuleId;
        uint32_t naturalCoastline_encodingRuleId;
        uint32_t naturalLand_encodingRuleId;
        uint32_t naturalCoastlineBroken_encodingRuleId;
        uint32_t naturalCoastlineLine_encodingRuleId;
        uint32_t highway_encodingRuleId;
        uint32_t oneway_encodingRuleId;
        uint32_t onewayReverse_encodingRuleId;
        uint32_t tunnel_encodingRuleId;
        uint32_t bridge_encodingRuleId;
        uint32_t layerLowest_encodingRuleId;

        QSet<uint32_t> positiveLayers_encodingRuleIds;
        QSet<uint32_t> zeroLayers_encodingRuleIds;
        QSet<uint32_t> negativeLayers_encodingRuleIds;

        void createRule(const uint32_t ruleType, const uint32_t ruleId, const QString& ruleTag, const QString& ruleValue);
        void createMissingRules();
    };

    class ObfMapSectionInfo_P;
    class OSMAND_CORE_API ObfMapSectionInfo : public ObfSectionInfo
    {
        Q_DISABLE_COPY(ObfMapSectionInfo);
    private:
        const std::unique_ptr<ObfMapSectionInfo_P> _d;
    protected:
        ObfMapSectionInfo(const std::weak_ptr<ObfInfo>& owner);

        bool _isBasemap;
        QList< std::shared_ptr<const ObfMapSectionLevel> > _levels;
    public:
        ObfMapSectionInfo();
        virtual ~ObfMapSectionInfo();

        const bool& isBasemap;
        const QList< std::shared_ptr<const ObfMapSectionLevel> >& levels;

        const std::shared_ptr<const ObfMapSectionDecodingEncodingRules>& encodingDecodingRules;

    friend class OsmAnd::ObfMapSectionReader_P;
    friend class OsmAnd::ObfReader_P;
    };

} // namespace OsmAnd

#endif // __OBF_MAP_SECTION_INFO_H_
