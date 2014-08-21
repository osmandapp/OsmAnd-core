#ifndef _OSMAND_CORE_TOOLS_EYEPIECE_H_
#define _OSMAND_CORE_TOOLS_EYEPIECE_H_

#include <memory>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QFile>

#include <OsmAndCoreTools.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapStyle.h>
#include <OsmAndCore/Map/MapStylesCollection.h>

namespace OsmAnd
{
    namespace EyePiece
    {
        struct OSMAND_CORE_TOOLS_API Configuration Q_DECL_FINAL
        {
            Configuration();
        };

        OSMAND_CORE_TOOLS_API bool OSMAND_CORE_TOOLS_CALL parseCommandLineArguments(const QStringList& cmdLineArgs, Configuration& cfg, QString& error);
        OSMAND_CORE_TOOLS_API void OSMAND_CORE_TOOLS_CALL rasterizeToStdOut(const Configuration& cfg);
        OSMAND_CORE_TOOLS_API QString OSMAND_CORE_TOOLS_CALL rasterizeToString(const Configuration& cfg);
    }
}

#endif // !defined(_OSMAND_CORE_TOOLS_EYEPIECE_H_)
