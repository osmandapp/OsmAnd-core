#include "ObfsCollection.h"
#include "ObfsCollection_P.h"

OsmAnd::ObfsCollection::ObfsCollection()
    : _p(new ObfsCollection_P(this))
{
}

OsmAnd::ObfsCollection::~ObfsCollection()
{
}

OsmAnd::ObfsCollection::SourceOriginId OsmAnd::ObfsCollection::addDirectory(const QString& dirPath, bool recursive /*= true*/)
{
    return addDirectory(QDir(dirPath), recursive);
}

OsmAnd::ObfsCollection::SourceOriginId OsmAnd::ObfsCollection::addDirectory(const QDir& dir, bool recursive /*= true*/)
{
    return _p->addDirectory(dir, recursive);
}

OsmAnd::ObfsCollection::SourceOriginId OsmAnd::ObfsCollection::addFile(const QString& filePath)
{
    return addFile(QFileInfo(filePath));
}

OsmAnd::ObfsCollection::SourceOriginId OsmAnd::ObfsCollection::addFile(const QFileInfo& fileInfo)
{
    return _p->addFile(fileInfo);
}

bool OsmAnd::ObfsCollection::remove(const SourceOriginId entryId)
{
    return _p->remove(entryId);
}

QVector< std::shared_ptr<const OsmAnd::ObfFile> >OsmAnd::ObfsCollection::getObfFiles() const
{
    return _p->getObfFiles();
}

std::shared_ptr<OsmAnd::ObfDataInterface> OsmAnd::ObfsCollection::obtainDataInterface() const
{
    return _p->obtainDataInterface();
}
