#include "EyePiece.h"

#include <iostream>
#include <sstream>

#include <ObfReader.h>
#include <Utilities.h>

#include <SkBitmap.h>
#include <SkCanvas.h>
#include <SkDevice.h>
#include <SkImageEncoder.h>

OsmAnd::EyePiece::Configuration::Configuration()
    : verbose(false)
    , styleName("default")
    , bbox(90, -180, -90, 180)
    , tileSide(256)
    , zoom(15)
    , is32bit(false)
{
}

OSMAND_CORE_UTILS_API bool OSMAND_CORE_UTILS_CALL OsmAnd::EyePiece::parseCommandLineArguments( const QStringList& cmdLineArgs, Configuration& cfg, QString& error )
{
    bool wasObfRootSpecified = false;
    
    for(auto itArg = cmdLineArgs.begin(); itArg != cmdLineArgs.end(); ++itArg)
    {
        auto arg = *itArg;
        if (arg == "-verbose")
        {
            cfg.verbose = true;
        }
        else if (arg.startsWith("-stylePath="))
        {
            auto path = arg.mid(strlen("-stylePath="));
            QDir dir(path);
            if(!dir.exists())
            {
                error = "Style directory '" + path + "' does not exist";
                return false;
            }

            Utilities::findFiles(dir, QStringList() << "*.render.xml", cfg.styleFiles);
        }
        else if (arg.startsWith("-style="))
        {
            cfg.styleName = arg.mid(strlen("-style="));
        }
        else if (arg.startsWith("-obfsDir="))
        {
            QDir obfRoot(arg.mid(strlen("-obfsDir=")));
            if(!obfRoot.exists())
            {
                error = "OBF directory does not exist";
                return false;
            }
            Utilities::findFiles(obfRoot, QStringList() << "*.obf", cfg.obfs);
            wasObfRootSpecified = true;
        }
        else if(arg.startsWith("-bbox="))
        {
            auto values = arg.mid(strlen("-bbox=")).split(",");
            cfg.bbox.left = values[0].toDouble();
            cfg.bbox.top = values[1].toDouble();
            cfg.bbox.right = values[2].toDouble();
            cfg.bbox.bottom =  values[3].toDouble();
        }
        else if(arg.startsWith("-zoom="))
        {
            cfg.zoom = arg.mid(strlen("-zoom=")).toInt();
        }
        else if(arg.startsWith("-tileSide="))
        {
            cfg.tileSide = arg.mid(strlen("-tileSide=")).toInt();
        }
        else if(arg == "-32bit")
        {
            cfg.is32bit = true;
        }
        else if(arg.startsWith("-output="))
        {
            cfg.output = arg.mid(strlen("-output="));
        }
    }

    if(!wasObfRootSpecified)
        Utilities::findFiles(QDir::current(), QStringList() << "*.obf", cfg.obfs);
    if(cfg.obfs.isEmpty())
    {
        error = "No OBF files loaded";
        return false;
    }

    return true;
}

void rasterize(std::ostream &output, const OsmAnd::EyePiece::Configuration& cfg);

OSMAND_CORE_UTILS_API void OSMAND_CORE_UTILS_CALL OsmAnd::EyePiece::rasterizeToStdOut( const Configuration& cfg )
{
    rasterize(std::cout, cfg);
}

OSMAND_CORE_UTILS_API QString OSMAND_CORE_UTILS_CALL OsmAnd::EyePiece::rasterizeToString( const Configuration& cfg )
{
    std::ostringstream output;
    rasterize(output, cfg);
    return QString::fromStdString(output.str());
}

void rasterize(std::ostream &output, const OsmAnd::EyePiece::Configuration& cfg)
{
    OsmAnd::RenderStyles stylesCollection;

    for(auto itStyleFile = cfg.styleFiles.begin(); itStyleFile != cfg.styleFiles.end(); ++itStyleFile)
    {
        auto styleFile = *itStyleFile;

        if(!stylesCollection.registerStyle(*styleFile))
        {
            output << "Failed to parse metadata of '" << qPrintable(styleFile->fileName()) << "'";
            return;
        }
    }

    std::shared_ptr<OsmAnd::RenderStyle> style;
    if(!stylesCollection.obtainCompleteStyle(cfg.styleName, style))
    {
        output << "Failed to resolve style '" << qPrintable(cfg.styleName) << "'";
        return;
    }

    QList< std::shared_ptr<OsmAnd::ObfReader> > obfData;
    for(auto itObf = cfg.obfs.begin(); itObf != cfg.obfs.end(); ++itObf)
    {
        auto obf = *itObf;
        std::shared_ptr<OsmAnd::ObfReader> obfReader(new OsmAnd::ObfReader(obf));
        obfData.push_back(obfReader);
    }

    // Collect all map objects
    QList< std::shared_ptr<OsmAnd::ObfMapSection::MapObject> > mapObjects;
    OsmAnd::AreaI bbox31(
            OsmAnd::Utilities::get31TileNumberY(cfg.bbox.top),
            OsmAnd::Utilities::get31TileNumberX(cfg.bbox.left),
            OsmAnd::Utilities::get31TileNumberY(cfg.bbox.bottom),
            OsmAnd::Utilities::get31TileNumberX(cfg.bbox.right)
        );
    OsmAnd::QueryFilter filter;
    filter._bbox31 = &bbox31;
    filter._zoom = &cfg.zoom;
    for(auto itObf = obfData.begin(); itObf != obfData.end(); ++itObf)
    {
        auto obf = *itObf;

        for(auto itMapSection = obf->mapSections.begin(); itMapSection != obf->mapSections.end(); ++itMapSection)
        {
            auto mapSection = *itMapSection;
            OsmAnd::ObfMapSection::loadMapObjects(obf.get(), mapSection.get(), &mapObjects, &filter, nullptr);
        }
    }
    
    // Calculate output size in pixels
    const auto tileWidth = OsmAnd::Utilities::getTileNumberX(cfg.zoom, cfg.bbox.right) - OsmAnd::Utilities::getTileNumberX(cfg.zoom, cfg.bbox.left);
    const auto tileHeight = OsmAnd::Utilities::getTileNumberX(cfg.zoom, cfg.bbox.top) - OsmAnd::Utilities::getTileNumberX(cfg.zoom, cfg.bbox.bottom);
    const auto pixelWidth = static_cast<int32_t>(tileWidth * cfg.tileSide);
    const auto pixelHeight = static_cast<int32_t>(tileHeight * cfg.tileSide);

    // Allocate render target
    SkBitmap renderSurface;
    renderSurface.setConfig(cfg.is32bit ? SkBitmap::kARGB_8888_Config : SkBitmap::kRGB_565_Config, pixelWidth, pixelHeight);
    if(!renderSurface.allocPixels())
    {
        output << "Failed to allocated render target " << pixelWidth << "x" << pixelHeight;
        return;
    }
    SkDevice renderTarget(renderSurface);

    // Create render canvas
    SkCanvas canvas(&renderTarget);

    // Perform actual rendering
    //canvas.drawColor(rc.getDefaultColor());
    /*if (result != NULL) {
        doRendering(result->result, canvas, req, &rc);
    }*/

    // Save rendered area
    if(!cfg.output.isEmpty())
    {
        std::unique_ptr<SkImageEncoder> encoder(CreatePNGImageEncoder());
        std::unique_ptr<SkImageEncoder> outputStream(CreatePNGImageEncoder());
        encoder->encodeFile(cfg.output.toLocal8Bit(), renderSurface, 100);
    }

    return;
}