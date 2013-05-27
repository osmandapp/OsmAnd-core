#include "EyePiece.h"

#include <iostream>
#include <sstream>

#include <SkBitmap.h>
#include <SkCanvas.h>
#include <SkDevice.h>
#include <SkImageEncoder.h>

#include <QtCore/qmath.h>

#include <Common.h>
#include <ObfReader.h>
#include <Utilities.h>
#include <Rasterizer.h>
#include <RasterizerContext.h>
#include <RasterizationStyleEvaluator.h>

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
    OsmAnd::RasterizationStyles stylesCollection;
    for(auto itStyleFile = cfg.styleFiles.begin(); itStyleFile != cfg.styleFiles.end(); ++itStyleFile)
    {
        auto styleFile = *itStyleFile;

        if(!stylesCollection.registerStyle(*styleFile))
            output << xT("Failed to parse metadata of '") << QStringToStlString(styleFile->fileName()) << xT("' or duplicate style") << std::endl;
    }
    std::shared_ptr<OsmAnd::RasterizationStyle> style;
    if(!stylesCollection.obtainStyle(cfg.styleName, style))
    {
        output << xT("Failed to resolve style '") << QStringToStlString(cfg.styleName) << xT("'") << std::endl;
        return;
    }
    style->dump();
    
    QList< std::shared_ptr<OsmAnd::ObfReader> > obfData;
    for(auto itObf = cfg.obfs.begin(); itObf != cfg.obfs.end(); ++itObf)
    {
        auto obf = *itObf;
        std::shared_ptr<OsmAnd::ObfReader> obfReader(new OsmAnd::ObfReader(obf));
        obfData.push_back(obfReader);
    }

    // Collect all map objects (this should be replaced by something like RasterizerViewport/RasterizerContext)
    QList< std::shared_ptr<OsmAnd::Model::MapObject> > mapObjects;
    OsmAnd::AreaI bbox31(
            qCeil(OsmAnd::Utilities::get31TileNumberY(cfg.bbox.top)),
            qFloor(OsmAnd::Utilities::get31TileNumberX(cfg.bbox.left)),
            qFloor(OsmAnd::Utilities::get31TileNumberY(cfg.bbox.bottom)),
            qCeil(OsmAnd::Utilities::get31TileNumberX(cfg.bbox.right))
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
    output << xT("Will rasterize ") << mapObjects.count() << xT(" objects onto ") << pixelWidth << xT("x") << pixelHeight << xT(" bitmap") << std::endl;
    /*for(auto itMapObject = mapObjects.begin(); itMapObject != mapObjects.end(); ++itMapObject)
    {
        const auto& mapObject = *itMapObject;
        output << xT("\t") << mapObject->id << std::endl;
    }*/

    // Allocate render target
    SkBitmap renderSurface;
    renderSurface.setConfig(cfg.is32bit ? SkBitmap::kARGB_8888_Config : SkBitmap::kRGB_565_Config, pixelWidth, pixelHeight);
    if(!renderSurface.allocPixels())
    {
        output << xT("Failed to allocated render target ") << pixelWidth << xT("x") << pixelHeight;
        return;
    }
    SkDevice renderTarget(renderSurface);

    // Create render canvas
    SkCanvas canvas(&renderTarget);

    // Perform actual rendering
    OsmAnd::RasterizerContext rasterizerContext(style);
    OsmAnd::Rasterizer::rasterize(rasterizerContext, true, canvas, cfg.bbox, cfg.zoom, cfg.tileSide, mapObjects, OsmAnd::PointI(), nullptr);

    // Save rendered area
    if(!cfg.output.isEmpty())
    {
        std::unique_ptr<SkImageEncoder> encoder(CreatePNGImageEncoder());
        std::unique_ptr<SkImageEncoder> outputStream(CreatePNGImageEncoder());
        encoder->encodeFile(cfg.output.toLocal8Bit(), renderSurface, 100);
    }

    return;
}