#include "Inspector.h"

#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <memory>

#include <QFile>
#include <QStringList>

#include <ObfReader.h>
#include <Utilities.h>

#include <Street.h>
#include <StreetIntersection.h>
#include <StreetGroup.h>
#include <Building.h>

OsmAnd::Inspector::Configuration::Configuration()
    : bbox(90, -180, -90, 180)
{
    verboseAddress = false;
    verboseStreetGroups = false;
    verboseStreets = false;
    verboseBuildings = false;
    verboseIntersections = false;
    verboseMap = false;
    verbosePoi = false;
    verboseAmenities = false;
    verboseTrasport = false;
    zoom = 15;
}

OsmAnd::Inspector::Configuration::Configuration(const QString& fileName)
    : bbox(90, -180, -90, 180)
{
    this->fileName = fileName;
    verboseAddress = false;
    verboseStreetGroups = false;
    verboseStreets = false;
    verboseBuildings = false;
    verboseIntersections = false;
    verboseMap = false;
    verbosePoi = false;
    verboseAmenities = false;
    verboseTrasport = false;
    zoom = 15;
}

// Forward declarations
void dump(std::ostream &output, const QString& filePath, const OsmAnd::Inspector::Configuration& cfg);
void printAddressDetailedInfo(std::ostream& output, const OsmAnd::Inspector::Configuration& cfg, OsmAnd::ObfReader* reader, OsmAnd::ObfAddressSection* section);
void printPOIDetailInfo(std::ostream& output, const OsmAnd::Inspector::Configuration& cfg, OsmAnd::ObfReader* reader, OsmAnd::ObfPoiSection* section);
void printMapDetailInfo(std::ostream& output, const OsmAnd::Inspector::Configuration& cfg, OsmAnd::ObfReader* reader, OsmAnd::ObfMapSection* section);
std::string formatBounds(uint32_t left, uint32_t right, uint32_t top, uint32_t bottom);
std::string formatGeoBounds(double l, double r, double t, double b);

OSMAND_CORE_UTILS_API void OSMAND_CORE_UTILS_CALL OsmAnd::Inspector::dumpToStdOut( const Configuration& cfg )
{
    dump(std::cout, cfg.fileName, cfg);
}

OSMAND_CORE_UTILS_API QString OSMAND_CORE_UTILS_CALL OsmAnd::Inspector::dumpToString( const Configuration& cfg )
{
    std::ostringstream output;
    dump(output, cfg.fileName, cfg);
    return QString::fromStdString(output.str());
}

OSMAND_CORE_UTILS_API bool OSMAND_CORE_UTILS_CALL OsmAnd::Inspector::parseCommandLineArguments( const QStringList& cmdLineArgs, Configuration& cfg, QString& error )
{
    for(auto itArg = cmdLineArgs.begin(); itArg != cmdLineArgs.end(); ++itArg)
    {
        auto arg = *itArg;
        if(arg == "-vaddress")
            cfg.verboseAddress = true;
        else if(arg == "-vstreets")
            cfg.verboseStreets = true;
        else if(arg == "-vstreetgroups")
            cfg.verboseStreetGroups = true;
        else if(arg == "-vbuildings")
            cfg.verboseBuildings = true;
        else if(arg == "-vintersections")
            cfg.verboseIntersections = true;
        else if(arg == "-vmap")
            cfg.verboseMap = true;
        else if(arg == "-vpoi")
            cfg.verbosePoi = true;
        else if(arg == "-vamenities")
            cfg.verboseAmenities = true;
        else if(arg == "-vtransport")
            cfg.verboseTrasport = true;
        else if(arg.startsWith("-zoom="))
            cfg.zoom = arg.mid(strlen("-zoom=")).toInt();
        else if(arg.startsWith("-bbox="))
        {
            auto values = arg.mid(strlen("-bbox=")).split(",");
            cfg.bbox.left = values[0].toDouble();
            cfg.bbox.top = values[1].toDouble();
            cfg.bbox.right = values[2].toDouble();
            cfg.bbox.bottom =  values[3].toDouble();
        }
        else if(arg.startsWith("-obf="))
        {
            cfg.fileName = arg.mid(strlen("-obf="));
        }
    }

    if(cfg.fileName.isEmpty())
    {
        error = "OBF file not defined";
        return false;
    }
    return true;
}

