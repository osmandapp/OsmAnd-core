#ifndef _OSMAND_CORE_OBF_FILE_H_
#define _OSMAND_CORE_OBF_FILE_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QFileInfo>

#include <OsmAndCore.h>

namespace OsmAnd {

    class ObfInfo;
    class ObfReader;

    class ObfFile_P;
    class OSMAND_CORE_API ObfFile
    {
        Q_DISABLE_COPY(ObfFile)
    private:
        const std::unique_ptr<ObfFile_P> _d;
    protected:
    public:
        ObfFile(const QString& filePath);
        virtual ~ObfFile();

        const QString filePath;

        const std::shared_ptr<ObfInfo>& obfInfo;

    friend class OsmAnd::ObfReader;
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_OBF_FILE_H_)
