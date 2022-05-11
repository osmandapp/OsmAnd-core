#include "TransportRoute.h"
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
