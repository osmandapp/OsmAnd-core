#include "RenderStyles.h"

#include <cassert>

#include "EmbeddedResources.h"

OsmAnd::RenderStyles::RenderStyles()
{
    bool ok;

    ok = registerEmbeddedStyle("default.render.xml");
    assert(ok);
}

OsmAnd::RenderStyles::~RenderStyles()
{
}

bool OsmAnd::RenderStyles::registerEmbeddedStyle( const QString& resourceName )
{
    assert(EmbeddedResources::containsResource(resourceName));

    std::shared_ptr<RenderStyle> style(new RenderStyle(this, resourceName));
    if(!style->parseMetadata())
        return false;
    _styles.insert(style->name, style);

    return true;
}

bool OsmAnd::RenderStyles::registerStyle( const QFile& file )
{
    std::shared_ptr<RenderStyle> style(new RenderStyle(this, file));
    if(!style->parseMetadata())
        return false;
    _styles.insert(style->name, style);

    return true;
}

bool OsmAnd::RenderStyles::obtainCompleteStyle( const QString& name, std::shared_ptr<OsmAnd::RenderStyle>& outStyle )
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

    outStyle = style;
    return true;
}
