#ifndef _OSMAND_CORE_OBF_ADDRESS_SECTION_READER_H_
#define _OSMAND_CORE_OBF_ADDRESS_SECTION_READER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QVector>
#include <QMap>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/Address.h>
#include <OsmAndCore/Data/DataCommonTypes.h>
#include <OsmAndCore/Data/ObfAddress.h>
#include <OsmAndCore/Nullable.h>
#include <OsmAndCore/Utilities.h>

namespace OsmAnd
{
    class Address;
    class Building;
    class IQueryController;
    class ObfAddressSectionInfo;
    class ObfBuilding;
    class ObfReader;
    class ObfStreet;
    class ObfStreetGroup;
    class ObfStreetIntersection;
    class Street;
    class StreetGroup;
    class StreetIntersection;

    class OSMAND_CORE_API ObfAddressSectionReader
    {
    public:
        enum class AddressNameIndexDataAtomType : uint32_t
        {
            Unknown = static_cast<int>(ObfAddressStreetGroupType::Unknown),
            CityOrTown = static_cast<int>(ObfAddressStreetGroupType::CityOrTown),
            Village = static_cast<int>(ObfAddressStreetGroupType::Village),
            Postcode = static_cast<int>(ObfAddressStreetGroupType::Postcode),
            Street = 4
        };

        inline static Bitmask<AddressNameIndexDataAtomType> fullObfAddressNameIndexDataAtomTypeMask()
        {
            return Bitmask<AddressNameIndexDataAtomType>()
                    .set(AddressNameIndexDataAtomType::CityOrTown)
                    .set(AddressNameIndexDataAtomType::Postcode)
                    .set(AddressNameIndexDataAtomType::Street)
                    .set(AddressNameIndexDataAtomType::Village);
        }

        class AddressReference
        {
        private:
            QString _name;
            QString _nameEn;
            AddressNameIndexDataAtomType _addressType;
            uint32_t _dataIndexOffset;
            uint32_t _containerIndexOffset;
            PointI _position31;

        public:
            AddressReference(
                    AddressNameIndexDataAtomType addressType = AddressNameIndexDataAtomType::Unknown,
                    QString name = {},
                    QString nameEn = {},
                    uint32_t dataIndexOffset = 0,
                    uint32_t containerIndexOffset = 0,
                    PointI position31 = {});

            QString name() const;
            QString nameEn() const;
            AddressNameIndexDataAtomType addressType() const;
            uint32_t dataIndexOffset() const;
            uint32_t containerIndexOffset() const;
            PointI position31() const;
        };

        using AddressReferenceVisitorFunction = std::function<void(std::unique_ptr<ObfAddressSectionReader::AddressReference> address)>;
        using BuildingVisitorFunction = std::function<void(std::unique_ptr<Building> building)>;
        using IntersectionVisitorFunction = std::function<void(std::unique_ptr<StreetIntersection> streetIntersection)>;
        using ObfBuildingVisitorFunction = std::function<void(std::unique_ptr<ObfBuilding> building)>;
        using ObfIntersectionVisitorFunction = std::function<void(std::unique_ptr<ObfStreetIntersection> streetIntersection)>;
        using ObfStreetGroupVisitorFunction = std::function<void(std::unique_ptr<ObfStreetGroup> streetGroup)>;
        using ObfStreetVisitorFunction = std::function<void(std::unique_ptr<ObfStreet> street)>;
        using StreetGroupVisitorFunction = std::function<void(std::unique_ptr<StreetGroup> streetGroup)>;
        using StreetVisitorFunction = std::function<void(std::unique_ptr<Street> street)>;
        using VisitorFunction = std::function<void(std::unique_ptr<Address> address)>;

        using StringMatcherFunction = std::function<bool(const QString&, const QString&)>;

        class Filter
        {
        private:
            Bitmask<AddressNameIndexDataAtomType> _addressNameIndexDataAtomTypeMask = fullObfAddressNameIndexDataAtomTypeMask();
            Bitmask<AddressType> _addressTypeMask = fullAddressTypeMask();
            Nullable<AreaI> _bbox31;
            Nullable<AreaI> _obfInfoAreaBbox31;
            ObfAddressStreetGroupTypesMask _streetGroupTypes = fullObfAddressStreetGroupTypesMask();
            QString _name;
            QVector<Filter> _filters;
            std::shared_ptr<const ObfAddress> _parent;

            StringMatcherFunction _stringMatcher = OsmAnd::MATCHES;
            const Qt::CaseSensitivity _caseSensitivity = Qt::CaseInsensitive;

            AddressReferenceVisitorFunction _addressReferenceVisitor;
            BuildingVisitorFunction _buildingVisitor;
            IntersectionVisitorFunction _intersectionVisitor;
            ObfBuildingVisitorFunction _obfBuildingVisitor;
            ObfIntersectionVisitorFunction _obfIntersectionVisitor;
            ObfStreetGroupVisitorFunction _obfStreetGroupVisitor;
            ObfStreetVisitorFunction _obfStreetVisitor;
            StreetGroupVisitorFunction _streetGroupVisitor;
            StreetVisitorFunction _streetVisitor;
            VisitorFunction _addressVisitor;

