/**
 * @file
 *
 * @section LICENSE
 *
 * OsmAnd - Android navigation software based on OSM maps.
 * Copyright (C) 2010-2014  OsmAnd Authors listed in AUTHORS file
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

#ifndef _OSMAND_CORE_Q_IO_DEVICE_INPUT_STREAM_H_
#define _OSMAND_CORE_Q_IO_DEVICE_INPUT_STREAM_H_

#include <memory>

#include <OsmAndCore/QtExtensions.h>
#include <QIODevice>

#include <OsmAndCore.h>
#include <google/protobuf/io/zero_copy_stream.h>

namespace OsmAnd
{
    namespace gpb = google::protobuf;

    /**
    Implementation of input stream for Google Protobuf via QIODevice
    */
    class OSMAND_CORE_API QIODeviceInputStream : public gpb::io::ZeroCopyInputStream
    {
    private:
        GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(QIODeviceInputStream);

        //! Pointer to I/O device
        const std::shared_ptr<QIODevice> _device;

        //! Device size
        const qint64 _deviceSize;

        //! Buffer
        uint8_t* const _buffer;

        //! Buffer size
        const size_t _bufferSize;

        //! Should close on destruction?
        const bool _closeOnDestruction;

        //! Constants
        enum {
            DefaultBufferSize = 4 * 1024, // 4Kb
        };
    protected:
    public:
        //! Ctor
        QIODeviceInputStream(const std::shared_ptr<QIODevice>& device, const size_t bufferSize = DefaultBufferSize);

        //! Dtor
        virtual ~QIODeviceInputStream();

        virtual bool Next(const void** data, int* size);
        virtual void BackUp(int count);
        virtual bool Skip(int count);
        virtual gpb::int64 ByteCount() const;
    };

} // namespace OsmAnd

#endif // _OSMAND_CORE_Q_IO_DEVICE_INPUT_STREAM_H_
