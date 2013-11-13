#include "Voyager.h"

#include <iostream>
#include <sstream>
#include <ctime>
#include <chrono>

#include <OsmAndCore/QtExtensions.h>
#include <QDateTime>
#include <QTextStream>

#include <OsmAndCore/Common.h>
#include <OsmAndCore/Data/ObfReader.h>
#include <OsmAndCore/Utilities.h>
#include <OsmAndCore/Routing/RoutePlanner.h>
#include <OsmAndCore/Routing/RoutePlannerContext.h>

OsmAnd::Voyager::Configuration::Configuration()
    : verbose(false)
    , generateXml(false)
    , doRecalculate(false)
    , vehicle("car")
    , memoryLimit(0)
    , startLatitude(0)
    , startLongitude(0)
    , endLatitude(0)
    , endLongitude(0)
    , leftSide(false)
    , routingConfig(new RoutingConfiguration())
{
}

OSMAND_CORE_UTILS_API bool OSMAND_CORE_UTILS_CALL OsmAnd::Voyager::parseCommandLineArguments( const QStringList& cmdLineArgs, Configuration& cfg, QString& error )
{
    bool wasObfRootSpecified = false;
    bool wasRouterConfigSpecified = false;
    for(auto itArg = cmdLineArgs.cbegin(); itArg != cmdLineArgs.cend(); ++itArg)
    {
        auto arg = *itArg;
        if (arg.startsWith("-config="))
        {
            QFile configFile(arg.mid(strlen("-config=")));
            if(!configFile.exists())
            {
                error = "Router configuration file does not exist";
                return false;
            }
            configFile.open(QIODevice::ReadOnly | QIODevice::Text);
            if(!RoutingConfiguration::parseConfiguration(&configFile, *cfg.routingConfig.get()))
            {
                error = "Bad router configuration";
                return false;
            }
            configFile.close();
            wasRouterConfigSpecified = true;
        }
        else if (arg == "-verbose")
        {
            cfg.verbose = true;
        }
        else if (arg == "-xml")
        {
            cfg.generateXml = true;
        }
        else if (arg == "-recalc")
        {
            cfg.doRecalculate = true;
        }
        else if (arg.startsWith("-obfsDir="))
        {
            QDir obfRoot(arg.mid(strlen("-obfsDir=")));
            if(!obfRoot.exists())
            {
                error = "OBF directory does not exist";
                return false;
            }
            Utilities::findFiles(obfRoot, QStringList() << "*.obf", cfg.obfs);
            wasObfRootSpecified = true;
        }
        else if (arg.startsWith("-vehicle="))
        {
            cfg.vehicle = arg.mid(strlen("-vehicle="));
        }
        else if (arg.startsWith("-memlimit="))
        {
            bool ok;
            cfg.memoryLimit = arg.mid(strlen("-memlimit=")).toInt(&ok);
            if(!ok || cfg.memoryLimit < 0)
            {
                error = "Bad memory limit";
                return false;
            }
        }
        else if (arg.startsWith("-start="))
        {
            auto coords = arg.mid(strlen("-start=")).split(QChar(';'));
            cfg.startLatitude = coords[0].toDouble();
            cfg.startLongitude = coords[1].toDouble();
        }
        else if (arg.startsWith("-waypoint="))
        {
            auto coords = arg.mid(strlen("-waypoint=")).split(QChar(';'));
            auto latitude = coords[0].toDouble();
            auto longitude = coords[1].toDouble();
            cfg.waypoints.push_back(std::pair<double, double>(latitude, longitude));
        }
        else if (arg.startsWith("-end="))
        {
            auto coords = arg.mid(strlen("-end=")).split(QChar(';'));
            cfg.endLatitude = coords[0].toDouble();
            cfg.endLongitude = coords[1].toDouble();
        }
        else if (arg == "-left")
        {
            cfg.leftSide = true;
        }
        else if (arg.startsWith("-gpx="))
        {
            cfg.gpxPath = arg.mid(strlen("-gpx="));
        }
    }

    if(!wasObfRootSpecified)
        Utilities::findFiles(QDir::current(), QStringList() << "*.obf", cfg.obfs);
    if(cfg.obfs.isEmpty())
    {
        error = "No OBF files loaded";
        return false;
    }
    if(!wasRouterConfigSpecified)
        RoutingConfiguration::loadDefault(*cfg.routingConfig);
    
    return true;
}

#if defined(_UNICODE) || defined(UNICODE)
void performJourney(std::wostream &output, const OsmAnd::Voyager::Configuration& cfg);
#else
void performJourney(std::ostream &output, const OsmAnd::Voyager::Configuration& cfg);
#endif

