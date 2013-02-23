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

OsmAnd::Inspector::Configuration::Configuration()
{
    verboseAddress = false;
    verboseStreetGroups = false;
    verboseStreets = false;
    verboseBuildings = false;
    verboseIntersections = false;
    verboseMap = false;
    verbosePoi = false;
    verboseTrasport = false;
    latTop = 85;
    latBottom = -85;
    lonLeft = -180;
    lonRight = 180;
    zoom = 15;
}

// Forward declarations
void dump(std::ostream &output, QString filePath, const OsmAnd::Inspector::Configuration& cfg);
void printAddressDetailedInfo(std::ostream& output, const OsmAnd::Inspector::Configuration& cfg, OsmAnd::ObfReader* reader, OsmAnd::ObfAddressSection* section);
void printPOIDetailInfo(std::ostream& output, const OsmAnd::Inspector::Configuration& cfg, OsmAnd::ObfReader* reader, OsmAnd::ObfPoiSection* section);
void printMapDetailInfo(std::ostream& output, const OsmAnd::Inspector::Configuration& cfg, OsmAnd::ObfReader* reader, OsmAnd::ObfMapSection* section);
std::string formatBounds(int left, int right, int top, int bottom);
std::string formatGeoBounds(double l, double r, double t, double b);

OSMAND_INSPECTOR_API void OSMAND_INSPECTOR_CALL OsmAnd::Inspector::dumpToStdOut( QString filePath, Configuration cfg /*= Configuration()*/ )
{
    dump(std::cout, filePath, cfg);
}

OSMAND_INSPECTOR_API QString OSMAND_INSPECTOR_CALL OsmAnd::Inspector::dumpToString( QString filePath, Configuration cfg /*= Configuration()*/ )
{
    std::ostringstream output;
    dump(output, filePath, cfg);
    return QString::fromStdString(output.str());
}

