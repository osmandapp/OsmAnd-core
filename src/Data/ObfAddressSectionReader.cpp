#include "ObfAddressSectionReader.h"
#include "ObfAddressSectionReader_P.h"
#include "ObfBuilding.h"
#include "ObfStreetGroup.h"

#include "ObfReader.h"

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
    QList< std::shared_ptr<const ObfStreetGroup> >* resultOut /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    ObfAddressSectionReader_P::loadStreetGroups(
        *reader->_p,
        section,
        filter,
        resultOut,
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
    const std::shared_ptr<const ObfStreet> &street,
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
        const ObfStreet &street,
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

OsmAnd::ObfAddressSectionReader::Filter::Filter(BinaryBoolFunction op)
    : _op(op)
{

}

OsmAnd::ObfAddressSectionReader::Filter& OsmAnd::ObfAddressSectionReader::Filter::setName(
        const QString &name)
{
    _name = name;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter &OsmAnd::ObfAddressSectionReader::Filter::setBbox(
        const OsmAnd::AreaI &bbox31)
{
    _bbox31 = bbox31;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter &OsmAnd::ObfAddressSectionReader::Filter::setAddressStreetGroupTypes(
        const OsmAnd::ObfAddressStreetGroupTypesMask &streetGroupTypes)
{
    _streetGroupTypes = streetGroupTypes;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter &OsmAnd::ObfAddressSectionReader::Filter::setFilters(
        QVector<Filter> filters)
{
    _filters = filters;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter &OsmAnd::ObfAddressSectionReader::Filter::setStringMatcherFunction(
        OsmAnd::ObfAddressSectionReader::StringMatcherFunction stringMatcher)
{
    _stringMatcher = stringMatcher;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter &OsmAnd::ObfAddressSectionReader::Filter::setVisitor(
        OsmAnd::ObfAddressSectionReader::VisitorFunction visitor)
{
    _addressVisitor = visitor;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter &OsmAnd::ObfAddressSectionReader::Filter::setVisitor(
        OsmAnd::ObfAddressSectionReader::StreetGroupVisitorFunction visitor)
{
    _streetGroupVisitor = visitor;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter &OsmAnd::ObfAddressSectionReader::Filter::setVisitor(
        OsmAnd::ObfAddressSectionReader::StreetVisitorFunction visitor)
{
    _streetVisitor = visitor;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter &OsmAnd::ObfAddressSectionReader::Filter::setVisitor(
        OsmAnd::ObfAddressSectionReader::BuildingVisitorFunction visitor)
{
    _buildingVisitor = visitor;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter &OsmAnd::ObfAddressSectionReader::Filter::setVisitor(
        OsmAnd::ObfAddressSectionReader::IntersectionVisitorFunction visitor)
{
    _intersectionVisitor = visitor;
    return *this;
}

uint OsmAnd::ObfAddressSectionReader::Filter::commonStartPartLength(
        const QString &name) const
{
    if (name.startsWith(_name))
        return _name.length();
    else if (_name.startsWith(name))
        return name.length();
    else
        return 0;
}

bool OsmAnd::ObfAddressSectionReader::Filter::matches(
        const QString &name,
        const QHash<QString, QString> &names) const
//        const OsmAnd::ObfAddressSectionReader::StringMatcherFunction &matcher) const
{
    if (_name.isNull())
        return true;
    bool result = _stringMatcher(name, _name);
    for (const auto& name : constOf(names))
        result = result || _stringMatcher(name, _name);
    return result;
}

bool OsmAnd::ObfAddressSectionReader::Filter::contains(
        const OsmAnd::AreaI &bbox31) const
{
//    return !_bbox.isSet() || _bbox->contains(bbox);
    return !_bbox31.isSet() || _bbox31->contains(bbox31) || bbox31.contains(*_bbox31) || _bbox31->intersects(bbox31);
}

bool OsmAnd::ObfAddressSectionReader::Filter::contains(
        const OsmAnd::PointI &point) const
{
    return !_bbox31.isSet() || _bbox31->contains(point);
}

bool OsmAnd::ObfAddressSectionReader::Filter::filter(
        const OsmAnd::Address& address,
//        const OsmAnd::ObfAddressSectionReader::StringMatcherFunction& matcher,
        bool callVisitor) const
{
    bool result = _op(
                contains(address.position31),
                matches(address.nativeName, address.localizedNames));
    if (result && callVisitor)
        _addressVisitor(address);
    return result;
}

//bool OsmAnd::ObfAddressSectionReader::Filter::match(const QString &name, const OsmAnd::AreaI &bbox) const
//{
//    return _op(matches(name), contains(bbox));
//}

bool OsmAnd::ObfAddressSectionReader::Filter::filter(
        const OsmAnd::StreetGroup &streetGroup,
//        const OsmAnd::ObfAddressSectionReader::StringMatcherFunction &matcher,
        bool callVisitor) const
{
    bool result = _op(
                filter(static_cast<const Address&>(streetGroup), false),
                hasStreetGroupType(streetGroup.type));
    if (result && callVisitor)
        _streetGroupVisitor(streetGroup);
    return result;
}

bool OsmAnd::ObfAddressSectionReader::Filter::hasType(
        const OsmAnd::AddressType type) const
{
    return _addressTypes.isSet(type);
}

bool OsmAnd::ObfAddressSectionReader::Filter::hasStreetGroupType(
        const OsmAnd::ObfAddressStreetGroupType type) const
{
    bool result = _streetGroupTypes.isSet(type);
    for (const Filter& filter : _filters)
        result = _op(result, filter.hasStreetGroupType(type));
    return result;
}
