/*
 * OsmAndApplication.h
 *
 *  Created on: Mar 15, 2013
 *      Author: victor
 */

#ifndef OSMANDAPPLICATION_H_
#define OSMANDAPPLICATION_H_

#include <memory>
#include <QThreadPool>
#include <QMap>

#include <OsmAndCore.h>
#include <OsmAndSettings.h>


namespace OsmAnd {

struct OSMAND_CORE_API OsmAndTask {

	virtual int getPriority() {
		return 1;
	}

    virtual void run() = 0;

    virtual const QString& getDescription() = 0;

    virtual ~OsmAndTask() {}

    virtual void onPostExecute() {}

    virtual void onPreExecute() {}
};


class OSMAND_CORE_API OsmAndApplication {
private:
    std::shared_ptr<OsmAndSettings> _settings;
    QMap<QString, std::shared_ptr<QThreadPool> > threadPoolsFamily;
	OsmAndApplication();
public:
    QMap<QString, QList<std::shared_ptr<OsmAndTask> > > running;

	virtual ~OsmAndApplication();

	static std::shared_ptr<OsmAndApplication> getAndInitializeApplication();

    const QString& getVersion() { return "0.1"; }

    inline std::shared_ptr<OsmAndSettings> getSettings() { return _settings; }

    // Threads
    const QString& getRunningFamily();

    void submitTask(const QString& family, std::shared_ptr<OsmAndTask> task, int maximumConcurrent = 1);

};

} /* namespace OsmAnd */
#endif /* OSMANDAPPLICATION_H_ */
