#ifndef _OSMAND_CORE_TOOLS_EYEPIECE_H_
#define _OSMAND_CORE_TOOLS_EYEPIECE_H_

#include <memory>
#include <iostream>
#include <sstream>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QFile>

#include <OsmAndCoreTools.h>

namespace OsmAndTools
{
    class OSMAND_CORE_TOOLS_API EyePiece Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(EyePiece);

    public:
        enum class ImageFormat
        {
            PNG,
            JPEG
        };

        struct OSMAND_CORE_TOOLS_API Configuration Q_DECL_FINAL
        {
            Configuration();

            unsigned int outputImageWidth;
            unsigned int outputImageHeight;
            ImageFormat outputImageFormat;
            QString outputImageFilename;

            static bool parseFromCommandLineArguments(
                const QStringList& commandLineArgs,
                Configuration& outConfiguration,
                QString& outError);
        };

    private:
#if defined(_UNICODE) || defined(UNICODE)
        bool glVerifyResult(std::wostream& output) const;
#else
        bool glVerifyResult(std::ostream& output) const;
#endif
        
        #if defined(_UNICODE) || defined(UNICODE)
        bool rasterize(std::wostream& output);
        #else
        bool rasterize(std::ostream& output);
        #endif
    protected:
    public:
        EyePiece(const Configuration& configuration);
        ~EyePiece();

        const Configuration configuration;

        bool rasterize(QString *pLog = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_TOOLS_EYEPIECE_H_)
