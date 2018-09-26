#include "ObfFile_P.h"

OsmAnd::ObfFile_P::ObfFile_P(ObfFile* owner_, const std::shared_ptr<const ObfInfo>& obfInfo_)
    : owner(owner_)
    , _obfInfo(obfInfo_)
{
}

OsmAnd::ObfFile_P::ObfFile_P(ObfFile* owner_)
    : owner(owner_)
{
}

OsmAnd::ObfFile_P::~ObfFile_P()
{
}
