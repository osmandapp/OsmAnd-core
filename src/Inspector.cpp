
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <map>
#include <memory>

#include <QFile>
#include <QStringList>

#include <ObfReader.h>
#include <Utilities.h>


class ObfInspector {
public:
    static int inspector(int argc, std::string* argv);
    static int inspector(std::vector<std::string> argv);
};

int ObfInspector::inspector(std::vector<std::string> argv) 
{
    return inspector(argv.size(), &(*argv.begin()));
}

// Options
bool verboseAddress = false;
bool verboseStreetGroups = false;
bool verboseStreets = false;
bool verboseBuildings = false;
bool verboseIntersections = false;
bool verboseMap = false;
bool verbosePoi = false;
bool verboseTrasport = false;
double latTop = 85;
double latBottom = -85;
double lonLeft = -180;
double lonRight = 180;
int zoom = 15;

// Forward declarations
void printUsage(std::string warning = std::string());
void printFileInformation(std::string fileName);
void printFileInformation(QFile* file);
void printAddressDetailedInfo(OsmAnd::ObfReader* reader, OsmAnd::ObfAddressSection* section);
void printPOIDetailInfo(OsmAnd::ObfReader* reader, OsmAnd::ObfPoiSection* section);
void printMapDetailInfo(OsmAnd::ObfReader* reader, OsmAnd::ObfMapSection* section);
std::string formatBounds(int left, int right, int top, int bottom);
std::string formatGeoBounds(double l, double r, double t, double b);

int ObfInspector::inspector(int argc, std::string argv[])
{
    if(argc <= 1)
    {
        printUsage();
        return -1;
    }

    std::string cmd = argv[1];
    if (cmd[0] == '-')
    {
        // command
        if (cmd == "-c" || cmd == "-combine") {
            if (argc < 5)
            {
                printUsage("Too few parameters to extract (require minimum 4)");
                return -1;
            }

            std::map<std::shared_ptr<QFile>, std::string> parts;
            /*for (int i = 3; i < argc; i++)
            {
                file = new File(args[i]);
                if (!file.exists()) {
                    System.err.std::cout << "File to extract from doesn't exist " + args[i]);
                    return;
                }
                parts.put(file, null);
                if (i < args.length - 1) {
                    if (args[i + 1].startsWith("-") || args[i + 1].startsWith("+")) {
                        parts.put(file, args[i + 1]);
                        i++;
                    }
                }
            }
            List<Float> extracted = combineParts(new File(args[1]), parts);
            if (extracted != null) {
                std::cout << "\n" + extracted.size() + " parts were successfully extracted to " + args[1]);
            }*/
        }
        else if (cmd.find("-v") == 0)
        {
            if (argc < 3)
            {
                printUsage("Missing file parameter");
                return -1;
            }

            for(int argIdx = 1; argIdx < argc - 1; argIdx++)
            {
                std::string arg = argv[argIdx];
                if(arg == "-vaddress")
                    verboseAddress = true;
                else if(arg == "-vstreets")
                    verboseStreets = true;
                else if(arg == "-vstreetgroups")
                    verboseStreetGroups = true;
                else if(arg == "-vbuildings")
                    verboseBuildings = true;
                else if(arg == "-vintersections")
                    verboseIntersections = true;
                else if(arg == "-vmap")
                    verboseMap = true;
                else if(arg == "-vpoi")
                    verbosePoi = true;
                else if(arg == "-vtransport")
                    verboseTrasport = true;
                else if(arg.find("-zoom=") == 0)
                {
                    zoom = atoi(arg.c_str() + 5);
                }
                else if(arg.find("-bbox=") == 0)
                {
                    auto values = QString(arg.c_str() + 5).split(",");
                    lonLeft = values[0].toDouble();
                    latTop = values[1].toDouble();
                    lonRight = values[2].toDouble();
                    latBottom =  values[3].toDouble();
                }
            }

            printFileInformation(argv[argc - 1]);
        } else {
            printUsage("Unknown command : " + cmd);
        }
    }
    else
    {
        printFileInformation(cmd);
    }
    return 0;
}

