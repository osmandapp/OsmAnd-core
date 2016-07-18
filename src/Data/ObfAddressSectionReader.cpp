#include "ObfAddressSectionReader.h"
#include "ObfAddressSectionReader_P.h"

#include "Building.h"
#include "ObfBuilding.h"
#include "ObfReader.h"
#include "ObfStreetGroup.h"
#include "ObfStreet.h"
#include "ObfStreetIntersection.h"
#include "StreetGroup.h"
#include "Street.h"
#include "StreetIntersection.h"

OsmAnd::ObfAddressSectionReader::ObfAddressSectionReader()
{
}

OsmAnd::ObfAddressSectionReader::~ObfAddressSectionReader()
{
}

void OsmAnd::ObfAddressSectionReader::loadStreetGroups(
    const std::shared_ptr<const ObfReader>& reader,
    const std::shared_ptr<const ObfAddressSectionInfo>& section,
    const Filter& filter,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    ObfAddressSectionReader_P::loadStreetGroups(
        *reader->_p,
        section,
        filter,
        queryController);
}

void OsmAnd::ObfAddressSectionReader::loadStreetsFromGroup(
        const std::shared_ptr<const ObfReader>& reader,
        const std::shared_ptr<const ObfStreetGroup>& streetGroup,
        const Filter& filter,
        const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    ObfAddressSectionReader_P::loadStreetsFromGroup(
        *reader->_p,
        streetGroup,
        filter,
        queryController);
}

void OsmAnd::ObfAddressSectionReader::loadBuildingsFromStreet(
        const std::shared_ptr<const ObfReader>& reader,
    const std::shared_ptr<const ObfStreet>& street,
    const Filter& filter,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    ObfAddressSectionReader_P::loadBuildingsFromStreet(
        *reader->_p,
        street,
        filter,
        queryController);
}

void OsmAnd::ObfAddressSectionReader::loadIntersectionsFromStreet(
        const std::shared_ptr<const ObfReader>& reader,
        const std::shared_ptr<const ObfStreet>& street,
        const Filter& filter,
        const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    ObfAddressSectionReader_P::loadIntersectionsFromStreet(
        *reader->_p,
        street,
        filter,
        queryController);
}

void OsmAnd::ObfAddressSectionReader::scanAddressesByName(
    const std::shared_ptr<const ObfReader>& reader,
    const std::shared_ptr<const ObfAddressSectionInfo>& section,
    const Filter& filter,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    ObfAddressSectionReader_P::scanAddressesByName(
        *reader->_p,
        section,
        filter,
        queryController);
}

OsmAnd::ObfAddressSectionReader::Filter::Filter(BinaryBoolFunction op)
    : _op(op)
{

}

std::shared_ptr<const OsmAnd::ObfAddress> OsmAnd::ObfAddressSectionReader::Filter::parent() const
{
    return _parent;
}

OsmAnd::Nullable<OsmAnd::AreaI> OsmAnd::ObfAddressSectionReader::Filter::obfInfoAreaBbox31() const
{
    return _obfInfoAreaBbox31;
}

uint OsmAnd::ObfAddressSectionReader::Filter::commonStartPartLength(
        const QString& name) const
{
    if (name.startsWith(_name, _caseSensitivity))
        return _name.length();
    else if (_name.startsWith(name, _caseSensitivity))
        return name.length();
    else
        return 0;
}

bool OsmAnd::ObfAddressSectionReader::Filter::matches(
        const QString& name,
        const QString& nameEn) const
{
    return !_name.isNull() || _stringMatcher(name, _name) || _stringMatcher(nameEn, _name);
}

bool OsmAnd::ObfAddressSectionReader::Filter::matches(
        const QString& name,
        const QHash<QString, QString>& names) const
//        const OsmAnd::ObfAddressSectionReader::StringMatcherFunction& matcher) const
{
    if (_name.isNull())
        return true;
    bool result = _stringMatcher(name, _name);
    for (const auto& name : constOf(names))
        result = result || _stringMatcher(name, _name);
    for (const Filter& filter : _filters)
        result = _op(result, filter.matches(name, names));
    return result;
}

