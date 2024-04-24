#ifndef _OSMAND_CORE_TRANSPORT_STOP_EXIT_H_
#define _OSMAND_CORE_TRANSPORT_STOP_EXIT_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/LatLon.h>
#include <OsmAndCore/Data/DataCommonTypes.h>

namespace OsmAnd
{
    
    class OSMAND_CORE_API TransportStopExit
    {
        Q_DISABLE_COPY_AND_MOVE(TransportStopExit);

    private:
    protected:
    public:
        TransportStopExit();
        TransportStopExit(uint32_t x31, uint32_t y31, QString ref);
        virtual ~TransportStopExit();

        uint32_t x31;
        uint32_t y31;
        QString ref;
        
        LatLon location;
        QString enName;
        
        void setLocation(int zoom, uint32_t dx, uint32_t dy);
        bool compareExit(std::shared_ptr<TransportStopExit>& thatObj);
    };
}

#endif // !defined(_OSMAND_CORE_TRANSPORT_STOP_EXIT_H_)
