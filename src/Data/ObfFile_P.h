#ifndef _OSMAND_CORE_OBF_FILE_P_H_
#define _OSMAND_CORE_OBF_FILE_P_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QMutex>

#include <OsmAndCore.h>

namespace OsmAnd {

    class ObfReader;
    class ObfInfo;

    class ObfFile;
    class OSMAND_CORE_API ObfFile_P
    {
        Q_DISABLE_COPY(ObfFile_P)
    private:
    protected:
        ObfFile_P(ObfFile* owner);

        ObfFile* const owner;

        mutable QMutex _obfInfoMutex;
        std::shared_ptr<ObfInfo> _obfInfo;
    public:
        virtual ~ObfFile_P();

    friend class OsmAnd::ObfFile;
    friend class OsmAnd::ObfReader;
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_OBF_FILE_P_H_)
