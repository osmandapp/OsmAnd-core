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

#ifndef __RASTERIZATION_STYLE_H_
#define __RASTERIZATION_STYLE_H_

#include <stdint.h>
#include <memory>

#include <QString>
#include <QStringList>
#include <QFile>
#include <QXmlStreamReader>
#include <QHash>

#include <OsmAndCore.h>
#include <RasterizationStyleExpression.h>

namespace OsmAnd {

    class RasterizationStyles;

    class OSMAND_CORE_API RasterizationStyle
    {
    public:
        class OSMAND_CORE_API Option : RasterizationStyleExpression
        {
        private:
        protected:
            Option(Type type, const QString& name, const QString& title, const QString& description, const QStringList& possibleValues);

            virtual bool evaluate() const;
        public:
            virtual ~Option();

            const QString title;
            const QString description;

        friend class RasterizationStyle;
        };
    private:
        bool parseMetadata(QXmlStreamReader& xmlReader);
        bool parse(QXmlStreamReader& xmlReader);

        QHash< QString, std::shared_ptr<Option> > _options;
    protected:
        RasterizationStyle(RasterizationStyles* owner, const QString& embeddedResourceName);
        RasterizationStyle(RasterizationStyles* owner, const QFile& externalStyleFile);

        QString _resourceName;
        QString _externalFileName;

        bool parseMetadata();
        bool parse();

        QString _name;
        QString _parentName;
        std::shared_ptr<RasterizationStyle> _parent;

        bool resolveDependencies();
    public:
        virtual ~RasterizationStyle();

        RasterizationStyles* const owner;

        const QString& resourceName;
        const QString& externalFileName;

        const QString& name;
        const QString& parentName;

        bool isStandalone() const;
        bool areDependenciesResolved() const;

        const QHash< QString, std::shared_ptr<Option> >& options;
        
    friend class RasterizationStyles;
    };


} // namespace OsmAnd

#endif // __RASTERIZATION_STYLE_H_
