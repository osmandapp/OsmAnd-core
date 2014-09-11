#ifndef _OSMAND_CORE_MAP_STYLE_H_
#define _OSMAND_CORE_MAP_STYLE_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QIODevice>
#include <QByteArray>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/MapStyleBuiltinValueDefinitions.h>

namespace OsmAnd
{
    class IMapStylesCollection;
    class MapStyleEvaluator;
    class MapStyleValueDefinition;

    enum class MapStyleRuleset
    {
        Invalid = -1,

        Point,
        Polyline,
        Polygon,
        Text,
        Order,
    };

    class MapStyle_P;
    class OSMAND_CORE_API MapStyle
    {
        Q_DISABLE_COPY_AND_MOVE(MapStyle);
    private:
        PrivateImplementation<MapStyle_P> _p;
    protected:
    public:
        MapStyle(
            const std::shared_ptr<const IMapStylesCollection>& const collection,
            const QString& name,
            const std::shared_ptr<QIODevice>& source);
        MapStyle(
            /*const std::shared_ptr<IMapStylesCollection>&*/const IMapStylesCollection* const collection,
            const QString& name,
            const QString& fileName);
        MapStyle(
            /*const std::shared_ptr<IMapStylesCollection>&*/const IMapStylesCollection* const collection,
            const QString& fileName);
        virtual ~MapStyle();

        /*const std::shared_ptr<const IMapStylesCollection>&*/const IMapStylesCollection* const collection;

        bool isMetadataLoaded() const;
        bool loadMetadata();

        const QString& title;
        const QString& name;
        const QString& parentName;

        bool isStandalone() const;

        bool isLoaded() const;
        bool load();

        bool resolveValueDefinition(const QString& name, std::shared_ptr<const MapStyleValueDefinition>& outDefinition) const;
        bool resolveAttribute(const QString& name, std::shared_ptr<const MapStyleRule>& outAttribute) const;

        void dump(const QString& prefix = QString::null) const;
        void dump(const MapStyleRulesetType type, const QString& prefix = QString::null) const;

        static std::shared_ptr<const MapStyleBuiltinValueDefinitions> getBuiltinValueDefinitions();

    friend class OsmAnd::MapStyle_P;
    friend class OsmAnd::MapStyleEvaluator;
    friend class OsmAnd::MapStyleEvaluator_P;
    friend class OsmAnd::MapStyleNode;
    friend class OsmAnd::MapPresentationEnvironment_P;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLE_H_)
