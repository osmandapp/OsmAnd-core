#include "ObfAddressSectionReader.h"
#include "ObfAddressSectionReader_P.h"

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
    QList< std::shared_ptr<const StreetGroup> >* resultOut /*= nullptr*/,
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
        const std::shared_ptr<const StreetGroup>& streetGroup,
        const Filter& filter,
        const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    ObfAddressSectionReader_P::loadStreetsFromGroup(
        *reader->_p,
        streetGroup,
        filter,
        queryController);
}

void OsmAnd::ObfAddressSectionReader::loadBuildingsFromStreet(const std::shared_ptr<const ObfReader>& reader,
    const std::shared_ptr<const ObfStreet> &street,
    const Filter& filter,
    QList< std::shared_ptr<const Building>>* resultOut /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    ObfAddressSectionReader_P::loadBuildingsFromStreet(
        *reader->_p,
        street,
        filter,
        resultOut,
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

OsmAnd::ObfAddressSectionReader::Filter::Filter(std::function<bool (bool, bool)> op)
    : _op(op)
{

}

OsmAnd::ObfAddressSectionReader::Filter& OsmAnd::ObfAddressSectionReader::Filter::setName(const QString &name)
{
    _name = name;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter &OsmAnd::ObfAddressSectionReader::Filter::setBbox(const OsmAnd::AreaI &bbox31)
{
    _bbox31 = bbox31;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter &OsmAnd::ObfAddressSectionReader::Filter::setAddressStreetGroupTypes(const OsmAnd::ObfAddressStreetGroupTypesMask &streetGroupTypes)
{
    _streetGroupTypes = streetGroupTypes;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter &OsmAnd::ObfAddressSectionReader::Filter::setFilters(QVector<const Filter&> filters)
{
    _filters = filters;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter &OsmAnd::ObfAddressSectionReader::Filter::setVisitor(const OsmAnd::ObfAddressSectionReader::VisitorFunction &visitor)
{
    _addressVisitor = visitor;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter &OsmAnd::ObfAddressSectionReader::Filter::setVisitor(const OsmAnd::ObfAddressSectionReader::StreetGroupVisitorFunction &visitor)
{
    _streetGroupVisitor = visitor;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter &OsmAnd::ObfAddressSectionReader::Filter::setVisitor(const OsmAnd::ObfAddressSectionReader::StreetVisitorFunction &visitor)
{
    _streetVisitor = visitor;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter &OsmAnd::ObfAddressSectionReader::Filter::setVisitor(const OsmAnd::ObfAddressSectionReader::BuildingVisitorFunction &visitor)
{
    _buildingVisitor = visitor;
    return *this;
}

OsmAnd::ObfAddressSectionReader::Filter &OsmAnd::ObfAddressSectionReader::Filter::setVisitor(const OsmAnd::ObfAddressSectionReader::IntersectionVisitorFunction &visitor)
{
    _intersectionVisitor = visitor;
    return *this;
}

bool OsmAnd::ObfAddressSectionReader::Filter::matches(const QString &name, const OsmAnd::ObfAddressSectionReader::StringMatcherFunction &matcher) const
{
    return matcher(_name, name);
}

bool OsmAnd::ObfAddressSectionReader::Filter::matches(
        const QString &name,
        QHash<QString, QString> &names,
        const OsmAnd::ObfAddressSectionReader::StringMatcherFunction &matcher) const
{
    if (_name.isNull())
        return true;

    bool result = false;
    result = result || matcher(name, _name);
    for (const auto& name : constOf(names))
    {
        result = result || matcher(name, _name);

        if (result)
            break;
    }

    return result;
}

bool OsmAnd::ObfAddressSectionReader::Filter::contains(const OsmAnd::AreaI &bbox31) const
{
//    return !_bbox.isSet() || _bbox->contains(bbox);
    return !_bbox31.isSet() || _bbox31->contains(bbox31) || bbox31.contains(*_bbox31) || _bbox31->intersects(bbox31);
}

bool OsmAnd::ObfAddressSectionReader::Filter::contains(const OsmAnd::PointI &point) const
{
    return !_bbox31.isSet() || _bbox31->contains(point);
}

//bool OsmAnd::ObfAddressSectionReader::Filter::match(const QString &name, const OsmAnd::AreaI &bbox) const
//{
//    return _op(matches(name), contains(bbox));
//}

bool OsmAnd::ObfAddressSectionReader::Filter::filter(
        const OsmAnd::StreetGroup &streetGroup,
        const OsmAnd::ObfAddressSectionReader::StringMatcherFunction &matcher) const
{
    bool result = true;
    if (result)
        _streetGroupVisitor(streetGroup);
    return result;
}

bool OsmAnd::ObfAddressSectionReader::Filter::match(
        const OsmAnd::Street &street,
        const StringMatcherFunction &matcher) const
{
    return _op(contains(street.position31()), matches(street.nativeName, matcher));
}

bool OsmAnd::ObfAddressSectionReader::Filter::hasType(const OsmAnd::AddressType type) const
{
    return _addressTypes.isSet(type);
}

bool OsmAnd::ObfAddressSectionReader::Filter::hasStreetGroupType(const OsmAnd::ObfAddressStreetGroupType type) const
{
    bool result = _streetGroupTypes.isSet(type);
    for (const Filter& filter : _filters)
        result = _op(result, filter.hasStreetGroupType(type));
    return result;
}
