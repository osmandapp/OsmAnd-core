#include "EyePiece.h"

#include <iostream>
#include <sstream>

#include <SkBitmap.h>
#include <SkCanvas.h>
#include <SkBitmapDevice.h>
#include <SkImageEncoder.h>

#include <OsmAndCore/QtExtensions.h>
#include <QtMath>

#include <OsmAndCore/Common.h>
#include <OsmAndCore/Utilities.h>
#include <OsmAndCore/ObfsCollection.h>
#include <OsmAndCore/ObfDataInterface.h>
#include <OsmAndCore/Data/ObfReader.h>
#include <OsmAndCore/Data/Model/BinaryMapObject.h>
#include <OsmAndCore/Map/Rasterizer.h>
#include <OsmAndCore/Map/RasterizerContext.h>
#include <OsmAndCore/Map/MapPresentationEnvironment.h>
#include <OsmAndCore/Map/MapStyleEvaluator.h>

OsmAnd::EyePiece::Configuration::Configuration()
    : verbose(false)
    , dumpRules(false)
    , styleName("default")
    , bbox(90, -180, -90, 179.9999999999)
    , tileSide(256)
    , densityFactor(1.0)
    , zoom(ZoomLevel15)
    , is32bit(true)
    , drawMap(false)
    , drawText(false)
    , drawIcons(false)
{
}

