#ifndef _OSMAND_CORE_OSMAND_CORE_PRIVATE_H_
#define _OSMAND_CORE_OSMAND_CORE_PRIVATE_H_

#include <memory>

#include <OsmAndCore/QtExtensions.h>
#include <QObject>

namespace OsmAnd {
    extern std::shared_ptr<QObject> gMainThreadTaskHost;
}

#endif // !defined(_OSMAND_CORE_OSMAND_CORE_PRIVATE_H_)