#include "MapStyles_P.h"

#include <cassert>

#include "MapStyle.h"
#include "MapStyle_P.h"
#include "EmbeddedResources.h"

OsmAnd::MapStyles_P::MapStyles_P( MapStyles* owner_ )
    : owner(owner_)
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
    assert(EmbeddedResources::containsResource(resourceName));

    std::shared_ptr<MapStyle> style(new MapStyle(owner, resourceName, true));
    if(!style->_d->parseMetadata())
        return false;

    {
        QWriteLocker scopedLocker(&_stylesLock);

        assert(!_styles.contains(style->name));
        _styles.insert(style->name, style);
    }
    
    return true;
}

bool OsmAnd::MapStyles_P::registerStyle( const QString& filePath )
{
    std::shared_ptr<MapStyle> style(new MapStyle(owner, filePath, false));
    if(!style->_d->parseMetadata())
        return false;

    {
        QWriteLocker scopedLocker(&_stylesLock);

        if(_styles.contains(style->name))
            return false;
        _styles.insert(style->name, style);
    }

    return true;
}

bool OsmAnd::MapStyles_P::obtainStyle(const QString& name, std::shared_ptr<const OsmAnd::MapStyle>& outStyle) const
{
    // Obtain style by name
    QHash< QString, std::shared_ptr<MapStyle> >::const_iterator citStyle;
    {
        QReadLocker scopedLocker(&_stylesLock);

        citStyle = _styles.constFind(name);
        if(citStyle == _styles.cend())
            return false;
    }

    const auto style = *citStyle;
    if(!style->_d->prepareIfNeeded())
        return false;

    outStyle = style;
    return true;
}
