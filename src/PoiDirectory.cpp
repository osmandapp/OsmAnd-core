#include "PoiDirectory.h"

OsmAnd::PoiDirectory::PoiDirectory()
{
}

OsmAnd::PoiDirectory::~PoiDirectory()
{
}

const QMultiHash< QString, QString >& OsmAnd::PoiDirectory::getPoiCategories(PoiDirectoryContext* context)
{
    if(!context->_categoriesCache.isEmpty())
        return context->_categoriesCache;

    for(auto itSource = context->sources.cbegin(); itSource != context->sources.cend(); ++itSource)
    {
        auto source = *itSource;
        
        for(auto itPoiSection = source->poiSections.begin(); itPoiSection != source->poiSections.end(); ++itPoiSection)
        {
            const auto& poiSection = *itPoiSection;

            auto itPoiData = context->_contexts.find(poiSection);
            if(itPoiData == context->_contexts.end())
            {
                std::shared_ptr<PoiDirectoryContext::PoiSectionContext> poiData(new PoiDirectoryContext::PoiSectionContext());
                itPoiData = context->_contexts.insert(poiSection, poiData);
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
                    std::shared_ptr<PoiDirectoryContext::PoiSectionContext::PoiCategoryData> poiCategoryData(new PoiDirectoryContext::PoiSectionContext::PoiCategoryData());
                    itCategoryData = poiData->_categoriesIds.insert(category->_name, poiCategoryData);

                    poiCategoryData->_id = categoryId;
                }
                const auto& categoryData = *itCategoryData;

                uint32_t subcategoryId = 0;
                for(auto itSubcategory = category->_subcategories.begin(); itSubcategory != category->_subcategories.end(); ++itSubcategory, subcategoryId++)
                {
                    const auto& subcategory = *itSubcategory;
                    context->_categoriesCache.insert(category->_name, subcategory);

                    categoryData->_subcategoriesIds.insert(subcategory, subcategoryId);
                }
            }
        }
    }

    return context->_categoriesCache;
}

void OsmAnd::PoiDirectory::queryPoiAmenities(
    PoiDirectoryContext* context,
    uint32_t zoom, uint32_t zoomDepth /*= 3*/, const AreaI* bbox31 /*= nullptr*/,
    QMultiHash< QString, QString >* desiredCategories /*= nullptr*/,
    QList< std::shared_ptr<OsmAnd::Model::Amenity> >* amenitiesOut /*= nullptr*/,
    std::function<bool (const std::shared_ptr<OsmAnd::Model::Amenity>&)> visitor /*= nullptr*/,
    IQueryController* controller /*= nullptr*/ )
{
    for(auto itSource = context->sources.cbegin(); itSource != context->sources.cend(); ++itSource)
    {
        auto source = *itSource;
        
        for(auto itPoiSection = source->poiSections.cbegin(); itPoiSection != source->poiSections.cend(); ++itPoiSection)
        {
            const auto& poiSection = *itPoiSection;

            if(controller && controller->isAborted())
                break;
            if(bbox31 && !bbox31->intersects(poiSection->area31))
                continue;
            
            auto itPoiData = context->_contexts.find(poiSection);
            if(itPoiData == context->_contexts.end())
            {
                std::shared_ptr<PoiDirectoryContext::PoiSectionContext> poiData(new PoiDirectoryContext::PoiSectionContext());
                itPoiData = context->_contexts.insert(poiSection, poiData);
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

            ObfPoiSection::loadAmenities(source.get(), poiSection.get(), zoom, zoomDepth, bbox31, desiredCategories ? &desiredCategoriesIds : nullptr, amenitiesOut, visitor, controller);
        }
    }
}