void dump(std::ostream &output, const QString& filePath, const OsmAnd::Inspector::Configuration& cfg)
{
    std::shared_ptr<QFile> file(new QFile(filePath));
    if(!file->exists())
    {
        output << "Binary OsmAnd index " << qPrintable(filePath) << " was not found." << std::endl;
        return;
    }

    if(!file->open(QIODevice::ReadOnly))
    {
        output << "Failed to open file " << qPrintable(file->fileName()) << std::endl;
        return;
    }

    OsmAnd::ObfReader obfMap(file);
    output << "Binary index " << qPrintable(file->fileName()) << " version = " << obfMap.version << std::endl;
    int idx = 1;
    for(auto itSection = obfMap.sections.begin(); itSection != obfMap.sections.end(); ++itSection, idx++)
    {
        OsmAnd::ObfSection* section = *itSection;

        std::string sectionType = "unknown";
        if(dynamic_cast<OsmAnd::ObfMapSection*>(section))
            sectionType = "Map";
        else if(dynamic_cast<OsmAnd::ObfTransportSection*>(section))
            sectionType = "Transport";
        else if(dynamic_cast<OsmAnd::ObfRoutingSection*>(section))
            sectionType = "Routing";
        else if(dynamic_cast<OsmAnd::ObfPoiSection*>(section))
            sectionType = "Poi";
        else if(dynamic_cast<OsmAnd::ObfAddressSection*>(section))
            sectionType = "Address";

        output << idx << ". " << sectionType << " data " << section->name.toStdString() << " - " << section->length << " bytes" << std::endl;

        if(dynamic_cast<OsmAnd::ObfTransportSection*>(section))
        {
            auto transportSection = dynamic_cast<OsmAnd::ObfTransportSection*>(section);
            int sh = (31 - OsmAnd::ObfTransportSection::StopZoom);
            output << "\tBounds " << formatBounds(transportSection->_left << sh, transportSection->_right << sh, transportSection->_top << sh, transportSection->_bottom << sh) << std::endl;
        }
        else if(dynamic_cast<OsmAnd::ObfRoutingSection*>(section))
        {
            auto routingSection = dynamic_cast<OsmAnd::ObfRoutingSection*>(section);
            double lonLeft = 180;
            double lonRight = -180;
            double latTop = -90;
            double latBottom = 90;
            for(auto itSubsection = routingSection->_subsections.begin(); itSubsection != routingSection->_subsections.end(); ++itSubsection)
            {
                auto subsection = itSubsection->get();

                lonLeft = std::min(lonLeft, OsmAnd::Utilities::get31LongitudeX(subsection->area31.left));
                lonRight = std::max(lonRight, OsmAnd::Utilities::get31LongitudeX(subsection->area31.right));
                latTop = std::max(latTop, OsmAnd::Utilities::get31LatitudeY(subsection->area31.top));
                latBottom = std::min(latBottom, OsmAnd::Utilities::get31LatitudeY(subsection->area31.bottom));
            }
            output << "\tBounds " << formatGeoBounds(lonLeft, lonRight, latTop, latBottom) << std::endl;
        }
        else if(dynamic_cast<OsmAnd::ObfMapSection*>(section))
        {
            auto mapSection = dynamic_cast<OsmAnd::ObfMapSection*>(section);
            int levelIdx = 1;
            for(auto itLevel = mapSection->mapLevels.begin(); itLevel != mapSection->mapLevels.end(); ++itLevel, levelIdx++)
            {
                auto level = (*itLevel);
                output << "\t" << idx << "." << levelIdx << " Map level minZoom = " << level->minZoom << ", maxZoom = " << level->maxZoom << std::endl;
                output << "\t\tBounds " << formatBounds(level->area31.left, level->area31.right, level->area31.top, level->area31.bottom) << std::endl;
            }

            if(cfg.verboseMap)
                printMapDetailInfo(output, cfg, &obfMap, mapSection);
        }
        else if(dynamic_cast<OsmAnd::ObfPoiSection*>(section) && cfg.verbosePoi)
        {
            printPOIDetailInfo(output, cfg, &obfMap, dynamic_cast<OsmAnd::ObfPoiSection*>(section));
        }
        else if (dynamic_cast<OsmAnd::ObfAddressSection*>(section) &&cfg. verboseAddress)
        {
            printAddressDetailedInfo(output, cfg, &obfMap, dynamic_cast<OsmAnd::ObfAddressSection*>(section));
        }
    }

    file->close();
}

