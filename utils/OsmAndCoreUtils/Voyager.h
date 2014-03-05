#ifndef _OSMAND_CORE_UTILS_VOYAGER_H_
#define _OSMAND_CORE_UTILS_VOYAGER_H_

#include <memory>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QFile>

#include <OsmAndCoreUtils.h>
#include <OsmAndCore/Routing/RoutingConfiguration.h>

namespace OsmAnd
{
    namespace Voyager
    {
        struct OSMAND_CORE_UTILS_API Configuration
        {
            Configuration();
            
            bool verbose;
            bool generateXml;
            bool doRecalculate;
            QFileInfoList obfs;
            QString vehicle;
            int memoryLimit;
            double startLatitude;
            double startLongitude;
            QList< std::pair<double, double> > waypoints;
            double endLatitude;
            double endLongitude;
            bool leftSide;
            QString gpxPath;

            std::shared_ptr<RoutingConfiguration> routingConfig;
        };
        OSMAND_CORE_UTILS_API bool OSMAND_CORE_UTILS_CALL parseCommandLineArguments(const QStringList& cmdLineArgs, Configuration& cfg, QString& error);
        OSMAND_CORE_UTILS_API void OSMAND_CORE_UTILS_CALL logJourneyToStdOut(const Configuration& cfg);
        OSMAND_CORE_UTILS_API QString OSMAND_CORE_UTILS_CALL logJourneyToString(const Configuration& cfg);
    }
}

#endif // !defined(_OSMAND_CORE_UTILS_VOYAGER_H_)
