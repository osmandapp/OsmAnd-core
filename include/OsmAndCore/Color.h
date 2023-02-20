#ifndef _OSMAND_CORE_COLOR_H_
#define _OSMAND_CORE_COLOR_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>

namespace OsmAnd
{
    union FColorARGB
    {
        inline FColorARGB()
            : a(1.0f)
            , r(1.0f)
            , g(1.0f)
            , b(1.0f)
        {
        }

        inline FColorARGB(const float a_, const float r_, const float g_, const float b_)
            : a(a_)
            , r(r_)
            , g(g_)
            , b(b_)
        {
        }

#if !defined(SWIG)
        float value[4];
        struct
        {
            float a;
            float r;
            float g;
            float b;
        };
#else
        // Fake unwrap for SWIG
        float a, r, g, b;
#endif // !defined(SWIG)

#if !defined(SWIG)
        inline bool operator==(const FColorARGB& other) const
        {
            return
                qFuzzyCompare(a, other.a) &&
                qFuzzyCompare(r, other.r) &&
                qFuzzyCompare(g, other.g) &&
                qFuzzyCompare(b, other.b);
        }

        inline bool operator!=(const FColorARGB& other) const
        {
            return
                !qFuzzyCompare(a, other.a) ||
                !qFuzzyCompare(r, other.r) ||
                !qFuzzyCompare(g, other.g) ||
                !qFuzzyCompare(b, other.b);
        }
#endif // !defined(SWIG)

        inline FColorARGB withAlpha(const float newAlpha) const
        {
            return FColorARGB(newAlpha, r, g, b);
        }

        inline FColorARGB& setAlpha(const float newAlpha)
        {
            a = newAlpha;
            return *this;
        }

        inline bool isTransparent() const
        {
            return qFuzzyIsNull(a);
        }

        float getRGBDelta(const FColorARGB& other) const;
    };

    union FColorRGBA
    {
        inline FColorRGBA()
            : r(1.0f)
            , g(1.0f)
            , b(1.0f)
            , a(1.0f)
        {
        }

        inline FColorRGBA(const float r_, const float g_, const float b_, const float a_)
            : r(r_)
            , g(g_)
            , b(b_)
            , a(a_)
        {
        }

#if !defined(SWIG)
        float value[4];
        struct
        {
            float r;
            float g;
            float b;
            float a;
        };
#else
        // Fake unwrap for SWIG
        float r, g, b, a;
#endif // !defined(SWIG)

#if !defined(SWIG)
        inline bool operator==(const FColorRGBA& other) const
        {
            return
                qFuzzyCompare(r, other.r) &&
                qFuzzyCompare(g, other.g) &&
                qFuzzyCompare(b, other.b) &&
                qFuzzyCompare(a, other.a);
        }

        inline bool operator!=(const FColorRGBA& other) const
        {
            return
                !qFuzzyCompare(a, other.a) ||
                !qFuzzyCompare(r, other.r) ||
                !qFuzzyCompare(g, other.g) ||
                !qFuzzyCompare(b, other.b);
        }
#endif // !defined(SWIG)

        inline FColorRGBA withAlpha(const float newAlpha) const
        {
            return FColorRGBA(r, g, b, newAlpha);
        }

        inline FColorRGBA& setAlpha(const float newAlpha)
        {
            a = newAlpha;
            return *this;
        }

        inline bool isTransparent() const
        {
            return qFuzzyIsNull(a);
        }

        float getRGBDelta(const FColorRGBA& other) const;
    };

    union FColorRGB
    {
        inline FColorRGB()
            : r(1.0f)
            , g(1.0f)
            , b(1.0f)
        {
        }

        inline FColorRGB(const float r_, const float g_, const float b_)
            : r(r_)
            , g(g_)
            , b(b_)
        {
        }

        explicit inline FColorRGB(const FColorARGB& other)
            : r(other.r)
            , g(other.g)
            , b(other.b)
        {
        }

        explicit inline FColorRGB(const FColorRGBA& other)
            : r(other.r)
            , g(other.g)
            , b(other.b)
        {
        }

#if !defined(SWIG)
        float value[3];
        struct
        {
            float r;
            float g;
            float b;
        };
#else
        // Fake unwrap for SWIG
        float r, g, b;
#endif // !defined(SWIG)

#if !defined(SWIG)
        inline bool operator==(const FColorRGB& other) const
        {
            return
                qFuzzyCompare(r, other.r) &&
                qFuzzyCompare(g, other.g) &&
                qFuzzyCompare(b, other.b);
        }

        inline bool operator!=(const FColorRGB& other) const
        {
            return
                !qFuzzyCompare(r, other.r) ||
                !qFuzzyCompare(g, other.g) ||
                !qFuzzyCompare(b, other.b);
        }

        inline operator FColorARGB() const
        {
            return FColorARGB(1.0f, r, g, b);
        }

