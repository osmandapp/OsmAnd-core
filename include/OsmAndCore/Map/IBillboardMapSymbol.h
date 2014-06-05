#ifndef _OSMAND_CORE_I_BILLBOARD_MAP_SYMBOL_H_
#define _OSMAND_CORE_I_BILLBOARD_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IBillboardMapSymbol
    {
        Q_DISABLE_COPY(IBillboardMapSymbol);
    private:
    protected:
        IBillboardMapSymbol();
    public:
        virtual ~IBillboardMapSymbol();

        virtual PointI getOffset() const = 0;
        virtual void setOffset(const PointI offset) = 0;

        virtual PointI getPosition31() const = 0;
        virtual void setPosition31(const PointI position) = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_BILLBOARD_MAP_SYMBOL_H_)
