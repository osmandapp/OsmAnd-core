#ifndef _OSMAND_CORE_DATA_COMMON_TYPES_H_
#define _OSMAND_CORE_DATA_COMMON_TYPES_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Bitmask.h>

namespace OsmAnd
{
    enum class ObfDataType
    {
        Map = 0,
        Routing = 1,
        Address = 2,
        POI = 3,
        Transport = 4
    };

    typedef Bitmask<ObfDataType> ObfDataTypesMask;

    inline ObfDataTypesMask fullObfDataTypesMask()
    {
        return ObfDataTypesMask()
            .set(ObfDataType::Map)
            .set(ObfDataType::Routing)
            .set(ObfDataType::Address)
            .set(ObfDataType::POI)
            .set(ObfDataType::Transport);
    }

    class ObfSectionInfo;

    union OSMAND_CORE_API ObfObjectId
    {
        uint64_t id;

#if !defined(SWIG)
        inline operator uint64_t() const
        {
            return id;
        }

        inline bool operator==(const ObfObjectId& that)
        {
            return this->id == that.id;
        }

        inline bool operator!=(const ObfObjectId& that)
        {
            return this->id != that.id;
        }

        inline bool operator==(const uint64_t& that)
        {
            return this->id == that;
        }

        inline bool operator!=(const uint64_t& that)
        {
            return this->id != that;
        }
#endif // !defined(SWIG)

        inline static ObfObjectId invalidId()
        {
            ObfObjectId invalidId;
            invalidId.id = std::numeric_limits<uint64_t>::max();
            return invalidId;
        }

        inline bool isOsmId() const
        {
            return (static_cast<int64_t>(id) > 0);
        }
        
        int64_t getOsmId() const;
        bool isShiftedID() const;
        bool isIdFromRelation() const;
        bool isIdFromSplit() const;
        int64_t makeAmenityRightShift() const;

        QString toString() const;

        static ObfObjectId generateUniqueId(
            const uint64_t rawId,
            const uint32_t offsetInObf,
            const std::shared_ptr<const ObfSectionInfo>& obfSectionInfo);

        static ObfObjectId generateUniqueId(
            const uint32_t offsetInObf,
            const std::shared_ptr<const ObfSectionInfo>& obfSectionInfo);

        static ObfObjectId fromRawId(const uint64_t rawId);
    };

#if !defined(SWIG)
    static_assert(sizeof(ObfObjectId) == 8, "ObfObjectId must be 8 bytes in size");
#endif // !defined(SWIG)

    union ObfMapSectionDataBlockId
    {
        uint64_t id;
        struct
        {
            int sectionRuntimeGeneratedId;
            uint32_t offset;
        };

#if !defined(SWIG)
        inline operator uint64_t() const
        {
            return id;
        }

        inline bool operator==(const ObfMapSectionDataBlockId& that)
        {
            return this->id == that.id;
        }

        inline bool operator!=(const ObfMapSectionDataBlockId& that)
        {
            return this->id != that.id;
        }

        inline bool operator==(const uint64_t& that)
        {
            return this->id == that;
        }

        inline bool operator!=(const uint64_t& that)
        {
            return this->id != that;
        }
#endif // !defined(SWIG)

        inline static ObfMapSectionDataBlockId invalidId()
        {
            ObfMapSectionDataBlockId invalidId;
            invalidId.id = std::numeric_limits<uint64_t>::max();
            return invalidId;
        }
    };

#if !defined(SWIG)
    static_assert(sizeof(ObfMapSectionDataBlockId) == 8, "ObfMapSectionDataBlockId must be 8 bytes in size");
#endif // !defined(SWIG)

    enum class RoutingDataLevel
    {
        Basemap,
        Detailed,

        __LAST
    };
    enum {
        RoutingDataLevelsCount = static_cast<int>(RoutingDataLevel::__LAST)
    };

    union ObfRoutingSectionDataBlockId
    {
        uint64_t id;
        struct
        {
            int sectionRuntimeGeneratedId;
            uint32_t offset;
        };

#if !defined(SWIG)
        inline operator uint64_t() const
        {
            return id;
        }

        inline bool operator==(const ObfRoutingSectionDataBlockId& that)
        {
            return this->id == that.id;
        }