        inline operator FColorRGBA() const
        {
            return FColorRGBA(r, g, b, 1.0f);
        }
#endif // !defined(SWIG)

        inline FColorARGB withAlpha(const float alpha) const
        {
            return FColorARGB(alpha, r, g, b);
        }

        inline float getRGBDelta(const FColorRGB& other) const
        {
            const auto redmean = (r + other.r) * 0.5f;
            const auto rDiff = r - other.r;
            const auto gDiff = g - other.g;
            const auto bDiff = b - other.b;

            const auto rCoeff = 2.0f + redmean;
            const auto gCoeff = 4.0f;
            const auto bCoeff = 2.0f + (1.0f - redmean);

            const auto rDelta = rCoeff * rDiff * rDiff;
            const auto gDelta = gCoeff * gDiff * gDiff;
            const auto bDelta = bCoeff * bDiff * bDiff;

            return rDelta + gDelta + bDelta;
        }
    };

    union ColorARGB
    {
        inline ColorARGB()
            : argb(0xFFFFFFFF)
        {
        }

#if !defined(SWIG)
        static ColorARGB fromSkColor(const SkColor skColor)
        {
            return ColorARGB(
                SkColorGetA(skColor),
                SkColorGetR(skColor),
                SkColorGetG(skColor),
                SkColorGetB(skColor));
        }
#endif // !defined(SWIG)

        explicit inline ColorARGB(const uint32_t argb_)
            : argb(argb_)
        {
        }

        inline ColorARGB(const uint8_t a_, const uint8_t r_, const uint8_t g_, const uint8_t b_)
            : b(b_)
            , g(g_)
            , r(r_)
            , a(a_)
        {
        }

        inline ColorARGB(const FColorARGB& other)
            : b(static_cast<uint8_t>(other.b * 255.0f))
            , g(static_cast<uint8_t>(other.g * 255.0f))
            , r(static_cast<uint8_t>(other.r * 255.0f))
            , a(static_cast<uint8_t>(other.a * 255.0f))
        {
        }

#if !defined(SWIG)
        uint8_t value[4];
        uint32_t argb;
        struct
        {
            uint8_t b;
            uint8_t g;
            uint8_t r;
            uint8_t a;
        };
#else
        // Fake unwrap for SWIG
        uint8_t a, r, g, b;
#endif // !defined(SWIG)

#if !defined(SWIG)
        inline bool operator==(const ColorARGB& other) const
        {
            return argb == other.argb;
        }

        inline bool operator!=(const ColorARGB& other) const
        {
            return argb != other.argb;
        }

        inline operator FColorARGB() const
        {
            return FColorARGB(a / 255.0f, r / 255.0f, g / 255.0f, b / 255.0f);
        }

        inline SkColor toSkColor() const
        {
            return SkColorSetARGB(a, r, g, b);
        }
#endif // !defined(SWIG)

        inline ColorARGB withAlpha(const uint8_t newAlpha) const
        {
            return ColorARGB(newAlpha, r, g, b);
        }

        inline ColorARGB& setAlpha(const uint8_t newAlpha)
        {
            a = newAlpha;
            return *this;
        }

        inline bool isTransparent() const
        {
            return (a == 0);
        }

        float getRGBDelta(const ColorARGB& other) const;

        inline QString toString() const
        {
            if (a == 0)
                return QLatin1Char('#') + QString::number(argb, 16).right(6);
            return QLatin1Char('#') + QString::number(argb, 16);
        }
    };

    union ColorRGBA
    {
        inline ColorRGBA()
            : rgba(0xFFFFFFFF)
        {
        }

#if !defined(SWIG)
        static ColorRGBA fromSkColor(const SkColor skColor)
        {
            return ColorRGBA(SkColorGetR(skColor), SkColorGetG(skColor), SkColorGetB(skColor), SkColorGetA(skColor));
        }
#endif // !defined(SWIG)

        explicit inline ColorRGBA(const uint32_t rgba_)
            : rgba(rgba_)
        {
        }

        inline ColorRGBA(const uint8_t r_, const uint8_t g_, const uint8_t b_, const uint8_t a_)
            : a(a_)
            , b(b_)
            , g(g_)
            , r(r_)
        {
        }

        inline ColorRGBA(const FColorRGBA& other)
            : a(static_cast<uint8_t>(other.a * 255.0f))
            , b(static_cast<uint8_t>(other.b * 255.0f))
            , g(static_cast<uint8_t>(other.g * 255.0f))
            , r(static_cast<uint8_t>(other.r * 255.0f))
        {
        }

#if !defined(SWIG)
        uint8_t value[4];
        uint32_t rgba;
        struct
        {
            uint8_t a;
            uint8_t b;
            uint8_t g;
            uint8_t r;
        };
#else
        // Fake unwrap for SWIG
        uint8_t r, g, b, a;
#endif // !defined(SWIG)

#if !defined(SWIG)
        inline bool operator==(const ColorRGBA& other) const
        {
            return rgba == other.rgba;
        }

