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
    class ObfReader;
    class ObfInfo;

    class ObfFile;
    class ObfFile_P
    {
        Q_DISABLE_COPY(ObfFile_P)
    private:
    protected:
        ObfFile_P(ObfFile* owner);

        ImplementationInterface<ObfFile> owner;

        mutable QMutex _obfInfoMutex;
        mutable std::shared_ptr<const ObfInfo> _obfInfo;

        mutable QMutex _lockCounterMutex;
        mutable QWaitCondition _lockCounterWaitCondition;
        mutable volatile int _lockCounter;
    public:
        virtual ~ObfFile_P();

        bool tryLockForReading() const;
        void lockForReading() const;
        void unlockFromReading() const;

        bool tryLockForWriting() const;
        void lockForWriting() const;
        void unlockFromWriting() const;

    friend class OsmAnd::ObfFile;
    friend class OsmAnd::ObfReader;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_FILE_P_H_)
