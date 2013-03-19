#include "Voyager.h"

OsmAnd::Voyager::Configuration::Configuration()
    : verbose(false)
    , startLatitude(0)
    , startLongitude(0)
    , endLatitude(0)
    , endLongitude(0)
{
}

void lookForObfs(const QDir& origin, QList< std::shared_ptr<QFile> > obfsCollection)
{
    auto obfs = origin.entryInfoList(QStringList() << "*.obf", QDir::Files);
    for(auto itObf = obfs.begin(); itObf != obfs.end(); ++itObf)
        obfsCollection.push_back(std::shared_ptr<QFile>(new QFile(itObf->absolutePath())));
    
    auto subdirs = origin.entryInfoList(QStringList(), QDir::AllDirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    for(auto itSubdir = subdirs.begin(); itSubdir != subdirs.end(); ++itSubdir)
        lookForObfs(QDir(itSubdir->absolutePath()), obfsCollection);
}

OSMAND_CORE_UTILS_API bool OSMAND_CORE_UTILS_CALL OsmAnd::Voyager::parseCommandLineArguments( const QStringList& cmdLineArgs, Configuration& cfg, QString& error )
{
    bool wasObfRootSpecified = false;
    bool wasRouterConfigSpecified = false;
    for(auto itArg = cmdLineArgs.begin(); itArg != cmdLineArgs.end(); ++itArg)
    {
        auto arg = *itArg;
        if (arg.startsWith("-config="))
        {
            QFile configFile(arg.mid(strlen("-config=")));
            if(!configFile.exists())
            {
                error = "Router configuration file does not exist";
                return false;
            }
            configFile.open(QIODevice::ReadOnly | QIODevice::Text);
            if(!RoutingConfiguration::parseConfiguration(&configFile, cfg.routingConfig))
            {
                error = "Bad router configuration";
                return false;
            }
            configFile.close();
            wasRouterConfigSpecified = true;
        }
        else if (arg.startsWith("-verbose"))
        {
            cfg.verbose = true;
        }
        else if (arg.startsWith("-obfsDir="))
        {
            QDir obfRoot(arg.mid(strlen("-obfsDir=")));
            if(!obfRoot.exists())
            {
                error = "OBF directory does not exist";
                return false;
            }
            lookForObfs(obfRoot, cfg.obfs);
            wasObfRootSpecified = true;
        }
        else if (arg.startsWith("-start="))
        {
            auto coords = arg.mid(strlen("-start=")).split(QChar(';'));
            cfg.startLatitude = coords[0].toDouble();
            cfg.startLongitude = coords[1].toDouble();
        }
        else if (arg.startsWith("-end="))
        {
            auto coords = arg.mid(strlen("-end=")).split(QChar(';'));
            cfg.endLatitude = coords[0].toDouble();
            cfg.endLongitude = coords[1].toDouble();
        }
    }

    if(!wasObfRootSpecified)
        lookForObfs(QDir::current(), cfg.obfs);
    if(!wasRouterConfigSpecified)
        RoutingConfiguration::loadDefault(cfg.routingConfig);
    
    return true;
}
