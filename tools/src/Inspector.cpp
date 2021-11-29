#include "Inspector.h"

#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <memory>

#include <OsmAndCore/QtExtensions.h>
#include <QFile>
#include <QStringList>

#include <OsmAndCore/Common.h>
#include <OsmAndCore/Utilities.h>
#include <OsmAndCore/Data/ObfReader.h>
#include <OsmAndCore/Data/ObfMapSectionInfo.h>
#include <OsmAndCore/Data/ObfMapSectionReader.h>
#include <OsmAndCore/Data/BinaryMapObject.h>
#include <OsmAndCore/Data/ObfAddressSectionInfo.h>
#include <OsmAndCore/Data/ObfAddressSectionReader.h>
#include <OsmAndCore/Data/Street.h>
#include <OsmAndCore/Data/StreetIntersection.h>
#include <OsmAndCore/Data/StreetGroup.h>
#include <OsmAndCore/Data/Building.h>
#include <OsmAndCore/Data/ObfTransportSectionInfo.h>
#include <OsmAndCore/Data/ObfTransportSectionReader.h>
#include <OsmAndCore/Data/ObfRoutingSectionInfo.h>
#include <OsmAndCore/Data/ObfRoutingSectionReader.h>
#include <OsmAndCore/Data/ObfPoiSectionInfo.h>
#include <OsmAndCore/Data/ObfPoiSectionReader.h>
#include <OsmAndCore/Data/Amenity.h>

OsmAndTools::Inspector::Configuration::Configuration()
    : bbox(90.0, -180.0, -90.0, 179.9999999999)
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
    zoom = OsmAnd::ZoomLevel15;
}

OsmAndTools::Inspector::Configuration::Configuration(const QString& fileName_)
    : bbox(90.0, -180.0, -90.0, 179.9999999999)
{
    fileName = fileName_;
    verboseAddress = false;
    verboseStreetGroups = false;
    verboseStreets = false;
    verboseBuildings = false;
    verboseIntersections = false;
    verboseMap = false;
    verbosePoi = false;
    verboseAmenities = false;
    verboseTrasport = false;
    zoom = OsmAnd::ZoomLevel15;
}

// Forward declarations
#if defined(_UNICODE) || defined(UNICODE)
void dump(std::wostream &output, const QString& filePath, const OsmAndTools::Inspector::Configuration& cfg);
void printMapDetailInfo(std::wostream& output, const OsmAndTools::Inspector::Configuration& cfg, const std::shared_ptr<OsmAnd::ObfReader>& reader, const std::shared_ptr<const OsmAnd::ObfMapSectionInfo>& section);
void printPOIDetailInfo(std::wostream& output, const OsmAndTools::Inspector::Configuration& cfg, const std::shared_ptr<OsmAnd::ObfReader>& reader, const std::shared_ptr<const OsmAnd::ObfPoiSectionInfo>& section);
void printAddressDetailedInfo(std::wostream& output, const OsmAndTools::Inspector::Configuration& cfg, const std::shared_ptr<OsmAnd::ObfReader>& reader, const std::shared_ptr<const OsmAnd::ObfAddressSectionInfo>& section);
std::wstring formatBounds(uint32_t left, uint32_t right, uint32_t top, uint32_t bottom);
std::wstring formatGeoBounds(double l, double r, double t, double b);
#else
void dump(std::ostream &output, const QString& filePath, const OsmAndTools::Inspector::Configuration& cfg);
void printMapDetailInfo(std::ostream& output, const OsmAndTools::Inspector::Configuration& cfg, const std::shared_ptr<OsmAnd::ObfReader>& reader, const std::shared_ptr<const OsmAnd::ObfMapSectionInfo>& section);
void printPOIDetailInfo(std::ostream& output, const OsmAndTools::Inspector::Configuration& cfg, const std::shared_ptr<OsmAnd::ObfReader>& reader, const std::shared_ptr<const OsmAnd::ObfPoiSectionInfo>& section);
void printAddressDetailedInfo(std::ostream& output, const OsmAndTools::Inspector::Configuration& cfg, const std::shared_ptr<OsmAnd::ObfReader>& reader, const std::shared_ptr<const OsmAnd::ObfAddressSectionInfo>& section);
std::string formatBounds(uint32_t left, uint32_t right, uint32_t top, uint32_t bottom);
std::string formatGeoBounds(double l, double r, double t, double b);
#endif

