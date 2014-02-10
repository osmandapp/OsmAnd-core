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

#endif // !defined(_OSMAND_CORE_Q_IO_DEVICE_INPUT_STREAM_H_)