bool OsmAnd::ObfAddressSectionReader::Filter::matches(
        const OsmAnd::AreaI& bbox31) const
{
//    return !_bbox.isSet() || _bbox->contains(bbox);
    bool result = !_bbox31.isSet() || _bbox31->contains(bbox31) || bbox31.contains(*_bbox31) || _bbox31->intersects(bbox31);
    for (const Filter& filter : _filters)
        result = _op(result, filter.matches(bbox31));
    return result;
}

bool OsmAnd::ObfAddressSectionReader::Filter::matches(
        const OsmAnd::PointI& point) const
{
    bool result = !_bbox31.isSet() || _bbox31->contains(point);
    for (const Filter& filter : _filters)
        result = _op(result, filter.matches(point));
    return result;
}

bool OsmAnd::ObfAddressSectionReader::Filter::matches(
        const OsmAnd::Address& address) const
{
    bool result = matches(address.position31());
    result = _op(result, matches(address.nativeName(), address.localizedNames()));
    for (const Filter& filter : _filters)
        result = _op(result, filter.matches(address));
    return result;
}

bool OsmAnd::ObfAddressSectionReader::Filter::matches(
        const OsmAnd::StreetGroup& streetGroup) const
{
    bool result = matches(streetGroup.type);
    result = _op(result, matches(static_cast<const Address&>(streetGroup)));
    for (const Filter& filter : _filters)
        result = _op(result, filter.matches(streetGroup));
    return result;
}

bool OsmAnd::ObfAddressSectionReader::Filter::matches(
        const Street& street) const
{
    bool result = matches(static_cast<const Address&>(street));
    for (const Filter& filter : _filters)
        result = _op(result, filter.matches(street));
    return result;

}

bool OsmAnd::ObfAddressSectionReader::Filter::matches(
        const StreetIntersection& streetIntersection) const
{
    bool result = matches(static_cast<const Address&>(streetIntersection)) || matches(static_cast<const Address&>(streetIntersection.street()));
    for (const Filter& filter : _filters)
        result = _op(result, filter.matches(streetIntersection));
    return result;
}

bool OsmAnd::ObfAddressSectionReader::Filter::matches(
        const OsmAnd::Building& building) const
{
    bool result = matches(static_cast<const Address&>(building)) || matches(static_cast<const Address&>(building.interpolation()));
    result = _op(result, matches(building.postcode()));
    for (const Filter& filter : _filters)
        result = _op(result, filter.matches(building));
    return result;
}

bool OsmAnd::ObfAddressSectionReader::Filter::matches(
        const OsmAnd::ObfAddressSectionReader::AddressReference& addressReference) const
{
    bool result = matches(addressReference.addressType());
    result = _op(result, matches(addressReference.position31()));
    result = _op(result, matches(addressReference.name(), addressReference.nameEn()));
    for (const Filter& filter : _filters)
        result = _op(result, filter.matches(addressReference));
    return result;
}

bool OsmAnd::ObfAddressSectionReader::Filter::operator()(
        std::unique_ptr<OsmAnd::Address> address) const
{
    bool result = address && matches(*address);
    if (result)
        _addressVisitor(std::move(address));
    return result;
}

bool OsmAnd::ObfAddressSectionReader::Filter::operator()(
        std::unique_ptr<OsmAnd::StreetGroup> streetGroup) const
{
    bool result = streetGroup && matches(*streetGroup);
    if (result)
        _streetGroupVisitor(std::move(streetGroup));
    return result;
}

bool OsmAnd::ObfAddressSectionReader::Filter::operator()(
        std::unique_ptr<OsmAnd::ObfStreetGroup> streetGroup) const
{
    bool result = streetGroup && matches(streetGroup->streetGroup());
    if (result)
        _obfStreetGroupVisitor(std::move(streetGroup));
    return result;
}

