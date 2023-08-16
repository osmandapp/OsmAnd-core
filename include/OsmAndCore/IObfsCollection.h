#ifndef _OSMAND_CORE_I_OBFS_COLLECTION_H_
#define _OSMAND_CORE_I_OBFS_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QList>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Callable.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Data/DataCommonTypes.h>
#include <OsmAndCore/Data/ObfFile.h>
#include <OsmAndCore/ResourcesManager.h>

namespace OsmAnd
{
    class ObfDataInterface;
    class ObfInfo;

    class OSMAND_CORE_API IObfsCollection
    {
        Q_DISABLE_COPY_AND_MOVE(IObfsCollection);
    private:
    protected:
        IObfsCollection();
    public:
        virtual ~IObfsCollection();

        virtual QList< std::shared_ptr<const ObfFile> > getObfFiles() const = 0;
        virtual std::shared_ptr<OsmAnd::ObfDataInterface> obtainDataInterface(
            const QList< std::shared_ptr<const ResourcesManager::LocalResource> > localResources) const = 0;
        virtual std::shared_ptr<OsmAnd::ObfDataInterface> obtainDataInterface(
            const std::shared_ptr<const ObfFile> obfFile) const = 0;
        virtual std::shared_ptr<ObfDataInterface> obtainDataInterface(
            const AreaI* const pBbox31 = nullptr,
            const ZoomLevel minZoomLevel = MinZoomLevel,
            const ZoomLevel maxZoomLevel = MaxZoomLevel,
            const ObfDataTypesMask desiredDataTypes = fullObfDataTypesMask()) const = 0;
        virtual std::shared_ptr<ObfDataInterface> obtainDataInterfaceByFilename( const QString filename = nullptr) const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_MANUAL_OBFS_COLLECTION_H_)
