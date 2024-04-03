#ifndef _OSMAND_CORE_ICONS_H_
#define _OSMAND_CORE_ICONS_H_

#include <QList>
#include "SkBitmap.h"

namespace OsmAnd
{
    struct IconData
    {
        SkBitmap bitmap;
        int width;
        int height;
        bool isColorable;
    };

    struct LayeredIconData
    {
        std::shared_ptr<const IconData> background;
        QList< std::shared_ptr<const IconData> > underlay;
        std::shared_ptr<const IconData> main;
        QList< std::shared_ptr<const IconData> > overlay;
    };
}

#endif // !defined(_OSMAND_CORE_ICONS_H_)