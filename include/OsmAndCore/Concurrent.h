/**
* @file
*
* @section LICENSE
*
* OsmAnd - Android navigation software based on OSM maps.
* Copyright (C) 2010-2013  OsmAnd Authors listed in AUTHORS file
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __CONCURRENT_H_
#define __CONCURRENT_H_

#include <stdint.h>
#include <memory>
#include <functional>

#include <QThreadPool>
#include <QEventLoop>
#include <QRunnable>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd {

    class OSMAND_CORE_API Concurrent
    {
    public:
        class OSMAND_CORE_API Task : public QRunnable
        {
        public:
            typedef std::function<bool (const Task*)> PreExecuteSignature;
            typedef std::function<void (const Task*, QEventLoop& eventLoop)> ExecuteSignature;
            typedef std::function<void (const Task*)> PostExecuteSignature;
        private:
        protected:
            volatile bool _isCancellationRequested;
        public:
            Task(ExecuteSignature executeMethod, PreExecuteSignature preExecuteMethod = nullptr, PostExecuteSignature postExecuteMethod = nullptr);
            virtual ~Task();

            const PreExecuteSignature preExecute;
            const ExecuteSignature execute;
            const PostExecuteSignature postExecute;

            void requestCancellation();
            bool isCancellationRequested() const;

            virtual void run();
        };
    private:
    protected:
        Concurrent();

        static std::unique_ptr<Concurrent> _instance;
    public:
        virtual ~Concurrent();

        static const std::unique_ptr<Concurrent>& instance();

        const std::unique_ptr<QThreadPool> localStoragePool;
        const std::unique_ptr<QThreadPool> networkPool;
    };

} // namespace OsmAnd

#endif // __CONCURRENT_H_