        inline bool operator!=(const ColorRGBA& other) const
        {
            return rgba != other.rgba;
        }

        inline operator FColorRGBA() const
        {
            return FColorRGBA(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
        }

        inline SkColor toSkColor() const
        {
            return SkColorSetARGB(a, r, g, b);
        }
#endif // !defined(SWIG)

        inline ColorRGBA withAlpha(const uint8_t newAlpha) const
        {
            return ColorRGBA(r, g, b, newAlpha);
        }

        inline ColorRGBA& setAlpha(const uint8_t newAlpha)
        {
            a = newAlpha;
            return *this;
        }

        inline bool isTransparent() const
        {
            return (a == 0);
        }

        float getRGBDelta(const ColorRGBA& other) const;
        
        inline QString toString() const
        {
            if (a == 0)
                return QLatin1Char('#') + QString::number(rgba, 16).right(6);
            return QLatin1Char('#') + QString::number(rgba, 16);
        }
    };

    union ColorHSV;

    union OSMAND_CORE_API ColorRGB
    {
        inline ColorRGB()
            : r(255)
            , g(255)
            , b(255)
        {
        }

#if !defined(SWIG)
        inline static ColorRGB fromSkColor(const SkColor skColor)
        {
            return ColorRGB(
                SkColorGetR(skColor),
                SkColorGetG(skColor),
                SkColorGetB(skColor));
        }
#endif // !defined(SWIG)

        inline ColorRGB(const uint8_t r_, const uint8_t g_, const uint8_t b_)
            : r(r_)
            , g(g_)
            , b(b_)
        {
        }

        explicit inline ColorRGB(const ColorARGB& other)
            : r(other.r)
            , g(other.g)
            , b(other.b)
        {
        }

        explicit inline ColorRGB(const ColorRGBA& other)
            : r(other.r)
            , g(other.g)
            , b(other.b)
        {
        }

        inline ColorRGB(const FColorRGB& other)
            : r(static_cast<uint8_t>(other.r * 255.0f))
            , g(static_cast<uint8_t>(other.g * 255.0f))
            , b(static_cast<uint8_t>(other.b * 255.0f))
        {
        }

#if !defined(SWIG)
        uint8_t value[3];
        struct
        {
            uint8_t r;
            uint8_t g;
            uint8_t b;
        };
#else
        // Fake unwrap for SWIG
        uint8_t r, g, b;
#endif // !defined(SWIG)

#if !defined(SWIG)
        inline bool operator==(const ColorRGB& other) const
        {
            return
                r == other.r &&
                g == other.g &&
                b == other.b;
        }

        inline bool operator!=(const ColorRGB& other) const
        {
            return
                r != other.r ||
                g != other.g ||
                b != other.b;
        }

        inline operator ColorARGB() const
        {
            return ColorARGB(255, r, g, b);
        }

        inline operator FColorRGB() const
        {
            return FColorRGB(r / 255.0f, g / 255.0f, b / 255.0f);
        }

        inline SkColor toSkColor() const
        {
            return SkColorSetRGB(r, g, b);
        }
#endif // !defined(SWIG)

        inline ColorARGB withAlpha(const uint8_t alpha) const
        {
            return ColorARGB(alpha, r, g, b);
        }

        inline float getRGBDelta(const ColorRGB& other) const
        {
            const auto redmean = (r + other.r) * 0.5f;
            const auto rDiff = r - other.r;
            const auto gDiff = g - other.g;
            const auto bDiff = b - other.b;

            const auto rCoeff = 2.0f + redmean / 255.0f;
            const auto gCoeff = 4.0f;
            const auto bCoeff = 2.0f + (255.0f - redmean) / 255.0f;

            const auto rDelta = rCoeff * rDiff * rDiff;
            const auto gDelta = gCoeff * gDiff * gDiff;
            const auto bDelta = bCoeff * bDiff * bDiff;

            return rDelta + gDelta + bDelta;
        }

        inline QString toString() const
        {
            const uint32_t value =
                (static_cast<uint32_t>(r) << 16) |
                (static_cast<uint32_t>(g) << 8) |
                (static_cast<uint32_t>(b) << 0);
            return QLatin1Char('#') + QString::number(value, 16).right(6);
        }

        ColorHSV toHSV() const;
    };

    union ColorHSV
    {
        inline ColorHSV()
            : h(0)
            , s(0)
            , v(100)
        {
        }

        inline ColorHSV(const double h_, const double s_, const double v_)
            : h(h_)
            , s(s_)
            , v(v_)
        {
        }

#if !defined(SWIG)
        float value[3];
        struct
        {
            float h;
            float s;
            float v;
        };
#else
        // Fake unwrap for SWIG
        float h, s, v;
#endif // !defined(SWIG)
    };
}

#endif // !defined(_OSMAND_CORE_COLOR_H_)
