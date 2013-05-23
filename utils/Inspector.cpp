#include "Inspector.h"

#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <memory>

#include <QFile>
#include <QStringList>

#include <Common.h>
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
    verboseMapObjects = false;
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
#if defined(_UNICODE) || defined(UNICODE)
void dump(std::wostream &output, const QString& filePath, const OsmAnd::Inspector::Configuration& cfg);
void printAddressDetailedInfo(std::wostream& output, const OsmAnd::Inspector::Configuration& cfg, OsmAnd::ObfReader* reader, OsmAnd::ObfAddressSection* section);
void printPOIDetailInfo(std::wostream& output, const OsmAnd::Inspector::Configuration& cfg, OsmAnd::ObfReader* reader, OsmAnd::ObfPoiSection* section);
void printMapDetailInfo(std::wostream& output, const OsmAnd::Inspector::Configuration& cfg, OsmAnd::ObfReader* reader, OsmAnd::ObfMapSection* section);
std::wstring formatBounds(uint32_t left, uint32_t right, uint32_t top, uint32_t bottom);
std::wstring formatGeoBounds(double l, double r, double t, double b);
#else
void dump(std::ostream &output, const QString& filePath, const OsmAnd::Inspector::Configuration& cfg);
void printAddressDetailedInfo(std::ostream& output, const OsmAnd::Inspector::Configuration& cfg, OsmAnd::ObfReader* reader, OsmAnd::ObfAddressSection* section);
void printPOIDetailInfo(std::ostream& output, const OsmAnd::Inspector::Configuration& cfg, OsmAnd::ObfReader* reader, OsmAnd::ObfPoiSection* section);
void printMapDetailInfo(std::ostream& output, const OsmAnd::Inspector::Configuration& cfg, OsmAnd::ObfReader* reader, OsmAnd::ObfMapSection* section);
std::string formatBounds(uint32_t left, uint32_t right, uint32_t top, uint32_t bottom);
std::string formatGeoBounds(double l, double r, double t, double b);
#endif

OSMAND_CORE_UTILS_API void OSMAND_CORE_UTILS_CALL OsmAnd::Inspector::dumpToStdOut( const Configuration& cfg )
{
#if defined(_UNICODE) || defined(UNICODE)
    dump(std::wcout, cfg.fileName, cfg);
#else
    dump(std::cout, cfg.fileName, cfg);
#endif
}

