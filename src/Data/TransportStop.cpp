#include "TransportStop.h"
#include "TransportStopExit.h"
#include <ICU.h>

#include <limits>

OsmAnd::TransportStop::TransportStop(const std::shared_ptr<const ObfTransportSectionInfo>& obfSection_)
    : obfSection(obfSection_)
    , id(ObfObjectId::invalidId())
{
}

OsmAnd::TransportStop::~TransportStop()
{
}

QString OsmAnd::TransportStop::getName(const QString lang, bool transliterate) const
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

void OsmAnd::TransportStop::addExit(std::shared_ptr<OsmAnd::TransportStopExit> &exit)
{
    exits.append(exit);
}

bool OsmAnd::TransportStop::isDeleted() const
{
    // A removed stop is written as a single reference resolving to -1
    return referencesToRoutes.size() == 1 && referencesToRoutes.first() == std::numeric_limits<uint32_t>::max();
}

bool OsmAnd::TransportStop::isMissingStop() const
{
    const auto missingStopName = QStringLiteral("#Missing Stop");
    return localizedName == missingStopName || enName == missingStopName;
}

bool OsmAnd::TransportStop::hasRoute(const uint64_t routeId) const
{
    return routesIds.contains(routeId);
}

bool OsmAnd::TransportStop::isRouteDeleted(const uint64_t routeId) const
{
    return deletedRoutesIds.contains(routeId);
}

bool OsmAnd::TransportStop::compareStop(const std::shared_ptr<OsmAnd::TransportStop>& thatObj)
{
    if (id == thatObj->id && location.latitude == thatObj->location.latitude && location.longitude == thatObj->location.longitude && enName == thatObj->enName && exits.size() == thatObj->exits.size())
    {
        if (exits.size() > 0) {
            for (auto exit1 : exits)
            {
                if (!exit1.get())
                    return false;
                bool contains = false;
                for (auto exit2 : thatObj->exits)
                {
                    if (exit1 == exit2) 
                    {
                        contains = true;
                        if (!exit1->compareExit(exit2))
                            return false;
                        break;
                    }
                }
                if (!contains)
                    return false;
            }
        }
    } 
    else
    {
        return false;
    }
    return true;
}
