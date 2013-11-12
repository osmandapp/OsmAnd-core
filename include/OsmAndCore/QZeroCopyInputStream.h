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

#ifndef __Q_ZERO_COPY_INPUT_STREAM_H_
#define __Q_ZERO_COPY_INPUT_STREAM_H_

#include <memory>

#include <QIODevice>

#include <OsmAndCore.h>
#include <google/protobuf/io/zero_copy_stream.h>

namespace OsmAnd {

    namespace gpb = google::protobuf;

    /**
    Implementation of zero-copy input stream for Google Protobuf via QIODevice
    */
    class OSMAND_CORE_API QZeroCopyInputStream : public gpb::io::ZeroCopyInputStream
    {
    private:
        GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(QZeroCopyInputStream);

        //! Pointer to I/O device
        const std::shared_ptr<QIODevice> _device;

        // Buffer
        uint8_t* const _buffer;

        //! Should close on destruction?
        const bool _closeOnDestruction;

        //! Constants
        enum {
            BufferSize = 64 * 1024, // 64Kb
        };
    protected:
    public:
        //! Ctor
        QZeroCopyInputStream(const std::shared_ptr<QIODevice>& device);

        //! Dtor
        virtual ~QZeroCopyInputStream();

        virtual bool Next(const void** data, int* size);
        virtual void BackUp(int count);
        virtual bool Skip(int count);
        virtual gpb::int64 ByteCount() const;
    };

} // namespace OsmAnd

#endif // __Q_ZERO_COPY_INPUT_STREAM_H_