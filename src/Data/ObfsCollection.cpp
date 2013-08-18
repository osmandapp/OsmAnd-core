#include "ObfsCollection.h"
#include "ObfsCollection_P.h"

#include "ObfReader.h"
#include "ObfDataInterface.h"

//const QString OsmAnd::ObfsCollection::indexFileName = QString::fromLatin1("obf.index");

OsmAnd::ObfsCollection::ObfsCollection( const QDir& dataPath_/*, const QDir& indexPath_ *//*= QDir::current()*/ )
    : _d(new ObfsCollection_P(this))
    , dataPath(dataPath_)
    //, indexPath(indexPath_)
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
        const auto& obfReader = itSource.value();

        //auto obfReaderClone = new ObfReader(*obfReader);
        //obfReaders.push_back(std::shared_ptr<ObfReader>(obfReaderClone));
    }

    return std::shared_ptr<ObfDataInterface>(new ObfDataInterface(obfReaders));
}
