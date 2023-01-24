#include "TransportStop.h"
#include "TransportStopExit.h"
#include <ICU.h>

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
