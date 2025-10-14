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
#include <OsmAndCore/CollatorStringMatcher.h>

namespace OsmAnd
{
    class ObfReader_P;
    class ObfAddressSectionInfo;
    
    
    
    class ObfAddressSectionReader_P Q_DECL_FINAL
    {
    public:
        typedef ObfAddressSectionReader::StreetGroupVisitorFunction StreetGroupVisitorFunction;
        typedef ObfAddressSectionReader::StreetVisitorFunction StreetVisitorFunction;
        typedef ObfAddressSectionReader::BuildingVisitorFunction BuildingVisitorFunction;
        typedef ObfAddressSectionReader::IntersectionVisitorFunction IntersectionVisitorFunction;

        enum class AddressNameIndexDataAtomType : uint32_t // CityBlock
        {
            Boundary = static_cast<int>(ObfAddressStreetGroupType::Boundary),
            CityOrTown = static_cast<int>(ObfAddressStreetGroupType::CityOrTown),
            Village = static_cast<int>(ObfAddressStreetGroupType::Village),
            Postcode = static_cast<int>(ObfAddressStreetGroupType::Postcode),
            Street = static_cast<int>(ObfAddressStreetGroupType::Street),
            Count = 5
        };
        


        struct AddressReference
        {
            AddressReference()
                : addressType(AddressNameIndexDataAtomType::Count)
                , dataIndexOffset(0)
                , cityIndexOffset(0)
            {
            }

            AddressNameIndexDataAtomType addressType;
            uint32_t dataIndexOffset; // refs
            uint32_t cityIndexOffset; // refsToCities
        };

        static bool dataDereferencedLessThan(AddressReference& o1, AddressReference& o2)
        {
            return o1.dataIndexOffset < o2.dataIndexOffset;
        }
        
        static bool cityDereferencedLessThan(AddressReference& o1, AddressReference& o2)
        {
            return o1.cityIndexOffset < o2.cityIndexOffset;
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
            QList< std::shared_ptr<const StreetGroup> >* resultOut,
            const AreaI* const bbox31,
            const ObfAddressStreetGroupTypesMask streetGroupTypesFilter,
            const StreetGroupVisitorFunction visitor,
            const std::shared_ptr<const IQueryController>& queryController);
        static void readCityHeader(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfAddressSectionInfo>& section,
            /*const ObfAddressStreetGroupType streetGroupType,*/
            const uint32_t groupOffset,
            std::shared_ptr<StreetGroup>& outStreetGroup,
            const AreaI* const bbox31,
            const std::shared_ptr<const IQueryController>& queryController);

        static void readStreetsFromGroup(
            const ObfReader_P& reader,
            const std::shared_ptr<const StreetGroup>& streetGroup,
            QList< std::shared_ptr<const Street> >* resultOut,
            const AreaI* const bbox31,
            const StreetVisitorFunction visitor,
            const std::shared_ptr<const IQueryController>& queryController);
        static void readStreet(
            const ObfReader_P& reader,
            const std::shared_ptr<const StreetGroup>& streetGroup,
            const uint32_t streetOffset,
            std::shared_ptr<Street>& outStreet,
            const AreaI* const bbox31,
            const std::shared_ptr<const IQueryController>& queryController);

        static void readBuildingsFromStreet(
            const ObfReader_P& reader,
            const std::shared_ptr<const Street>& street,
            QList< std::shared_ptr<const Building> >* resultOut,
            const AreaI* const bbox31,
            const BuildingVisitorFunction visitor,
            const std::shared_ptr<const IQueryController>& queryController);
        static void readBuilding(
            const ObfReader_P& reader,
            const std::shared_ptr<const Street>& street,
            const std::shared_ptr<const StreetGroup>& streetGroup,
            const uint32_t buildingOffset,
            std::shared_ptr<Building>& outBuilding,
            const AreaI* const bbox31,
            const std::shared_ptr<const IQueryController>& queryController);

