#include "ObfFile.h"
#include "ObfFile_P.h"

OsmAnd::ObfFile::ObfFile( const QString& filePath_ )
    : _d(new ObfFile_P(this))
    , filePath(filePath_)
    , obfInfo(_d->_obfInfo)
{
}

OsmAnd::ObfFile::~ObfFile()
{
}
