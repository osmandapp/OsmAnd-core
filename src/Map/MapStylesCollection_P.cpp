#include "MapStylesCollection_P.h"
#include "MapStylesCollection.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QBuffer>
#include <QFileInfo>
#include "restore_internal_warnings.h"
#include "QtCommon.h"

#include "UnresolvedMapStyle.h"
#include "ResolvedMapStyle.h"
#include "CoreResourcesEmbeddedBundle.h"
#include "QKeyValueIterator.h"
#include "Logging.h"

OsmAnd::MapStylesCollection_P::MapStylesCollection_P(MapStylesCollection* owner_)
    : _stylesLock(QReadWriteLock::Recursive)
    , owner(owner_)
{
    bool ok = true;
    ok = ok && addStyleFromCoreResource(QLatin1String("map/styles/default.render.xml"));
    assert(ok);
}

OsmAnd::MapStylesCollection_P::~MapStylesCollection_P()
{
}

std::shared_ptr<OsmAnd::UnresolvedMapStyle> OsmAnd::MapStylesCollection_P::getEditableStyleByName_noSync(
    const QString& name) const
{
    const auto styleName = getFullyQualifiedStyleName(name);

    const auto citStyle = _styles.constFind(styleName);
    if (citStyle == _styles.cend())
        return nullptr;
    return *citStyle;
}

bool OsmAnd::MapStylesCollection_P::addStyleFromCoreResource(const QString& resourceName)
{
    QWriteLocker scopedLocker(&_stylesLock);

    assert(getCoreResourcesProvider()->containsResource(resourceName));
    const auto styleContent = getCoreResourcesProvider()->getResource(resourceName);
    const std::shared_ptr<QBuffer> styleContentBuffer(new QBuffer());
    styleContentBuffer->setData(styleContent);

    std::shared_ptr<UnresolvedMapStyle> style(new UnresolvedMapStyle(
        styleContentBuffer,
        QFileInfo(resourceName).fileName()));
    if (!style->loadMetadata())
        return false;

    auto styleName = style->name.toLower();
    assert(styleName.endsWith(QLatin1String(".render.xml")));
    assert(!_styles.contains(styleName));
    _styles.insert(styleName, style);

    return true;
}

bool OsmAnd::MapStylesCollection_P::addStyleFromFile(const QString& filePath, const bool doNotReplace)
{
    QWriteLocker scopedLocker(&_stylesLock);

    std::shared_ptr<UnresolvedMapStyle> style(new UnresolvedMapStyle(filePath));
    if (!style->loadMetadata())
        return false;

    const auto styleName = getFullyQualifiedStyleName(style->name);
    if (doNotReplace && _styles.contains(styleName))
        return false;
    _styles.insert(styleName, style);

    return true;
}

bool OsmAnd::MapStylesCollection_P::addStyleFromByteArray(const QByteArray& data, const QString& name, const bool doNotReplace)
{
    QWriteLocker scopedLocker(&_stylesLock);

    const std::shared_ptr<QBuffer> styleContentBuffer(new QBuffer());
    styleContentBuffer->setData(data);
    std::shared_ptr<UnresolvedMapStyle> style(new UnresolvedMapStyle(styleContentBuffer, name));
    if (!style->loadMetadata())
        return false;

    const auto styleName = getFullyQualifiedStyleName(name);
    if (doNotReplace && _styles.contains(styleName))
        return false;
    _styles.insert(styleName, style);

    return true;
}

QList< std::shared_ptr<const OsmAnd::UnresolvedMapStyle> > OsmAnd::MapStylesCollection_P::getCollection() const
{
    QReadLocker scopedLocker(&_stylesLock);

    return copyAs< QList< std::shared_ptr<const UnresolvedMapStyle> > >(_styles.values());
}

std::shared_ptr<const OsmAnd::UnresolvedMapStyle> OsmAnd::MapStylesCollection_P::getStyleByName(const QString& name) const
{
    QReadLocker scopedLocker(&_stylesLock);

    const auto styleName = getFullyQualifiedStyleName(name);

    const auto citStyle = _styles.constFind(styleName);
    if (citStyle == _styles.cend())
        return nullptr;
    return *citStyle;
}

std::shared_ptr<const OsmAnd::ResolvedMapStyle> OsmAnd::MapStylesCollection_P::getResolvedStyleByName(const QString& name) const
{
    QMutexLocker scopedLocker(&_resolvedStylesLock);

    const auto styleName = getFullyQualifiedStyleName(name);

    // Check if such style was already resolved
    auto& resolvedStyle = _resolvedStyles[styleName];
    if (resolvedStyle)
        return resolvedStyle;

    // Get style inheritance chain
    QList< std::shared_ptr<UnresolvedMapStyle> > stylesChain;
    {
        QReadLocker scopedLocker(&_stylesLock);

        auto style = getEditableStyleByName_noSync(name);
        while (style)
        {
            stylesChain.push_back(style);

            // In case this is root-style, do nothing
            if (style->isStandalone())
                break;

            // Otherwise, obtain next parent
            const auto parentStyle = getEditableStyleByName_noSync(style->parentName);
            if (!parentStyle)
            {
                LogPrintf(LogSeverityLevel::Error,
                    "Failed to resolve style '%s': parent-in-chain '%s' was not found",
                    qPrintable(name),
                    qPrintable(style->parentName));
                return nullptr;
            }
            style = parentStyle;
        }
    }

    // From top-most parent to style, load it
    auto itStyle = iteratorOf(stylesChain);
    itStyle.toBack();
    while (itStyle.hasPrevious())
    {
        const auto& style = itStyle.previous();

        if (!style->load())
        {
            LogPrintf(LogSeverityLevel::Error,
                "Failed to resolve style '%s': parent-in-chain '%s' failed to load",
                qPrintable(name),
                qPrintable(style->name));
            return nullptr;
        }
    }

    return ResolvedMapStyle::resolveMapStylesChain(copyAs< QList< std::shared_ptr<const UnresolvedMapStyle> > >(stylesChain));;
}

QString OsmAnd::MapStylesCollection_P::getFullyQualifiedStyleName(const QString& name)
{
    auto styleName = name.toLower();
    if (!styleName.endsWith(QLatin1String(".render.xml")))
        styleName.append(QLatin1String(".render.xml"));
    return styleName;
}
