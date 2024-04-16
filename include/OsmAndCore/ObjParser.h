#ifndef _OSMAND_CORE_OBJ_PARSER_H_
#define _OSMAND_CORE_OBJ_PARSER_H_

#include "OsmAndCore/Map/Model3D.h"

#include <QString>

namespace OsmAnd
{
    class OSMAND_CORE_API ObjParser
    {
    private:
        const QString _objFilePath;
        const QString _mtlDirPath;

    public:
        ObjParser(const QString& objFilePath, const QString& mtlDirPath);
        virtual ~ObjParser();

        std::shared_ptr<const Model3D> parse(const bool translateToOrigin = true) const;
    };   
}

#endif // !defined(_OSMAND_CORE_OBJ_PARSER_H_)