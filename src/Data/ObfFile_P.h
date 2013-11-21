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

#ifndef _OSMAND_CORE_OBF_FILE_P_H_
#define _OSMAND_CORE_OBF_FILE_P_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QMutex>

#include <OsmAndCore.h>

namespace OsmAnd {

    class ObfReader;
    class ObfInfo;

    class ObfFile;
    class OSMAND_CORE_API ObfFile_P
    {
        Q_DISABLE_COPY(ObfFile_P)
    private:
    protected:
        ObfFile_P(ObfFile* owner);

        ObfFile* const owner;

        mutable QMutex _obfInfoMutex;
        std::shared_ptr<ObfInfo> _obfInfo;
    public:
        virtual ~ObfFile_P();

    friend class OsmAnd::ObfFile;
    friend class OsmAnd::ObfReader;
    };

} // namespace OsmAnd

#endif // _OSMAND_CORE_OBF_FILE_P_H_
