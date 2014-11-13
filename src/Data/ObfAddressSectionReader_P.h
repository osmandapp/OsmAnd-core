#ifndef _OSMAND_CORE_OBF_ADDRESS_SECTION_READER_P_H_
#define _OSMAND_CORE_OBF_ADDRESS_SECTION_READER_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include <QList>
#include <QSet>

#include "CommonTypes.h"
#include "DataCommonTypes.h"
#include "ObfMapSectionInfo_P.h"
#include "MapCommonTypes.h"

namespace OsmAnd
{
    class ObfReader_P;
    class ObfAddressSectionInfo;
    class ObfAddressBlocksSectionInfo;
    class ObfAddressSectionReader;
    class StreetGroup;
    class Street;
    class Building;
    class StreetIntersection;
    class IQueryController;

    class ObfAddressSectionReader_P Q_DECL_FINAL
    {
    private:
        ObfAddressSectionReader_P();
        ~ObfAddressSectionReader_P();
    protected:
        static void read(const ObfReader_P& reader, const std::shared_ptr<ObfAddressSectionInfo>& section);

        static void readAddressBlocksSectionHeader(const ObfReader_P& reader, const std::shared_ptr<ObfAddressBlocksSectionInfo>& section);

        static void readStreetGroups(const ObfReader_P& reader, const std::shared_ptr<const ObfAddressSectionInfo>& section,
            QList< std::shared_ptr<const StreetGroup> >* resultOut,
            std::function<bool (const std::shared_ptr<const OsmAnd::StreetGroup>&)> visitor,
            const IQueryController* const controller,
            QSet<ObfAddressBlockType>* blockTypeFilter = nullptr);
        static void readStreetGroupsFromAddressBlocksSection(
            const ObfReader_P& reader, const std::shared_ptr<const ObfAddressBlocksSectionInfo>& section,
            QList< std::shared_ptr<const StreetGroup> >* resultOut,
            std::function<bool (const std::shared_ptr<const OsmAnd::StreetGroup>&)> visitor,
            const IQueryController* const controller);
        static void readStreetGroupHeader( const ObfReader_P& reader, const std::shared_ptr<const ObfAddressBlocksSectionInfo>& section,
            unsigned int offset, std::shared_ptr<OsmAnd::StreetGroup>& outGroup );

        static void readStreetsFromGroup(const ObfReader_P& reader, const std::shared_ptr<const StreetGroup>& group,
            QList< std::shared_ptr<const Street> >* resultOut,
            std::function<bool (const std::shared_ptr<const OsmAnd::Street>&)> visitor,
            const IQueryController* const controller);

        static void readStreet(const ObfReader_P& reader, const std::shared_ptr<const StreetGroup>& group,
            const std::shared_ptr<Street>& street);

        static void readBuildingsFromStreet(const ObfReader_P& reader, const std::shared_ptr<const Street>& street,
            QList< std::shared_ptr<const Building> >* resultOut,
            std::function<bool (const std::shared_ptr<const OsmAnd::Building>&)> visitor,
            const IQueryController* const controller);

        static void readBuilding(const ObfReader_P& reader, const std::shared_ptr<const Street>& street,
            const std::shared_ptr<Building>& building);

        static void readIntersectionsFromStreet(const ObfReader_P& reader, const std::shared_ptr<const Street>& street,
            QList< std::shared_ptr<const StreetIntersection> >* resultOut,
            std::function<bool (const std::shared_ptr<const OsmAnd::StreetIntersection>&)> visitor,
            const IQueryController* const controller);

        static void readIntersectedStreet(const ObfReader_P& reader, const std::shared_ptr<const Street>& street,
            const std::shared_ptr<StreetIntersection>& intersection);

        static void loadStreetGroups(const ObfReader_P& reader, const std::shared_ptr<const ObfAddressSectionInfo>& section,
            QList< std::shared_ptr<const StreetGroup> >* resultOut = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::StreetGroup>&)> visitor = nullptr,
            const IQueryController* const controller = nullptr,
            QSet<ObfAddressBlockType>* blockTypeFilter = nullptr);

        static void loadStreetsFromGroup(const ObfReader_P& reader, const std::shared_ptr<const StreetGroup>& group,
            QList< std::shared_ptr<const Street> >* resultOut = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Street>&)> visitor = nullptr,
            const IQueryController* const controller = nullptr);

        static void loadBuildingsFromStreet(const ObfReader_P& reader, const std::shared_ptr<const Street>& street,
            QList< std::shared_ptr<const Building> >* resultOut = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Building>&)> visitor = nullptr,
            const IQueryController* const controller = nullptr);

        static void loadIntersectionsFromStreet(const ObfReader_P& reader, const std::shared_ptr<const Street>& street,
            QList< std::shared_ptr<const StreetIntersection> >* resultOut = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::StreetIntersection>&)> visitor = nullptr,
            const IQueryController* const controller = nullptr);

    friend class OsmAnd::ObfReader_P;
    friend class OsmAnd::ObfAddressSectionReader;
    };
} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_OBF_ADDRESS_SECTION_READER_P_H_)