void printMapDetailInfo(std::ostream& output, const OsmAnd::Inspector::Configuration& cfg, OsmAnd::ObfReader* reader, OsmAnd::ObfMapSection* section)
{
    QList< std::shared_ptr<OsmAnd::ObfMapSection::MapObject> > mapObjects;
    OsmAnd::QueryFilter filter;
    OsmAnd::AreaI bbox31;
    bbox31.top = OsmAnd::Utilities::get31TileNumberY(cfg.bbox.top);
    bbox31.bottom = OsmAnd::Utilities::get31TileNumberY(cfg.bbox.bottom);
    bbox31.left = OsmAnd::Utilities::get31TileNumberX(cfg.bbox.left);
    bbox31.right = OsmAnd::Utilities::get31TileNumberX(cfg.bbox.right);
    filter._bbox31 = &bbox31;
    OsmAnd::ObfMapSection::loadMapObjects(reader, section, &mapObjects, &filter);
    output << "\tTotal map objects: " << mapObjects.count() << std::endl;
}

void printPOIDetailInfo(std::ostream& output, const OsmAnd::Inspector::Configuration& cfg, OsmAnd::ObfReader* reader, OsmAnd::ObfPoiSection* section)
{
    output << "\tBounds " << formatGeoBounds(section->_areaGeo.left, section->_areaGeo.right, section->_areaGeo.top, section->_areaGeo.bottom) << std::endl;

    QList< std::shared_ptr<OsmAnd::Model::Amenity::Category> > categories;
    OsmAnd::ObfPoiSection::loadCategories(reader, section, categories);
    output << "\tCategories:" << std::endl;
    for(auto itCategory = categories.begin(); itCategory != categories.end(); ++itCategory)
    {
        auto category = *itCategory;
        output << "\t\t" << category->_name.toStdString() << std::endl;
        for(auto itSubcategory = category->_subcategories.begin(); itSubcategory != category->_subcategories.end(); ++itSubcategory)
            output << "\t\t\t" << (*itSubcategory).toStdString() << std::endl;
    }

    QList< std::shared_ptr<OsmAnd::Model::Amenity> > amenities;
    OsmAnd::QueryFilter filter;
    OsmAnd::AreaI bbox31;
    bbox31.top = OsmAnd::Utilities::get31TileNumberY(cfg.bbox.top);
    bbox31.bottom = OsmAnd::Utilities::get31TileNumberY(cfg.bbox.bottom);
    bbox31.left = OsmAnd::Utilities::get31TileNumberX(cfg.bbox.left);
    bbox31.right = OsmAnd::Utilities::get31TileNumberX(cfg.bbox.right);
    filter._bbox31 = &bbox31;
    OsmAnd::ObfPoiSection::loadAmenities(reader, section, nullptr, &amenities, &filter);
    output << "\tAmenities, " << amenities.count() << " item(s)";
    if(!cfg.verboseAmenities)
    {
        output << std::endl;
        return;
    }
    output << ":" << std::endl;;
    for(auto itAmenity = amenities.begin(); itAmenity != amenities.end(); ++itAmenity)
    {
        auto amenity = *itAmenity;

        auto type = categories[amenity->_categoryId]->_name.toStdString() + ":" + categories[amenity->_categoryId]->_subcategories[amenity->_subcategoryId].toStdString();
        output << "\t\t" << amenity->_latinName.toStdString() << " [" << amenity->_id << "], " << type  << ", lat " << amenity->_latitude << " lon " << amenity->_longitude << std::endl;
    }
}

