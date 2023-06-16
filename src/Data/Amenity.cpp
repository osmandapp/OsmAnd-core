#include "Amenity.h"

#include "ObfPoiSectionInfo.h"
#include "QKeyValueIterator.h"
#include "zlibUtilities.h"
#include "Logging.h"
#include <ICU.h>

OsmAnd::Amenity::Amenity(const std::shared_ptr<const ObfPoiSectionInfo>& obfSection_)
    : obfSection(obfSection_)
    , id(ObfObjectId::invalidId())
{
}

OsmAnd::Amenity::~Amenity()
{
}

void OsmAnd::Amenity::evaluateTypes()
{
    const auto& decodedCategories = getDecodedCategories();
    for (const auto& decodedCategory : constOf(decodedCategories))
    {
        if (type.isEmpty())
            type = decodedCategory.category;
        if (!subType.isEmpty())
            subType += QStringLiteral(";");
        subType += decodedCategory.subcategory;
    }
}

QList<OsmAnd::Amenity::DecodedCategory> OsmAnd::Amenity::getDecodedCategories() const
{
    QList<DecodedCategory> result;

    const auto sectionCategories = obfSection->getCategories();
    if (!sectionCategories)
        return result;

    for (const auto& category : constOf(categories))
    {
        const auto mainCategoryIndex = category.getMainCategoryIndex();
        const auto subCategoryIndex = category.getSubCategoryIndex();

        if (mainCategoryIndex >= sectionCategories->mainCategories.size())
        {
            LogPrintf(LogSeverityLevel::Warning,
                "Amenity %s (%s) references non-existent category %d",
                qPrintable(id.toString()),
                qPrintable(nativeName),
                mainCategoryIndex);
            continue;
        }
        const auto& mainCategory = sectionCategories->mainCategories[mainCategoryIndex];
        const auto& subCategories = sectionCategories->subCategories[mainCategoryIndex];
        
        if (subCategoryIndex >= subCategories.size())
        {
            LogPrintf(LogSeverityLevel::Warning,
                "Amenity %s (%s) references non-existent subcategory %d in category %d",
                qPrintable(id.toString()),
                qPrintable(nativeName),
                subCategoryIndex,
                mainCategoryIndex);
            continue;
        }
        const auto& subCategory = subCategories[subCategoryIndex];
        
        DecodedCategory decodedCategory;
        decodedCategory.category = mainCategory;
        decodedCategory.subcategory = subCategory;
        result.push_back(decodedCategory);
    }

    return result;
}

QList<OsmAnd::Amenity::DecodedValue> OsmAnd::Amenity::getDecodedValues() const
{
    QList<DecodedValue> result;

    const auto sectionSubtypes = obfSection->getSubtypes();
    if (!sectionSubtypes)
        return result;

    for (const auto& valueEntry : rangeOf(constOf(values)))
    {
        const auto subtypeIndex = valueEntry.key();
        if (subtypeIndex >= sectionSubtypes->subtypes.size())
        {
            LogPrintf(LogSeverityLevel::Warning,
                "Amenity %s (%s) references non-existent subtype %d",
                qPrintable(id.toString()),
                qPrintable(nativeName),
                subtypeIndex);
            continue;
        }

        const auto& subtype = sectionSubtypes->subtypes[subtypeIndex];
        const auto& value = valueEntry.value();

        DecodedValue decodedValue;
        decodedValue.declaration = subtype;

        switch (value.type())
        {
            case QVariant::Int:
            case QVariant::UInt:
            {
                const auto intValue = value.toInt();
                if (subtype->possibleValues.size() > intValue)
                    decodedValue.value = subtype->possibleValues[intValue];
                break;
            }
            case QVariant::String:
            {
                const auto stringValue = value.toString();
                decodedValue.value = stringValue;
                break;
            }
            case QVariant::ByteArray:
            {
                const auto dataValue = value.toByteArray();
                const auto stringValue = QString::fromUtf8(zlibUtilities::gzipDecompress(dataValue));
                decodedValue.value = stringValue;
                break;
            }
            default:
                break;
        }
        result.push_back(decodedValue);
    }
    return result;
}

QHash<QString, QString> OsmAnd::Amenity::getDecodedValuesHash() const
{
    QHash<QString, QString> result;
    
    const auto sectionSubtypes = obfSection->getSubtypes();
    if (!sectionSubtypes)
        return result;

    for (const auto& valueEntry : rangeOf(constOf(values)))
    {
        const auto subtypeIndex = valueEntry.key();
        if (subtypeIndex >= sectionSubtypes->subtypes.size())
        {
            LogPrintf(LogSeverityLevel::Warning,
                "Amenity %s (%s) references non-existent subtype %d",
                qPrintable(id.toString()),
                qPrintable(nativeName),
                subtypeIndex);
            continue;
        }

        const auto& subtype = sectionSubtypes->subtypes[subtypeIndex];
        const auto& value = valueEntry.value();
        
        QString resultTag = subtype->tagName;
        QString resultValue;

        switch (value.type())
        {
            case QVariant::Int:
            case QVariant::UInt:
            {
                const auto intValue = value.toInt();
                if (subtype->possibleValues.size() > intValue)
                    resultValue = subtype->possibleValues[intValue];
                break;
            }
            case QVariant::String:
            {
                const auto stringValue = value.toString();
                resultValue = stringValue;
                break;
            }
            case QVariant::ByteArray:
            {
                const auto dataValue = value.toByteArray();
                const auto stringValue = QString::fromUtf8(zlibUtilities::gzipDecompress(dataValue));
                resultValue = stringValue;
                break;
            }
            default:
                break;
        }
        result[resultTag] = resultValue;
    }
    return result;
}

QHash< QString, QHash<QString, QList< std::shared_ptr<const OsmAnd::Amenity> > > > OsmAnd::Amenity::groupByCategories(
    const QList< std::shared_ptr<const OsmAnd::Amenity> >& input)
{
    QHash< QString, QHash<QString, QList< std::shared_ptr<const Amenity> > > > result;

    for (const auto& amenity : constOf(input))
    {
        const auto& decodedCategories = amenity->getDecodedCategories();
        for (const auto& decodedCategory : constOf(decodedCategories))
        {
            result[decodedCategory.category][decodedCategory.subcategory].push_back(amenity);
        }
    }

    return result;
}

OsmAnd::Amenity::DecodedCategory::DecodedCategory()
{
}

OsmAnd::Amenity::DecodedCategory::~DecodedCategory()
{
}

OsmAnd::Amenity::DecodedValue::DecodedValue()
{
}

OsmAnd::Amenity::DecodedValue::~DecodedValue()
{
}

QString OsmAnd::Amenity::getName(const QString lang, bool transliterate) const
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
