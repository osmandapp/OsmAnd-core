#ifndef _OSMAND_CORE_OSMAND_CORE_PRIVATE_H_
#define _OSMAND_CORE_OSMAND_CORE_PRIVATE_H_

#include <memory>

#include "QtExtensions.h"
#include <QThread>
#include <QObject>

namespace OsmAnd
{
    //TODO: use const!
    extern QThread* gMainThread;
    extern std::shared_ptr<QObject> gMainThreadRootObject;
}

#endif // !defined(_OSMAND_CORE_OSMAND_CORE_PRIVATE_H_)