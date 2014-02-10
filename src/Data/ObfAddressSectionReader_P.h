#ifndef _OSMAND_CORE_OBF_ADDRESS_SECTION_READER_P_H_
#define _OSMAND_CORE_OBF_ADDRESS_SECTION_READER_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include <QList>
#include <QSet>

#include "CommonTypes.h"
#include "DataTypes.h"
#include "ObfMapSectionInfo_P.h"
#include "MapTypes.h"

namespace OsmAnd
{
    class ObfReader_P;
    class ObfAddressSectionInfo;
    class ObfAddressBlocksSectionInfo;
    class ObfAddressSectionReader;
    namespace Model
    {
        class StreetGroup;
        class Street;
        class Building;
        class StreetIntersection;
    }
    class IQueryController;

    class ObfAddressSectionReader_P
    {
    private:
        ObfAddressSectionReader_P();
        ~ObfAddressSectionReader_P();
    protected:
        static void read(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<ObfAddressSectionInfo>& section);

        static void readAddressBlocksSectionHeader(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<ObfAddressBlocksSectionInfo>& section);

        static void readStreetGroups(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfAddressSectionInfo>& section,
            QList< std::shared_ptr<const Model::StreetGroup> >* resultOut,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::StreetGroup>&)> visitor,
            const IQueryController* const controller,
            QSet<ObfAddressBlockType>* blockTypeFilter = nullptr);
        static void readStreetGroupsFromAddressBlocksSection(
            const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfAddressBlocksSectionInfo>& section,
            QList< std::shared_ptr<const Model::StreetGroup> >* resultOut,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::StreetGroup>&)> visitor,
            const IQueryController* const controller);
        static void readStreetGroupHeader( const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfAddressBlocksSectionInfo>& section,
            unsigned int offset, std::shared_ptr<OsmAnd::Model::StreetGroup>& outGroup );

        static void readStreetsFromGroup(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const Model::StreetGroup>& group,
            QList< std::shared_ptr<const Model::Street> >* resultOut,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::Street>&)> visitor,
            const IQueryController* const controller);

        static void readStreet(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const Model::StreetGroup>& group,
            const std::shared_ptr<Model::Street>& street);

        static void readBuildingsFromStreet(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const Model::Street>& street,
            QList< std::shared_ptr<const Model::Building> >* resultOut,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::Building>&)> visitor,
            const IQueryController* const controller);

        static void readBuilding(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const Model::Street>& street,
            const std::shared_ptr<Model::Building>& building);

        static void readIntersectionsFromStreet(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const Model::Street>& street,
            QList< std::shared_ptr<const Model::StreetIntersection> >* resultOut,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::StreetIntersection>&)> visitor,
            const IQueryController* const controller);

        static void readIntersectedStreet(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const Model::Street>& street,
            const std::shared_ptr<Model::StreetIntersection>& intersection);

        static void loadStreetGroups(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const ObfAddressSectionInfo>& section,
            QList< std::shared_ptr<const Model::StreetGroup> >* resultOut = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::StreetGroup>&)> visitor = nullptr,
            const IQueryController* const controller = nullptr,
            QSet<ObfAddressBlockType>* blockTypeFilter = nullptr);

        static void loadStreetsFromGroup(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const Model::StreetGroup>& group,
            QList< std::shared_ptr<const Model::Street> >* resultOut = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::Street>&)> visitor = nullptr,
            const IQueryController* const controller = nullptr);

        static void loadBuildingsFromStreet(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const Model::Street>& street,
            QList< std::shared_ptr<const Model::Building> >* resultOut = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::Building>&)> visitor = nullptr,
            const IQueryController* const controller = nullptr);

        static void loadIntersectionsFromStreet(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<const Model::Street>& street,
            QList< std::shared_ptr<const Model::StreetIntersection> >* resultOut = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::StreetIntersection>&)> visitor = nullptr,
            const IQueryController* const controller = nullptr);

    friend class OsmAnd::ObfReader_P;
    friend class OsmAnd::ObfAddressSectionReader;
    };
} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_OBF_ADDRESS_SECTION_READER_P_H_)
