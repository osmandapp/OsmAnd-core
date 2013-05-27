#ifndef OSMANDSETTINGS_H
#define OSMANDSETTINGS_H

#include <memory>
#include <QString>
#include <QSettings>
#include <OsmAndCore.h>
#include <QList>
#include <QMap>

namespace OsmAnd {



// TODO consider to move to separate entity
class OSMAND_CORE_API ApplicationMode;
typedef std::shared_ptr<ApplicationMode> AppMode;

class OSMAND_CORE_API ApplicationMode {
    public :
        const QString defaultName;
        const QString _id;
        ApplicationMode(const ApplicationMode&);
        ApplicationMode(QString name, QString _id);


    public:
        static AppMode DEFAULT;
        static AppMode CAR;
        static AppMode BICYCLE;
        static AppMode PEDESTRIAN;
        //
        static QList<AppMode > getApplicationModes();
};

class OSMAND_CORE_API OsmAndSettingsBase {
public:
    QSettings _globalSettings;
    AppMode APP_MODE;
    virtual ~OsmAndSettingsBase(){}
};

class  OSMAND_CORE_API OsmAndPreference {
private :
    OsmAndSettingsBase*_settings;

    QString _id;
    bool _global;
    QMap<AppMode, QVariant> _defaultValues;
    QVariant _defaultValue;

    bool _cache;
    // don't cache for now
    // T cachedValue;
    // Object cachedPreference;
protected :
    OsmAndPreference(OsmAndSettingsBase* s,const char* id, QVariant defaultValue, bool global = true):
        _settings(s), _defaultValue(defaultValue), _id(id), _global(global){}

    void makeGlobal() { _global = true; }
    void makeProfile() { _global = false; }
    void cache() { _cache = true; }
    friend class OsmAndSettingsBase;
    friend class OsmAndSettings;

public:
    QString getId() { return _id;}

    QVariant getDefaultValue();

    bool present();

    QVariant get();

    bool set(const QVariant& obj);

    bool setModeValue(AppMode m, const QVariant& obj);

    QVariant getModeValue(AppMode m);

};

class OSMAND_CORE_API OsmAndSettings : public OsmAndSettingsBase {
public:
    OsmAndSettings();
    OsmAndPreference APPLICATION_DIRECTORY; // string
    OsmAndPreference MAP_SCALE; // float
    OsmAndPreference MAP_SHOW_LATITUDE; // float
    OsmAndPreference MAP_SHOW_LONGITUDE; // float
    OsmAndPreference MAP_SHOW_ZOOM; // float

    OsmAndPreference TARGET_LATITUDE; // float
    OsmAndPreference TARGET_LONGITUDE; // float
    virtual ~OsmAndSettings(){}
};






}
#endif // OSMANDSETTINGS_H
