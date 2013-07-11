#include "RasterizationStyles.h"

#include <cassert>

#include "EmbeddedResources.h"
#include "RasterizationStyle.h"

OsmAnd::RasterizationStyles::RasterizationStyles()
{
    bool ok;

    ok = registerEmbeddedStyle("default.render.xml");
    assert(ok);
}

OsmAnd::RasterizationStyles::~RasterizationStyles()
{
}

bool OsmAnd::RasterizationStyles::registerEmbeddedStyle( const QString& resourceName )
{
    assert(EmbeddedResources::containsResource(resourceName));

    std::shared_ptr<RasterizationStyle> style(new RasterizationStyle(this, resourceName));
    if(!style->parseMetadata())
        return false;
    _styles.insert(style->name, style);

    return true;
}

bool OsmAnd::RasterizationStyles::registerStyle( const QFile& file )
{
    std::shared_ptr<RasterizationStyle> style(new RasterizationStyle(this, file));
    if(!style->parseMetadata())
        return false;
    if(_styles.contains(style->name))
        return false;
    _styles.insert(style->name, style);

    return true;
}

bool OsmAnd::RasterizationStyles::obtainStyle( const QString& name, std::shared_ptr<OsmAnd::RasterizationStyle>& outStyle )
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
