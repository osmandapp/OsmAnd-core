#ifndef _OSMAND_CORE_I_MAP_STYLE_H_
#define _OSMAND_CORE_I_MAP_STYLE_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QList>
#include <QHash>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IMapStyle
    {
        Q_DISABLE_COPY_AND_MOVE(IMapStyle);
    private:
    protected:
        IMapStyle();
    public:
        virtual ~IMapStyle();
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_STYLE_H_)
