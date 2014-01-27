/**
* @file
*
* @section LICENSE
*
* OsmAnd - Android navigation software based on OSM maps.
* Copyright (C) 2010-2014  OsmAnd Authors listed in AUTHORS file
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

#ifndef _OSMAND_CORE_OBF_ADDRESS_SECTION_INFO_H_
#define _OSMAND_CORE_OBF_ADDRESS_SECTION_INFO_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/ObfSectionInfo.h>

namespace OsmAnd {

    class ObfAddressSectionReader_P;
    class ObfReader_P;
    class ObfAddressBlocksSectionInfo;

    class OSMAND_CORE_API ObfAddressSectionInfo : public ObfSectionInfo
    {
        Q_DISABLE_COPY(ObfAddressSectionInfo)
    private:
    protected:
        ObfAddressSectionInfo(const std::weak_ptr<ObfInfo>& owner);

        //NOTE:?
        QString _latinName;

        QList< std::shared_ptr<const ObfAddressBlocksSectionInfo> > _addressBlocksSections;
    public:
        virtual ~ObfAddressSectionInfo();

        const QList< std::shared_ptr<const ObfAddressBlocksSectionInfo> >& addressBlocksSections;

        friend class OsmAnd::ObfAddressSectionReader_P;
        friend class OsmAnd::ObfReader_P;
    };

    class OSMAND_CORE_API ObfAddressBlocksSectionInfo : public ObfSectionInfo
    {
        Q_DISABLE_COPY(ObfAddressBlocksSectionInfo)
    private:
    protected:
        ObfAddressBlocksSectionInfo(const std::shared_ptr<const ObfAddressSectionInfo>& addressSection, const std::weak_ptr<ObfInfo>& owner);

        ObfAddressBlockType _type;
    public:
        virtual ~ObfAddressBlocksSectionInfo();

        const ObfAddressBlockType& type;

        friend class OsmAnd::ObfAddressSectionReader_P;
        friend class OsmAnd::ObfReader_P;
    };

} // namespace OsmAnd

#endif // _OSMAND_CORE_OBF_ADDRESS_SECTION_INFO_H_
