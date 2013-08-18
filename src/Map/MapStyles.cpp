#include "MapStyles.h"

#include <cassert>

#include "EmbeddedResources.h"
#include "MapStyle.h"

OsmAnd::MapStyles::MapStyles()
{
    bool ok = true;

    ok = registerEmbeddedStyle("map_styles/default.render.xml") || ok;

    assert(ok);
}

OsmAnd::MapStyles::~MapStyles()
{
}

bool OsmAnd::MapStyles::registerEmbeddedStyle( const QString& resourceName )
{
    assert(EmbeddedResources::containsResource(resourceName));

    std::shared_ptr<MapStyle> style(new MapStyle(this, resourceName));
    if(!style->parseMetadata())
        return false;
    _styles.insert(style->name, style);

    return true;
}

bool OsmAnd::MapStyles::registerStyle( const QFileInfo& file )
{
    std::shared_ptr<MapStyle> style(new MapStyle(this, file));
    if(!style->parseMetadata())
        return false;
    if(_styles.contains(style->name))
        return false;
    _styles.insert(style->name, style);

    return true;
}

bool OsmAnd::MapStyles::obtainStyle( const QString& name, std::shared_ptr<OsmAnd::MapStyle>& outStyle )
{
    auto itStyle = _styles.find(name);
    if(itStyle == _styles.end())
        return false;

    auto style = *itStyle;
    if(!style->isStandalone() && !style->areDependenciesResolved())
    {
        if(!style->resolveDependencies())
            return false;
    }

    if(style->isStandalone())
        style->registerString(QString());

    if(!style->parse())
        return false;

    if(!style->isStandalone())
        style->mergeInherited();

    outStyle = style;
    return true;
}