void printUsage(std::string warning)
{
    if(!warning.empty())
        std::cout << warning << std::endl;
    std::cout << "Inspector is console utility for working with binary indexes of OsmAnd." << std::endl;
    std::cout << "It allows print info about file, extract parts and merge indexes." << std::endl;
    std::cout << "\nUsage for print info : inspector [-vaddress] [-vstreetgroups] [-vstreets] [-vbuildings] [-vintersections] [-vmap] [-vpoi] [-vtransport] [-zoom=Zoom] [-bbox=LeftLon,TopLat,RightLon,BottomLan] [file]" << std::endl;
    std::cout << "  Prints information about [file] binary index of OsmAnd." << std::endl;
    std::cout << "  -v.. more verbose output (like all cities and their streets or all map objects with tags/values and coordinates)" << std::endl;
    std::cout << "\nUsage for combining indexes : inspector -c file_to_create (file_from_extract ((+|-)parts_to_extract)? )*" << std::endl;
    std::cout << "\tCreate new file of extracted parts from input file. [parts_to_extract] could be parts to include or exclude." << std::endl;
    std::cout << "  Example : inspector -c output_file input_file +1,2,3\n\tExtracts 1, 2, 3 parts (could be find in print info)" << std::endl;
    std::cout << "  Example : inspector -c output_file input_file -2,3\n\tExtracts all parts excluding 2, 3" << std::endl;
    std::cout << "  Example : inspector -c output_file input_file1 input_file2 input_file3\n\tSimply combine 3 files" << std::endl;
    std::cout << "  Example : inspector -c output_file input_file1 input_file2 -4\n\tCombine all parts of 1st file and all parts excluding 4th part of 2nd file" << std::endl;
}

void printFileInformation(std::string fileName)
{
    QFile file(QString::fromStdString(fileName));
    if(!file.exists())
    {
        std::cout << "Binary OsmAnd index " << fileName << " was not found." << std::endl;
        return;
    }

    printFileInformation(&file);
}

void printFileInformation(QFile* file)
{
    if(!file->open(QIODevice::ReadOnly))
    {
        std::cout << "Failed to open file " << file->fileName().toStdString().c_str() << std::endl;
        return;
    }

    OsmAnd::ObfReader obfMap(file);
    std::cout << "Binary index " << file->fileName().toStdString() << " version = " << obfMap.getVersion() << std::endl;
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

        std::cout << idx << ". " << sectionType << " data " << section->_name.toStdString() << " - " << section->_length << " bytes" << std::endl;
        
        if(dynamic_cast<OsmAnd::ObfTransportSection*>(section))
        {
            auto transportSection = dynamic_cast<OsmAnd::ObfTransportSection*>(section);
            int sh = (31 - OsmAnd::ObfReader::TransportStopZoom);
            std::cout << "\tBounds " << formatBounds(transportSection->_left << sh, transportSection->_right << sh, transportSection->_top << sh, transportSection->_bottom << sh) << std::endl;
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
            std::cout << "\tBounds " << formatGeoBounds(lonLeft, lonRight, latTop, latBottom) << std::endl;
        }
        else if(dynamic_cast<OsmAnd::ObfMapSection*>(section))
        {
            auto mapSection = dynamic_cast<OsmAnd::ObfMapSection*>(section);
            int levelIdx = 1;
            for(auto itLevel = mapSection->_levels.begin(); itLevel != mapSection->_levels.end(); ++itLevel, levelIdx++)
            {
                OsmAnd::ObfMapSection::MapRoot* level = itLevel->get();
                std::cout << "\t" << idx << "." << levelIdx << " Map level minZoom = " << level->_minZoom << ", maxZoom = " << level->_maxZoom << ", size = " << level->_length << " bytes" << std::endl;
                std::cout << "\t\tBounds " << formatBounds(level->_left, level->_right, level->_top, level->_bottom) << std::endl;
            }

            if(verboseMap)
                printMapDetailInfo(&obfMap, mapSection);
        }
        else if(dynamic_cast<OsmAnd::ObfPoiSection*>(section) && verbosePoi)
        {
            printPOIDetailInfo(&obfMap, dynamic_cast<OsmAnd::ObfPoiSection*>(section));
        }
        else if (dynamic_cast<OsmAnd::ObfAddressSection*>(section) && verboseAddress)
        {
            printAddressDetailedInfo(&obfMap, dynamic_cast<OsmAnd::ObfAddressSection*>(section));
        }
    }

    file->close();
}

void printMapDetailInfo(OsmAnd::ObfReader* reader, OsmAnd::ObfMapSection* section)
{

}

