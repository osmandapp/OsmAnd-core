#include "MapStyles_P.h"

#include <cassert>

#include "MapStyle.h"
#include "MapStyle_P.h"
#include "EmbeddedResources.h"

OsmAnd::MapStyles_P::MapStyles_P( MapStyles* owner_ )
    : owner(owner_)
    , _stylesLock(QReadWriteLock::Recursive)
{
    bool ok = true;

    // Register all embedded styles
    ok = registerEmbeddedStyle("map/styles/default.render.xml") || ok;

    assert(ok);
}

OsmAnd::MapStyles_P::~MapStyles_P()
{
}

bool OsmAnd::MapStyles_P::registerEmbeddedStyle(const QString& resourceName)
{
    QWriteLocker scopedLocker(&_stylesLock);

    assert(EmbeddedResources::containsResource(resourceName));

    std::shared_ptr<MapStyle> style(new MapStyle(owner, resourceName, true));
    if(!style->_p->parseMetadata())
        return false;

    assert(!_styles.contains(style->name));
    _styles.insert(style->name, style);
    
    return true;
}

bool OsmAnd::MapStyles_P::registerStyle( const QString& filePath )
{
    QWriteLocker scopedLocker(&_stylesLock);

    std::shared_ptr<MapStyle> style(new MapStyle(owner, filePath, false));
    if(!style->_p->parseMetadata())
        return false;

    if(_styles.contains(style->name))
        return false;
    _styles.insert(style->name, style);

    return true;
}

bool OsmAnd::MapStyles_P::obtainStyle(const QString& name, std::shared_ptr<const OsmAnd::MapStyle>& outStyle) const
{
    QReadLocker scopedLocker(&_stylesLock);

    const auto citStyle = _styles.constFind(name);
    if(citStyle == _styles.cend())
        return false;
    const auto& style = *citStyle;

    if(!style->_p->prepareIfNeeded())
        return false;

    outStyle = style;
    return true;
}
