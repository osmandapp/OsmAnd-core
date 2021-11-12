#ifndef _SWIG_UTILITIES_H_
#define _SWIG_UTILITIES_H_

#include <OsmAndCore/stdlib_common.h>
#include <OsmAndCore/Map/MapMarker.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QByteArray>
#include <QFile>

#include <SkImage.h>

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

#ifdef SWIG
        %apply (char *BYTE, size_t LENGTH) { (const char* const pBuffer, const size_t bufferSize) }
#endif // SWIG
        inline static QByteArray createQByteArrayAsCopyOf(const char* const pBuffer, const size_t bufferSize)
        {
            return QByteArray(reinterpret_cast<const char*>(pBuffer), bufferSize);
        }

        inline static QByteArray emptyQByteArray()
        {
            return QByteArray();
        }

        inline static QByteArray qDecompress(const QByteArray& compressedData)
        {
            return qUncompress(compressedData);
        }

        inline static OsmAnd::MapMarker::OnSurfaceIconKey getOnSurfaceIconKey(const int value)
        {
            return reinterpret_cast<OsmAnd::MapMarker::OnSurfaceIconKey>(value);
        }

#ifdef SWIG
        %apply (char *BYTE, size_t LENGTH) { (const char* const pBuffer, const size_t bufferSize) }
#endif // SWIG
        inline static std::shared_ptr<const SkBitmap> createSkBitmapARGB888With(
            const unsigned int width,
            const unsigned int height,
            const char* const pBuffer,
            const size_t bufferSize)
        {
            const std::shared_ptr<SkBitmap> bitmap(new SkBitmap());

            if (!bitmap->tryAllocPixels(SkImageInfo::Make(
                width,
                height,
                SkColorType::kRGBA_8888_SkColorType,
                SkAlphaType::kUnpremul_SkAlphaType)))
            {
                return nullptr;
            }
            if (bitmap->computeByteSize() < bufferSize)
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