        inline bool operator!=(const ObfRoutingSectionDataBlockId& that)
        {
            return this->id != that.id;
        }

        inline bool operator==(const uint64_t& that)
        {
            return this->id == that;
        }

        inline bool operator!=(const uint64_t& that)
        {
            return this->id != that;
        }
#endif // !defined(SWIG)

        inline static ObfRoutingSectionDataBlockId invalidId()
        {
            ObfRoutingSectionDataBlockId invalidId;
            invalidId.id = std::numeric_limits<uint64_t>::max();
            return invalidId;
        }
    };

#if !defined(SWIG)
    static_assert(sizeof(ObfRoutingSectionDataBlockId) == 8, "ObfRoutingSectionDataBlockId must be 8 bytes in size");
#endif // !defined(SWIG)

    class ObfRoutingSectionInfo;
    typedef std::function < bool(
        const std::shared_ptr<const ObfRoutingSectionInfo>& section,
        const ObfRoutingSectionDataBlockId& blockId,
        const ObfObjectId roadId,
        const AreaI& bbox) > FilterRoadsByIdFunction;

    union OSMAND_CORE_API ObfPoiCategoryId
    {
        uint32_t value;

        inline uint32_t getMainCategoryIndex() const
        {
            return (value & 0x7F);
        }

        inline uint32_t getSubCategoryIndex() const
        {
            return (value >> 7);
        }

#if !defined(SWIG)
        inline operator uint32_t() const
        {
            return value;
        }

        inline bool operator==(const ObfPoiCategoryId& that)
        {
            return this->value == that.value;
        }

        inline bool operator!=(const ObfPoiCategoryId& that)
        {
            return this->value != that.value;
        }

        inline bool operator==(const uint32_t& that)
        {
            return this->value == that;
        }

        inline bool operator!=(const uint32_t& that)
        {
            return this->value != that;
        }
#endif // !defined(SWIG)

        static inline ObfPoiCategoryId create(const uint32_t mainCategoryIndex, const uint32_t subCategoryIndex)
        {
            ObfPoiCategoryId id;
            id.value = (subCategoryIndex << 7) | (mainCategoryIndex & 0x7F);
            return id;
        }
    };

#if !defined(SWIG)
    static_assert(sizeof(ObfPoiCategoryId) == 4, "ObfPoiCategoryId must be 4 bytes in size");
#endif // !defined(SWIG)

    enum class ObfAddressStreetGroupType : int32_t // CityBlocks
    {
        Unknown = -1,
        Boundary = 0,
        CityOrTown = 1,
        Village = 3,
        Postcode = 2,
        Street = 4
    };

    typedef Bitmask<ObfAddressStreetGroupType> ObfAddressStreetGroupTypesMask;

    inline ObfAddressStreetGroupTypesMask fullObfAddressStreetGroupTypesMask()
    {
        return ObfAddressStreetGroupTypesMask()
            .set(ObfAddressStreetGroupType::Boundary)
            .set(ObfAddressStreetGroupType::CityOrTown)
            .set(ObfAddressStreetGroupType::Village)
            .set(ObfAddressStreetGroupType::Postcode)
            .set(ObfAddressStreetGroupType::Street);
    }

    enum class ObfAddressStreetGroupSubtype : int32_t // CityType
    {
        Unknown = -1,
        
        City = 0, // 0. City
        Town,     // 1. Town
        Village,  // 2. Village
        Hamlet,   // 3. Hamlet - Small village
        Suburb,   // 4. Mostly district of the city (introduced to avoid duplicate streets in city) -
                  //    however BOROUGH, DISTRICT, NEIGHBOURHOOD could be used as well for that purpose
                  //    Main difference stores own streets to search and list by it
        // 5.2 stored in city / villages sections written as city type
        Boundary, // 5. boundary no streets
        // 5.3 stored in city / villages sections written as city type
        Postcode, // 6. write this could be activated after 5.2 release
        
        // not stored entities but registered to uniquely identify streets as SUBURB
        Borough,
        District,
        Neighbourhood,
        Census
    };
}

#endif // !defined(_OSMAND_CORE_DATA_COMMON_TYPES_H_)