OSMAND_CORE_UTILS_API QString OSMAND_CORE_UTILS_CALL OsmAnd::Inspector::dumpToString( const Configuration& cfg )
{
#if defined(_UNICODE) || defined(UNICODE)
    std::wostringstream output;
    dump(output, cfg.fileName, cfg);
    return QString::fromStdWString(output.str());
#else
    std::ostringstream output;
    dump(output, cfg.fileName, cfg);
    return QString::fromStdString(output.str());
#endif
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
        else if(arg == "-vmapObjects")
            cfg.verboseMapObjects = true;
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

#if defined(_UNICODE) || defined(UNICODE)
void dump(std::wostream &output, const QString& filePath, const OsmAnd::Inspector::Configuration& cfg)
#else
void dump(std::ostream &output, const QString& filePath, const OsmAnd::Inspector::Configuration& cfg)
#endif
{
    std::shared_ptr<QFile> file(new QFile(filePath));
    if(!file->exists())
    {
        output << _L("Binary OsmAnd index ") << QStringToStdXString(filePath) << _L(" was not found.") << std::endl;
        return;
    }

    if(!file->open(QIODevice::ReadOnly))
    {
        output << _L("Failed to open file ") << QStringToStdXString(file->fileName()) << std::endl;
        return;
    }

    OsmAnd::ObfReader obfMap(file);
    output << _L("Binary index ") << QStringToStdXString(file->fileName()) << _L(" version = ") << obfMap.version << std::endl;
    int idx = 1;
    for(auto itSection = obfMap.sections.begin(); itSection != obfMap.sections.end(); ++itSection, idx++)
    {
        OsmAnd::ObfSection* section = *itSection;

        auto sectionType = _L("unknown");
        if(dynamic_cast<OsmAnd::ObfMapSection*>(section))
            sectionType = _L("Map");
        else if(dynamic_cast<OsmAnd::ObfTransportSection*>(section))
            sectionType = _L("Transport");
        else if(dynamic_cast<OsmAnd::ObfRoutingSection*>(section))
            sectionType = _L("Routing");
        else if(dynamic_cast<OsmAnd::ObfPoiSection*>(section))
            sectionType = _L("Poi");
        else if(dynamic_cast<OsmAnd::ObfAddressSection*>(section))
            sectionType = _L("Address");

        output << idx << _L(". ") << sectionType << _L(" data ") << QStringToStdXString(section->name) << _L(" - ") << section->length << _L(" bytes") << std::endl;

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
            output << _L("\tBounds ") << formatGeoBounds(lonLeft, lonRight, latTop, latBottom) << std::endl;
        }
        else if(dynamic_cast<OsmAnd::ObfMapSection*>(section))
        {
            auto mapSection = dynamic_cast<OsmAnd::ObfMapSection*>(section);
            int levelIdx = 1;
            for(auto itLevel = mapSection->mapLevels.begin(); itLevel != mapSection->mapLevels.end(); ++itLevel, levelIdx++)
            {
                auto level = (*itLevel);
                output << _L("\t") << idx << _L(".") << levelIdx << _L(" Map level minZoom = ") << level->minZoom << _L(", maxZoom = ") << level->maxZoom << std::endl;
                output << _L("\t\tBounds ") << formatBounds(level->area31.left, level->area31.right, level->area31.top, level->area31.bottom) << std::endl;
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

#if defined(_UNICODE) || defined(UNICODE)
void printMapDetailInfo(std::wostream& output, const OsmAnd::Inspector::Configuration& cfg, OsmAnd::ObfReader* reader, OsmAnd::ObfMapSection* section)
#else
void printMapDetailInfo(std::ostream& output, const OsmAnd::Inspector::Configuration& cfg, OsmAnd::ObfReader* reader, OsmAnd::ObfMapSection* section)
#endif
{
    QList< std::shared_ptr<OsmAnd::Model::MapObject> > mapObjects;
    OsmAnd::QueryFilter filter;
    OsmAnd::AreaI bbox31;
    bbox31.top = OsmAnd::Utilities::get31TileNumberY(cfg.bbox.top);
    bbox31.bottom = OsmAnd::Utilities::get31TileNumberY(cfg.bbox.bottom);
    bbox31.left = OsmAnd::Utilities::get31TileNumberX(cfg.bbox.left);
    bbox31.right = OsmAnd::Utilities::get31TileNumberX(cfg.bbox.right);
    filter._bbox31 = &bbox31;
    filter._zoom = &cfg.zoom;
    OsmAnd::ObfMapSection::loadMapObjects(reader, section, &mapObjects, &filter);
    output << _L("\tTotal map objects: ") << mapObjects.count() << std::endl;
    if(cfg.verboseMapObjects)
    {
        for(auto itMapObject = mapObjects.begin(); itMapObject != mapObjects.end(); ++itMapObject)
        {
            auto mapObject = *itMapObject;
            output << _L("\t\t") << mapObject->id << std::endl;
            if(mapObject->names.count() > 0)
            {
                output << _L("\t\t\tNames:");
                for(auto itName = mapObject->names.begin(); itName != mapObject->names.end(); ++itName)
                    output << itName.value().toStdWString() << _L(", ");
                output << std::endl;
            }
            else
                output << _L("\t\t\tNames: [none]") << std::endl;
        }
    }
}

#if defined(_UNICODE) || defined(UNICODE)
void printPOIDetailInfo(std::wostream& output, const OsmAnd::Inspector::Configuration& cfg, OsmAnd::ObfReader* reader, OsmAnd::ObfPoiSection* section)
#else
void printPOIDetailInfo(std::ostream& output, const OsmAnd::Inspector::Configuration& cfg, OsmAnd::ObfReader* reader, OsmAnd::ObfPoiSection* section)
#endif
{
    output << _L("\tBounds ") << formatGeoBounds(section->_areaGeo.left, section->_areaGeo.right, section->_areaGeo.top, section->_areaGeo.bottom) << std::endl;

    QList< std::shared_ptr<OsmAnd::Model::Amenity::Category> > categories;
    OsmAnd::ObfPoiSection::loadCategories(reader, section, categories);
    output << _L("\tCategories:") << std::endl;
    for(auto itCategory = categories.begin(); itCategory != categories.end(); ++itCategory)
    {
        auto category = *itCategory;
        output << _L("\t\t") << QStringToStdXString(category->_name) << std::endl;
        for(auto itSubcategory = category->_subcategories.begin(); itSubcategory != category->_subcategories.end(); ++itSubcategory)
            output << _L("\t\t\t") << QStringToStdXString(*itSubcategory) << std::endl;
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
    output << _L("\tAmenities, ") << amenities.count() << _L(" item(s)");
    if(!cfg.verboseAmenities)
    {
        output << std::endl;
        return;
    }
    output << ":" << std::endl;;
    for(auto itAmenity = amenities.begin(); itAmenity != amenities.end(); ++itAmenity)
    {
        auto amenity = *itAmenity;

        output << _L("\t\t") <<
            QStringToStdXString(amenity->_latinName) << _L(" [") << amenity->_id << _L("], ") <<
            QStringToStdXString(categories[amenity->_categoryId]->_name) << _L(":") << QStringToStdXString(categories[amenity->_categoryId]->_subcategories[amenity->_subcategoryId]) <<
            _L(", lat ") << amenity->_latitude << _L(" lon ") << amenity->_longitude << std::endl;
    }
}

#if defined(_UNICODE) || defined(UNICODE)
void printAddressDetailedInfo(std::wostream& output, const OsmAnd::Inspector::Configuration& cfg, OsmAnd::ObfReader* reader, OsmAnd::ObfAddressSection* section)
#else
void printAddressDetailedInfo(std::ostream& output, const OsmAnd::Inspector::Configuration& cfg, OsmAnd::ObfReader* reader, OsmAnd::ObfAddressSection* section)
#endif
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

        output << _L("\t") << strTypes[typeIdx] << _L(", ") << streetGroups.size() << _L(" group(s)");
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
            output << _L("\t\t'") << QStringToStdXString(g->_latinName) << _L("' [") << g->_id << _L("], ") << streets.size() << _L(" street(s)");
            if(!cfg.verboseStreets)
            {
                output << std::endl;
                continue;
            }
            output << _L(":") << std::endl;
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
                output << _L("\t\t\t'") << QStringToStdXString(s->latinName) << _L("' [") << s->id << _L("], ") << buildings.size() << _L(" building(s), ") << intersections.size() << _L(" intersection(s)") << std::endl;
                if(cfg.verboseBuildings && buildings.size() > 0)
                {
                    output << _L("\t\t\t\tBuildings:") << std::endl;
                    for(auto itBuilding = buildings.begin(); itBuilding != buildings.end(); ++itBuilding)
                    {
                        auto building = *itBuilding;

                        if(building->_interpolationInterval != 0)
                            output << _L("\t\t\t\t\t") << QStringToStdXString(building->_latinName) << _L("-") << QStringToStdXString(building->_latinName2) << _L(" (+") << building->_interpolationInterval << _L(") [") << building->_id << _L("]") << std::endl;
                        else if(building->_interpolation != OsmAnd::Model::Building::Interpolation::Invalid)
                            output << _L("\t\t\t\t\t") << QStringToStdXString(building->_latinName) << _L("-") << QStringToStdXString(building->_latinName2) << _L(" (") << building->_interpolation << _L(") [") << building->_id << _L("]") << std::endl;
                    }
                }
                if(cfg.verboseIntersections && intersections.size() > 0)
                {
                    output << _L("\t\t\t\tIntersects with:") << std::endl;
                    for(auto itIntersection = intersections.begin(); itIntersection != intersections.end(); ++itIntersection)
                    {
                        auto intersection = *itIntersection;

                        output << _L("\t\t\t\t\t") << QStringToStdXString(intersection->latinName) << std::endl;
                    }
                }
            }
        }
    }
}

#if defined(_UNICODE) || defined(UNICODE)
std::wstring formatBounds(uint32_t left, uint32_t right, uint32_t top, uint32_t bottom)
#else
std::string formatBounds(uint32_t left, uint32_t right, uint32_t top, uint32_t bottom)
#endif
{
    double l = OsmAnd::Utilities::get31LongitudeX(left);
    double r = OsmAnd::Utilities::get31LongitudeX(right);
    double t = OsmAnd::Utilities::get31LatitudeY(top);
    double b = OsmAnd::Utilities::get31LatitudeY(bottom);
    return formatGeoBounds(l, r, t, b);
}

#if defined(_UNICODE) || defined(UNICODE)
std::wstring formatGeoBounds(double l, double r, double t, double b)
#else
std::string formatGeoBounds(double l, double r, double t, double b)
#endif
{
#if defined(_UNICODE) || defined(UNICODE)
    std::wostringstream oStream;
#else
    std::ostringstream oStream;
#endif
    oStream << _L("(left top - right bottom) : ") << l << _L(", ") << t << _L(" NE - ") << r << _L(", ") << b << _L(" NE");
    return oStream.str();
}
