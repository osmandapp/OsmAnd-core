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

#ifndef __OBF_FILE_H_
#define __OBF_FILE_H_

#include <stdint.h>
#include <memory>

#include <QString>
#include <QFileInfo>

#include <OsmAndCore.h>

namespace OsmAnd {

    class ObfInfo;
    class ObfReader;

    class ObfFile_P;
    class OSMAND_CORE_API ObfFile
    {
        Q_DISABLE_COPY(ObfFile)
    private:
        const std::unique_ptr<ObfFile_P> _d;
    protected:
    public:
        ObfFile(const QFileInfo& fileInfo);
        ObfFile(const QString& filePath);
        virtual ~ObfFile();

        const QFileInfo fileInfo;

        const std::shared_ptr<ObfInfo>& obfInfo;

    friend class OsmAnd::ObfReader;
    };

} // namespace OsmAnd

#endif // __OBF_FILE_H_
