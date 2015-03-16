#ifndef _OSMAND_CORE_I_ON_PATH_MAP_SYMBOL_H_
#define _OSMAND_CORE_I_ON_PATH_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QVector>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IOnPathMapSymbol
    {
        Q_DISABLE_COPY_AND_MOVE(IOnPathMapSymbol);

    public:
        struct PinPoint Q_DECL_FINAL
        {
            PointI point31;

            unsigned int basePathPointIndex;
            float normalizedOffsetFromBasePathPoint;
        };

    protected:
        IOnPathMapSymbol();
    public:
        virtual ~IOnPathMapSymbol();
        
        virtual QVector<PointI> getPath31() const = 0;
        virtual std::shared_ptr< const QVector<PointI> > getPath31SharedRef() const = 0;
        virtual void setPath31(const QVector<PointI>& path31) = 0;
        virtual void setPath31(const std::shared_ptr< const QVector<PointI> >& sharedPath31) = 0;
        
        virtual PinPoint getPinPointOnPath() const = 0;
        virtual void setPinPointOnPath(const PinPoint& pinPoint) = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_ON_PATH_MAP_SYMBOL_H_)
