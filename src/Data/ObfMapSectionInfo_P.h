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

#ifndef __OBF_MAP_SECTION_INFO_P_H_
#define __OBF_MAP_SECTION_INFO_P_H_

#include <stdint.h>
#include <memory>
#include <tuple>

#include <QMutex>
#include <QSet>
#include <QHash>
#include <QMap>
#include <QString>

#include <OsmAndCore.h>

namespace OsmAnd {

    class ObfMapSectionReader_P;

    class ObfMapSectionInfo;
    class ObfMapSectionInfo_P
    {
    private:
    protected:
        ObfMapSectionInfo_P(ObfMapSectionInfo* owner);

        ObfMapSectionInfo* const owner;

        QMutex _rulesMutex;
        struct Rules
        {
            typedef std::tuple<QString, QString, uint32_t> DecodingRule;

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
        };
        std::shared_ptr<Rules> _rules;
    public:
        virtual ~ObfMapSectionInfo_P();

    friend class OsmAnd::ObfMapSectionInfo;
    friend class OsmAnd::ObfMapSectionReader_P;
    };

} // namespace OsmAnd

#endif // __OBF_MAP_SECTION_INFO_P_H_
