#ifndef _OSMAND_CORE_TOOLS_EYEPIECE_H_
#define _OSMAND_CORE_TOOLS_EYEPIECE_H_

#include <OsmAndCore/stdlib_common.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <iostream>
#include <sstream>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QFile>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/IObfsCollection.h>
#include <OsmAndCore/IGeoTiffCollection.h>
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
            std::shared_ptr<const OsmAnd::IGeoTiffCollection> geotiffCollection;
            std::shared_ptr<OsmAnd::IMapStylesCollection> stylesCollection;
            QString styleName;
            QHash< QString, QString > styleSettings;
            unsigned int outputImageWidth;
            unsigned int outputImageHeight;
            QString outputImageFilename;
            ImageFormat outputImageFormat;
            OsmAnd::PointI target31;
            float zoom;
            float azimuth;
            float elevationAngle;
            unsigned int frames;
            OsmAnd::PointI endTarget31;
            float endZoom;
            float endAzimuth;
            float endElevationAngle;
            float fov;
            unsigned int referenceTileSize;
            float displayDensityFactor;
            float mapScale;
            float symbolsScale;
            QString locale;
            bool verbose;
#if defined(OSMAND_TARGET_OS_linux)
            bool useLegacyContext;
#endif

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
