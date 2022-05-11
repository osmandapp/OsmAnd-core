#include "Address.h"
#include <ICU.h>

OsmAnd::Address::Address(const std::shared_ptr<const ObfAddressSectionInfo>& obfSection_, const AddressType addressType_)
    : obfSection(obfSection_)
    , addressType(addressType_)
    , id(ObfObjectId::invalidId())
{
}

OsmAnd::Address::~Address()
{
}

QString OsmAnd::Address::toString() const
{
    return ADDRESS_TYPE_NAMES[addressType];
}

QString OsmAnd::Address::getName(const QString lang, bool transliterate) const
{
    if (transliterate && localizedNames.contains(QStringLiteral("en")))
        return localizedNames[QStringLiteral("en")];
    
    QString name = QString();
    if (!lang.isEmpty() && !localizedNames.isEmpty() && localizedNames.contains(lang))
        name = localizedNames[lang];

    if (name.isEmpty())
        name = nativeName;

    if (transliterate && !name.isEmpty())
        return OsmAnd::ICU::transliterateToLatin(name);
    else
        return name;
}
