#ifndef _OSMAND_CORE_MODEL_POSTCODE_AREA_H_
#define _OSMAND_CORE_MODEL_POSTCODE_AREA_H_

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Data/StreetGroup.h>

namespace OsmAnd {

    class ObfAddressSectionReader_P;

    class OSMAND_CORE_API PostcodeArea : public StreetGroup
    {
    private:
    protected:
        PostcodeArea();
    public:
        virtual ~PostcodeArea();

        friend class OsmAnd::ObfAddressSectionReader_P;
    };
} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_MODEL_POSTCODE_AREA_H_)