OSMAND_CORE_TOOLS_API void OSMAND_CORE_TOOLS_CALL OsmAndTools::Inspector::dumpToStdOut(const Configuration& cfg)
{
#if defined(_UNICODE) || defined(UNICODE)
    dump(std::wcout, cfg.fileName, cfg);
#else
    dump(std::cout, cfg.fileName, cfg);
#endif
}

OSMAND_CORE_TOOLS_API QString OSMAND_CORE_TOOLS_CALL OsmAndTools::Inspector::dumpToString(const Configuration& cfg)
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

OSMAND_CORE_TOOLS_API bool OSMAND_CORE_TOOLS_CALL OsmAndTools::Inspector::parseCommandLineArguments(const QStringList& cmdLineArgs, Configuration& cfg, QString& error)
{
    for (const auto& arg : OsmAnd::constOf(cmdLineArgs))
    {
        if (arg == "-vaddress")
            cfg.verboseAddress = true;
        else if (arg == "-vstreets")
            cfg.verboseStreets = true;
        else if (arg == "-vstreetgroups")
            cfg.verboseStreetGroups = true;
        else if (arg == "-vbuildings")
            cfg.verboseBuildings = true;
        else if (arg == "-vintersections")
            cfg.verboseIntersections = true;
        else if (arg == "-vmap")
            cfg.verboseMap = true;
        else if (arg == "-vmapObjects")
            cfg.verboseMapObjects = true;
        else if (arg == "-vpoi")
            cfg.verbosePoi = true;
        else if (arg == "-vamenities")
            cfg.verboseAmenities = true;
        else if (arg == "-vtransport")
            cfg.verboseTrasport = true;
        else if (arg.startsWith("-zoom="))
            cfg.zoom = static_cast<OsmAnd::ZoomLevel>(arg.mid(strlen("-zoom=")).toInt());
        else if (arg.startsWith("-bbox="))
        {
            auto values = arg.mid(strlen("-bbox=")).split(",");
            cfg.bbox.left() = values[0].toDouble();
            cfg.bbox.top() = values[1].toDouble();
            cfg.bbox.right() = values[2].toDouble();
            cfg.bbox.bottom() = values[3].toDouble();
        }
        else if (arg.startsWith("-obf="))
        {
            cfg.fileName = arg.mid(strlen("-obf="));
        }
    }

    if (cfg.fileName.isEmpty())
    {
        error = "OBF file not defined";
        return false;
    }
    return true;
}

