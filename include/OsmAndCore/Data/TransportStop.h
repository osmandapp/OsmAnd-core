#ifndef _OSMAND_CORE_TRANSPORT_STOP_H_
#define _OSMAND_CORE_TRANSPORT_STOP_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QVector>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/LatLon.h>
#include <OsmAndCore/Data/DataCommonTypes.h>

namespace OsmAnd
{
    class ObfTransportSectionInfo;
    class TransportStopExit;
    
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
        QVector<uint32_t> referencesToRoutes;
        QList<std::shared_ptr<TransportStopExit>> exits;

        ObfObjectId id;
        LatLon location;
        QString enName;
        QString localizedName;
        
        void addExit(std::shared_ptr<TransportStopExit> &exit);
        bool compareStop(const std::shared_ptr<OsmAnd::TransportStop>& thatObj);
    };
}

#endif // !defined(_OSMAND_CORE_TRANSPORT_STOP_H_)