void printAddressDetailedInfo(std::ostream& output, const OsmAnd::Inspector::Configuration& cfg, OsmAnd::ObfReader* reader, OsmAnd::ObfAddressSection* section)
{
    OsmAnd::ObfAddressSection::AddressBlocksSection::Type types[] = {
        OsmAnd::ObfAddressSection::AddressBlocksSection::Type::CitiesOrTowns,
        OsmAnd::ObfAddressSection::AddressBlocksSection::Type::Villages,
        OsmAnd::ObfAddressSection::AddressBlocksSection::Type::Postcodes,
    };
    char* strTypes[] = {
        "Cities/Towns section",
        "Villages section",
        "Postcodes section",
    };
    for(int typeIdx = 0; typeIdx < sizeof(types)/sizeof(types[0]); typeIdx++)
    {
        auto type = types[typeIdx];

        QList< std::shared_ptr<OsmAnd::Model::StreetGroup> > streetGroups;
        OsmAnd::ObfAddressSection::loadStreetGroups(reader, section, streetGroups, 1 << types[typeIdx]);

        output << "\t" << strTypes[typeIdx] << ", " << streetGroups.size() << " group(s)";
        if(!cfg.verboseStreetGroups)
        {
            output << std::endl;
            continue;
        }
        output << ":" << std::endl;
        for(auto itStreetGroup = streetGroups.begin(); itStreetGroup != streetGroups.end(); itStreetGroup++)
        {
            auto g = *itStreetGroup;

            QList< std::shared_ptr<OsmAnd::Model::Street> > streets;
            OsmAnd::ObfAddressSection::loadStreetsFromGroup(reader, g.get(), streets);
            output << "\t\t'" << g->_latinName.toStdString() << "' [" << g->_id << "], " << streets.size() << " street(s)";
            if(!cfg.verboseStreets)
            {
                output << std::endl;
                continue;
            }
            output << ":" << std::endl;
            for(auto itStreet = streets.begin(); itStreet != streets.end(); ++itStreet)
            {
                auto s = *itStreet;
                //TODO: proper filter
                /*
                const bool isInside =
                    cfg.latTop >= s->latitude &&
                    cfg.latBottom <= s->latitude &&
                    cfg.lonLeft <= s->longitude &&
                    cfg.lonRight >= s->longitude;
                if(!isInside)
                    continue;
                */
                QList< std::shared_ptr<OsmAnd::Model::Building> > buildings;
                OsmAnd::ObfAddressSection::loadBuildingsFromStreet(reader, s.get(), buildings);
                QList< std::shared_ptr<OsmAnd::Model::StreetIntersection> > intersections;
                OsmAnd::ObfAddressSection::loadIntersectionsFromStreet(reader, s.get(), intersections);
                output << "\t\t\t'" << qPrintable(s->latinName) << "' [" << s->id << "], " << buildings.size() << " building(s), " << intersections.size() << " intersection(s)" << std::endl;
                if(cfg.verboseBuildings && buildings.size() > 0)
                {
                    output << "\t\t\t\tBuildings:" << std::endl;
                    for(auto itBuilding = buildings.begin(); itBuilding != buildings.end(); ++itBuilding)
                    {
                        auto building = *itBuilding;

                        if(building->_interpolationInterval != 0)
                            output << "\t\t\t\t\t" << building->_latinName.toStdString() << "-" << building->_latinName2.toStdString() << " (+" << building->_interpolationInterval << ") [" << building->_id << "]" << std::endl;
                        else if(building->_interpolation != OsmAnd::Model::Building::Interpolation::Invalid)
                            output << "\t\t\t\t\t" << building->_latinName.toStdString() << "-" << building->_latinName2.toStdString() << " (" << building->_interpolation << ") [" << building->_id << "]" << std::endl;
                    }
                }
                if(cfg.verboseIntersections && intersections.size() > 0)
                {
                    output << "\t\t\t\tIntersects with:" << std::endl;
                    for(auto itIntersection = intersections.begin(); itIntersection != intersections.end(); ++itIntersection)
                    {
                        auto intersection = *itIntersection;

                        output << "\t\t\t\t\t" << intersection->latinName.toStdString() << std::endl;
                    }
                }
            }
        }
    }
}

std::string formatBounds(uint32_t left, uint32_t right, uint32_t top, uint32_t bottom)
{
    double l = OsmAnd::Utilities::get31LongitudeX(left);
    double r = OsmAnd::Utilities::get31LongitudeX(right);
    double t = OsmAnd::Utilities::get31LatitudeY(top);
    double b = OsmAnd::Utilities::get31LatitudeY(bottom);
    return formatGeoBounds(l, r, t, b);
}

std::string formatGeoBounds(double l, double r, double t, double b)
{
    std::ostringstream oStream;
    oStream << "(left top - right bottom) : " << l << ", " << t << " NE - " << r << ", " << b << " NE";
    return oStream.str();
}
