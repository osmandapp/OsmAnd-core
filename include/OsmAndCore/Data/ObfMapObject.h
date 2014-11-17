#ifndef _OSMAND_CORE_OBF_MAP_OBJECT_H_
#define _OSMAND_CORE_OBF_MAP_OBJECT_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/MapObject.h>

namespace OsmAnd
{
    class ObfSectionInfo;

    class OSMAND_CORE_API ObfMapObject : public MapObject
    {
        Q_DISABLE_COPY_AND_MOVE(ObfMapObject);
    private:
    protected:
        ObfMapObject(const std::shared_ptr<const ObfSectionInfo>& obfSection);
    public:
        virtual ~ObfMapObject();

        // General information
        ObfObjectId id;
        const std::shared_ptr<const ObfSectionInfo> obfSection;
        virtual QString toString() const;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_MAP_OBJECT_H_)
