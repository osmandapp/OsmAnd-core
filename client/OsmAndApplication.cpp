/*
 * OsmAndApplication.cpp
 *
 *  Created on: Mar 15, 2013
 *      Author: victor
 */

#include "OsmAndApplication.h"

namespace OsmAnd {

OsmAndApplication::OsmAndApplication() : _settings(std::shared_ptr<OsmAndSettings>(new OsmAndSettings()))
{
}

OsmAndApplication::~OsmAndApplication() {
}

struct TaskRunnableWraper : public QRunnable {
    std::shared_ptr<OsmAnd::OsmAndTask>& task;
    const QString& family;
    const OsmAndApplication* app;
    TaskRunnableWraper(const OsmAndApplication* app, const QString& family, std::shared_ptr<OsmAndTask> t) :
        app(app), task(t), family(family) {}

    virtual void run() {
        QList<std::shared_ptr<OsmAndTask> > l =  app->running[family];
        //TODO run in another thread
        task->onPreExecute();
        l.push_back(task);

        task->run();

        //TODO run in another thread
        l.removeOne(task);
        task->onPostExecute();
    }
};

std::shared_ptr<OsmAndApplication> globalApplicationRef;
std::shared_ptr<OsmAndApplication> OsmAndApplication::getAndInitializeApplication() {
    if (globalApplicationRef.get() == nullptr) {
        globalApplicationRef = std::shared_ptr<OsmAndApplication>(new OsmAndApplication());
    }

    return globalApplicationRef;
}

const QString& OsmAndApplication::getRunningFamily() {
    auto a = this->threadPoolsFamily.begin();
    while(a != this->threadPoolsFamily.end()) {
        if((*a)->activeThreadCount() > 0 && running[a.key()].size() > 0) {
            return a.key();
        }
    }
    return "";
}

void OsmAndApplication::submitTask(const QString& family, std::shared_ptr<OsmAndTask> task, int maximumConcurrent) {
    std::shared_ptr<QThreadPool> p = this->threadPoolsFamily[family];
    if(!p) {
        p = std::shared_ptr<QThreadPool> (new QThreadPool());
        this->threadPoolsFamily[family] = p;
    }
    p->setMaxThreadCount(maximumConcurrent);
    // FIXME
    //p->start(new TaskRunnableWraper(this, family, task), task->getPriority());
}

} /* namespace OsmAnd */
