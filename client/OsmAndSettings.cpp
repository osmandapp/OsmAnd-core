#include "OsmAndSettings.h"
namespace OsmAnd {
ApplicationMode::ApplicationMode(QString name, QString id): defaultName(name), _id(id) {
}
ApplicationMode::ApplicationMode(const ApplicationMode& am): defaultName(am.defaultName), _id(am._id) {
}

AppMode ApplicationMode::DEFAULT(new ApplicationMode("Default", "DEFAULT"));
AppMode ApplicationMode::PEDESTRIAN(new ApplicationMode("Pedestrian", "PEDESTRIAN"));
AppMode ApplicationMode::CAR(new ApplicationMode("Car", "CAR"));
AppMode ApplicationMode::BICYCLE(new ApplicationMode("Bicycle", "BICYCLE"));
QList<AppMode> ApplicationMode::getApplicationModes() {
    QList<AppMode>  list;
    list.append(DEFAULT);
    list.append(CAR);
    list.append(BICYCLE);
    list.append(PEDESTRIAN);
    return list;
}


OsmAndSettings::OsmAndSettings():
    APPLICATION_DIRECTORY(this, "application_directory", QString("")),
    TILE_SOURCE(this, "tile_source", QString("")),
    MAP_SCALE(this, "map_scale", 1.0),
    MAP_SHOW_LATITUDE(this, "show_lat", 0),
    MAP_SHOW_LONGITUDE(this, "show_lon", 0),
    MAP_SHOW_ZOOM(this, "show_zoom", 5),

    TARGET_LATITUDE(this, "target_lat", -360),
    TARGET_LONGITUDE(this, "target_lon", -360),
    START_LATITUDE(this, "start_lat", -360),
    START_LONGITUDE(this, "start_lon", -360)
{
    APP_MODE = ApplicationMode::DEFAULT;
    APPLICATION_DIRECTORY.cache();
    MAP_SCALE.cache();
    TILE_SOURCE.cache();
    TARGET_LATITUDE.cache();
    TARGET_LONGITUDE.cache();
    START_LATITUDE.cache();
    START_LONGITUDE.cache();

}


QVariant OsmAndPreference::getDefaultValue() {
    if(!_global && _defaultValues.contains(_settings->APP_MODE))
    {
        return  _defaultValues.value(_settings->APP_MODE);
    }
    return _defaultValue;
 }

QVariant OsmAndPreference::get() {
    if(_global) {
        if(_settings->_globalSettings.contains(_id)) { return _settings->_globalSettings.value(_id); }
    } else {
        return getModeValue(_settings->APP_MODE);
    }
    return getDefaultValue();
}
bool OsmAndPreference::present() {
    if(_global) {
        return _settings->_globalSettings.contains(_id);
    } else {
        _settings->_globalSettings.beginGroup(_settings->APP_MODE->_id);
        bool pr = _settings->_globalSettings.contains(_id);
        _settings->_globalSettings.beginGroup(_settings->APP_MODE->_id);
        return pr;
    }
}

bool OsmAndPreference::set(const QVariant& obj) {
    if(!_global) return setModeValue(_settings->APP_MODE, obj);
    _settings->_globalSettings.setValue(_id, obj);
    return true;
}

bool OsmAndPreference::setModeValue(AppMode m, const QVariant& obj){
    _settings->_globalSettings.beginGroup(m->_id);
    _settings->_globalSettings.setValue(_id, obj);
    _settings->_globalSettings.endGroup();
    return true;
}

QVariant OsmAndPreference::getModeValue(AppMode m) {
    _settings->_globalSettings.beginGroup(m->_id);
    if(_settings->_globalSettings.contains(_id))
    {
        QVariant v = _settings->_globalSettings.value(_id);
        _settings->_globalSettings.endGroup();
        return v;
    }
    _settings->_globalSettings.endGroup();
    if(!_global && _defaultValues.contains(m))
    {
        return  _defaultValues.contains(m);
    }
    return _defaultValue;
}

}

