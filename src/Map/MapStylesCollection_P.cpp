#include "MapStylesCollection_P.h"
#include "MapStylesCollection.h"

#include <QBuffer>
#include <QFileInfo>

#include "MapStyle.h"
#include "MapStyle_P.h"
#include "EmbeddedResources.h"

OsmAnd::MapStylesCollection_P::MapStylesCollection_P(MapStylesCollection* owner_)
    : owner(owner_)
    , _stylesLock(QReadWriteLock::Recursive)
{
    bool ok = true;

    // Register all embedded styles
    ok = registerEmbeddedStyle(QLatin1String("map/styles/default.render.xml")) || ok;

    assert(ok);
}

OsmAnd::MapStylesCollection_P::~MapStylesCollection_P()
{
}

bool OsmAnd::MapStylesCollection_P::registerEmbeddedStyle(const QString& resourceName)
{
    QWriteLocker scopedLocker(&_stylesLock);

    assert(EmbeddedResources::containsResource(resourceName));

    auto styleContent = EmbeddedResources::decompressResource(resourceName);
    std::shared_ptr<MapStyle> style(new MapStyle(
        owner.get(),
        QFileInfo(resourceName).fileName(),
        std::shared_ptr<QIODevice>(new QBuffer(&styleContent))));
    if(!style->loadMetadata() || !style->loadStyle())
        return false;

    assert(!_styles.contains(style->name));
    _styles.insert(style->name, style);

    return true;
}

bool OsmAnd::MapStylesCollection_P::registerStyle(const QString& filePath)
{
    QWriteLocker scopedLocker(&_stylesLock);

    std::shared_ptr<MapStyle> style(new MapStyle(owner.get(), filePath));
    if(!style->loadMetadata())
        return false;

    if(_styles.contains(style->name))
        return false;
    _styles.insert(style->name, style);

    return true;
}

bool OsmAnd::MapStylesCollection_P::obtainStyle(const QString& name, std::shared_ptr<const OsmAnd::MapStyle>& outStyle) const
{
    QReadLocker scopedLocker(&_stylesLock);

    const auto citStyle = _styles.constFind(name);
    if(citStyle == _styles.cend())
        return false;
    const auto& style = *citStyle;

    if(!style->loadStyle())
        return false;

    outStyle = style;
    return true;
}
