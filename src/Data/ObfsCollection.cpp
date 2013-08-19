#include "ObfsCollection.h"
#include "ObfsCollection_P.h"

#include "ObfReader.h"
#include "ObfDataInterface.h"

OsmAnd::ObfsCollection::ObfsCollection( const QDir& dataDir_ )
    : _d(new ObfsCollection_P(this))
    , dataDir(dataDir_)
{
}

OsmAnd::ObfsCollection::~ObfsCollection()
{
}

std::shared_ptr<OsmAnd::ObfDataInterface> OsmAnd::ObfsCollection::obtainDataInterface() const
{
    QMutexLocker scopedLock(&_d->_sourcesMutex);

    // Refresh sources, if collection was not yet initialized
    if(!_d->_sourcesRefreshedOnce)
        _d->refreshSources();

    QList< std::shared_ptr<ObfReader> > obfReaders;
    for(auto itSource = _d->_sources.begin(); itSource != _d->_sources.end(); ++itSource)
    {
        const auto& obfFile = itSource.value();

        auto obfReader = new ObfReader(obfFile);
        obfReaders.push_back(std::shared_ptr<ObfReader>(obfReader));
    }

    return std::shared_ptr<ObfDataInterface>(new ObfDataInterface(obfReaders));
}
