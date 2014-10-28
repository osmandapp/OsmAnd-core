#include "Productizer.h"

#include <OsmAndCore/stdlib_common.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <iomanip>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QSet>
#include <QFile>
#include <QXmlStreamWriter>
#include <OsmAndCore/restore_internal_warnings.h>
#include <OsmAndCore/QtCommon.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/QKeyValueIterator.h>

#include <OsmAndCoreTools.h>
#include <OsmAndCoreTools/Utilities.h>

OsmAndTools::Productizer::Productizer(const Configuration& configuration_)
    : configuration(configuration_)
{
}

OsmAndTools::Productizer::~Productizer()
{
}

#if defined(_UNICODE) || defined(UNICODE)
bool OsmAndTools::Productizer::productize(std::wostream& output)
#else
bool OsmAndTools::Productizer::productize(std::ostream& output)
#endif
{
    if (configuration.verbose)
        output << xT("Going to create IAP product definitions for ") << configuration.regions.size() << xT(" region(s)...") << std::endl;

    // Find subregions for each region (unflatten regions hierarchy)
    QHash< QString, QSet< QString > > subregionsByParent;
    for (const auto& region : configuration.regions)
    {
        // Root elements should be skipped
        if (region->parentId.isNull())
            continue;

        subregionsByParent[region->parentId].insert(region->id);
    }

    // Compute price tier for each region:
    QHash< QString, int> priceTierByRegion;

    // Price tiers:
    //  0 - free/not-a-product
    //  1 - country
    //  2 - large country
    //  3 - "part of the world"
    //  4 - "world"

    // Special price tiers:
    priceTierByRegion[QString::null] = 4;
    priceTierByRegion[OsmAnd::WorldRegions::AfricaRegionId] = 3;
    priceTierByRegion[OsmAnd::WorldRegions::AsiaRegionId] = 3;
    priceTierByRegion[OsmAnd::WorldRegions::AustraliaAndOceaniaRegionId] = 3;
    priceTierByRegion[OsmAnd::WorldRegions::CentralAmericaRegionId] = 3;
    priceTierByRegion[OsmAnd::WorldRegions::EuropeRegionId] = 3;
    priceTierByRegion[OsmAnd::WorldRegions::NorthAmericaRegionId] = 3;
    priceTierByRegion[OsmAnd::WorldRegions::SouthAmericaRegionId] = 3;
    priceTierByRegion[QLatin1String("us_northamerica")] = 2;
    priceTierByRegion[QLatin1String("canada_northamerica")] = 2;
    priceTierByRegion[OsmAnd::WorldRegions::RussiaRegionId] = 2;
    priceTierByRegion[QLatin1String("germany_europe")] = 1;
    priceTierByRegion[QLatin1String("italy_europe")] = 1;
    priceTierByRegion[QLatin1String("france_europe")] = 1;
    priceTierByRegion[QLatin1String("gb_england_europe")] = 1;

    // Everything else has price trier of a country (with exceptions)
    for (const auto& region : configuration.regions)
    {
        // Skip if a special price is defined
        if (priceTierByRegion.contains(region->id))
            continue;

        // Subregions of following countries are treated as bundle-only regions:
        //  - Germany
        //  - Italy
        //  - France
        //  - USA
        //  - Canada
        //  - England
        //  - Russia
        bool isBundleOnly = false;
        isBundleOnly = isBundleOnly || (region->parentId == QLatin1String("germany_europe"));
        isBundleOnly = isBundleOnly || (region->parentId == QLatin1String("italy_europe"));
        isBundleOnly = isBundleOnly || (region->parentId == QLatin1String("france_europe"));
        isBundleOnly = isBundleOnly || (region->parentId == QLatin1String("us_northamerica"));
        isBundleOnly = isBundleOnly || (region->parentId == QLatin1String("canada_northamerica"));
        isBundleOnly = isBundleOnly || (region->parentId == QLatin1String("gb_england_europe"));
        isBundleOnly = isBundleOnly || (region->parentId == QLatin1String("russia"));
        if (isBundleOnly)
        {
            if (configuration.verbose)
                output << xT("Region '") << QStringToStlString(region->id) << xT("' is treated as bundle-only region") << std::endl;

            priceTierByRegion[region->id] = 0;
            continue;
        }

        priceTierByRegion[region->id] = 1;
    }

    QFile file(configuration.outputProductsFilename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    {
        output << xT("Failed to open '") << QStringToStlString(configuration.outputProductsFilename) << xT("' for writing") << std::endl;
        return false;
    }

    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument(QLatin1String("1.0"), true);
    
    // <products>
    writer.writeStartElement(QLatin1String("products"));

    bool success = true;
    for (;;)
    {
        // Write all products that are based on regions
        for (const auto& itPriceEntry : OsmAnd::rangeOf(priceTierByRegion))
        {
            const auto& regionId = itPriceEntry.key();
            const auto& priceTier = itPriceEntry.value();

            if (configuration.verbose)
                output << xT("Product from region '") << QStringToStlString(regionId) << xT("' has ") << priceTier << xT(" price tier") << std::endl;

            // <regionAsProduct>
            writer.writeStartElement(QLatin1String("regionAsProduct"));
            writer.writeAttribute(QLatin1String("regionId"), regionId);
            writer.writeAttribute(QLatin1String("priceTier"), QString::number(priceTier));

            // Write names (if available)
            const auto citRegion = configuration.regions.constFind(regionId);
            if (citRegion != configuration.regions.cend())
            {
                const auto& region = *citRegion;

                writer.writeAttribute(QLatin1String("name"), region->name);
                for (const auto& itLocalizedNameEntry : OsmAnd::rangeOf(region->localizedNames))
                {
                    const auto& lang = itLocalizedNameEntry.key();
                    const auto& name = itLocalizedNameEntry.value();

                    // <localizedName/>
                    writer.writeStartElement(QLatin1String("localizedName"));
                    writer.writeAttribute(QLatin1String("lang"), lang);
                    writer.writeAttribute(QLatin1String("name"), name);
                    writer.writeEndElement();
                }
            }

            // </regionAsProduct>
            writer.writeEndElement();
        }

        break;
    }

    // </products>
    writer.writeEndElement();

    writer.writeEndDocument();
    file.close();
    
    return success;
}

bool OsmAndTools::Productizer::productize(QString *pLog /*= nullptr*/)
{
    if (pLog != nullptr)
    {
#if defined(_UNICODE) || defined(UNICODE)
        std::wostringstream output;
        const bool success = productize(output);
        *pLog = QString::fromStdWString(output.str());
        return success;
#else
        std::ostringstream output;
        const bool success = productize(output);
        *pLog = QString::fromStdString(output.str());
        return success;
#endif
    }
    else
    {
#if defined(_UNICODE) || defined(UNICODE)
        return productize(std::wcout);
#else
        return productize(std::cout);
#endif
    }
}

OsmAndTools::Productizer::Configuration::Configuration()
    : verbose(false)
{
}

bool OsmAndTools::Productizer::Configuration::parseFromCommandLineArguments(
    const QStringList& commandLineArgs,
    Configuration& outConfiguration,
    QString& outError)
{
    outConfiguration = Configuration();

    for (const auto& arg : commandLineArgs)
    {
        if (arg.startsWith(QLatin1String("-regions=")))
        {
            const auto value = Utilities::resolvePath(arg.mid(strlen("-regions=")));
            if (!QFile(value).exists())
            {
                outError = QString("'%1' file does not exist").arg(value);
                return false;
            }

            const auto ok = OsmAnd::WorldRegions(value).loadWorldRegions(outConfiguration.regions, nullptr);
            if (!ok)
            {
                outError = QString("'%1' file can not be parsed as OCBF file").arg(value);
                return false;
            }
        }
        else if (arg.startsWith(QLatin1String("-outputProductsFilename=")))
        {
            outConfiguration.outputProductsFilename = Utilities::resolvePath(arg.mid(strlen("-outputProductsFilename=")));
        }
        else if (arg == QLatin1String("-verbose"))
        {
            outConfiguration.verbose = true;
        }
        else
        {
            outError = QString("Unrecognized argument: '%1'").arg(arg);
            return false;
        }
    }

    return true;
}
