#include "IFavoriteLocation.h"

OsmAnd::IFavoriteLocation::IFavoriteLocation()
{
}

OsmAnd::IFavoriteLocation::~IFavoriteLocation()
{
}

uint32_t OsmAnd::IFavoriteLocation::hash() const
{
    return qHash(getPosition31().x) +
            qHash(getPosition31().y) +
            qHash(getTitle()) +
            qHash(getGroup()) +
            qHash(getDescription()) +
            qHash(getColor().toString()) +
            (isHidden() ? 0 : 1);
}

bool OsmAnd::IFavoriteLocation::isEqual(IFavoriteLocation* favoriteLocation) const
{
    if (favoriteLocation == NULL)
        return false;
    
    return getPosition31() == favoriteLocation->getPosition31() &&
            getTitle() == favoriteLocation->getTitle() &&
            getDescription() == favoriteLocation->getDescription() &&
            getGroup() == favoriteLocation->getGroup() &&
            getColor() == favoriteLocation->getColor() &&
            isHidden() == favoriteLocation->isHidden();
}
