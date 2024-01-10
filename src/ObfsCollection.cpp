#include "ObfsCollection.h"
#include "ObfsCollection_P.h"

OsmAnd::ObfsCollection::ObfsCollection()
    : _p(new ObfsCollection_P(this))
{
}

OsmAnd::ObfsCollection::~ObfsCollection()
{
}

QList<OsmAnd::ObfsCollection::SourceOriginId> OsmAnd::ObfsCollection::getSourceOriginIds() const
{
    return _p->getSourceOriginIds();
}

bool OsmAnd::ObfsCollection::hasDirectory(const QString& dirPath)
{
    return _p->hasDirectory(QDir(dirPath));
}

void OsmAnd::ObfsCollection::removeDirectory(const QString& dirPath)
{
    return _p->removeDirectory(QDir(dirPath));
}

OsmAnd::ObfsCollection::SourceOriginId OsmAnd::ObfsCollection::addDirectory(
    const QString& dirPath,
    const QString& filterByDirPath /*= QString()*/,
    bool recursive /*= true*/)
{
    const auto dir = filterByDirPath.isEmpty() ? QDir() : QDir(filterByDirPath);
    const auto filterBy = filterByDirPath.isEmpty() ? nullptr : &dir;
    return addDirectory(QDir(dirPath), filterBy, recursive);
}

OsmAnd::ObfsCollection::SourceOriginId OsmAnd::ObfsCollection::addDirectory(
    const QDir& dir,
    const QDir* const filterBy /*= nullptr*/,
    bool recursive /*= true*/)
{
    return _p->addDirectory(dir, filterBy, recursive);
}

OsmAnd::ObfsCollection::SourceOriginId OsmAnd::ObfsCollection::addFile(const QString& filePath)
{
    return addFile(QFileInfo(filePath));
}

OsmAnd::ObfsCollection::SourceOriginId OsmAnd::ObfsCollection::addFile(const QFileInfo& fileInfo)
{
    return _p->addFile(fileInfo);
}

void OsmAnd::ObfsCollection::setIndexCacheFile(const QString& filePath)
{
    _p->setIndexCacheFile(QFileInfo(filePath));
}

void OsmAnd::ObfsCollection::setIndexCacheFile(const QFileInfo& fileInfo)
{
    _p->setIndexCacheFile(fileInfo);
}

bool OsmAnd::ObfsCollection::remove(const SourceOriginId entryId)
{
    return _p->remove(entryId);
}

QList< std::shared_ptr<const OsmAnd::ObfFile> >OsmAnd::ObfsCollection::getObfFiles() const
{
    return _p->getObfFiles();
}

std::shared_ptr<OsmAnd::ObfDataInterface> OsmAnd::ObfsCollection::obtainDataInterface(
    const std::shared_ptr<const ObfFile> obfFile) const
{
    return _p->obtainDataInterface(obfFile);
}

std::shared_ptr<OsmAnd::ObfDataInterface> OsmAnd::ObfsCollection::obtainDataInterface(
    const QList< std::shared_ptr<const ResourcesManager::LocalResource> > localResources) const
{
    return _p->obtainDataInterface(localResources);
}

std::shared_ptr<OsmAnd::ObfDataInterface> OsmAnd::ObfsCollection::obtainDataInterface(
    const AreaI* const pBbox31 /*= nullptr*/,
    const ZoomLevel minZoomLevel /*= MinZoomLevel*/,
    const ZoomLevel maxZoomLevel /*= MaxZoomLevel*/,
    const ObfDataTypesMask desiredDataTypes /*= fullObfDataTypesMask()*/) const
{
    return _p->obtainDataInterface(pBbox31, minZoomLevel, maxZoomLevel, desiredDataTypes);
}