        static void readIntersectionsFromStreet(
            const ObfReader_P& reader,
            const std::shared_ptr<const Street>& street,
            QList< std::shared_ptr<const StreetIntersection> >* resultOut,
            const AreaI* const bbox31,
            const IntersectionVisitorFunction visitor,
            const std::shared_ptr<const IQueryController>& queryController);
        static void readStreetIntersection(
            const ObfReader_P& reader,
            const std::shared_ptr<const Street>& street,
            std::shared_ptr<StreetIntersection>& outIntersection,
            const AreaI* const bbox31,
            const std::shared_ptr<const IQueryController>& queryController);

        static void readAddressesByName(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfAddressSectionInfo>& section,
            const QString& query,
            const StringMatcherMode matcherMode,
            QList< std::shared_ptr<const OsmAnd::Address> >* outAddresses,
            const AreaI* const bbox31,
            const ObfAddressStreetGroupTypesMask streetGroupTypesFilter,
            const bool includeStreets,
            const bool strictMatch,
            const ObfAddressSectionReader::VisitorFunction visitor,
            const std::shared_ptr<const IQueryController>& queryController);
        static void scanNameIndex(
            const ObfReader_P& reader,
            const QString& query,
            QHash<AddressNameIndexDataAtomType, QVector<AddressReference>>& outAddressReferences,
            const AreaI* const bbox31,
            const ObfAddressStreetGroupTypesMask streetGroupTypesFilter,
            const bool includeStreets,
            const bool strictMatch,
            const std::shared_ptr<const IQueryController>& queryController);
        static void readNameIndexData(
            const ObfReader_P& reader,
            const uint32_t baseOffset,
            QHash<AddressNameIndexDataAtomType, QVector<AddressReference>>& outAddressReferences,
            const AreaI* const bbox31,
            const ObfAddressStreetGroupTypesMask streetGroupTypesFilter,
            const bool includeStreets,
            const std::shared_ptr<const IQueryController>& queryController);
        static void readNameIndexDataAtom(
            const ObfReader_P& reader,
            const uint32_t baseOffset,
            QHash<AddressNameIndexDataAtomType, QVector<AddressReference>>& outAddressReferences,
            const AreaI* const bbox31,
            const ObfAddressStreetGroupTypesMask streetGroupTypesFilter,
            const bool includeStreets,
            const std::shared_ptr<const IQueryController>& queryController);
    public:
        static void loadStreetGroups(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfAddressSectionInfo>& section,
            QList< std::shared_ptr<const StreetGroup> >* resultOut,
            const AreaI* const bbox31,
            const ObfAddressStreetGroupTypesMask streetGroupTypesFilter,
            const StreetGroupVisitorFunction visitor,
            const std::shared_ptr<const IQueryController>& queryController);

        static void loadStreetsFromGroup(
            const ObfReader_P& reader,
            const std::shared_ptr<const StreetGroup>& streetGroup,
            QList< std::shared_ptr<const Street> >* resultOut,
            const AreaI* const bbox31,
            const StreetVisitorFunction visitor,
            const std::shared_ptr<const IQueryController>& queryController);

        static void loadBuildingsFromStreet(
            const ObfReader_P& reader,
            const std::shared_ptr<const Street>& street,
            QList< std::shared_ptr<const Building> >* resultOut,
            const AreaI* const bbox31,
            const BuildingVisitorFunction visitor,
            const std::shared_ptr<const IQueryController>& queryController);

        static void loadIntersectionsFromStreet(
            const ObfReader_P& reader,
            const std::shared_ptr<const Street>& street,
            QList< std::shared_ptr<const StreetIntersection> >* resultOut,
            const AreaI* const bbox31,
            const IntersectionVisitorFunction visitor,
            const std::shared_ptr<const IQueryController>& queryController);

        static void scanAddressesByName(
            const ObfReader_P& reader,
            const std::shared_ptr<const ObfAddressSectionInfo>& section,
            const QString& query,
            const StringMatcherMode matcherMode,
            QList< std::shared_ptr<const OsmAnd::Address> >* outAddresses,
            const AreaI* const bbox31,
            const ObfAddressStreetGroupTypesMask streetGroupTypesFilter,
            const bool includeStreets,
            const bool strictMatch,
            const ObfAddressSectionReader::VisitorFunction visitor,
            const std::shared_ptr<const IQueryController>& queryController);

    friend class OsmAnd::ObfReader_P;
    friend class OsmAnd::ObfAddressSectionReader;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_ADDRESS_SECTION_READER_P_H_)
