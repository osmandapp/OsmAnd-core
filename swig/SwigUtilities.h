#ifndef _SWIG_UTILITIES_H_
#define _SWIG_UTILITIES_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QByteArray>
#include <QFile>

#include <SkBitmap.h>

namespace OsmAnd
{
    struct SwigUtilities
    {
        inline static QByteArray readEntireFile(const QString& filename)
        {
            QFile file(filename);
            if (!file.open(QIODevice::ReadOnly))
                return QByteArray();

            const auto data = file.readAll();

            file.close();

            return data;
        }

        inline static QByteArray readPartOfFile(const QString& filename, const size_t offset, const size_t length)
        {
            QFile file(filename);
            if (!file.open(QIODevice::ReadOnly))
                return QByteArray();

            if (!file.seek(offset))
                return QByteArray();

            QByteArray data;
            data.resize(length);

            auto pData = data.data();
            size_t totalBytesRead = 0;
            while (totalBytesRead < length)
            {
                const auto bytesRead = file.read(pData, length - totalBytesRead);
                if (bytesRead < 0)
                    break;
                pData += bytesRead;
                totalBytesRead += static_cast<size_t>(bytesRead);
            }
            file.close();

            if (totalBytesRead != length)
                return QByteArray();
            return data;
        }

        inline static QByteArray emptyQByteArray()
        {
            return QByteArray();
        }

        inline static QByteArray qDecompress(const QByteArray& compressedData)
        {
            return qUncompress(compressedData);
        }

        inline static std::shared_ptr<const SkBitmap> createSkBitmapARGB888With(
            const unsigned int width,
            const unsigned int height,
            const uint8_t* const pBuffer,
            const size_t bufferSize)
        {
            const std::shared_ptr<SkBitmap> bitmap(new SkBitmap());

            bitmap->setConfig(SkBitmap::kARGB_8888_Config, width, height);
            bitmap->allocPixels();
            if (bitmap->getSize() < bufferSize)
                return nullptr;
            memcpy(bitmap->getPixels(), pBuffer, bufferSize);

            return bitmap;
        }

    private:
        SwigUtilities();
        ~SwigUtilities();
    };
}

#endif // !defined(_SWIG_UTILITIES_H_)
