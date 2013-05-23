#include "Voyager.h"

#include <iostream>
#include <sstream>
#include <ctime>
#include <chrono>

#include <QDateTime>

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

void performJourney(std::ostream &output, const OsmAnd::Voyager::Configuration& cfg);

OSMAND_CORE_UTILS_API void OSMAND_CORE_UTILS_CALL OsmAnd::Voyager::logJourneyToStdOut( const Configuration& cfg )
{
    performJourney(std::cout, cfg);
}

OSMAND_CORE_UTILS_API QString OSMAND_CORE_UTILS_CALL OsmAnd::Voyager::logJourneyToString( const Configuration& cfg )
{
    std::ostringstream output;
    performJourney(output, cfg);
    return QString::fromStdString(output.str());
}

void performJourney(std::ostream &output, const OsmAnd::Voyager::Configuration& cfg)
{
    if(cfg.generateXml)
        output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;

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
            output << "<!--";
        output << "Failed to find road near start point";
        if(cfg.generateXml)
            output << "-->";
        output << std::endl;
        return;
    }
    std::shared_ptr<OsmAnd::Model::Road> endRoad;
    if(!OsmAnd::RoutePlanner::findClosestRoadPoint(&plannerContext, cfg.endLatitude, cfg.endLongitude, &endRoad))
    {
        if(cfg.generateXml)
            output << "<!--";
        output << "Failed to find road near end point";
        if(cfg.generateXml)
            output << "-->";
        output << std::endl;
        return;
    }

    if(cfg.verbose)
    {
        if(cfg.generateXml)
            output << "<!--";
        output << "Start point (LAT " << cfg.startLatitude << "; LON " << cfg.startLongitude << "):";
        if(cfg.generateXml)
            output << "-->";
        output << std::endl;
        if(cfg.generateXml)
            output << "<!--";
        output << "\tRoad name(s): ";
        if(startRoad->names.size() == 0)
        {
            output << "\t[none] (" << startRoad->id << ")";
            if(cfg.generateXml)
                output << "-->";
            output << std::endl;
        }
        else
        {
            for(auto itName = startRoad->names.begin(); itName != startRoad->names.end(); ++itName)
                output << qPrintable(itName.value()) << "; ";
            output << " (" << startRoad->id << ")";
            if(cfg.generateXml)
                output << "-->";
            output << std::endl;
        }
        output << std::endl;
    
        if(cfg.generateXml)
            output << "<!--";
        output << "End point (LAT " << cfg.endLatitude << "; LON " << cfg.endLongitude << "):";
        if(cfg.generateXml)
            output << "-->";
        output << std::endl;
        if(cfg.generateXml)
            output << "<!--";
        output << "\tRoad name(s): ";
        if(endRoad->names.size() == 0)
        {
            output << "\t[none] (" << endRoad->id << ")";
            if(cfg.generateXml)
                output << "-->";
            output << std::endl;
        }
        else
        {
            for(auto itName = endRoad->names.begin(); itName != endRoad->names.end(); ++itName)
                output << qPrintable(itName.value()) << "; ";
            output << " (" << endRoad->id << ")";
            if(cfg.generateXml)
                output << "-->";
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
            output << "<!--";
        output << "Started route calculation " << qPrintable(QTime::currentTime().toString());
        if(cfg.generateXml)
            output << "-->";
        output << std::endl;
    }
    auto routeFound = OsmAnd::RoutePlanner::calculateRoute(&plannerContext, points, cfg.leftSide, nullptr, &route);
    auto routeCalculationFinish = std::chrono::steady_clock::now();
    if(cfg.verbose)
    {
        if(cfg.generateXml)
            output << "<!--";
        output << "Finished route calculation " << qPrintable(QTime::currentTime().toString()) << ", took " << std::chrono::duration<double, std::milli> (routeCalculationFinish - routeCalculationStart).count() << " ms";
        if(cfg.generateXml)
            output << "-->";
        output << std::endl;
    }
    if(cfg.doRecalculate)
    {
        auto routeRecalculationStart = std::chrono::steady_clock::now();
        if(cfg.verbose)
        {
            if(cfg.generateXml)
                output << "<!--";
            output << "Started route recalculation " << qPrintable(QTime::currentTime().toString());
            if(cfg.generateXml)
                output << "-->";
            output << std::endl;
        }
        routeFound = OsmAnd::RoutePlanner::calculateRoute(&plannerContext, points, cfg.leftSide, nullptr, &route);
        auto routeRecalculationFinish = std::chrono::steady_clock::now();
        if(cfg.verbose)
        {
            if(cfg.generateXml)
                output << "<!--";
            output << "Finished route recalculation " << qPrintable(QTime::currentTime().toString()) << ", took " << std::chrono::duration<double, std::milli> (routeRecalculationFinish - routeRecalculationStart).count() << " ms";
            if(cfg.generateXml)
                output << "-->";
            output << std::endl;
        }
    }
    if(!routeFound)
    {
        if(cfg.generateXml)
            output << "<!--";
        output << "FAILED TO FIND ROUTE!";
        if(cfg.generateXml)
            output << "-->";
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
        output << "<test" << std::endl;
    else
        output << "ROUTE:" << std::endl;

    output << "\tregions=\"\"" << std::endl;
    output << "\tdescription=\"\"" << std::endl;
    output << "\tbest_percent=\"\"" << std::endl;
    output << "\tvehicle=\"" << qPrintable(cfg.vehicle) << "\"" << std::endl;
    output << "\tstart_lat=\"" << cfg.startLatitude << "\"" << std::endl;
    output << "\tstart_lon=\"" << cfg.startLongitude << "\"" << std::endl;
    output << "\ttarget_lat=\"" << cfg.endLatitude << "\"" << std::endl;
    output << "\ttarget_lon=\"" << cfg.endLongitude << "\"" << std::endl;
    output << "\tloadedTiles=\"" << 0 << "\"" << std::endl;
    output << "\tvisitedSegments=\"" << 0 << "\"" << std::endl;
    output << "\tcomplete_distance=\"" << totalDistance << "\"" << std::endl;
    output << "\tcomplete_time=\"" << totalTime << "\"" << std::endl;
    output << "\trouting_time=\"" << 0 << "\"" << std::endl;
    output << "\tvehicle=\"" << qPrintable(cfg.vehicle) << "\"" << std::endl;

    if(cfg.generateXml)
        output << ">" << std::endl;
    else
        output << std::endl;

    for(auto itSegment = route.begin(); itSegment != route.end(); ++itSegment)
    {
        auto segment = *itSegment;

        if(cfg.generateXml)
            output << "\t<segment" << std::endl;
        else
            output << "\tSEGMENT:" << std::endl;

        output << "\t\tid=\"" << segment->road->id << "\"" << std::endl;
        output << "\t\tstart=\"" << segment->startPointIndex << "\"" << std::endl;
        output << "\t\tend=\"" << segment->endPointIndex << "\"" << std::endl;

        QString name;
        if(!segment->road->names.isEmpty())
            name += segment->road->names.begin().value();
        /*String ref = res.getObject().getRef();
        if (ref != null) {
            name += " (" + ref + ") ";
        }*/
        output << "\t\tname=\"" << qPrintable(name) << "\"" << std::endl;

        output << "\t\ttime=\"" << segment->time << "\"" << std::endl;
        output << "\t\tdistance=\"" << segment->distance << "\"" << std::endl;
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
        output << "\t\tstart_bearing=\"" << segment->getBearingBegin() << "\"" << std::endl;
        output << "\t\tend_bearing=\"" << segment->getBearingEnd() << "\"" << std::endl;
        //additional.append("description = \"").append(res.getDescription()).append("\" ");
        
        if(cfg.generateXml)
            output << "\t/>" << std::endl;
        else
            output << std::endl;
    }

    if(cfg.generateXml)
        output << "</test>" << std::endl;
}