void printPOIDetailInfo(OsmAnd::ObfReader* reader, OsmAnd::ObfPoiSection* section)
{
    std::cout << "\tBounds " << formatGeoBounds(section->_leftLongitude, section->_rightLongitude, section->_topLatitude, section->_bottomLatitude) << std::endl;

    std::list< std::shared_ptr<OsmAnd::ObfPoiSection::PoiCategory> > categories;
    OsmAnd::ObfPoiSection::loadCategories(reader, section, categories);
    std::cout << "\tCategories:" << std::endl;
    for(auto itCategory = categories.begin(); itCategory != categories.end(); ++itCategory)
    {
        auto category = *itCategory;
        std::cout << "\t\t" << category->_name.toStdString() << std::endl;
        for(auto itSubcategory = category->_subcategories.begin(); itSubcategory != category->_subcategories.end(); ++itSubcategory)
            std::cout << "\t\t\t" << (*itSubcategory).toStdString() << std::endl;
    }

    std::list< std::shared_ptr<OsmAnd::Model::Amenity> > amenities;
    OsmAnd::ObfPoiSection::loadAmenities(reader, section, amenities);
    return;
}

void printAddressDetailedInfo(OsmAnd::ObfReader* reader, OsmAnd::ObfAddressSection* section)
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

        std::cout << "\t" << strTypes[typeIdx] << ", " << streetGroups.size() << " group(s)";
        if(!verboseStreetGroups)
        {
            std::cout << std::endl;
            continue;
        }
        std::cout << ":" << std::endl;
        for(auto itStreetGroup = streetGroups.begin(); itStreetGroup != streetGroups.end(); itStreetGroup++)
        {
            auto g = *itStreetGroup;

            std::list< std::shared_ptr<OsmAnd::Model::Street> > streets;
            OsmAnd::ObfAddressSection::loadStreetsFromGroup(reader, g.get(), streets);
            std::cout << "\t\t'" << g->_latinName.toStdString() << "' [" << g->_id << "], " << streets.size() << " street(s)";
            if(!verboseStreets)
            {
                std::cout << std::endl;
                continue;
            }
            std::cout << ":" << std::endl;
            for(auto itStreet = streets.begin(); itStreet != streets.end(); ++itStreet)
            {
                auto s = *itStreet;

                const bool isInside =
                    latTop >= s->_latitude &&
                    latBottom <= s->_latitude &&
                    lonLeft <= s->_longitude &&
                    lonRight >= s->_longitude;
                if(!isInside)
                    continue;

                std::list< std::shared_ptr<OsmAnd::Model::Building> > buildings;
                OsmAnd::ObfAddressSection::loadBuildingsFromStreet(reader, s.get(), buildings);
                std::list< std::shared_ptr<OsmAnd::Model::Street::IntersectedStreet> > intersections;
                OsmAnd::ObfAddressSection::loadIntersectionsFromStreet(reader, s.get(), intersections);
                std::cout << "\t\t\t'" << s->_latinName.toStdString() << "' [" << s->_id << "], " << buildings.size() << " building(s), " << intersections.size() << " intersection(s)" << std::endl;
                if(verboseBuildings && buildings.size() > 0)
                {
                    std::cout << "\t\t\t\tBuildings:" << std::endl;
                    for(auto itBuilding = buildings.begin(); itBuilding != buildings.end(); ++itBuilding)
                    {
                        auto building = *itBuilding;

                        if(building->_interpolationInterval != 0)
                            std::cout << "\t\t\t\t\t" << building->_latinName.toStdString() << "-" << building->_latinName2.toStdString() << " (+" << building->_interpolationInterval << ") [" << building->_id << "]" << std::endl;
                        else if(building->_interpolation != OsmAnd::Model::Building::Interpolation::Invalid)
                            std::cout << "\t\t\t\t\t" << building->_latinName.toStdString() << "-" << building->_latinName2.toStdString() << " (" << building->_interpolation << ") [" << building->_id << "]" << std::endl;
                    }
                }
                if(verboseIntersections && intersections.size() > 0)
                {
                    std::cout << "\t\t\t\tIntersects with:" << std::endl;
                    for(auto itIntersection = intersections.begin(); itIntersection != intersections.end(); ++itIntersection)
                    {
                        auto intersection = *itIntersection;

                        std::cout << "\t\t\t\t\t" << intersection->_latinName.toStdString() << std::endl;
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
