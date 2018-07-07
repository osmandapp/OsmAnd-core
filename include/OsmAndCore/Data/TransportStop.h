#ifndef _OSMAND_CORE_TRANSPORT_STOP_H_
#define _OSMAND_CORE_TRANSPORT_STOP_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/Data/DataCommonTypes.h>
#include <OsmAndCore/Data/ObfTransportSectionInfo.h>

namespace OsmAnd
{
    class OSMAND_CORE_API TransportStop
    {
        Q_DISABLE_COPY_AND_MOVE(TransportStop);

    private:
    protected:
    public:
        TransportStop(const std::shared_ptr<const ObfTransportSectionInfo>& obfSection);
        virtual ~TransportStop();

        virtual QString getName(const QString lang, bool transliterate) const;
        
        const std::shared_ptr<const ObfTransportSectionInfo> obfSection;

        uint32_t offset;
        ObfTransportSectionInfo::ReferencesToRoutes referencesToRoutes;

        ObfObjectId id;
        PointI position31;
        QString enName;
        QString localizedName;
    };
}

#endif // !defined(_OSMAND_CORE_TRANSPORT_STOP_H_)
