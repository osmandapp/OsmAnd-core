#ifndef _OSMAND_CORE_OBF_ADDRESS_SECTION_READER_P_H_
#define _OSMAND_CORE_OBF_ADDRESS_SECTION_READER_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "DataCommonTypes.h"
#include "ObfAddressSectionReader.h"
#include "ObfAddressSectionInfo.h"
#include "ObfStreet.h"
#include "Nullable.h"

namespace OsmAnd
{
    class ObfReader_P;
    class ObfAddressSectionInfo;
    
    class ObfAddressSectionReader_P Q_DECL_FINAL
    {
    public:
        using AddressReference = ObfAddressSectionReader::AddressReference;
        using AddressNameIndexDataAtomType = ObfAddressSectionReader::AddressNameIndexDataAtomType;
        using BuildingVisitorFunction = ObfAddressSectionReader::BuildingVisitorFunction;
        using Filter = ObfAddressSectionReader::Filter;
        using FilterBuilder = ObfAddressSectionReader::FilterBuilder;
        using IntersectionVisitorFunction = ObfAddressSectionReader::IntersectionVisitorFunction;
        using StreetGroupVisitorFunction = ObfAddressSectionReader::StreetGroupVisitorFunction;
        using StreetVisitorFunction = ObfAddressSectionReader::StreetVisitorFunction;

        static bool dereferencedLessThan(AddressReference& o1, AddressReference& o2) {
//            if(o1.addressType < o2.addressType) {
//                return true;
//            }
            return o1.dataIndexOffset() < o2.dataIndexOffset();
        }

    private:
        ObfAddressSectionReader_P();
        ~ObfAddressSectionReader_P();
    protected:
        static void read(
                const ObfReader_P& reader,
                const std::shared_ptr<ObfAddressSectionInfo>& section);

        static void readStreetGroups(
                const ObfReader_P& reader,
                const std::shared_ptr<const ObfAddressSectionInfo>& section,
                const Filter &filter,
                const std::shared_ptr<const IQueryController>& queryController);
        static void readStreetGroupsFromBlock(const ObfReader_P& reader,
                const std::shared_ptr<const ObfAddressSectionInfo>& section,
                const Filter &filter,
                const std::shared_ptr<const IQueryController>& queryController);
        static void readStreetGroup(
                const ObfReader_P& reader,
                const std::shared_ptr<const ObfAddressSectionInfo>& section, const ObfAddressStreetGroupType streetGroupType,
                const uint32_t groupOffset,
                const Filter &filter,
                const std::shared_ptr<const IQueryController>& queryController);

        static void readStreetsFromGroup(
                const ObfReader_P& reader,
                const std::shared_ptr<const ObfStreetGroup>& streetGroup,
                const Filter &filter,
                const std::shared_ptr<const IQueryController>& queryController);
        static void readStreet(
                const ObfReader_P& reader,
                const std::shared_ptr<const ObfStreetGroup>& streetGroup,
                const uint32_t streetOffset,
                const Filter &filter,
                const std::shared_ptr<const IQueryController>& queryController);

        static void readBuildingsFromStreet(const ObfReader_P& reader,
                const std::shared_ptr<const ObfStreet> &street,
                const Filter &filter,
                const std::shared_ptr<const IQueryController>& queryController);
        static void readBuilding(const ObfReader_P& reader,
                const std::shared_ptr<const ObfStreet> &street,
                const std::shared_ptr<const ObfStreetGroup>& streetGroup,
                const uint32_t buildingOffset,
                const Filter& filter,
                const std::shared_ptr<const IQueryController>& queryController);

        static void readIntersectionsFromStreet(
                const ObfReader_P& reader,
                const std::shared_ptr<const ObfStreet>& street,
                const Filter &filter,
                const std::shared_ptr<const IQueryController>& queryController);
        static void readStreetIntersection(
                const ObfReader_P& reader,
                const std::shared_ptr<const ObfStreet>& street,
                const Filter &filter,
                const std::shared_ptr<const IQueryController>& queryController);

        static void readAddressesByName(
                const ObfReader_P& reader,
                const std::shared_ptr<const ObfAddressSectionInfo>& section,
                const Filter &filter,
                const std::shared_ptr<const IQueryController>& queryController);
        static void scanNameIndex(
                const ObfReader_P& reader,
                const Filter& filter,
                const std::shared_ptr<const IQueryController>& queryController);
        static void readNameIndexData(
                const ObfReader_P& reader,
                const uint32_t baseOffset,
                const Filter& filter,
                const std::shared_ptr<const IQueryController>& queryController);
        static void readNameIndexDataAtom(
                const ObfReader_P& reader,
                const uint32_t baseOffset,
                const Filter& filter,
                const std::shared_ptr<const IQueryController>& queryController);
    public:
        static void loadStreetGroups(
                const ObfReader_P& reader,
                const std::shared_ptr<const ObfAddressSectionInfo>& section,
                const Filter &filter,
                const std::shared_ptr<const IQueryController>& queryController);

        static void loadStreetsFromGroup(
                const ObfReader_P& reader,
                const std::shared_ptr<const ObfStreetGroup>& streetGroup,
                const Filter& filter,
                const std::shared_ptr<const IQueryController>& queryController);

        static void loadBuildingsFromStreet(
                const ObfReader_P& reader,
                const std::shared_ptr<const ObfStreet>& street,
                const Filter& filter,
                const std::shared_ptr<const IQueryController>& queryController);

        static void loadIntersectionsFromStreet(const ObfReader_P& reader,
                const std::shared_ptr<const ObfStreet>& street,
                const Filter& filter,
                const std::shared_ptr<const IQueryController>& queryController);

        static void scanAddressesByName(
                const ObfReader_P& reader,
                const std::shared_ptr<const ObfAddressSectionInfo>& section,
                const Filter& filter,
                const std::shared_ptr<const IQueryController>& queryController);

        friend class OsmAnd::ObfReader_P;
        friend class OsmAnd::ObfAddressSectionReader;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_ADDRESS_SECTION_READER_P_H_)