bool OsmAnd::ObfAddressSectionReader::Filter::operator()(
        std::unique_ptr<OsmAnd::Street> street) const
{
    bool result = street && matches(*street);
    if (result)
        _streetVisitor(std::move(street));
    return result;
}

bool OsmAnd::ObfAddressSectionReader::Filter::operator()(
        std::unique_ptr<OsmAnd::ObfStreet> street) const
{
    bool result = street && matches(street->street());
    if (result)
        _obfStreetVisitor(std::move(street));
    return result;
}

bool OsmAnd::ObfAddressSectionReader::Filter::operator()(
        std::unique_ptr<OsmAnd::StreetIntersection> streetIntersection) const
{
    bool result = streetIntersection && matches(*streetIntersection);
    if (result)
        _intersectionVisitor(std::move(streetIntersection));
    return result;
}

bool OsmAnd::ObfAddressSectionReader::Filter::operator()(
        std::unique_ptr<OsmAnd::ObfStreetIntersection> streetIntersection) const
{
    bool result = streetIntersection && matches(streetIntersection->streetIntersection());
    if (result)
        _obfIntersectionVisitor(std::move(streetIntersection));
    return result;
}

bool OsmAnd::ObfAddressSectionReader::Filter::operator()(
        std::unique_ptr<OsmAnd::Building> building) const
{
    bool result = building && matches(*building);
    if (result)
        _buildingVisitor(std::move(building));
    return result;
}

bool OsmAnd::ObfAddressSectionReader::Filter::operator()(
        std::unique_ptr<OsmAnd::ObfBuilding> building) const
{
    bool result = building && matches(building->building());
    if (result)
        _obfBuildingVisitor(std::move(building));
    return result;
}

bool OsmAnd::ObfAddressSectionReader::Filter::operator()(
        std::unique_ptr<OsmAnd::ObfAddressSectionReader::AddressReference> addressReference) const
{
    bool result = addressReference && matches(*addressReference);
    if (result)
        _addressReferenceVisitor(std::move(addressReference));
    return result;
}

bool OsmAnd::ObfAddressSectionReader::Filter::matches(
        const OsmAnd::AddressType type) const
{
    bool result = _addressTypeMask.isSet(type);
    for (const Filter& filter : _filters)
        result = _op(result, filter.matches(type));
    return result;
}

bool OsmAnd::ObfAddressSectionReader::Filter::matches(
        const OsmAnd::ObfAddressSectionReader::AddressNameIndexDataAtomType type) const
{

}

bool OsmAnd::ObfAddressSectionReader::Filter::matches(
        const OsmAnd::ObfAddressStreetGroupType type) const
{
    bool result = _streetGroupTypes.isSet(type);
    for (const Filter& filter : _filters)
        result = _op(result, filter.matches(type));
    return result;
}

QString OsmAnd::ObfAddressSectionReader::AddressReference::name() const
{
    return _name;
}

QString OsmAnd::ObfAddressSectionReader::AddressReference::nameEn() const
{
    return _nameEn;
}

OsmAnd::ObfAddressSectionReader::AddressNameIndexDataAtomType OsmAnd::ObfAddressSectionReader::AddressReference::addressType() const
{
    return _addressType;
}

uint32_t OsmAnd::ObfAddressSectionReader::AddressReference::dataIndexOffset() const
{
    return _dataIndexOffset;
}

uint32_t OsmAnd::ObfAddressSectionReader::AddressReference::containerIndexOffset() const
{
    return _containerIndexOffset;
}

OsmAnd::PointI OsmAnd::ObfAddressSectionReader::AddressReference::position31() const
{
    return _position31;
}

OsmAnd::ObfAddressSectionReader::AddressReference::AddressReference(
        AddressNameIndexDataAtomType addressType,
        QString name,
        QString nameEn,
        uint32_t dataIndexOffset,
        uint32_t containerIndexOffset,
        PointI position31)
    : _name(name)
    , _nameEn(nameEn)
    , _addressType(addressType)
    , _dataIndexOffset(dataIndexOffset)
    , _containerIndexOffset(containerIndexOffset)
    , _position31(position31)
{

}

