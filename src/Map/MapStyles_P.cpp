#include "MapStyles_P.h"

#include <cassert>

#include "MapStyle.h"
#include "MapStyle_P.h"
#include "EmbeddedResources.h"

OsmAnd::MapStyles_P::MapStyles_P( MapStyles* owner_ )
    : owner(owner_)
{
    bool ok = true;

    ok = registerEmbeddedStyle("map/styles/default.render.xml") || ok;

    assert(ok);
}

OsmAnd::MapStyles_P::~MapStyles_P()
{

}

bool OsmAnd::MapStyles_P::registerEmbeddedStyle( const QString& resourceName )
{
    assert(EmbeddedResources::containsResource(resourceName));

    std::shared_ptr<MapStyle> style(new MapStyle(owner, resourceName, true));
    if(!style->_d->parseMetadata())
        return false;
    _styles.insert(style->name, style);

    return true;
}

bool OsmAnd::MapStyles_P::registerStyle( const QString& filePath )
{
    std::shared_ptr<MapStyle> style(new MapStyle(owner, filePath, false));
    if(!style->_d->parseMetadata())
        return false;
    if(_styles.contains(style->name))
        return false;
    _styles.insert(style->name, style);

    return true;
}

bool OsmAnd::MapStyles_P::obtainStyle( const QString& name, std::shared_ptr<const OsmAnd::MapStyle>& outStyle )
{
    auto itStyle = _styles.find(name);
    if(itStyle == _styles.end())
        return false;

    auto style = *itStyle;
    if(!style->isStandalone() && !style->areDependenciesResolved())
    {
        if(!style->_d->resolveDependencies())
            return false;
    }

    if(style->isStandalone())
        style->_d->registerString(QString());

    if(!style->_d->parse())
        return false;

    if(!style->isStandalone())
        style->_d->mergeInherited();

    outStyle = style;
    return true;
}