OSMAND_CORE_UTILS_API bool OSMAND_CORE_UTILS_CALL OsmAnd::EyePiece::parseCommandLineArguments( const QStringList& cmdLineArgs, Configuration& cfg, QString& error )
{
    bool wasObfRootSpecified = false;
    
    for(auto itArg = cmdLineArgs.cbegin(); itArg != cmdLineArgs.cend(); ++itArg)
    {
        auto arg = *itArg;
        if (arg == "-verbose")
        {
            cfg.verbose = true;
        }
        else if (arg == "-dumpRules")
        {
            cfg.dumpRules = true;
        }
        else if (arg == "-map")
        {
            cfg.drawMap = true;
        }
        else if (arg == "-text")
        {
            cfg.drawText = true;
        }
        else if (arg == "-icons")
        {
            cfg.drawIcons = true;
        }
        else if (arg.startsWith("-stylesPath="))
        {
            auto path = arg.mid(strlen("-stylesPath="));
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
            cfg.obfsDir = obfRoot;
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
            cfg.zoom = static_cast<ZoomLevel>(arg.mid(strlen("-zoom=")).toInt());
        }
        else if(arg.startsWith("-tileSide="))
        {
            cfg.tileSide = arg.mid(strlen("-tileSide=")).toInt();
        }
        else if(arg.startsWith("-density="))
        {
            cfg.densityFactor = arg.mid(strlen("-density=")).toFloat();
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

    if(!cfg.drawMap && !cfg.drawText && !cfg.drawIcons)
    {
        cfg.drawMap = true;
        cfg.drawText = true;
        cfg.drawIcons = true;
    }

    if(!wasObfRootSpecified)
        cfg.obfsDir = QDir::current();

    return true;
}

#if defined(_UNICODE) || defined(UNICODE)
void rasterize(std::wostream &output, const OsmAnd::EyePiece::Configuration& cfg);
#else
void rasterize(std::ostream &output, const OsmAnd::EyePiece::Configuration& cfg);
#endif

OSMAND_CORE_UTILS_API void OSMAND_CORE_UTILS_CALL OsmAnd::EyePiece::rasterizeToStdOut( const Configuration& cfg )
{
#if defined(_UNICODE) || defined(UNICODE)
    rasterize(std::wcout, cfg);
#else
    rasterize(std::cout, cfg);
#endif
}

OSMAND_CORE_UTILS_API QString OSMAND_CORE_UTILS_CALL OsmAnd::EyePiece::rasterizeToString( const Configuration& cfg )
{
#if defined(_UNICODE) || defined(UNICODE)
    std::wostringstream output;
    rasterize(output, cfg);
    return QString::fromStdWString(output.str());
#else
    std::ostringstream output;
    rasterize(output, cfg);
    return QString::fromStdString(output.str());
#endif
}

#if defined(_UNICODE) || defined(UNICODE)
void rasterize(std::wostream &output, const OsmAnd::EyePiece::Configuration& cfg)
#else
void rasterize(std::ostream &output, const OsmAnd::EyePiece::Configuration& cfg)
#endif
{
    // Obtain and configure rasterization style context
    OsmAnd::MapStylesCollection stylesCollection;
    for(auto itStyleFile = cfg.styleFiles.cbegin(); itStyleFile != cfg.styleFiles.cend(); ++itStyleFile)
    {
        const auto& styleFile = *itStyleFile;

        if(!stylesCollection.registerStyle(styleFile.absoluteFilePath()))
            output << xT("Failed to parse metadata of '") << QStringToStlString(styleFile.fileName()) << xT("' or duplicate style") << std::endl;
    }
    std::shared_ptr<const OsmAnd::MapStyle> style;
    if(!stylesCollection.obtainBakedStyle(cfg.styleName, style))
    {
        output << xT("Failed to resolve style '") << QStringToStlString(cfg.styleName) << xT("'") << std::endl;
        return;
    }
    if(cfg.dumpRules)
        style->dump();
    
    OsmAnd::ObfsCollection obfsCollection;
    obfsCollection.addDirectory(cfg.obfsDir);

    // Collect all map objects (this should be replaced by something like RasterizerViewport/RasterizerContext)
    QList< std::shared_ptr<const OsmAnd::Model::BinaryMapObject> > mapObjects;
    OsmAnd::AreaI bbox31(
            OsmAnd::Utilities::get31TileNumberY(cfg.bbox.top),
            OsmAnd::Utilities::get31TileNumberX(cfg.bbox.left),
            OsmAnd::Utilities::get31TileNumberY(cfg.bbox.bottom),
            OsmAnd::Utilities::get31TileNumberX(cfg.bbox.right)
        );
    const auto& obfDI = obfsCollection.obtainDataInterface();
    OsmAnd::MapFoundationType mapFoundation;
    obfDI->loadMapObjects(&mapObjects, &mapFoundation, bbox31, cfg.zoom, nullptr);
    bool basemapAvailable;
    obfDI->loadBasemapPresenceFlag(basemapAvailable);
    
    // Calculate output size in pixels
    const auto tileWidth = OsmAnd::Utilities::getTileNumberX(cfg.zoom, cfg.bbox.right) - OsmAnd::Utilities::getTileNumberX(cfg.zoom, cfg.bbox.left);
    const auto tileHeight = OsmAnd::Utilities::getTileNumberY(cfg.zoom, cfg.bbox.bottom) - OsmAnd::Utilities::getTileNumberY(cfg.zoom, cfg.bbox.top);
    const auto pixelWidth = qCeil(tileWidth * cfg.tileSide);
    const auto pixelHeight = qCeil(tileHeight * cfg.tileSide);
    output << xT("Will rasterize ") << mapObjects.count() << xT(" objects onto ") << pixelWidth << xT("x") << pixelHeight << xT(" bitmap") << std::endl;

    // Allocate render target
    SkBitmap renderSurface;
    renderSurface.setConfig(cfg.is32bit ? SkBitmap::kARGB_8888_Config : SkBitmap::kRGB_565_Config, pixelWidth, pixelHeight);
    if(!renderSurface.allocPixels())
    {
        output << xT("Failed to allocated render target ") << pixelWidth << xT("x") << pixelHeight;
        return;
    }
    SkBitmapDevice renderTarget(renderSurface);

    // Create render canvas
    SkCanvas canvas(&renderTarget);

    // Perform actual rendering
    std::shared_ptr<OsmAnd::RasterizerEnvironment> rasterizerEnv(new OsmAnd::RasterizerEnvironment(style, cfg.densityFactor));
    std::shared_ptr<OsmAnd::RasterizerContext> rasterizerContext(new OsmAnd::RasterizerContext(rasterizerEnv));
    OsmAnd::Rasterizer::prepareContext(*rasterizerContext, bbox31, cfg.zoom, mapFoundation, mapObjects);

    OsmAnd::Rasterizer rasterizer(rasterizerContext);
    if(cfg.drawMap)
    {
        rasterizer.rasterizeMap(canvas);
    }
    /*if(cfg.drawText)
        OsmAnd::Rasterizer::rasterizeText(rasterizerContext, !cfg.drawMap, canvas, nullptr);*/

    // Save rendered area
    if(!cfg.output.isEmpty())
    {
        std::unique_ptr<SkImageEncoder> encoder(CreatePNGImageEncoder());
        encoder->encodeFile(cfg.output.toLocal8Bit(), renderSurface, 100);
    }

    return;
}