OsmAnd::ObfAddressSectionReader::FilterBuilder::FilterBuilder(
        OsmAnd::ObfAddressSectionReader::Filter filter)
    : _filter(std::unique_ptr<Filter>(new Filter{filter}))
{

}

OsmAnd::ObfAddressSectionReader::FilterBuilder::FilterBuilder(OsmAnd::BinaryBoolFunction op)
    : _filter(std::unique_ptr<Filter>(new Filter{op}))
{

}

OsmAnd::ObfAddressSectionReader::FilterBuilder& OsmAnd::ObfAddressSectionReader::FilterBuilder::setAddressNameIndexDataAtomType(
        const Bitmask<OsmAnd::ObfAddressSectionReader::AddressNameIndexDataAtomType>& addressNameIndexDataAtomType)
{
    _filter->_addressNameIndexDataAtomTypeMask = addressNameIndexDataAtomType;
    return *this;
}

OsmAnd::ObfAddressSectionReader::FilterBuilder& OsmAnd::ObfAddressSectionReader::FilterBuilder::setAddressTypes(
        const OsmAnd::Bitmask<OsmAnd::AddressType>& addressTypeMask)
{
    _filter->_addressTypeMask = addressTypeMask;
    return *this;
}

OsmAnd::ObfAddressSectionReader::FilterBuilder& OsmAnd::ObfAddressSectionReader::FilterBuilder::setName(
        const QString& name)
{
    _filter->_name = name;
    return *this;
}

OsmAnd::ObfAddressSectionReader::FilterBuilder& OsmAnd::ObfAddressSectionReader::FilterBuilder::setBbox(
        const OsmAnd::AreaI& bbox31)
{
    _filter->_bbox31 = bbox31;
    return *this;
}

OsmAnd::ObfAddressSectionReader::FilterBuilder &OsmAnd::ObfAddressSectionReader::FilterBuilder::setObfInfoAreaBbox(
        const OsmAnd::AreaI& bbox31)
{
    _filter->_obfInfoAreaBbox31 = bbox31;
    return *this;
}

OsmAnd::ObfAddressSectionReader::FilterBuilder& OsmAnd::ObfAddressSectionReader::FilterBuilder::setAddressStreetGroupTypes(
        const OsmAnd::ObfAddressStreetGroupTypesMask& streetGroupTypes)
{
    _filter->_streetGroupTypes = streetGroupTypes;
    return *this;
}

OsmAnd::ObfAddressSectionReader::FilterBuilder& OsmAnd::ObfAddressSectionReader::FilterBuilder::setFilters(
        QVector<Filter> filters)
{
    _filter->_filters = filters;
    return *this;
}

OsmAnd::ObfAddressSectionReader::FilterBuilder& OsmAnd::ObfAddressSectionReader::FilterBuilder::setParent(
        std::shared_ptr<const ObfAddress> address)
{
    _filter->_parent = address;
    return *this;
}

OsmAnd::ObfAddressSectionReader::FilterBuilder& OsmAnd::ObfAddressSectionReader::FilterBuilder::setStringMatcherFunction(
        OsmAnd::ObfAddressSectionReader::StringMatcherFunction stringMatcher)
{
    _filter->_stringMatcher = stringMatcher;
    return *this;
}

OsmAnd::ObfAddressSectionReader::FilterBuilder& OsmAnd::ObfAddressSectionReader::FilterBuilder::setVisitor(
        OsmAnd::ObfAddressSectionReader::VisitorFunction visitor)
{
    _filter->_addressVisitor = visitor;
    return *this;
}

OsmAnd::ObfAddressSectionReader::FilterBuilder& OsmAnd::ObfAddressSectionReader::FilterBuilder::setVisitor(
        OsmAnd::ObfAddressSectionReader::AddressReferenceVisitorFunction visitor)
{
    _filter->_addressReferenceVisitor = visitor;
    return *this;
}

