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

#ifndef _OSMAND_CORE_OBF_READER_H_
#define _OSMAND_CORE_OBF_READER_H_

#include <cstdint>
#include <memory>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/Data/ObfInfo.h>

class QIODevice;

namespace OsmAnd {

    class ObfFile;

    class ObfMapSectionReader;
    class ObfAddressSectionReader;
    class ObfRoutingSectionReader;
    class ObfPoiSectionReader;
    class ObfTransportSectionReader;

    class ObfReader_P;
    class OSMAND_CORE_API ObfReader
    {
        Q_DISABLE_COPY(ObfReader)
    private:
        const std::unique_ptr<ObfReader_P> _d;
    protected:
    public:
        ObfReader(const std::shared_ptr<const ObfFile>& obfFile);
        ObfReader(const std::shared_ptr<QIODevice>& input);
        virtual ~ObfReader();

        const std::shared_ptr<const ObfFile> obfFile;

        std::shared_ptr<ObfInfo> obtainInfo() const;

    friend class OsmAnd::ObfMapSectionReader;
    friend class OsmAnd::ObfAddressSectionReader;
    friend class OsmAnd::ObfRoutingSectionReader;
    friend class OsmAnd::ObfPoiSectionReader;
    friend class OsmAnd::ObfTransportSectionReader;
    };

} // namespace OsmAnd

#endif // _OSMAND_CORE_OBF_READER_H_