OSMAND_CORE_UTILS_API void OSMAND_CORE_UTILS_CALL OsmAnd::Voyager::logJourneyToStdOut( const Configuration& cfg )
{
#if defined(_UNICODE) || defined(UNICODE)
    performJourney(std::wcout, cfg);
#else
    performJourney(std::cout, cfg);
#endif
}

OSMAND_CORE_UTILS_API QString OSMAND_CORE_UTILS_CALL OsmAnd::Voyager::logJourneyToString( const Configuration& cfg )
{
#if defined(_UNICODE) || defined(UNICODE)
    std::wostringstream output;
    performJourney(output, cfg);
    return QString::fromStdWString(output.str());
#else
    std::ostringstream output;
    performJourney(output, cfg);
    return QString::fromStdString(output.str());
#endif
}

#if defined(_UNICODE) || defined(UNICODE)
void performJourney(std::wostream &output, const OsmAnd::Voyager::Configuration& cfg)
#else
void performJourney(std::ostream &output, const OsmAnd::Voyager::Configuration& cfg)
#endif
{
    if(cfg.generateXml)
    {
#if defined(_UNICODE) || defined(UNICODE)
        output << xT("<?xml version=\"1.0\" encoding=\"UTF-16\"?>") << std::endl;
#else
        output << xT("<?xml version=\"1.0\" encoding=\"UTF-8\"?>") << std::endl;
#endif
    }

    QList< std::shared_ptr<OsmAnd::ObfReader> > obfData;
    for(auto itObf = cfg.obfs.cbegin(); itObf != cfg.obfs.cend(); ++itObf)
    {
        const auto& obf = *itObf;
        std::shared_ptr<OsmAnd::ObfReader> obfReader(new OsmAnd::ObfReader(std::shared_ptr<QIODevice>(new QFile(obf.absoluteFilePath()))));
        obfData.push_back(obfReader);
    }

    OsmAnd::RoutePlannerContext plannerContext(obfData, cfg.routingConfig, cfg.vehicle, false);
    std::shared_ptr<const OsmAnd::Model::Road> startRoad;
    if(!OsmAnd::RoutePlanner::findClosestRoadPoint(&plannerContext, cfg.startLatitude, cfg.startLongitude, &startRoad))
    {
        if(cfg.generateXml)
            output << xT("<!--");
        output << xT("Failed to find road near start point");
        if(cfg.generateXml)
            output << xT("-->");
        output << std::endl;
        return;
    }
    std::shared_ptr<const OsmAnd::Model::Road> endRoad;
    if(!OsmAnd::RoutePlanner::findClosestRoadPoint(&plannerContext, cfg.endLatitude, cfg.endLongitude, &endRoad))
    {
        if(cfg.generateXml)
            output << xT("<!--");
        output << xT("Failed to find road near end point");
        if(cfg.generateXml)
            output << xT("-->");
        output << std::endl;
        return;
    }

    if(cfg.verbose)
    {
        if(cfg.generateXml)
            output << xT("<!--");
        output << xT("Start point (LAT ") << cfg.startLatitude << xT("; LON ") << cfg.startLongitude << xT("):");
        if(cfg.generateXml)
            output << xT("-->");
        output << std::endl;
        if(cfg.generateXml)
            output << xT("<!--");
        output << xT("\tRoad name(s): ");
        if(startRoad->names.size() == 0)
        {
            output << xT("\t[none] (") << startRoad->id << xT(")");
            if(cfg.generateXml)
                output << xT("-->");
            output << std::endl;
        }
        else
        {
            for(auto itName = startRoad->names.cbegin(); itName != startRoad->names.cend(); ++itName)
                output << QStringToStlString(itName.value()) << xT("; ");
            output << xT(" (") << startRoad->id << xT(")");
            if(cfg.generateXml)
                output << xT("-->");
            output << std::endl;
        }
        output << std::endl;
    
        if(cfg.generateXml)
            output << xT("<!--");
        output << xT("End point (LAT ") << cfg.endLatitude << xT("; LON ") << cfg.endLongitude << xT("):");
        if(cfg.generateXml)
            output << xT("-->");
        output << std::endl;
        if(cfg.generateXml)
            output << xT("<!--");
        output << xT("\tRoad name(s): ");
        if(endRoad->names.size() == 0)
        {
            output << xT("\t[none] (") << endRoad->id << xT(")");
            if(cfg.generateXml)
                output << xT("-->");
            output << std::endl;
        }
        else
        {
            for(auto itName = endRoad->names.cbegin(); itName != endRoad->names.cend(); ++itName)
                output << QStringToStlString(itName.value()) << xT("; ");
            output << xT(" (") << endRoad->id << xT(")");
            if(cfg.generateXml)
                output << xT("-->");
            output << std::endl;
        }
        output << std::endl;
    }

    QList< std::pair<double, double> > points;
    points.push_back(std::pair<double, double>(cfg.startLatitude, cfg.startLongitude));
    for(auto itIntermediatePoint = cfg.waypoints.cbegin(); itIntermediatePoint != cfg.waypoints.cend(); ++itIntermediatePoint)
        points.push_back(*itIntermediatePoint);
    points.push_back(std::pair<double, double>(cfg.endLatitude, cfg.endLongitude));

    QList< std::shared_ptr<OsmAnd::RouteSegment> > route;
    auto routeCalculationStart = std::chrono::steady_clock::now();
    if(cfg.verbose)
    {
        if(cfg.generateXml)
            output << xT("<!--");
        output << xT("Started route calculation ") << QStringToStlString(QTime::currentTime().toString());
        if(cfg.generateXml)
            output << xT("-->");
        output << std::endl;
    }
    OsmAnd::RouteCalculationResult routeFound =
            OsmAnd::RoutePlanner::calculateRoute(&plannerContext, points, cfg.leftSide, nullptr);
    route = routeFound.list;
    auto routeCalculationFinish = std::chrono::steady_clock::now();
    if(cfg.verbose)
    {
        if(cfg.generateXml)
            output << xT("<!--");
        output << xT("Finished route calculation ") << QStringToStlString(QTime::currentTime().toString()) << xT(", took ") << std::chrono::duration<double, std::milli> (routeCalculationFinish - routeCalculationStart).count() << xT(" ms");
        if(cfg.generateXml)
            output << xT("-->");
        output << std::endl;
    }
    if(cfg.doRecalculate)
    {
        auto routeRecalculationStart = std::chrono::steady_clock::now();
        if(cfg.verbose)
        {
            if(cfg.generateXml)
                output << xT("<!--");
            output << xT("Started route recalculation ") << QStringToStlString(QTime::currentTime().toString());
            if(cfg.generateXml)
                output << xT("-->");
            output << std::endl;
        }
        routeFound = OsmAnd::RoutePlanner::calculateRoute(&plannerContext, points, cfg.leftSide, nullptr);
        route = routeFound.list;
        auto routeRecalculationFinish = std::chrono::steady_clock::now();
        if(cfg.verbose)
        {
            if(cfg.generateXml)
                output << xT("<!--");
            output << xT("Finished route recalculation ") << QStringToStlString(QTime::currentTime().toString()) << xT(", took ") << std::chrono::duration<double, std::milli> (routeRecalculationFinish - routeRecalculationStart).count() << xT(" ms");
            if(cfg.generateXml)
                output << xT("-->");
            output << std::endl;
        }
    }
    if(routeFound.warnMessage == "")
    {
        if(cfg.generateXml)
            output << xT("<!--");
        output << xT("FAILED TO FIND ROUTE!") << QStringToStlString(routeFound.warnMessage);
        if(cfg.generateXml)
            output << xT("-->");
        output << std::endl;
    }

    float totalTime = 0.0f;
    float totalDistance = 0.0f;
    for(auto itSegment = route.cbegin(); itSegment != route.cend(); ++itSegment)
    {
        auto segment = *itSegment;

        totalTime += segment->time;
        totalDistance += segment->distance;
    }

    if(cfg.generateXml)
        output << xT("<test") << std::endl;
    else
        output << xT("ROUTE:") << std::endl;

    output << xT("\tregions=\"\"") << std::endl;
    output << xT("\tdescription=\"\"") << std::endl;
    output << xT("\tbest_percent=\"\"") << std::endl;
    output << xT("\tvehicle=\"") << QStringToStlString(cfg.vehicle) << xT("\"") << std::endl;
    output << xT("\tstart_lat=\"") << cfg.startLatitude << xT("\"") << std::endl;
    output << xT("\tstart_lon=\"") << cfg.startLongitude << xT("\"") << std::endl;
    output << xT("\ttarget_lat=\"") << cfg.endLatitude << xT("\"") << std::endl;
    output << xT("\ttarget_lon=\"") << cfg.endLongitude << xT("\"") << std::endl;
    output << xT("\tloadedTiles=\"") << 0 << xT("\"") << std::endl;
    output << xT("\tvisitedSegments=\"") << 0 << xT("\"") << std::endl;
    output << xT("\tcomplete_distance=\"") << totalDistance << xT("\"") << std::endl;
    output << xT("\tcomplete_time=\"") << totalTime << xT("\"") << std::endl;
    output << xT("\trouting_time=\"") << 0 << xT("\"") << std::endl;

    if(cfg.generateXml)
        output << xT(">") << std::endl;
    else
        output << std::endl;

    std::unique_ptr<QFile> gpxFile;
    std::unique_ptr<QTextStream> gpxStream;
    if(!cfg.gpxPath.isEmpty())
    {
        gpxFile.reset(new QFile(cfg.gpxPath));
        gpxFile->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
        gpxStream.reset(new QTextStream(gpxFile.get()));
    }
    if(gpxFile && gpxFile->isOpen())
    {
        *(gpxStream.get()) << "<?xml version='1.0' encoding='UTF-8' standalone='yes' ?>\n";
        *(gpxStream.get()) << "<gpx version=\"1.0\" creator=\"OsmAnd Voyager tool\" xmlns=\"http://www.topografix.com/GPX/1/1\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\">\n";
        *(gpxStream.get()) << "\t<trk>\n";
        *(gpxStream.get()) << "\t\t<trkseg>\n";
    }
    for(auto itSegment = route.cbegin(); itSegment != route.cend(); ++itSegment)
    {
        auto segment = *itSegment;

        if(cfg.generateXml)
            output << xT("\t<segment") << std::endl;
        else
            output << xT("\tSEGMENT:") << std::endl;

        output << xT("\t\tid=\"") << segment->road->id << xT("\"") << std::endl;
        output << xT("\t\tstart=\"") << segment->startPointIndex << xT("\"") << std::endl;
        output << xT("\t\tend=\"") << segment->endPointIndex << xT("\"") << std::endl;

        QString name;
        if(!segment->road->names.isEmpty())
            name += segment->road->names.cbegin().value();
        /*String ref = res.getObject().getRef();
        if (ref != null) {
            name += " (" + ref + ") ";
        }*/
        output << xT("\t\tname=\"") << QStringToStlString(name) << xT("\"") << std::endl;

        output << xT("\t\ttime=\"") << segment->time << xT("\"") << std::endl;
        output << xT("\t\tdistance=\"") << segment->distance << xT("\"") << std::endl;
        /*float ms = res.getObject().getMaximumSpeed();
        if(ms > 0) {
            additional.append("maxspeed = \"").append(ms * 3.6f).append("\" ").append(res.getObject().getHighway()).append(" ");
        }*/
        /*
        if (res.getTurnType() != null) {
            additional.append("turn = \"").append(res.getTurnType()).append("\" ");
            additional.append("turn_angle = \"").append(res.getTurnType().getTurnAngle()).append("\" ");
            if (res.getTurnType().getLanes() != null) {
                additional.append("lanes = \"").append(Arrays.toString(res.getTurnType().getLanes())).append("\" ");
            }
        }*/
        output << xT("\t\tstart_bearing=\"") << segment->getBearingBegin() << xT("\"") << std::endl;
        output << xT("\t\tend_bearing=\"") << segment->getBearingEnd() << xT("\"") << std::endl;
        //additional.append("description = \"").append(res.getDescription()).append("\" ");
        
        if(cfg.generateXml)
            output << xT("\t/>") << std::endl;
        else
            output << std::endl;

        if(gpxFile && gpxFile->isOpen())
        {
            output << xT("\t\tstart=\"") << segment->startPointIndex << xT("\"") << std::endl;
            output << xT("\t\tend=\"") << segment->endPointIndex << xT("\"") << std::endl;
            for(auto pointIdx = segment->startPointIndex; pointIdx < segment->endPointIndex; pointIdx++)
            {
                const auto& point = segment->road->points[pointIdx];

                *(gpxStream.get()) << "\t\t\t<trkpt lon=\"" << OsmAnd::Utilities::get31LongitudeX(point.x) << "\" lat=\"" << OsmAnd::Utilities::get31LatitudeY(point.y) << "\"/>\n";
            }
        }
    }

    if(gpxFile && gpxFile->isOpen())
    {
        *(gpxStream.get()) << "\t\t</trkseg>\n";
        *(gpxStream.get()) << "\t</trk>\n";
        *(gpxStream.get()) << "</gpx>\n";
        gpxStream->flush();
        gpxFile->close();
    }

    if(cfg.generateXml)
        output << xT("</test>") << std::endl;
}
