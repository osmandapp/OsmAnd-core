#include "MapStylesCollection_P.h"
#include "MapStylesCollection.h"

#include <QBuffer>
#include <QFileInfo>

#include "MapStyle.h"
#include "MapStyle_P.h"
#include "EmbeddedResources.h"
#include "QKeyValueIterator.h"

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
    if (!style->loadMetadata() || !style->load())
        return false;

    assert(!_styles.contains(style->name));
    _styles.insert(style->name, style);
    _order.push_back(style);

    return true;
}

bool OsmAnd::MapStylesCollection_P::registerStyle(const QString& filePath)
{
    QWriteLocker scopedLocker(&_stylesLock);

    std::shared_ptr<MapStyle> style(new MapStyle(owner.get(), filePath));
    if (!style->loadMetadata())
        return false;

    if (_styles.contains(style->name))
        return false;
    _styles.insert(style->name, style);
    _order.push_back(style);

    return true;
}

QList< std::shared_ptr<const OsmAnd::MapStyle> > OsmAnd::MapStylesCollection_P::getCollection() const
{
    QReadLocker scopedLocker(&_stylesLock);

    QList< std::shared_ptr<const MapStyle> > result;
    for(const auto& mapStyle : constOf(_order))
        result.push_back(mapStyle);

    return result;
}

std::shared_ptr<const OsmAnd::MapStyle> OsmAnd::MapStylesCollection_P::getAsIsStyle(const QString& name) const
{
    QReadLocker scopedLocker(&_stylesLock);

    const auto citStyle = _styles.constFind(name);
    if (citStyle == _styles.cend())
        return nullptr;
    return *citStyle;
}

bool OsmAnd::MapStylesCollection_P::obtainBakedStyle(const QString& styleName_, std::shared_ptr<const MapStyle>& outStyle) const
{
    QReadLocker scopedLocker(&_stylesLock);

    auto styleName = styleName_.toLower();
    if (!styleName.endsWith(QLatin1String(".render.xml")))
        styleName.append(QLatin1String(".render.xml"));

    const auto citStyle = _styles.constFind(styleName);
    if (citStyle == _styles.cend())
        return false;
    const auto& style = *citStyle;

    if (!style->load())
        return false;

    outStyle = style;
    return true;
}
