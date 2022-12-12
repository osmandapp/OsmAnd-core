#ifndef _OSMAND_CORE_OBJ_LOADER_H_
#define _OSMAND_CORE_OBJ_LOADER_H_

#include "QtExtensions.h"
#include "Model3D.h"

#include <QString>
#include <QVector>

namespace OsmAnd
{
    class ObjParser
    {
    private:
        QString _objFilename;
        QString _mtlDirPath;

    public:
        ObjParser(const QString& objFilename, const QString& mtlDirPath);
        virtual ~ObjParser();

        bool parse(std::shared_ptr<Model3D>& outModel) const;
    };   
}

#endif // !defined(_OSMAND_CORE_OBJ_LOADER_H_)