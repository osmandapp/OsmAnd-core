#ifndef _OSMAND_CORE_ICONS_H_
#define _OSMAND_CORE_ICONS_H_

#include <QByteArray>
#include <QList>
#include "SkBitmap.h"

namespace OsmAnd
{
    struct IconData
    {
        QByteArray bytes;
        SkBitmap bitmap;
        int width;
        int height;
        bool isColorable;

        char* getBytesArray(size_t* LENGTH)
        {
            *LENGTH = bytes.size();
            return bytes.data();
        }
    };

    struct LayeredIconData
    {
        IconData background;
        QList<IconData> underlay;
        IconData main;
        QList<IconData> overlay;
    };
}

#endif // !defined(_OSMAND_CORE_ICONS_H_)