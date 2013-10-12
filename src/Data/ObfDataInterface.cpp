#include "ObfDataInterface.h"
#include "ObfDataInterface_P.h"

#include "ObfReader.h"
#include "ObfInfo.h"
#include "ObfMapSectionReader.h"
#include "IQueryController.h"

OsmAnd::ObfDataInterface::ObfDataInterface( const QList< std::shared_ptr<ObfReader> >& readers )
    : _d(new ObfDataInterface_P(this, readers))
{
}

OsmAnd::ObfDataInterface::~ObfDataInterface()
{
}

void OsmAnd::ObfDataInterface::obtainObfFiles( QList< std::shared_ptr<const ObfFile> >* outFiles /*= nullptr*/, IQueryController* controller /*= nullptr*/ )
{
    for(auto itObfReader = _d->readers.cbegin(); itObfReader != _d->readers.cend(); ++itObfReader)
    {
        if(controller && controller->isAborted())
            return;

        const auto& obfReader = *itObfReader;

        // Initialize OBF file
        obfReader->obtainInfo();

        if(outFiles)
            outFiles->push_back(obfReader->obfFile);
    }
}

void OsmAnd::ObfDataInterface::obtainBasemapPresenceFlag( bool& basemapPresent, IQueryController* controller /*= nullptr*/ )
{
    basemapPresent = false;
    for(auto itObfReader = _d->readers.cbegin(); itObfReader != _d->readers.cend(); ++itObfReader)
    {
        if(controller && controller->isAborted())
            return;

        const auto& obfReader = *itObfReader;
        const auto& obfInfo = obfReader->obtainInfo();
        basemapPresent = basemapPresent || obfInfo->isBasemap;
    }
}

void OsmAnd::ObfDataInterface::obtainMapObjects( QList< std::shared_ptr<const OsmAnd::Model::MapObject> >* resultOut, MapFoundationType* foundationOut, const AreaI& area31, const ZoomLevel zoom, IQueryController* controller /*= nullptr*/ )
{
    if(foundationOut)
        *foundationOut = MapFoundationType::Undefined;

    // Iterate through all OBF readers
    for(auto itObfReader = _d->readers.cbegin(); itObfReader != _d->readers.cend(); ++itObfReader)
    {
        // Check if request is aborted
        if(controller && controller->isAborted())
            return;

        // Iterate over all map sections of each OBF reader
        const auto& obfReader = *itObfReader;
        const auto& obfInfo = obfReader->obtainInfo();
        for(auto itMapSection = obfInfo->mapSections.cbegin(); itMapSection != obfInfo->mapSections.cend(); ++itMapSection)
        {
            // Check if request is aborted
            if(controller && controller->isAborted())
                return;

            // Read objects from each map section
            const auto& mapSection = *itMapSection;
            OsmAnd::ObfMapSectionReader::loadMapObjects(obfReader, mapSection, zoom, &area31, resultOut, foundationOut, nullptr, controller);
        }
    }
}
