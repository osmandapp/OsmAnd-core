#include "ObfFile.h"
#include "ObfFile_P.h"

#include <QFile>

OsmAnd::ObfFile::ObfFile(const QString& filePath_)
    : _p(new ObfFile_P(this))
    , filePath(filePath_)
    , fileSize(QFile(filePath).size())
    , obfInfo(_p->_obfInfo)
{
}

OsmAnd::ObfFile::ObfFile(const QString& filePath_, const uint64_t fileSize_)
    : _p(new ObfFile_P(this))
    , filePath(filePath_)
    , fileSize(fileSize_)
    , obfInfo(_p->_obfInfo)
{
}

OsmAnd::ObfFile::~ObfFile()
{
}

bool OsmAnd::ObfFile::tryLockForReading() const
{
    return _p->tryLockForReading();
}

void OsmAnd::ObfFile::lockForReading() const
{
    _p->lockForReading();
}

void OsmAnd::ObfFile::unlockFromReading() const
{
    _p->unlockFromReading();
}

bool OsmAnd::ObfFile::tryLockForWriting() const
{
    return _p->tryLockForWriting();
}

void OsmAnd::ObfFile::lockForWriting() const
{
    _p->lockForWriting();
}

void OsmAnd::ObfFile::unlockFromWriting() const
{
    _p->unlockFromWriting();
}
