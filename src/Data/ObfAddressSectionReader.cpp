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

QString OsmAnd::ObfAddressSectionReader::Filter::name() const
{
    return _name;
}

std::shared_ptr<const OsmAnd::ObfAddress> OsmAnd::ObfAddressSectionReader::Filter::parent() const
{
    return _parent;
}

OsmAnd::Nullable<OsmAnd::AreaI> OsmAnd::ObfAddressSectionReader::Filter::obfInfoAreaBbox31() const
{
    return _obfInfoAreaBbox31;
}

OsmAnd::ObfAddressSectionReader::Filter::Filter(BinaryBoolFunction op)
    : _op(op)
{
    
}

OsmAnd::ObfAddressSectionReader::Filter& OsmAnd::ObfAddressSectionReader::Filter::setName(
        const QString& name)
{
    _name = name;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter& OsmAnd::ObfAddressSectionReader::Filter::setBbox(
        const OsmAnd::AreaI& bbox31)
{
    _bbox31 = bbox31;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter &OsmAnd::ObfAddressSectionReader::Filter::setObfInfoAreaBbox(
        const OsmAnd::AreaI& bbox31)
{
    _obfInfoAreaBbox31 = bbox31;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter& OsmAnd::ObfAddressSectionReader::Filter::setAddressStreetGroupTypes(
        const OsmAnd::ObfAddressStreetGroupTypesMask& streetGroupTypes)
{
    _streetGroupTypes = streetGroupTypes;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter& OsmAnd::ObfAddressSectionReader::Filter::setFilters(
        QVector<Filter> filters)
{
    _filters = filters;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter& OsmAnd::ObfAddressSectionReader::Filter::setParent(
        std::shared_ptr<const ObfAddress> address)
{
    _parent = address;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter& OsmAnd::ObfAddressSectionReader::Filter::setStringMatcherFunction(
        OsmAnd::ObfAddressSectionReader::StringMatcherFunction stringMatcher)
{
    _stringMatcher = stringMatcher;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter& OsmAnd::ObfAddressSectionReader::Filter::setVisitor(
        OsmAnd::ObfAddressSectionReader::VisitorFunction visitor)
{
    _addressVisitor = visitor;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter& OsmAnd::ObfAddressSectionReader::Filter::setVisitor(
        OsmAnd::ObfAddressSectionReader::AddressReferenceVisitorFunction visitor)
{
    _addressReferenceVisitor = visitor;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter& OsmAnd::ObfAddressSectionReader::Filter::setVisitor(
        OsmAnd::ObfAddressSectionReader::StreetGroupVisitorFunction visitor)
{
    _streetGroupVisitor = visitor;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter& OsmAnd::ObfAddressSectionReader::Filter::setVisitor(
        OsmAnd::ObfAddressSectionReader::ObfStreetGroupVisitorFunction visitor)
{
    _obfStreetGroupVisitor = visitor;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter& OsmAnd::ObfAddressSectionReader::Filter::setVisitor(
        OsmAnd::ObfAddressSectionReader::StreetVisitorFunction visitor)
{
    _streetVisitor = visitor;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter& OsmAnd::ObfAddressSectionReader::Filter::setVisitor(
        OsmAnd::ObfAddressSectionReader::ObfStreetVisitorFunction visitor)
{
    _obfStreetVisitor = visitor;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter& OsmAnd::ObfAddressSectionReader::Filter::setVisitor(
        OsmAnd::ObfAddressSectionReader::BuildingVisitorFunction visitor)
{
    _buildingVisitor = visitor;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter& OsmAnd::ObfAddressSectionReader::Filter::setVisitor(
        OsmAnd::ObfAddressSectionReader::ObfBuildingVisitorFunction visitor)
{
    _obfBuildingVisitor = visitor;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter& OsmAnd::ObfAddressSectionReader::Filter::setVisitor(
        OsmAnd::ObfAddressSectionReader::IntersectionVisitorFunction visitor)
{
    _intersectionVisitor = visitor;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter& OsmAnd::ObfAddressSectionReader::Filter::setVisitor(
        OsmAnd::ObfAddressSectionReader::ObfIntersectionVisitorFunction visitor)
{
    _obfIntersectionVisitor = visitor;
    return *this;
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
    bool result= _op(matches(address.position31()),
                     matches(address.nativeName(), address.localizedNames()));
    for (const Filter& filter : _filters)
        result = _op(result, filter.matches(address));
    return result;
}

bool OsmAnd::ObfAddressSectionReader::Filter::matches(
        const OsmAnd::StreetGroup& streetGroup) const
{
    bool result = _op(matches(streetGroup.type),
               matches(static_cast<const Address&>(streetGroup)));
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
    bool result = _op(matches(static_cast<const Address&>(building)) || matches(static_cast<const Address&>(building.interpolation())),
                      matches(building.postcode()));
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
    bool result = _addressTypes.isSet(type);
    for (const Filter& filter : _filters)
        result = _op(result, filter.matches(type));
    return result;
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
