#ifndef _OSMAND_CORE_BINARY_OBF_CONSTANTS_H_
#define _OSMAND_CORE_BINARY_OBF_CONSTANTS_H_

#include <QString>

namespace OsmAnd {

    struct ObfConstants {

        ObfConstants() = delete; 
        
        static bool isTagIndexedForSearchAsName(const QString& tag);
        static bool isTagIndexedForSearchAsId(const QString& tag);
        static bool isTagIndexedAsSearchRelated(const QString& tag);
    };

}

#endif // !defined(_OSMAND_CORE_BINARY_OBF_CONSTANTS_H_)