#ifndef _OSMAND_CORE_UTILS_EYEPIECE_H_
#define _OSMAND_CORE_UTILSEYEPIECE_H_

#include <memory>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QFile>

#include <OsmAndCoreUtils.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapStyle.h>
#include <OsmAndCore/Map/MapStylesCollection.h>

namespace OsmAnd
{
    namespace EyePiece
    {
        struct OSMAND_CORE_UTILS_API Configuration
        {
            Configuration();

            bool verbose;
            bool dumpRules;
            bool is32bit;
            bool drawMap;
            bool drawText;
            bool drawIcons;
            QFileInfoList styleFiles;
            QDir obfsDir;
            QString styleName;
            AreaD bbox;
            ZoomLevel zoom;
            uint32_t tileSide;
            float densityFactor;
            QString output;
        };
        OSMAND_CORE_UTILS_API bool OSMAND_CORE_UTILS_CALL parseCommandLineArguments(const QStringList& cmdLineArgs, Configuration& cfg, QString& error);
        OSMAND_CORE_UTILS_API void OSMAND_CORE_UTILS_CALL rasterizeToStdOut(const Configuration& cfg);
        OSMAND_CORE_UTILS_API QString OSMAND_CORE_UTILS_CALL rasterizeToString(const Configuration& cfg);
    }
}

#endif // !defined(_OSMAND_CORE_UTILS_EYEPIECE_H_)
