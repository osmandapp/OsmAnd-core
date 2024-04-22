#ifndef _OSMAND_CORE_TRANSPORT_ROUTE_H_
#define _OSMAND_CORE_TRANSPORT_ROUTE_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QList>
#include <QVector>
#include "TransportStop.h"
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

        virtual QString getName(const QString lang, bool transliterate) const;
        
        uint32_t offset;

        ObfObjectId id;
        QString enName;
        QString localizedName;
        
        QList<std::shared_ptr<TransportStop>> forwardStops;
        QString ref;
        QString oper;
        QString type;
        uint32_t dist;
        QString color;
        QList< QVector<PointI> > forwardWays31;

        bool compareRoute(std::shared_ptr<const OsmAnd::TransportRoute>& thatObj) const;
    };
}

#endif // !defined(_OSMAND_CORE_TRANSPORT_ROUTE_H_)
