#include "Amenity.h"

#include "ObfPoiSectionInfo.h"
#include "QKeyValueIterator.h"
#include "Logging.h"

OsmAnd::Amenity::Amenity(const std::shared_ptr<const ObfPoiSectionInfo>& obfSection_)
    : obfSection(obfSection_)
    , zoomLevel(InvalidZoomLevel)
    , id(ObfObjectId::invalidId())
{
}

OsmAnd::Amenity::~Amenity()
{
}

QHash<QString, QStringList> OsmAnd::Amenity::getDecodedCategories() const
{
    QHash<QString, QStringList> result;

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
        
        auto itResultEntry = result.find(mainCategory);
        if (itResultEntry == result.cend())
            itResultEntry = result.insert(mainCategory, QStringList());
        itResultEntry.value().push_back(subCategory);
    }

    return result;
}

QHash<QString, QString> OsmAnd::Amenity::getDecodedValues() const
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

        switch (value.type())
        {
            case QVariant::Int:
            {
                const auto intValue = value.toInt();

                if (subtype->isText && subtype->possibleValues.size() > intValue)
                    result.insert(subtype->name, subtype->possibleValues[intValue]);
                else
                    result.insert(subtype->name, QString::number(intValue));
                break;
            }
            case QVariant::String:
            {
                const auto stringValue = value.toString();

                result.insert(subtype->name, stringValue);
                break;
            }
            default:
                break;
        }
    }

    return result;
}

QHash< QString, QHash<QString, QList< std::shared_ptr<const OsmAnd::Amenity> > > > OsmAnd::Amenity::groupByCategories(
    const QList< std::shared_ptr<const OsmAnd::Amenity> >& input)
{
    QHash< QString, QHash<QString, QList< std::shared_ptr<const OsmAnd::Amenity> > > > result;

    for (const auto& amenity : constOf(input))
    {
        const auto& categories = amenity->getDecodedCategories();

        for (const auto& itCategoriesEntry : rangeOf(constOf(categories)))
        {
            const auto& mainCategory = itCategoriesEntry.key();
            auto& groupedAmenities = result[mainCategory];

            for (const auto& subCategory : constOf(itCategoriesEntry.value()))
                groupedAmenities[subCategory].push_back(amenity);
        }
    }

    return result;
}
