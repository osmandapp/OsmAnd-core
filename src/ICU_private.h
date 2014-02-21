#ifndef _OSMAND_CORE_ICU_PRIVATE_H_
#define _OSMAND_CORE_ICU_PRIVATE_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QStringList>
#include <QVector>

#include <OsmAndCore.h>

namespace OsmAnd
{
    namespace ICU
    {
        void initialize();
        void release();
    }
}

#endif // !defined(_OSMAND_CORE_ICU_PRIVATE_H_)
