#ifndef _OSMAND_CORE_TILE_DB_H_
#define _OSMAND_CORE_TILE_DB_H_

#include <OsmAndCore/stdlib_common.h>
#include <array>

#include <OsmAndCore/QtExtensions.h>
#include <QDir>
#include <QFile>
#include <QString>
#include <QMutex>
#include <QSqlDatabase>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd {

    //TODO: total shit, refactor or remove. incompatible with resources model, see obfscollection
    class OSMAND_CORE_API TileDB
    {
    public:
    private:
    protected:
        mutable QMutex _indexMutex;
        QSqlDatabase _indexDb;

        bool openIndex();
    public:
        TileDB(const QDir& dataPath, const QString& indexFilename = {});
        virtual ~TileDB();

        const QDir dataPath;
        const QString indexFilename;

        bool rebuildIndex();
        bool obtainTileData(const TileId tileId, const ZoomLevel zoom, QByteArray& data);
    };

}

#endif // !defined(_OSMAND_CORE_TILE_DB_H_)
