#include "ObfsCollection.h"
#include "ObfsCollection_P.h"

#include "ObfReader.h"
#include "ObfDataInterface.h"
#include "Logging.h"

OsmAnd::ObfsCollection::ObfsCollection()
    : _d(new ObfsCollection_P(this))
{
}

OsmAnd::ObfsCollection::~ObfsCollection()
{
}

OsmAnd::ObfsCollection::EntryId OsmAnd::ObfsCollection::registerDirectory(const QString& dirPath, bool recursive /*= true*/)
{
    return registerDirectory(QDir(dirPath), recursive);
}

OsmAnd::ObfsCollection::EntryId OsmAnd::ObfsCollection::registerDirectory(const QDir& dir, bool recursive /*= true*/)
{
    return _d->registerDirectory(dir, recursive);
}

OsmAnd::ObfsCollection::EntryId OsmAnd::ObfsCollection::registerExplicitFile(const QString& filePath)
{
    return registerExplicitFile(QFileInfo(filePath));
}

OsmAnd::ObfsCollection::EntryId OsmAnd::ObfsCollection::registerExplicitFile(const QFileInfo& fileInfo)
{
    return _d->registerExplicitFile(fileInfo);
}

bool OsmAnd::ObfsCollection::unregister(const EntryId entryId)
{
    return _d->unregister(entryId);
}

void OsmAnd::ObfsCollection::notifyFileSystemChange() const
{
    _d->notifyFileSystemChange();
}

QStringList OsmAnd::ObfsCollection::getCollectedSources() const
{
    return _d->getCollectedSources();
}

void OsmAnd::ObfsCollection::registerCollectedSourcesUpdateObserver(void* tag, const CollectedSourcesUpdateObserverSignature observer) const
{
    _d->registerCollectedSourcesUpdateObserver(tag, observer);
}

void OsmAnd::ObfsCollection::unregisterCollectedSourcesUpdateObserver(void* tag) const
{
    _d->unregisterCollectedSourcesUpdateObserver(tag);
}

void OsmAnd::ObfsCollection::setSourcesSetModifier(const SourcesSetModifierSignature modifier)
{
    _d->setSourcesSetModifier(modifier);
}

std::shared_ptr<OsmAnd::ObfDataInterface> OsmAnd::ObfsCollection::obtainDataInterface() const
{
    return _d->obtainDataInterface();
}
