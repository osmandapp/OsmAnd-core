#ifndef _OSMAND_CORE_OBF_ADDRESS_SECTION_READER_H_
#define _OSMAND_CORE_OBF_ADDRESS_SECTION_READER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/DataCommonTypes.h>
#include <OsmAndCore/CollatorStringMatcher.h>

namespace OsmAnd
{
    class ObfReader;
    class ObfAddressSectionInfo;
    class Address;
    class StreetGroup;
    class Street;
    class Building;
    class IQueryController;

    class OSMAND_CORE_API ObfAddressSectionReader
    {
    public:
        typedef std::function<bool(const std::shared_ptr<const OsmAnd::Address>& address)> VisitorFunction;
        typedef std::function<bool(const std::shared_ptr<const OsmAnd::StreetGroup>& streetGroup)> StreetGroupVisitorFunction;
        typedef std::function<bool(const std::shared_ptr<const OsmAnd::Street>& street)> StreetVisitorFunction;
        typedef std::function<bool(const std::shared_ptr<const OsmAnd::Building>& building)> BuildingVisitorFunction;
        typedef std::function<bool(const std::shared_ptr<const OsmAnd::Street>& streetIntersection)> IntersectionVisitorFunction;

    private:
        ObfAddressSectionReader();
        ~ObfAddressSectionReader();
    protected:
    public:
        static void loadStreetGroups(
            const std::shared_ptr<const ObfReader>& reader,
            const std::shared_ptr<const ObfAddressSectionInfo>& section,
            QList< std::shared_ptr<const StreetGroup> >* resultOut = nullptr,
            const AreaI* const bbox31 = nullptr,
            const ObfAddressStreetGroupTypesMask streetGroupTypesFilter = fullObfAddressStreetGroupTypesMask(),
            const StreetGroupVisitorFunction visitor = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);

        static void loadStreetsFromGroup(
            const std::shared_ptr<const ObfReader>& reader,
            const std::shared_ptr<const StreetGroup>& streetGroup,
            QList< std::shared_ptr<const Street> >* resultOut = nullptr,
            const AreaI* const bbox31 = nullptr,
            const StreetVisitorFunction visitor = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);

        static void loadBuildingsFromStreet(
            const std::shared_ptr<const ObfReader>& reader,
            const std::shared_ptr<const Street>& street,
            QList< std::shared_ptr<const Building> >* resultOut = nullptr,
            const AreaI* const bbox31 = nullptr,
            const BuildingVisitorFunction visitor = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);

        static void loadIntersectionsFromStreet(
            const std::shared_ptr<const ObfReader>& reader,
            const std::shared_ptr<const Street>& street,
            QList< std::shared_ptr<const Street> >* resultOut = nullptr,
            const AreaI* const bbox31 = nullptr,
            const IntersectionVisitorFunction visitor = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);

        static void scanAddressesByName(
            const std::shared_ptr<const ObfReader>& reader,
            const std::shared_ptr<const ObfAddressSectionInfo>& section,
            const QString& query,
            const StringMatcherMode matcherMode,
            QList< std::shared_ptr<const OsmAnd::Address> >* outAddresses,
            const AreaI* const bbox31 = nullptr,
            const ObfAddressStreetGroupTypesMask streetGroupTypesFilter = fullObfAddressStreetGroupTypesMask(),
            const bool includeStreets = true,
            const bool strictMatch = false,
            const ObfAddressSectionReader::VisitorFunction visitor = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_OBF_ADDRESS_SECTION_READER_H_)
