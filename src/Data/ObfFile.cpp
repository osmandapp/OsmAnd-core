#include "ObfFile.h"
#include "ObfFile_P.h"

OsmAnd::ObfFile::ObfFile(const QString& filePath_, const uint64_t fileSize_)
    : _p(new ObfFile_P(this))
    , filePath(filePath_)
    , fileSize(fileSize_)
    , obfInfo(_p->_obfInfo)
    , isLockedForRemoval(_p->_isLockedForRemoval)
{
}

OsmAnd::ObfFile::~ObfFile()
{
}

void OsmAnd::ObfFile::lockForRemoval() const
{
    _p->lockForRemoval();
}
