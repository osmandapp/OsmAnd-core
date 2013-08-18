#include "ObfDataInterface_P.h"
#include "ObfDataInterface.h"

OsmAnd::ObfDataInterface_P::ObfDataInterface_P( ObfDataInterface* owner_, const QList< std::shared_ptr<ObfReader> >& readers_ )
    : owner(owner_)
    , readers(readers_)
{
}

OsmAnd::ObfDataInterface_P::~ObfDataInterface_P()
{
}
