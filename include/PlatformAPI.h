/*
 * PlatformAPI.h
 *
 *  Created on: Mar 15, 2013
 *      Author: victor
 */

#ifndef PLATFORMAPI_H_
#define PLATFORMAPI_H_

#include <memory>
#include <qstring.h>

#include <OsmAndCore.h>

namespace OsmAnd {

class OSMAND_CORE_API SettingsPreference {

private:
	SettingsPreference();
public:
	virtual ~SettingsPreference();

	QString& getString(QString& key, QString& defValue);
	float getFloat(QString& key, float defValue);
	bool getBoolean(QString& key, bool defValue);
	int getInt(QString& key, int defValue);
	bool getLong(QString& key, long defValue);

	bool contains(QString& key);

	SettingsPreference& putString(QString& key, QString& value);
	SettingsPreference& putBoolean(QString& key, bool value);
	SettingsPreference& putFloat(QString& key, float value);
	SettingsPreference& putInt(QString& key, int value);
	SettingsPreference& putLong(QString& key, long value);
	SettingsPreference& remove(QString& key);
	bool commit();
};


class OSMAND_CORE_API PlatformAPI {
private:

	PlatformAPI();
public:
	static std::shared_ptr<PlatformAPI> get();

	virtual ~PlatformAPI();
public:
	SettingsPreference& getPreferenceObject(QString& key);
};



} /* namespace OsmAnd */
#endif /* PLATFORMAPI_H_ */
