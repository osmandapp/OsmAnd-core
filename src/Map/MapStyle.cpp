#include "MapStyle.h"
#include "MapStyle_P.h"

#include <OsmAndCore/QtExtensions.h>
#include <QFileInfo>
#include <QAtomicInt>
#include <QMutex>

#include "MapStyleRule.h"
#include "QKeyValueIterator.h"
#include "Logging.h"

OsmAnd::MapStyle::MapStyle(
    /*const std::shared_ptr<const IMapStylesCollection>&*/const IMapStylesCollection* const collection_,
    const QString& name_,
    const std::shared_ptr<QIODevice>& source_)
    : _p(new MapStyle_P(this, name_, source_))
    , collection(collection_)
    , title(_p->_title)
    , name(_p->_name)
    , parentName(_p->_parentName)
{
    _p->parseMetadata();
}

OsmAnd::MapStyle::MapStyle(
    /*const std::shared_ptr<IMapStylesCollection>&*/const IMapStylesCollection* const collection_,
    const QString& name_,
    const QString& fileName_)
    : MapStyle(collection_, name_, std::shared_ptr<QIODevice>(new QFile(fileName_)))
{
}

OsmAnd::MapStyle::MapStyle(
    /*const std::shared_ptr<const IMapStylesCollection>&*/const IMapStylesCollection* const collection_,
    const QString& fileName_)
    : MapStyle(collection_, QFileInfo(fileName_).fileName().remove(QLatin1String(".render.xml")), fileName_)
{
}

OsmAnd::MapStyle::~MapStyle()
{
}

bool OsmAnd::MapStyle::isStandalone() const
{
    return _p->isStandalone();
}

bool OsmAnd::MapStyle::isMetadataLoaded() const
{
    return _p->isMetadataLoaded();
}

bool OsmAnd::MapStyle::loadMetadata()
{
    return _p->loadMetadata();
}

bool OsmAnd::MapStyle::isLoaded() const
{
    return _p->isLoaded();
}

bool OsmAnd::MapStyle::load()
{
    return _p->load();
}

bool OsmAnd::MapStyle::resolveValueDefinition(const QString& name, std::shared_ptr<const MapStyleValueDefinition>& outDefinition) const
{
    return _p->resolveValueDefinition(name, outDefinition);
}

bool OsmAnd::MapStyle::resolveAttribute(const QString& name, std::shared_ptr<const MapStyleRule>& outAttribute) const
{
    return _p->resolveAttribute(name, outAttribute);
}

void OsmAnd::MapStyle::dump(const QString& prefix /*= QString::null*/) const
{
    return _p->dump(prefix);
}

void OsmAnd::MapStyle::dump(const MapStyleRulesetType type, const QString& prefix /*= QString::null*/) const
{
    return _p->dump(type, prefix);
}

static QMutex g_OsmAnd_MapStyle_builtinValueDefinitionsMutex;
static std::shared_ptr<const OsmAnd::MapStyleBuiltinValueDefinitions> g_OsmAnd_MapStyle_builtinValueDefinitions;

std::shared_ptr<const OsmAnd::MapStyleBuiltinValueDefinitions> OsmAnd::MapStyle::getBuiltinValueDefinitions()
{
    QMutexLocker scopedLocker(&g_OsmAnd_MapStyle_builtinValueDefinitionsMutex);

    if(!static_cast<bool>(g_OsmAnd_MapStyle_builtinValueDefinitions))
        g_OsmAnd_MapStyle_builtinValueDefinitions.reset(new OsmAnd::MapStyleBuiltinValueDefinitions());

    return g_OsmAnd_MapStyle_builtinValueDefinitions;
}
