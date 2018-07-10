#ifndef _OSMAND_CORE_TRANSPORT_ROUTE_H_
#define _OSMAND_CORE_TRANSPORT_ROUTE_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QList>
#include <QVector>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/Data/DataCommonTypes.h>

namespace OsmAnd
{
    class TransportStop;
    class Way;
    
    class OSMAND_CORE_API TransportRoute
    {
        Q_DISABLE_COPY_AND_MOVE(TransportRoute);

    private:
    protected:
    public:
        TransportRoute();
        virtual ~TransportRoute();

        QList<std::shared_ptr<TransportStop>> forwardStops;
        QString ref;
        QString oper;
        QString type;
        uint32_t dist;
        QString color;
        QList<QVector<PointI>> forwardWays31;
    };
}

#endif // !defined(_OSMAND_CORE_TRANSPORT_ROUTE_H_)