            const BinaryBoolFunction _op;
        public:
            Filter(BinaryBoolFunction op = AND);

            QString name() const;
            Filter& setAddressNameIndexDataAtomType(const Bitmask<AddressNameIndexDataAtomType>& addressNameIndexDataAtomType);
            Filter& setAddressStreetGroupTypes(const ObfAddressStreetGroupTypesMask& streetGroupTypes);
            Filter& setAddressTypes(const Bitmask<AddressType>& addressTypes);
            Filter& setBbox(const AreaI& bbox31);
            Filter& setFilters(QVector<Filter> filters);
            Filter& setName(const QString& name);
            Filter& setObfInfoAreaBbox(const AreaI& bbox31);
            Filter& setParent(std::shared_ptr<const ObfAddress> address);

            Filter& setStringMatcherFunction(StringMatcherFunction stringMatcher);
//            Filter& setCaseSensitivity(Qt::CaseSensitivity caseSensitivity);

            Filter& setVisitor(VisitorFunction visitor);
            Filter& setVisitor(AddressReferenceVisitorFunction visitor);
            Filter& setVisitor(StreetGroupVisitorFunction visitor);
            Filter& setVisitor(ObfStreetGroupVisitorFunction visitor);
            Filter& setVisitor(StreetVisitorFunction visitor);
            Filter& setVisitor(ObfStreetVisitorFunction visitor);
            Filter& setVisitor(BuildingVisitorFunction visitor);
            Filter& setVisitor(ObfBuildingVisitorFunction visitor);
            Filter& setVisitor(IntersectionVisitorFunction visitor);
            Filter& setVisitor(ObfIntersectionVisitorFunction visitor);

            Filter& buildTopLevelFilters();


            uint commonStartPartLength(const QString& name) const;

            bool matches(const QString& name, const QString& nameEn) const;
            bool matches(const QString& name, const QHash<QString, QString>& names = {}) const;
            bool matches(const AreaI& bbox31) const;
            bool matches(const PointI& point) const;
            bool matches(const AddressType type) const;
            bool matches(const AddressNameIndexDataAtomType type) const;
            bool matches(const ObfAddressStreetGroupType type) const;

            bool matches(const Address& address) const;
            bool matches(const StreetGroup& streetGroup) const;
            bool matches(const Street& street) const;
            bool matches(const StreetIntersection& street) const;
            bool matches(const Building& building) const;
            bool matches(const AddressReference& addressReference) const;

            bool operator()(std::unique_ptr<Address> address) const;
            bool operator()(std::unique_ptr<StreetGroup> streetGroup) const;
            bool operator()(std::unique_ptr<ObfStreetGroup> streetGroup) const;
            bool operator()(std::unique_ptr<Street> street) const;
            bool operator()(std::unique_ptr<ObfStreet> street) const;
            bool operator()(std::unique_ptr<Building> building) const;
            bool operator()(std::unique_ptr<ObfStreetIntersection> streetIntersection) const;
            bool operator()(std::unique_ptr<StreetIntersection> streetIntersection) const;
            bool operator()(std::unique_ptr<ObfBuilding> building) const;
            bool operator()(std::unique_ptr<AddressReference> addressReference) const;
            std::shared_ptr<const ObfAddress> parent() const;
            Nullable<AreaI> obfInfoAreaBbox31() const;
        };


    private:
        ObfAddressSectionReader();
        ~ObfAddressSectionReader();
    protected:
    public:
        static void loadStreetGroups(
                const std::shared_ptr<const ObfReader>& reader,
                const std::shared_ptr<const ObfAddressSectionInfo>& section,
                const Filter& filter = Filter{},
                const std::shared_ptr<const IQueryController>& queryController = nullptr);

        static void loadStreetsFromGroup(
                const std::shared_ptr<const ObfReader>& reader,
                const std::shared_ptr<const ObfStreetGroup>& streetGroup,
                const Filter& filter = Filter{},
                const std::shared_ptr<const IQueryController>& queryController = nullptr);

        static void loadBuildingsFromStreet(
                const std::shared_ptr<const ObfReader>& reader,
                const std::shared_ptr<const ObfStreet>& street,
                const Filter& filter = Filter{},
                const std::shared_ptr<const IQueryController>& queryController = nullptr);

        static void loadIntersectionsFromStreet(
                const std::shared_ptr<const ObfReader>& reader,
                const std::shared_ptr<const ObfStreet>& street,
                const Filter& filter = Filter{},
                const std::shared_ptr<const IQueryController>& queryController = nullptr);

        static void scanAddressesByName(
                const std::shared_ptr<const ObfReader>& reader,
                const std::shared_ptr<const ObfAddressSectionInfo>& section,
                const Filter& filter = Filter{},
                const std::shared_ptr<const IQueryController>& queryController = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_OBF_ADDRESS_SECTION_READER_H_)
