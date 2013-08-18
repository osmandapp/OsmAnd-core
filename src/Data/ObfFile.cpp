#include "ObfFile.h"
#include "ObfFile_P.h"

OsmAnd::ObfFile::ObfFile( const QFileInfo& fileInfo_ )
    : _d(new ObfFile_P(this))
    , fileInfo(fileInfo_)
    , obfInfo(_d->_obfInfo)
{
}

OsmAnd::ObfFile::ObfFile( const QString& filePath )
    : _d(new ObfFile_P(this))
    , fileInfo(filePath)
    , obfInfo(_d->_obfInfo)
{
}

OsmAnd::ObfFile::~ObfFile()
{
}
