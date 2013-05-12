/*
 * OsmAndApplication.h
 *
 *  Created on: Mar 15, 2013
 *      Author: victor
 */

#ifndef OSMANDAPPLICATION_H_
#define OSMANDAPPLICATION_H_

#include <memory>
#include <qthreadpool.h>
#include <qmap.h>
#include <OsmAndCore.h>
#include "OsmAndSettings.h"


namespace OsmAnd {

struct OSMAND_CORE_API OsmAndTask {

	virtual int getPriority() {
		return 1;
	}

	virtual QString& getDescription() = 0;

    virtual ~OsmAndTask() {}
};


class OSMAND_CORE_API OsmAndApplication {
private:
    std::shared_ptr<OsmAndSettings> _settings;
	OsmAndApplication();
public:
	virtual ~OsmAndApplication();

	static std::shared_ptr<OsmAndApplication> getAndInitializeApplication();

    QString getVersion() { return "0.1"; }

    std::shared_ptr<OsmAndSettings> getSettings() { return _settings; }

};

} /* namespace OsmAnd */
#endif /* OSMANDAPPLICATION_H_ */