void dump(std::ostream &output, QString filePath, const OsmAnd::Inspector::Configuration& cfg)
{
    QFile file(filePath);
    if(!file.exists())
    {
        output << "Binary OsmAnd index " << filePath.toStdString() << " was not found." << std::endl;
        return;
    }

    if(!file.open(QIODevice::ReadOnly))
    {
        output << "Failed to open file " << file.fileName().toStdString().c_str() << std::endl;
        return;
    }

    OsmAnd::ObfReader obfMap(&file);
    output << "Binary index " << file.fileName().toStdString() << " version = " << obfMap.getVersion() << std::endl;
    const auto& sections = obfMap.getSections();
    int idx = 1;
    for(auto itSection = sections.begin(); itSection != sections.end(); ++itSection, idx++)
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

        output << idx << ". " << sectionType << " data " << section->_name.toStdString() << " - " << section->_length << " bytes" << std::endl;

        if(dynamic_cast<OsmAnd::ObfTransportSection*>(section))
        {
            auto transportSection = dynamic_cast<OsmAnd::ObfTransportSection*>(section);
            int sh = (31 - OsmAnd::ObfReader::TransportStopZoom);
            output << "\tBounds " << formatBounds(transportSection->_left << sh, transportSection->_right << sh, transportSection->_top << sh, transportSection->_bottom << sh) << std::endl;
        }
        else if(dynamic_cast<OsmAnd::ObfRoutingSection*>(section))
        {
            auto routingSection = dynamic_cast<OsmAnd::ObfRoutingSection*>(section);
            double lonLeft = 180;
            double lonRight = -180;
            double latTop = -90;
            double latBottom = 90;
            for(auto itSubregion = routingSection->_subregions.begin(); itSubregion != routingSection->_subregions.end(); ++itSubregion)
            {
                OsmAnd::ObfRoutingSection::Subregion* subregion = itSubregion->get();

                lonLeft = std::min(lonLeft, OsmAnd::Utilities::get31LongitudeX(subregion->_left));
                lonRight = std::max(lonRight, OsmAnd::Utilities::get31LongitudeX(subregion->_right));
                latTop = std::max(latTop, OsmAnd::Utilities::get31LatitudeY(subregion->_top));
                latBottom = std::min(latBottom, OsmAnd::Utilities::get31LatitudeY(subregion->_bottom));
            }
            output << "\tBounds " << formatGeoBounds(lonLeft, lonRight, latTop, latBottom) << std::endl;
        }
        else if(dynamic_cast<OsmAnd::ObfMapSection*>(section))
        {
            auto mapSection = dynamic_cast<OsmAnd::ObfMapSection*>(section);
            int levelIdx = 1;
            for(auto itLevel = mapSection->_levels.begin(); itLevel != mapSection->_levels.end(); ++itLevel, levelIdx++)
            {
                OsmAnd::ObfMapSection::MapRoot* level = itLevel->get();
                output << "\t" << idx << "." << levelIdx << " Map level minZoom = " << level->_minZoom << ", maxZoom = " << level->_maxZoom << ", size = " << level->_length << " bytes" << std::endl;
                output << "\t\tBounds " << formatBounds(level->_left, level->_right, level->_top, level->_bottom) << std::endl;
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

    file.close();
}

void printMapDetailInfo(std::ostream& output, const OsmAnd::Inspector::Configuration& cfg, OsmAnd::ObfReader* reader, OsmAnd::ObfMapSection* section)
{

}

void printPOIDetailInfo(std::ostream& output, const OsmAnd::Inspector::Configuration& cfg, OsmAnd::ObfReader* reader, OsmAnd::ObfPoiSection* section)
{
    output << "\tBounds " << formatGeoBounds(section->_leftLongitude, section->_rightLongitude, section->_topLatitude, section->_bottomLatitude) << std::endl;

    std::list< std::shared_ptr<OsmAnd::ObfPoiSection::PoiCategory> > categories;
    OsmAnd::ObfPoiSection::loadCategories(reader, section, categories);
    output << "\tCategories:" << std::endl;
    for(auto itCategory = categories.begin(); itCategory != categories.end(); ++itCategory)
    {
        auto category = *itCategory;
        output << "\t\t" << category->_name.toStdString() << std::endl;
        for(auto itSubcategory = category->_subcategories.begin(); itSubcategory != category->_subcategories.end(); ++itSubcategory)
            output << "\t\t\t" << (*itSubcategory).toStdString() << std::endl;
    }

    std::list< std::shared_ptr<OsmAnd::Model::Amenity> > amenities;
    OsmAnd::ObfPoiSection::loadAmenities(reader, section, amenities);
    return;
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

        std::list< std::shared_ptr<OsmAnd::Model::StreetGroup> > streetGroups;
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

            std::list< std::shared_ptr<OsmAnd::Model::Street> > streets;
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

                const bool isInside =
                    cfg.latTop >= s->_latitude &&
                    cfg.latBottom <= s->_latitude &&
                    cfg.lonLeft <= s->_longitude &&
                    cfg.lonRight >= s->_longitude;
                if(!isInside)
                    continue;

                std::list< std::shared_ptr<OsmAnd::Model::Building> > buildings;
                OsmAnd::ObfAddressSection::loadBuildingsFromStreet(reader, s.get(), buildings);
                std::list< std::shared_ptr<OsmAnd::Model::Street::IntersectedStreet> > intersections;
                OsmAnd::ObfAddressSection::loadIntersectionsFromStreet(reader, s.get(), intersections);
                output << "\t\t\t'" << s->_latinName.toStdString() << "' [" << s->_id << "], " << buildings.size() << " building(s), " << intersections.size() << " intersection(s)" << std::endl;
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

                        output << "\t\t\t\t\t" << intersection->_latinName.toStdString() << std::endl;
                    }
                }
            }
        }
    }
}

std::string formatBounds(int left, int right, int top, int bottom)
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
    static std::locale enUS("en-US");
    oStream.imbue(enUS);
    oStream << "(left top - right bottom) : " << l << ", " << t << " NE - " << r << ", " << b << " NE";
    return oStream.str();
}
