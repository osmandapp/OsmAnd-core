#ifndef _OSMAND_CORE_OBF_FILE_P_H_
#define _OSMAND_CORE_OBF_FILE_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QMutex>
#include <QWaitCondition>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"

namespace OsmAnd
{
    class ObfReader_P;
    class ObfInfo;

    class ObfFile;
    class ObfFile_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(ObfFile_P);
    private:
    protected:
        ObfFile_P(ObfFile* owner);

        ImplementationInterface<ObfFile> owner;

        mutable QMutex _obfInfoMutex;
        mutable std::shared_ptr<const ObfInfo> _obfInfo;
    public:
        virtual ~ObfFile_P();

    friend class OsmAnd::ObfFile;
    friend class OsmAnd::ObfReader_P;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_FILE_P_H_)