#if defined(_UNICODE) || defined(UNICODE)
void dump(std::wostream &output, const QString& filePath, const OsmAndTools::Inspector::Configuration& cfg)
#else
void dump(std::ostream &output, const QString& filePath, const OsmAndTools::Inspector::Configuration& cfg)
#endif
{
    std::shared_ptr<QFile> file(new QFile(filePath));
    if (!file->exists())
    {
        output << xT("OBF '") << QStringToStlString(filePath) << xT("' does not exist.") << std::endl;
        return;
    }

    if (!file->open(QIODevice::ReadOnly))
    {
        output << xT("Failed to open OBF '") << QStringToStlString(file->fileName()) << xT("'") << std::endl;
        return;
    }

    std::shared_ptr<OsmAnd::ObfReader> obfReader(new OsmAnd::ObfReader(file));
    const auto& obfInfo = obfReader->obtainInfo();

    output << xT("OBF '") << QStringToStlString(file->fileName()) << xT("' version = ") << obfInfo->version << std::endl;
    int idx = 1;
    for (auto itSection = obfInfo->mapSections.cbegin(); itSection != obfInfo->mapSections.cend(); ++itSection, idx++)
    {
        const auto& section = *itSection;

        output << idx << xT(". Map data '") << QStringToStlString(section->name) << xT("' - ") << section->length << xT(" bytes") << std::endl;
        int levelIdx = 1;
        for (auto itLevel = section->levels.cbegin(); itLevel != section->levels.cend(); ++itLevel, levelIdx++)
        {
            auto level = (*itLevel);
            output << xT("\t") << idx << xT(".") << levelIdx << xT(" Map level minZoom = ") << level->minZoom << xT(", maxZoom = ") << level->maxZoom << std::endl;
            output << xT("\t\tBounds ") << formatBounds(level->area31.left(), level->area31.right(), level->area31.top(), level->area31.bottom()) << std::endl;
        }

        if (cfg.verboseMap)
            printMapDetailInfo(output, cfg, obfReader, section);
    }
    for (auto itSection = obfInfo->transportSections.cbegin(); itSection != obfInfo->transportSections.cend(); ++itSection, idx++)
    {
        const auto& section = *itSection;

        output << idx << xT(". Transport data '") << QStringToStlString(section->name) << xT("' - ") << section->length << xT(" bytes") << std::endl;
        output << "\tBounds " << formatBounds(section->area31.left() << (31 - 24), section->area31.right() << (31 - 24), section->area31.top() << (31 - 24), section->area31.bottom() << (31 - 24)) << std::endl;
    }
    for (auto itSection = obfInfo->routingSections.cbegin(); itSection != obfInfo->routingSections.cend(); ++itSection, idx++)
    {
        const auto& section = *itSection;

        output << idx << xT(". Routing data '") << QStringToStlString(section->name) << xT("' - ") << section->length << xT(" bytes") << std::endl;
    }
    for (auto itSection = obfInfo->poiSections.cbegin(); itSection != obfInfo->poiSections.cend(); ++itSection, idx++)
    {
        const auto& section = *itSection;

        output << idx << xT(". POI data '") << QStringToStlString(section->name) << xT("' - ") << section->length << xT(" bytes") << std::endl;
        if (cfg.verbosePoi)
            printPOIDetailInfo(output, cfg, obfReader, section);
    }
    for (auto itSection = obfInfo->addressSections.cbegin(); itSection != obfInfo->addressSections.cend(); ++itSection, idx++)
    {
        const auto& section = *itSection;

        output << idx << xT(". Address data '") << QStringToStlString(section->name) << xT("' - ") << section->length << xT(" bytes") << std::endl;
        printAddressDetailedInfo(output, cfg, obfReader, section);
    }

    file->close();
}

#if defined(_UNICODE) || defined(UNICODE)
void printMapDetailInfo(std::wostream& output, const OsmAndTools::Inspector::Configuration& cfg, const std::shared_ptr<OsmAnd::ObfReader>& reader, const std::shared_ptr<const OsmAnd::ObfMapSectionInfo>& section)
#else
void printMapDetailInfo(std::ostream& output, const OsmAndTools::Inspector::Configuration& cfg, const std::shared_ptr<OsmAnd::ObfReader>& reader, const std::shared_ptr<const OsmAnd::ObfMapSectionInfo>& section)
#endif
{
    OsmAnd::AreaI bbox31;
    bbox31.top() = OsmAnd::Utilities::get31TileNumberY(cfg.bbox.top());
    bbox31.bottom() = OsmAnd::Utilities::get31TileNumberY(cfg.bbox.bottom());
    bbox31.left() = OsmAnd::Utilities::get31TileNumberX(cfg.bbox.left());
    bbox31.right() = OsmAnd::Utilities::get31TileNumberX(cfg.bbox.right());
    if (cfg.verboseMapObjects)
    {
        QList< std::shared_ptr<const OsmAnd::BinaryMapObject> > mapObjects;
        OsmAnd::ObfMapSectionReader::loadMapObjects(reader, section, cfg.zoom, &bbox31, &mapObjects);
        output << xT("\tTotal map objects: ") << mapObjects.count() << std::endl;
        for (auto itMapObject = mapObjects.cbegin(); itMapObject != mapObjects.cend(); ++itMapObject)
        {
            auto mapObject = *itMapObject;
            output << xT("\t\t") << mapObject->id << std::endl;
            if (mapObject->captions.count() > 0)
            {
                output << xT("\t\t\tNames:") << std::endl;
                for (auto itCaption = mapObject->captions.cbegin(); itCaption != mapObject->captions.cend(); ++itCaption)
                {
                    const auto& attribute = mapObject->attributeMapping->decodeMap[itCaption.key()];
                    output
                        << xT("\t\t\t\t")
                        << QStringToStlString(attribute.tag)
                        << xT(" = ")
                        << QStringToStlString(itCaption.value())
                        << std::endl;
                }
            }
        }
    }
    else
    {
        uint32_t mapObjectsCount = 0;
        OsmAnd::ObfMapSectionReader::loadMapObjects(reader, section, cfg.zoom, &bbox31, nullptr, nullptr, nullptr,
            [&mapObjectsCount]
            (const std::shared_ptr<const OsmAnd::BinaryMapObject>& mapObject) -> bool
            {
                mapObjectsCount++;
                return false;
            });
        output << xT("\tTotal map objects: ") << mapObjectsCount << std::endl;
    }
}


