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

#include <OsmAndCore.h>
#include <OsmAndCore/IObfsCollection.h>
#include <OsmAndCore/Map/IMapStylesCollection.h>

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

            std::shared_ptr<OsmAnd::IObfsCollection> obfsCollection;
            std::shared_ptr<OsmAnd::IMapStylesCollection> stylesCollection;
            QString styleName;
            unsigned int outputImageWidth;
            unsigned int outputImageHeight;
            QString outputImageFilename;
            ImageFormat outputImageFormat;
            OsmAnd::PointI target31;
            float zoom;
            float azimuth;
            float elevationAngle;
            float fov;
            unsigned int referenceTileSize;
            float displayDensityFactor;
            QString locale;
            bool verbose;

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
