#include "MapStylesCollection.h"
#include "MapStylesCollection_P.h"

OsmAnd::MapStylesCollection::MapStylesCollection()
    : _p(new MapStylesCollection_P(this))
{
}

OsmAnd::MapStylesCollection::~MapStylesCollection()
{
}

bool OsmAnd::MapStylesCollection::addStyleFromFile(const QString& filePath, const bool doNotReplace /*= false*/)
{
    return _p->addStyleFromFile(filePath, doNotReplace);
}

QList< std::shared_ptr<const OsmAnd::UnresolvedMapStyle> > OsmAnd::MapStylesCollection::getCollection() const
{
    return _p->getCollection();
}

std::shared_ptr<const OsmAnd::UnresolvedMapStyle> OsmAnd::MapStylesCollection::getStyleByName(const QString& name) const
{
    return _p->getStyleByName(name);
}

std::shared_ptr<const OsmAnd::ResolvedMapStyle> OsmAnd::MapStylesCollection::getResolvedStyleByName(const QString& name) const
{
    return _p->getResolvedStyleByName(name);
}
