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
            Unknown = ObfAddressStreetGroupType::Unknown,
            CityOrTown = static_cast<int>(ObfAddressStreetGroupType::CityOrTown),
            Village = static_cast<int>(ObfAddressStreetGroupType::Village),
            Postcode = static_cast<int>(ObfAddressStreetGroupType::Postcode),
            Street = 4
        };

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

        using VisitorFunction = std::function<void(std::unique_ptr<Address> address)>;
        using AddressReferenceVisitorFunction = std::function<void(std::unique_ptr<ObfAddressSectionReader::AddressReference> address)>;
        using StreetGroupVisitorFunction = std::function<void(std::unique_ptr<StreetGroup> streetGroup)>;
        using ObfStreetGroupVisitorFunction = std::function<void(std::unique_ptr<ObfStreetGroup> streetGroup)>;
        using StreetVisitorFunction = std::function<void(std::unique_ptr<Street> street)>;
        using ObfStreetVisitorFunction = std::function<void(std::unique_ptr<ObfStreet> street)>;
        using BuildingVisitorFunction = std::function<void(std::unique_ptr<Building> building)>;
        using ObfBuildingVisitorFunction = std::function<void(std::unique_ptr<ObfBuilding> building)>;
        using IntersectionVisitorFunction = std::function<void(std::unique_ptr<StreetIntersection> streetIntersection)>;
        using ObfIntersectionVisitorFunction = std::function<void(std::unique_ptr<ObfStreetIntersection> streetIntersection)>;

        using StringMatcherFunction = std::function<bool(const QString&, const QString&)>;

        class Filter
        {
        private:
            QString _name;
            Nullable<AreaI> _bbox31;
            Nullable<AreaI> _obfInfoAreaBbox31;
            Bitmask<AddressType> _addressTypes;
            ObfAddressStreetGroupTypesMask _streetGroupTypes = fullObfAddressStreetGroupTypesMask();
            QVector<Filter> _filters;
            std::shared_ptr<const ObfAddress> _parent;

            StringMatcherFunction _stringMatcher = OsmAnd::MATCHES;

            VisitorFunction _addressVisitor;
            AddressReferenceVisitorFunction _addressReferenceVisitor;
            StreetGroupVisitorFunction _streetGroupVisitor;
            ObfStreetGroupVisitorFunction _obfStreetGroupVisitor;
            StreetVisitorFunction _streetVisitor;
            ObfStreetVisitorFunction _obfStreetVisitor;
            BuildingVisitorFunction _buildingVisitor;
            ObfBuildingVisitorFunction _obfBuildingVisitor;
            IntersectionVisitorFunction _intersectionVisitor;
            ObfIntersectionVisitorFunction _obfIntersectionVisitor;

            const BinaryBoolFunction _op;
        public:
            Filter(BinaryBoolFunction op = AND);

            QString name() const;
            Filter& setName(const QString& name);
            Filter& setBbox(const AreaI& bbox31);
            Filter& setObfInfoAreaBbox(const AreaI& bbox31);
            Filter& setAddressStreetGroupTypes(const ObfAddressStreetGroupTypesMask& streetGroupTypes);
            Filter& setFilters(QVector<Filter> filters);
            Filter& setParent(std::shared_ptr<const ObfAddress> address);

            Filter& setStringMatcherFunction(StringMatcherFunction stringMatcher);

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

            uint commonStartPartLength(const QString& name) const;

            bool matches(const QString& name, const QHash<QString, QString>& names = {}) const;
            bool matches(const AreaI& bbox31) const;
            bool matches(const PointI& point) const;
            bool matches(const AddressType type) const;
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
