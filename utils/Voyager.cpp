#include "Voyager.h"

#include <iostream>
#include <sstream>
#include <ctime>
#include <chrono>

#include <QDateTime>

#include <Common.h>
#include <ObfReader.h>
#include <Utilities.h>
#include <RoutePlanner.h>
#include <RoutePlannerContext.h>

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
    for(auto itArg = cmdLineArgs.begin(); itArg != cmdLineArgs.end(); ++itArg)
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
    for(auto itObf = cfg.obfs.begin(); itObf != cfg.obfs.end(); ++itObf)
    {
        auto obf = *itObf;
        std::shared_ptr<OsmAnd::ObfReader> obfReader(new OsmAnd::ObfReader(obf));
        obfData.push_back(obfReader);
    }

    OsmAnd::RoutePlannerContext plannerContext(obfData, cfg.routingConfig, cfg.vehicle, false);
    std::shared_ptr<OsmAnd::Model::Road> startRoad;
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
    std::shared_ptr<OsmAnd::Model::Road> endRoad;
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
            for(auto itName = startRoad->names.begin(); itName != startRoad->names.end(); ++itName)
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
            for(auto itName = endRoad->names.begin(); itName != endRoad->names.end(); ++itName)
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
    for(auto itIntermediatePoint = cfg.waypoints.begin(); itIntermediatePoint != cfg.waypoints.end(); ++itIntermediatePoint)
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
    auto routeFound = OsmAnd::RoutePlanner::calculateRoute(&plannerContext, points, cfg.leftSide, nullptr, &route);
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
        routeFound = OsmAnd::RoutePlanner::calculateRoute(&plannerContext, points, cfg.leftSide, nullptr, &route);
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
    if(!routeFound)
    {
        if(cfg.generateXml)
            output << xT("<!--");
        output << xT("FAILED TO FIND ROUTE!");
        if(cfg.generateXml)
            output << xT("-->");
        output << std::endl;
    }

    float totalTime = 0.0f;
    float totalDistance = 0.0f;
    for(auto itSegment = route.begin(); itSegment != route.end(); ++itSegment)
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
    output << xT("\tvehicle=\"") << QStringToStlString(cfg.vehicle) << xT("\"") << std::endl;

    if(cfg.generateXml)
        output << xT(">") << std::endl;
    else
        output << std::endl;

    for(auto itSegment = route.begin(); itSegment != route.end(); ++itSegment)
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
            name += segment->road->names.begin().value();
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
    }

    if(cfg.generateXml)
        output << xT("</test>") << std::endl;
}