OsmAnd::ObfAddressSectionReader::FilterBuilder& OsmAnd::ObfAddressSectionReader::FilterBuilder::setVisitor(
        OsmAnd::ObfAddressSectionReader::StreetGroupVisitorFunction visitor)
{
    _filter->_streetGroupVisitor = visitor;
    return *this;
}

OsmAnd::ObfAddressSectionReader::FilterBuilder& OsmAnd::ObfAddressSectionReader::FilterBuilder::setVisitor(
        OsmAnd::ObfAddressSectionReader::ObfStreetGroupVisitorFunction visitor)
{
    _filter->_obfStreetGroupVisitor = visitor;
    return *this;
}

OsmAnd::ObfAddressSectionReader::FilterBuilder& OsmAnd::ObfAddressSectionReader::FilterBuilder::setVisitor(
        OsmAnd::ObfAddressSectionReader::StreetVisitorFunction visitor)
{
    _filter->_streetVisitor = visitor;
    return *this;
}

OsmAnd::ObfAddressSectionReader::FilterBuilder& OsmAnd::ObfAddressSectionReader::FilterBuilder::setVisitor(
        OsmAnd::ObfAddressSectionReader::ObfStreetVisitorFunction visitor)
{
    _filter->_obfStreetVisitor = visitor;
    return *this;
}

OsmAnd::ObfAddressSectionReader::FilterBuilder& OsmAnd::ObfAddressSectionReader::FilterBuilder::setVisitor(
        OsmAnd::ObfAddressSectionReader::BuildingVisitorFunction visitor)
{
    _filter->_buildingVisitor = visitor;
    return *this;
}

OsmAnd::ObfAddressSectionReader::FilterBuilder& OsmAnd::ObfAddressSectionReader::FilterBuilder::setVisitor(
        OsmAnd::ObfAddressSectionReader::ObfBuildingVisitorFunction visitor)
{
    _filter->_obfBuildingVisitor = visitor;
    return *this;
}

OsmAnd::ObfAddressSectionReader::FilterBuilder& OsmAnd::ObfAddressSectionReader::FilterBuilder::setVisitor(
        OsmAnd::ObfAddressSectionReader::IntersectionVisitorFunction visitor)
{
    _filter->_intersectionVisitor = visitor;
    return *this;
}

OsmAnd::ObfAddressSectionReader::FilterBuilder& OsmAnd::ObfAddressSectionReader::FilterBuilder::setVisitor(
        OsmAnd::ObfAddressSectionReader::ObfIntersectionVisitorFunction visitor)
{
    _filter->_obfIntersectionVisitor = visitor;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter OsmAnd::ObfAddressSectionReader::FilterBuilder::buildTopLevelFilters()
{
//    auto addressNameIndexDataAtomTypeMask = fullObfAddressNameIndexDataAtomTypeMask().unset(_filter->_addressNameIndexDataAtomTypeMask).unset(AddressNameIndexDataAtomType::Unknown);
    bool topLevelFiltersNeeded = false;
    AddressType lower;
    for (AddressType type : ADDRESS_TYPE_NAMES.keys())
        if (_filter->_addressTypeMask.isSet(type))
            lower = type;
    Bitmask<AddressType> addressTypeMask;
    for (AddressType type : ADDRESS_TYPE_NAMES.keys())
        if (type == lower)
            break;
        else if (!_filter->_addressTypeMask.isSet(type))
        {
            addressTypeMask.set(type);
            topLevelFiltersNeeded = true;
        }
//    auto addressTypeMask = fullAddressTypeMask().unset(_filter->_addressTypeMask);
    if (topLevelFiltersNeeded)
    {
        Filter topLevelFilter = FilterBuilder()
    //            .setAddressNameIndexDataAtomType(addressNameIndexDataAtomTypeMask)
                .setAddressTypes(addressTypeMask)
                .build();
        return FilterBuilder(OR)
                .setFilters({topLevelFilter, *_filter})
                .build();
    }
    else
        return *_filter;
}

OsmAnd::ObfAddressSectionReader::Filter OsmAnd::ObfAddressSectionReader::FilterBuilder::build()
{
    return buildTopLevelFilters();
}
