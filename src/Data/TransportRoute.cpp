#include "TransportRoute.h"
#include "TransportStop.h"
#include <ICU.h>

OsmAnd::TransportRoute::TransportRoute()
    : id(ObfObjectId::invalidId())
    , dist(0)
{
}

OsmAnd::TransportRoute::~TransportRoute()
{
}

QString OsmAnd::TransportRoute::getName(const QString lang, bool transliterate) const
{
    QString name = QString();
    if (lang == QStringLiteral("en"))
    {
        if (!enName.isEmpty())
            name = enName;
        else if (!localizedName.isEmpty())
            name = OsmAnd::ICU::transliterateToLatin(localizedName);
    }
    else if (!localizedName.isEmpty())
    {
        if (transliterate)
            name = (!enName.isEmpty()) ? enName : OsmAnd::ICU::transliterateToLatin(localizedName);
        else
            name = localizedName;
    }
    else if (!enName.isEmpty())
    {
        name = enName;
    }
    return name;
}

bool OsmAnd::TransportRoute::compareRoute(std::shared_ptr<const OsmAnd::TransportRoute>& thatObj) const
{
    if (id == thatObj->id && enName == thatObj->enName && ref == thatObj->ref &&
        oper == thatObj->oper && type == thatObj->type && color == thatObj->color && dist == thatObj->dist &&
        forwardStops.size() == thatObj->forwardStops.size() && (forwardWays31.size() == thatObj->forwardWays31.size()))
    {
        for (int i = 0; i < forwardStops.size(); i++)
        {
            if (!forwardStops[i]->compareStop(thatObj->forwardStops[i]))
                return false;
        }
        for (int i = 0; i < forwardWays31.size(); i++) 
        {
            if (!(forwardWays31.at(i) == thatObj->forwardWays31.at(i)))
                return false;
        }
        return true;
    }
    else
    {
        return false;
    }
}