void printPOIDetailInfo(
#if defined(_UNICODE) || defined(UNICODE)
    std::wostream& output,
#else
    std::ostream& output,
#endif
    const OsmAndTools::Inspector::Configuration& cfg,
    const std::shared_ptr<OsmAnd::ObfReader>& reader,
    const std::shared_ptr<const OsmAnd::ObfPoiSectionInfo>& section)
{
    output << xT("\tBounds ") << formatBounds(section->area31.left(), section->area31.right(), section->area31.top(), section->area31.bottom()) << std::endl;
    std::shared_ptr<const OsmAnd::ObfPoiSectionCategories> categories;
    OsmAnd::ObfPoiSectionReader::loadCategories(reader, section, categories);
    output << xT("\tCategories:") << std::endl;
    for (auto mainCategoryIndex = 0; mainCategoryIndex < categories->mainCategories.size(); mainCategoryIndex++)
    {
        output << xT("\t\t") << QStringToStlString(categories->mainCategories[mainCategoryIndex]) << std::endl;
        for (const auto& subCategory : categories->subCategories[mainCategoryIndex])
            output << xT("\t\t\t") << QStringToStlString(subCategory) << std::endl;
    }

    QList< std::shared_ptr<const OsmAnd::Amenity> > amenities;
    OsmAnd::AreaI bbox31;
    bbox31.top() = OsmAnd::Utilities::get31TileNumberY(cfg.bbox.top());
    bbox31.bottom() = OsmAnd::Utilities::get31TileNumberY(cfg.bbox.bottom());
    bbox31.left() = OsmAnd::Utilities::get31TileNumberX(cfg.bbox.left());
    bbox31.right() = OsmAnd::Utilities::get31TileNumberX(cfg.bbox.right());
    OsmAnd::ObfPoiSectionReader::loadAmenities(reader, section, &amenities, &bbox31);
    output << xT("\tAmenities, ") << amenities.count() << xT(" item(s)");
    if (!cfg.verboseAmenities)
    {
        output << std::endl;
        return;
    }
    output << ":" << std::endl;
    for (auto itAmenity = amenities.cbegin(); itAmenity != amenities.cend(); ++itAmenity)
    {
        auto amenity = *itAmenity;

        /*output << xT("\t\t") <<
            QStringToStlString(amenity->nativeName) << xT(" [") << amenity->id << xT("], ") <<
            QStringToStlString(categories->mainCategories[amenity->categoryId]->name) << xT(":") << QStringToStlString(categories[amenity->categoryId]->subcategories[amenity->subcategoryId]) <<
            xT(", lat ") << OsmAnd::Utilities::get31LatitudeY(amenity->point31.y) << xT(" lon ") << OsmAnd::Utilities::get31LongitudeX(amenity->point31.x) << std::endl;*/
    }
}

