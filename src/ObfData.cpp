#include "ObfData.h"

OsmAnd::ObfData::ObfData()
    : _combinedPoiCategoriesInvalidated(true)
{
}

OsmAnd::ObfData::~ObfData()
{
}

void OsmAnd::ObfData::addSource( const std::shared_ptr<ObfReader>& source )
{
    assert(source);

    _sourcesMutex.lock();
    assert(!_sources.contains(source));
    _sources.insert(source, std::shared_ptr<Data>(new Data()));
    invalidateCombinedPoiCategories();
    _sourcesMutex.unlock();
}

void OsmAnd::ObfData::removeSource( const std::shared_ptr<ObfReader>& source )
{
    assert(source);

    _sourcesMutex.lock();
    assert(_sources.contains(source));
    _sources.remove(source);
    invalidateCombinedPoiCategories();
    _sourcesMutex.unlock();
}

void OsmAnd::ObfData::clearSources()
{
    _sourcesMutex.lock();
    _sources.clear();
    invalidateCombinedPoiCategories();
    _sourcesMutex.unlock();
}

QList< std::shared_ptr<OsmAnd::ObfReader> > OsmAnd::ObfData::getSources()
{
    _sourcesMutex.lock();
    const auto& sources = _sources.keys();
    _sourcesMutex.unlock();
    return sources;
}

void OsmAnd::ObfData::invalidateCombinedPoiCategories()
{
    _combinedPoiCategoriesInvalidated = true;
    _combinedPoiCategories.clear();
}

QMultiHash< QString, QString > OsmAnd::ObfData::getPoiCategories()
{
    if(!_combinedPoiCategoriesInvalidated)
        return _combinedPoiCategories;

    _sourcesMutex.lock();
    const auto sources = _sources;
    _sourcesMutex.unlock();

    for(auto itSource = sources.cbegin(); itSource != sources.cend(); ++itSource)
    {
        const auto& source = itSource.key();
        const auto& sourceData = itSource.value();

        for(auto itPoiSection = source->_poiSections.begin(); itPoiSection != source->_poiSections.end(); ++itPoiSection)
        {
            const auto& poiSection = *itPoiSection;

            auto itPoiData = sourceData->_poiData.find(poiSection);
            if(itPoiData == sourceData->_poiData.end())
            {
                std::shared_ptr<Data::PoiData> poiData(new Data::PoiData());
                itPoiData = sourceData->_poiData.insert(poiSection, poiData);
            }
            const auto& poiData = *itPoiData;

            // Check if we have loaded categories for that poi section already
            if(poiData->_categories.isEmpty())
                ObfPoiSection::loadCategories(source.get(), poiSection.get(), poiData->_categories);

            uint32_t categoryId = 0;
            for(auto itCategory = poiData->_categories.begin(); itCategory != poiData->_categories.end(); ++itCategory, categoryId++)
            {
                const auto& category = *itCategory;

                auto itCategoryData = poiData->_categoriesIds.find(category->_name);
                if(itCategoryData == poiData->_categoriesIds.end())
                {
                    std::shared_ptr<Data::PoiData::PoiCategoryData> poiCategoryData(new Data::PoiData::PoiCategoryData());
                    itCategoryData = poiData->_categoriesIds.insert(category->_name, poiCategoryData);

                    poiCategoryData->_id = categoryId;
                }
                const auto& categoryData = *itCategoryData;

                uint32_t subcategoryId = 0;
                for(auto itSubcategory = category->_subcategories.begin(); itSubcategory != category->_subcategories.end(); ++itSubcategory, subcategoryId++)
                {
                    const auto& subcategory = *itSubcategory;
                    _combinedPoiCategories.insert(category->_name, subcategory);

                    categoryData->_subcategoriesIds.insert(subcategory, subcategoryId);
                }
            }
        }
    }

    _combinedPoiCategoriesInvalidated = false;
    return _combinedPoiCategories;
}

void OsmAnd::ObfData::queryPoiAmenities(
    QMultiHash< QString, QString >* desiredCategories /*= nullptr*/,
    QList< std::shared_ptr<OsmAnd::Model::Amenity> >* amenitiesOut /*= nullptr*/,
    IQueryFilter* filter /*= nullptr*/, uint32_t zoomToSkipFilter /*= 3*/,
    std::function<bool (std::shared_ptr<OsmAnd::Model::Amenity>)>* visitor /*= nullptr*/,
    IQueryController* controller /*= nullptr*/ )
{
    _sourcesMutex.lock();
    const auto sources = _sources;
    _sourcesMutex.unlock();

    for(auto itSource = sources.cbegin(); itSource != sources.cend(); ++itSource)
    {
        const auto& source = itSource.key();
        const auto& sourceData = itSource.value();

        for(auto itPoiSection = source->_poiSections.cbegin(); itPoiSection != source->_poiSections.cend(); ++itPoiSection)
        {
            const auto& poiSection = *itPoiSection;

            if(controller && controller->isAborted())
                break;
            if(filter)
            {
                if (poiSection->_right31 < filter->_bboxLeft31 || poiSection->_left31 > filter->_bboxRight31 || poiSection->_top31 > filter->_bboxBottom31 || poiSection->_bottom31 < filter->_bboxTop31)
                    continue;
            }

            auto itPoiData = sourceData->_poiData.find(poiSection);
            if(itPoiData == sourceData->_poiData.end())
            {
                std::shared_ptr<Data::PoiData> poiData(new Data::PoiData());
                itPoiData = sourceData->_poiData.insert(poiSection, poiData);
            }
            const auto& poiData = *itPoiData;

            QSet<uint32_t> desiredCategoriesIds;
            if(desiredCategories)
            {
                const auto& keys = desiredCategories->uniqueKeys();
                for(auto itDesiredCat = keys.cbegin(); itDesiredCat != keys.cend(); ++itDesiredCat)
                {
                    const auto& desiredCat = *itDesiredCat;

                    auto itCategory = poiData->_categoriesIds.find(desiredCat);
                    if(itCategory == poiData->_categoriesIds.end())
                        continue;
                    const auto& category = *itCategory;

                    const auto& values = desiredCategories->values(desiredCat);
                    if(values.isEmpty())
                    {
                        // If there are no subcategories, it means whole category
                        uint32_t id = (category->_id << 16) | 0xFFFF;
                        desiredCategoriesIds.insert(id);
                        continue;
                    }
                    for(auto itDesiredSubcat = values.cbegin(); itDesiredSubcat != values.cend(); ++itDesiredSubcat)
                    {
                        const auto& desiredSubcat = *itDesiredSubcat;

                        auto itSubcategoryId = category->_subcategoriesIds.find(desiredSubcat);
                        if(itSubcategoryId == category->_subcategoriesIds.end())
                            continue;
                        auto subcategoryId = *itSubcategoryId;

                        uint32_t id = (category->_id << 16) | subcategoryId;
                        desiredCategoriesIds.insert(id);
                    }
                }
            }
            
            ObfPoiSection::loadAmenities(source.get(), poiSection.get(), desiredCategories ? &desiredCategoriesIds : nullptr, amenitiesOut, filter, zoomToSkipFilter, visitor, controller);
        }
    }
}
