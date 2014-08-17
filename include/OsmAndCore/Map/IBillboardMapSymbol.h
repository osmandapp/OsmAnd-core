#ifndef _OSMAND_CORE_I_BILLBOARD_MAP_SYMBOL_H_
#define _OSMAND_CORE_I_BILLBOARD_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IBillboardMapSymbol
    {
        Q_DISABLE_COPY_AND_MOVE(IBillboardMapSymbol);
    private:
    protected:
        IBillboardMapSymbol();
    public:
        virtual ~IBillboardMapSymbol();

        virtual PointI getOffset() const = 0;
        virtual void setOffset(const PointI offset) = 0;

        virtual PointI getPosition31() const = 0;
        virtual void setPosition31(const PointI position) = 0;

        virtual QList<PointI> getAdditionalPositions31() const = 0;
        virtual void setAdditionalPositions31(const QList<PointI>& additionalPositions31) = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_BILLBOARD_MAP_SYMBOL_H_)