#if defined(_UNICODE) || defined(UNICODE)
void printAddressDetailedInfo(std::wostream& output, const OsmAndTools::Inspector::Configuration& cfg, const std::shared_ptr<OsmAnd::ObfReader>& reader, const std::shared_ptr<const OsmAnd::ObfAddressSectionInfo>& section)
#else
void printAddressDetailedInfo(std::ostream& output, const OsmAndTools::Inspector::Configuration& cfg, const std::shared_ptr<OsmAnd::ObfReader>& reader, const std::shared_ptr<const OsmAnd::ObfAddressSectionInfo>& section)
#endif
{
    OsmAnd::ObfAddressStreetGroupType types[] =
    {
        OsmAnd::ObfAddressStreetGroupType::CityOrTown,
        OsmAnd::ObfAddressStreetGroupType::Village,
        OsmAnd::ObfAddressStreetGroupType::Postcode,
    };
    const char* const strTypes[] =
    {
        "Cities/Towns section",
        "Villages section",
        "Postcodes section",
    };

    OsmAnd::AreaI bbox31;
    bbox31.top() = OsmAnd::Utilities::get31TileNumberY(cfg.bbox.top());
    bbox31.bottom() = OsmAnd::Utilities::get31TileNumberY(cfg.bbox.bottom());
    bbox31.left() = OsmAnd::Utilities::get31TileNumberX(cfg.bbox.left());
    bbox31.right() = OsmAnd::Utilities::get31TileNumberX(cfg.bbox.right());

    for (int typeIdx = 0; typeIdx < sizeof(types) / sizeof(types[0]); typeIdx++)
    {
        // auto type = types[typeIdx];

        QList< std::shared_ptr<const OsmAnd::StreetGroup> > streetGroups;
        OsmAnd::ObfAddressStreetGroupTypesMask typesMask;
        typesMask.set(static_cast<OsmAnd::ObfAddressStreetGroupType>(types[typeIdx]));
        OsmAnd::ObfAddressSectionReader::loadStreetGroups(reader, section, &streetGroups, &bbox31, typesMask);

        output << xT("\t") << strTypes[typeIdx] << xT(", ") << streetGroups.size() << xT(" group(s)");
        if (!cfg.verboseStreetGroups)
        {
            output << std::endl;
            continue;
        }
        output << ":" << std::endl;
        for (auto itStreetGroup = streetGroups.cbegin(); itStreetGroup != streetGroups.cend(); itStreetGroup++)
        {
            auto g = *itStreetGroup;

            QList< std::shared_ptr<const OsmAnd::Street> > streets;
            OsmAnd::ObfAddressSectionReader::loadStreetsFromGroup(reader, g, &streets);
            output
                << xT("\t\t'")
                << QStringToStlString(g->nativeName)
                << xT("' [")
                << QStringToStlString(g->id.toString())
                << xT("], ")
                << streets.size()
                << xT(" street(s)");
            if (!cfg.verboseStreets)
            {
                output << std::endl;
                continue;
            }
            output << xT(":") << std::endl;
            for (auto itStreet = streets.cbegin(); itStreet != streets.cend(); ++itStreet)
            {
                auto s = *itStreet;
                //TODO: proper filter
                //const bool isInside =
                //    cfg.latTop >= s->latitude &&
                //    cfg.latBottom <= s->latitude &&
                //    cfg.lonLeft <= s->longitude &&
                //    cfg.lonRight >= s->longitude;
                //if(!isInside)
                //    continue;
                //
                QList< std::shared_ptr<const OsmAnd::Building> > buildings;
                OsmAnd::ObfAddressSectionReader::loadBuildingsFromStreet(reader, s, &buildings);
                QList< std::shared_ptr<const OsmAnd::StreetIntersection> > intersections;
                OsmAnd::ObfAddressSectionReader::loadIntersectionsFromStreet(reader, s, &intersections);
                output << xT("\t\t\t'") << QStringToStlString(s->nativeName) << xT("' [") << s->id << xT("], ") << buildings.size() << xT(" building(s), ") << intersections.size() << xT(" intersection(s)") << std::endl;
                if (cfg.verboseBuildings && buildings.size() > 0)
                {
                    output << xT("\t\t\t\tBuildings:") << std::endl;
                    for (auto itBuilding = buildings.cbegin(); itBuilding != buildings.cend(); ++itBuilding)
                    {
                        auto building = *itBuilding;

                        /*if (building->_interpolationInterval != 0)
                            output << xT("\t\t\t\t\t") << QStringToStlString(building->_latinName) << xT("-") << QStringToStlString(building->_latinName2) << xT(" (+") << building->_interpolationInterval << xT(") [") << building->_id << xT("]") << std::endl;
                        else if (building->_interpolation != OsmAnd::Building::Interpolation::Invalid)
                            output << xT("\t\t\t\t\t") << QStringToStlString(building->_latinName) << xT("-") << QStringToStlString(building->_latinName2) << xT(" (") << building->_interpolation << xT(") [") << building->_id << xT("]") << std::endl;*/
                    }
                }
                if (cfg.verboseIntersections && intersections.size() > 0)
                {
                    output << xT("\t\t\t\tIntersects with:") << std::endl;
                    for (auto itIntersection = intersections.cbegin(); itIntersection != intersections.cend(); ++itIntersection)
                    {
                        auto intersection = *itIntersection;

                        output << xT("\t\t\t\t\t") << QStringToStlString(intersection->nativeName) << std::endl;
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
    oStream << xT("(left top - right bottom) : ") << l << xT(", ") << t << xT(" NE - ") << r << xT(", ") << b << xT(" NE");
    return oStream.str();
}
