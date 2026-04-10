#include "ICU.h"
#include "ICU_private.h"
#include "CollatorStringMatcher.h"

#include <cassert>
#include <cstring>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QByteArray>
#include <QElapsedTimer>
#include <QVector>
#include <QThread>
#include <QHash>
#include <QReadWriteLock>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <unicode/uclean.h>
#include <unicode/udata.h>
#include <unicode/ubidi.h>
#include <unicode/ushape.h>
#include <unicode/translit.h>
#include <unicode/brkiter.h>
#include <unicode/coll.h>
#include <unicode/normalizer2.h>
#include "restore_internal_warnings.h"

#include "CoreResourcesEmbeddedBundle.h"
#include "Logging.h"

std::unique_ptr<QByteArray> g_IcuData;
const Transliterator* g_pIcuAnyToLatinTransliterator = nullptr;
const Transliterator* g_pIcuAccentsAndDiacriticsConverter = nullptr;
const BreakIterator* g_pIcuLineBreakIterator = nullptr;
const Normalizer2* g_pIcuNFDNormalizer = nullptr;
const Normalizer2* g_pIcuNFCNormalizer = nullptr;
const Collator* g_pIcuCollator = nullptr;
QReadWriteLock icuResourcesLock;
bool g_icuInitialized = false;

QReadWriteLock collatorsLock;
QHash<Qt::HANDLE, Collator*> collatorsMap;
static std::bitset<65536> s_isUnsafeChar;

// RAII wrapper for ICU Transliterator
using TransliteratorPtr = std::unique_ptr<icu::Transliterator, void(*)(icu::Transliterator*)>;

// Helper to create RAII transliterator clones
inline TransliteratorPtr makeTransliteratorClone(const icu::Transliterator* src) {
    return TransliteratorPtr(src ? src->clone() : nullptr, [](icu::Transliterator* p) { delete p; });
}

// Thread-local caches
thread_local TransliteratorPtr tl_anyToLatin(nullptr, [](icu::Transliterator* p) { delete p; });
thread_local TransliteratorPtr tl_accentsConverter(nullptr, [](icu::Transliterator* p) { delete p; });
thread_local UnicodeString tl_stripDiacriticsDecomposed;
thread_local UnicodeString tl_stripDiacriticsFiltered;
thread_local UnicodeString tl_stripDiacriticsComposed;

#ifndef NDEBUG
OSMAND_CORE_API bool OSMAND_CORE_CALL OsmAnd::ICU::testStripDiacritics()
{
    struct StripDiacriticsTestCase
    {
        const char* name;
        QString input;
        QString expected;
    };

    const auto formatCodePoints = [](const QString& value)
    {
        if (value.isEmpty())
            return QStringLiteral("<empty>");

        QStringList codePoints;
        const auto valueUcs4 = value.toUcs4();
        codePoints.reserve(valueUcs4.size());
        for (const auto codePoint : valueUcs4)
            codePoints.push_back(QStringLiteral("U+%1").arg(codePoint, codePoint > 0xFFFF ? 6 : 4, 16, QLatin1Char('0')).toUpper());
        return codePoints.join(QLatin1Char(' '));
    };

    const auto fromUcs4 = [](const QVector<uint>& codePoints)
    {
        return QString::fromUcs4(codePoints.constData(), codePoints.size());
    };

    const QString supplementaryMnOnly = fromUcs4(QVector<uint>({ 0x1E000 }));
    const QString supplementaryMn = fromUcs4(QVector<uint>({ 'A', 0x1E000 }));
    const QString supplementaryMc = fromUcs4(QVector<uint>({ 'A', 0x1D165 }));

    const QVector<StripDiacriticsTestCase> testCases =
    {
        { "empty", QString(), QString() },
        { "ascii", QStringLiteral("Plain ASCII street 123"), QStringLiteral("Plain ASCII street 123") },
        { "digits-punctuation", QStringLiteral("1234-_. /?!"), QStringLiteral("1234-_. /?!") },
        { "latin-precomposed-e-acute", QStringLiteral("\u00E9"), QStringLiteral("e") },
        { "latin-precomposed-a-ring", QStringLiteral("\u00C5"), QStringLiteral("A") },
        { "latin-creme-brulee", QStringLiteral("Cr\u00E8me Br\u00FBl\u00E9e"), QStringLiteral("Creme Brulee") },
        { "latin-a-circumflex-acute", QStringLiteral("\u1EA5"), QStringLiteral("a") },
        { "latin-decomposed-e-acute", QStringLiteral("e\u0301"), QStringLiteral("e") },
        { "latin-decomposed-a-ring", QStringLiteral("A\u030A"), QStringLiteral("A") },
        { "latin-multi-mark", QStringLiteral("a\u0323\u0301"), QStringLiteral("a") },
        { "latin-unchanged-specials", QStringLiteral("\u00DF\u00E6\u0153\u00F8\u0142"), QStringLiteral("\u00DF\u00E6\u0153\u00F8\u0142") },
        { "greek-tonos", QStringLiteral("\u03AC"), QStringLiteral("\u03B1") },
        { "cyrillic-breve", QStringLiteral("\u0439"), QStringLiteral("\u0438") },
        { "arabic-harakat", QStringLiteral("\u0645\u064F\u062D\u064E\u0645\u064E\u0651\u062F"), QStringLiteral("\u0645\u062D\u0645\u062F") },
        { "hangul-ga", QStringLiteral("\uAC00"), QStringLiteral("\uAC00") },
        { "hangul-han", QStringLiteral("\uD55C"), QStringLiteral("\uD55C") },
        { "hangul-mixed", QStringLiteral("\uD55C\uAE00 \uD14C\uC2A4\uD2B8"), QStringLiteral("\uD55C\uAE00 \uD14C\uC2A4\uD2B8") },
        { "cjk-unchanged", QStringLiteral("\u6771\u4EAC\u6F22\u5B57"), QStringLiteral("\u6771\u4EAC\u6F22\u5B57") },
        { "indic-spacing-mark-aa", QStringLiteral("\u0915\u093E"), QStringLiteral("\u0915\u093E") },
        { "indic-spacing-mark-i", QStringLiteral("\u0915\u093F"), QStringLiteral("\u0915\u093F") },
        { "indic-nonspacing-mark-u", QStringLiteral("\u0915\u0941"), QStringLiteral("\u0915") },
        { "indic-anusvara", QStringLiteral("\u0915\u0902"), QStringLiteral("\u0915") },
        { "indic-nukta", QStringLiteral("\u0915\u093C"), QStringLiteral("\u0915") },
        { "isolated-combining-mark", QStringLiteral("\u0301"), QString() },
        { "leading-combining-mark", QStringLiteral("\u0301a"), QStringLiteral("a") },
        { "trailing-combining-mark", QStringLiteral("a\u0301"), QStringLiteral("a") },
        { "wrapping-combining-marks", QStringLiteral("\u0301a\u0301"), QStringLiteral("a") },
        { "supplementary-mn-mark-only", supplementaryMnOnly, QString() },
        { "supplementary-mn-mark", supplementaryMn, QStringLiteral("A") },
        { "supplementary-mc-mark", supplementaryMc, supplementaryMc },
        { "mixed-scripts", QStringLiteral("Cr\u00E8me, \u0645\u064F\u062D\u064E\u0645\u064E\u0651\u062F, \uD55C\uAE00, \u0915\u093E!"),
            QStringLiteral("Creme, \u0645\u062D\u0645\u062F, \uD55C\uAE00, \u0915\u093E!") }
    };

    auto mismatches = 0;
    for (const auto& testCase : testCases)
    {
        const auto actual = OsmAnd::ICU::stripDiacritics(testCase.input);
        if (actual == testCase.expected)
            continue;

        mismatches++;
        const auto printableInput = testCase.input.isEmpty() ? QStringLiteral("<empty>") : testCase.input;
        const auto printableExpected = testCase.expected.isEmpty() ? QStringLiteral("<empty>") : testCase.expected;
        const auto printableActual = actual.isEmpty() ? QStringLiteral("<empty>") : actual;
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning,
            "testStripDiacritics mismatch [%s]\n  input:    %s\n  input cp:    %s\n  expected: %s\n  expected cp: %s\n  actual:   %s\n  actual cp:   %s",
            testCase.name,
            qPrintable(printableInput),
            qPrintable(formatCodePoints(testCase.input)),
            qPrintable(printableExpected),
            qPrintable(formatCodePoints(testCase.expected)),
            qPrintable(printableActual),
            qPrintable(formatCodePoints(actual)));
    }

    if (mismatches == 0)
    {
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "testStripDiacritics passed: %d cases", testCases.size());
        return true;
    }

    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "testStripDiacritics failed: %d/%d cases mismatched", mismatches, testCases.size());
    return false;
}

OSMAND_CORE_API bool OSMAND_CORE_CALL OsmAnd::ICU::testStripDiacriticsPerformance()
{
    struct StripDiacriticsPerformanceCase
    {
        const char* name;
        QVector<QString> inputs;
        int iterations;
    };

    const auto formatCodePoints = [](const QString& value)
    {
        if (value.isEmpty())
            return QStringLiteral("<empty>");

        QStringList codePoints;
        const auto valueUcs4 = value.toUcs4();
        codePoints.reserve(valueUcs4.size());
        for (const auto codePoint : valueUcs4)
            codePoints.push_back(QStringLiteral("U+%1").arg(codePoint, codePoint > 0xFFFF ? 6 : 4, 16, QLatin1Char('0')).toUpper());
        return codePoints.join(QLatin1Char(' '));
    };

    const auto measure = [](const QVector<QString>& inputs, const int iterations, QString (OSMAND_CORE_CALL *stripper)(const QString&), quint64& checksum) -> qint64
    {
        checksum = 0;

        QElapsedTimer timer;
        timer.start();
        for (int iteration = 0; iteration < iterations; ++iteration)
        {
            for (const auto& input : inputs)
            {
                const auto output = stripper(input);
                checksum += static_cast<quint64>(output.size()) * 1315423911u;
                checksum += output.isEmpty() ? 0u : output.at(0).unicode();
            }
        }

        return timer.nsecsElapsed();
    };

    const QVector<StripDiacriticsPerformanceCase> testCases =
    {
        {
            "ascii-hot-path",
            {
                QStringLiteral("Plain ASCII street 123"),
                QStringLiteral("Main Street North East 101"),
                QStringLiteral("No accents or diacritics here, just plain ASCII")
            },
            20000
        },
        {
            "latin-accents",
            {
                QStringLiteral("Cr\u00E8me Br\u00FBl\u00E9e"),
                QStringLiteral("P\u0159\u00EDli\u0161 \u017Elu\u0165ou\u010Dk\u00FD k\u016F\u0148"),
                QStringLiteral("S\u00E3o Tom\u00E9 and Pr\u00EDncipe"),
                QStringLiteral("a\u0323\u0301 e\u0301 A\u030A")
            },
            12000
        },
        {
            "arabic-harakat",
            {
                QStringLiteral("\u0645\u064F\u062D\u064E\u0645\u064E\u0651\u062F"),
                QStringLiteral("\u0627\u0644\u0644\u064F\u063A\u064E\u0629\u064F \u0627\u0644\u0652\u0639\u064E\u0631\u064E\u0628\u0650\u064A\u064E\u0651\u0629\u064F")
            },
            12000
        },
        {
            "hangul-noop",
            {
                QStringLiteral("\uD55C\uAE00 \uD14C\uC2A4\uD2B8"),
                QStringLiteral("\uAC00\uB098\uB2E4\uB77C \uB9C8\uBC14\uC0AC")
            },
            12000
        },
        {
            "indic-marks",
            {
                QStringLiteral("\u0915\u093E"),
                QStringLiteral("\u0915\u093F"),
                QStringLiteral("\u0915\u0941"),
                QStringLiteral("\u0915\u0902"),
                QStringLiteral("\u0915\u093C")
            },
            12000
        },
        {
            "mixed-scripts",
            {
                QStringLiteral("Cr\u00E8me, \u0645\u064F\u062D\u064E\u0645\u064E\u0651\u062F, \uD55C\uAE00, \u0915\u093E!"),
                QStringLiteral("\u00C5land, \u03AC\u03BB\u03C6\u03B1, \u0439\u043E\u0434, \u6771\u4EAC")
            },
            10000
        }
    };

    for (const auto& testCase : testCases)
    {
        for (const auto& input : testCase.inputs)
        {
            const auto reference = OsmAnd::ICU::stripAccentsAndDiacritics(input);
            const auto optimized = OsmAnd::ICU::stripDiacritics(input);
            //if (reference == optimized)
                continue;

            const auto printableInput = input.isEmpty() ? QStringLiteral("<empty>") : input;
            const auto printableReference = reference.isEmpty() ? QStringLiteral("<empty>") : reference;
            const auto printableOptimized = optimized.isEmpty() ? QStringLiteral("<empty>") : optimized;
            OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info,
                "testStripDiacriticsPerformance parity mismatch [%s]\n  input:      %s\n  input cp:      %s\n  reference:  %s\n  reference cp:  %s\n  optimized:  %s\n  optimized cp:  %s",
                testCase.name,
                qPrintable(printableInput),
                qPrintable(formatCodePoints(input)),
                qPrintable(printableReference),
                qPrintable(formatCodePoints(reference)),
                qPrintable(printableOptimized),
                qPrintable(formatCodePoints(optimized)));
            return false;
        }
    }

    quint64 totalReferenceChecksum = 0;
    quint64 totalOptimizedChecksum = 0;
    qint64 totalReferenceNs = 0;
    qint64 totalOptimizedNs = 0;

    for (const auto& testCase : testCases)
    {
        const int warmupIterations = qMax(64, testCase.iterations / 20);
        quint64 warmupChecksum = 0;
        measure(testCase.inputs, warmupIterations, &OsmAnd::ICU::stripAccentsAndDiacritics, warmupChecksum);
        measure(testCase.inputs, warmupIterations, &OsmAnd::ICU::stripDiacritics, warmupChecksum);

        quint64 referenceChecksum = 0;
        const auto referenceNs = measure(testCase.inputs, testCase.iterations, &OsmAnd::ICU::stripAccentsAndDiacritics, referenceChecksum);

        quint64 optimizedChecksum = 0;
        const auto optimizedNs = measure(testCase.inputs, testCase.iterations, &OsmAnd::ICU::stripDiacritics, optimizedChecksum);

        const auto callsCount = static_cast<quint64>(testCase.iterations) * static_cast<quint64>(testCase.inputs.size());
        const auto referencePerCallNs = callsCount > 0 ? static_cast<double>(referenceNs) / static_cast<double>(callsCount) : 0.0;
        const auto optimizedPerCallNs = callsCount > 0 ? static_cast<double>(optimizedNs) / static_cast<double>(callsCount) : 0.0;
        const auto speedup = optimizedNs > 0 ? static_cast<double>(referenceNs) / static_cast<double>(optimizedNs) : 0.0;

        totalReferenceChecksum += referenceChecksum;
        totalOptimizedChecksum += optimizedChecksum;
        totalReferenceNs += referenceNs;
        totalOptimizedNs += optimizedNs;

        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info,
            "testStripDiacriticsPerformance [%s]: calls=%llu, reference=%.3f ms (%.1f ns/call), optimized=%.3f ms (%.1f ns/call), speedup=%.2fx, checksum=%llu/%llu",
            testCase.name,
            static_cast<unsigned long long>(callsCount),
            static_cast<double>(referenceNs) / 1000000.0,
            referencePerCallNs,
            static_cast<double>(optimizedNs) / 1000000.0,
            optimizedPerCallNs,
            speedup,
            static_cast<unsigned long long>(referenceChecksum),
            static_cast<unsigned long long>(optimizedChecksum));
    }

    const auto totalSpeedup = totalOptimizedNs > 0 ? static_cast<double>(totalReferenceNs) / static_cast<double>(totalOptimizedNs) : 0.0;
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info,
        "testStripDiacriticsPerformance completed: %d cases, reference=%.3f ms, optimized=%.3f ms, total speedup=%.2fx, checksum=%llu/%llu",
        testCases.size(),
        static_cast<double>(totalReferenceNs) / 1000000.0,
        static_cast<double>(totalOptimizedNs) / 1000000.0,
        totalSpeedup,
        static_cast<unsigned long long>(totalReferenceChecksum),
        static_cast<unsigned long long>(totalOptimizedChecksum));

    return true;
}
#endif

inline bool ensureIcuInitializedLocked()
{
    if (g_icuInitialized)
        return true;

    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "ICU resources are not initialized.");
    return false;
}

// Thread-safe and fast wrapper over Collator->clone() @alex-osm version
const Collator* getThreadSafeCollator()
{
    const auto threadId = QThread::currentThreadId();

    {
        QReadLocker readLocker(&collatorsLock);

        const auto citCollator = collatorsMap.constFind(threadId);
        if (citCollator != collatorsMap.cend())
            return *citCollator;
    }

    {
        QWriteLocker readLocker(&collatorsLock);

        if (g_pIcuCollator)
        {
            const auto clone = g_pIcuCollator->clone();
            if (clone)
            {
                collatorsMap.insert(threadId, clone);

                QObject::connect(QThread::currentThread(), &QThread::finished, [=](){
                    QWriteLocker writeLocker(&collatorsLock);

                    if (auto collator = collatorsMap.take(threadId))
                        delete collator;
                });
                return clone;
            }
        }
    }

    return nullptr;
}

void initializeCharFilter()
{
    if (g_pIcuNFDNormalizer == nullptr)
        return;

    s_isUnsafeChar.reset();
    for (int32_t i = 0; i < 65536; ++i)
    {
        if (QChar::category(i) == QChar::Mark_NonSpacing)
        {
            s_isUnsafeChar.set(i);
            continue;
        }

        UErrorCode status = U_ZERO_ERROR;
        UnicodeString decomposed;
        g_pIcuNFDNormalizer->normalize(UnicodeString(static_cast<UChar32>(i)), decomposed, status);
        if (U_FAILURE(status))
            continue;

        for (int32_t offset = 0; offset < decomposed.length();)
        {
            const auto codePoint = decomposed.char32At(offset);
            offset += U16_LENGTH(codePoint);
            if (QChar::category(static_cast<uint>(codePoint)) == QChar::Mark_NonSpacing)
            {
                s_isUnsafeChar.set(i);
                break;
            }
        }
    }
}

bool OsmAnd::ICU::initialize()
{
    QWriteLocker icuWriteLocker(&icuResourcesLock);

    // Initialize ICU
    UErrorCode icuError = U_ZERO_ERROR;
    g_IcuData = std::unique_ptr<QByteArray>(new QByteArray(
        getCoreResourcesProvider()->getResource(QLatin1String("misc/icu4c/icu-data-l.dat"))));
    udata_setCommonData(g_IcuData->constData(), &icuError);
    if (U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "Failed to initialize ICU data: %d", icuError);
        return false;
    }
    u_init(&icuError);
    if (U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "Failed to initialize ICU: %d", icuError);
        return false;
    }

    // Allocate resources:
    g_pIcuAnyToLatinTransliterator = Transliterator::createInstance(UnicodeString("Any-Latin/BGN"), UTRANS_FORWARD, icuError);
    if (U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "Failed to create global ICU Any-to-Latin transliterator: %d", icuError);
        return false;
    }

    icuError = U_ZERO_ERROR;
    g_pIcuAccentsAndDiacriticsConverter = Transliterator::createInstance(UnicodeString("NFD; [:Mn:] Remove; NFC"), UTRANS_FORWARD, icuError);
    if (U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "Failed to create global ICU accents&diacritics converter: %d", icuError);
        return false;
    }

    // "Line-break Boundary" is best for breaking text into lines when wrapping text
    icuError = U_ZERO_ERROR;
    g_pIcuLineBreakIterator = BreakIterator::createLineInstance(Locale::getRoot(), icuError);
    if (U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "Failed to create global ICU line break iterator: %d", icuError);
        return false;
    }

    icuError = U_ZERO_ERROR;
    g_pIcuNFDNormalizer = Normalizer2::getNFDInstance(icuError);
    if (U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "Failed to get NFD Normalizer: %d", icuError);
        return false;
    }

    icuError = U_ZERO_ERROR;
    g_pIcuNFCNormalizer = Normalizer2::getNFCInstance(icuError);
    if (U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "Failed to get NFC Normalizer: %d", icuError);
        return false;
    }

    initializeCharFilter();

    Collator *collator = nullptr;
    icuError = U_ZERO_ERROR;
    Locale locale = Locale::getDefault();
    if (std::strcmp(locale.getLanguage(), "ro") ||
        std::strcmp(locale.getLanguage(), "cs") ||
        std::strcmp(locale.getLanguage(), "sk"))
    {
        collator = Collator::createInstance(Locale("en", "US"), icuError);
    }
    else
    {
        collator = Collator::createInstance(icuError);
    }
    
    if (U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "Failed to create global ICU collator: %d", icuError);
        return false;
    }
    else
    {
        collator->setStrength(Collator::PRIMARY);
        g_pIcuCollator = collator;
    }

    g_icuInitialized = true;
    
    return true;
}

void OsmAnd::ICU::release()
{
    QWriteLocker icuWriteLocker(&icuResourcesLock);

    g_icuInitialized = false;
    tl_anyToLatin.reset();
    tl_accentsConverter.reset();

    // Release resources:
    {
        QWriteLocker collatorsWriteLocker(&collatorsLock);
        qDeleteAll(collatorsMap);
        collatorsMap.clear();
    }

    delete g_pIcuCollator;
    g_pIcuCollator = nullptr;

    delete g_pIcuAccentsAndDiacriticsConverter;
    g_pIcuAccentsAndDiacriticsConverter = nullptr;

    g_pIcuNFDNormalizer = nullptr;
    g_pIcuNFCNormalizer = nullptr;

    delete g_pIcuAnyToLatinTransliterator;
    g_pIcuAnyToLatinTransliterator = nullptr;

    delete g_pIcuLineBreakIterator;
    g_pIcuLineBreakIterator = nullptr;

    g_IcuData.reset();

    // Keep ICU runtime loaded during shutdown: worker threads may still destroy
    // thread-local ICU objects after ReleaseCore() starts.
}

// Ensure thread-local transliterators are initialized
inline bool initThreadLocalTransliteratorsLocked() {
    if (!ensureIcuInitializedLocked())
        return false;

    if (!tl_anyToLatin) {
        if (!g_pIcuAnyToLatinTransliterator) {
            OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Global Any-to-Latin transliterator not initialized.");
            return false;
        }
        tl_anyToLatin = makeTransliteratorClone(g_pIcuAnyToLatinTransliterator);
        if (!tl_anyToLatin) {
            OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Failed to clone Any-to-Latin transliterator.");
            return false;
        }
    }

    if (!tl_accentsConverter && g_pIcuAccentsAndDiacriticsConverter) {
        tl_accentsConverter = makeTransliteratorClone(g_pIcuAccentsAndDiacriticsConverter);
        if (!tl_accentsConverter) {
            OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Failed to clone accents/diacritics transliterator.");
            return false;
        }
    }

    return true;
}

OSMAND_CORE_API QString OSMAND_CORE_CALL OsmAnd::ICU::convertToVisualOrder(const QString& input)
{
    QString output;
    const auto len = input.length();
    UErrorCode icuError = U_ZERO_ERROR;
    bool ok = true;

    // Allocate ICU BiDi context
    const auto pContext = ubidi_openSized(len, 0, &icuError);
    if (pContext == nullptr || U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        return input;
    }

    // Configure context to reorder from logical to visual
    ubidi_setReorderingMode(pContext, UBIDI_REORDER_DEFAULT);

    ubidi_setPara(pContext, reinterpret_cast<const UChar*>(input.unicode()), len, UBIDI_DEFAULT_LTR, nullptr, &icuError);
    ok = U_SUCCESS(icuError);
    if (ok)
    {
        auto const direction = ubidi_getDirection(pContext);
        if (direction == UBIDI_LTR)
        {
            ubidi_close(pContext);
            return input;
        }
        QVector<UChar> reordered(len);
        ubidi_writeReordered(
            pContext,
            reordered.data(),
            len,
            UBIDI_DO_MIRRORING | UBIDI_REMOVE_BIDI_CONTROLS,
            &icuError);
        ok = U_SUCCESS(icuError);

        if (ok)
        {
            QVector<UChar> reshaped(len);
            const auto newLen = u_shapeArabic(reordered.constData(), len, reshaped.data(), len, U_SHAPE_TEXT_DIRECTION_VISUAL_LTR | U_SHAPE_LETTERS_SHAPE | U_SHAPE_LENGTH_FIXED_SPACES_AT_END, &icuError);
            ok = U_SUCCESS(icuError);

            if (ok)
            {
                output = qMove(QString(reinterpret_cast<const QChar*>(reshaped.constData()), newLen));
            }
        }
    }

    // Release context
    ubidi_close(pContext);

    if (!ok)
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        return input;
    }
    return output;
}

OSMAND_CORE_API OsmAnd::ICU::TextDirection OSMAND_CORE_CALL OsmAnd::ICU::getTextDirection(const QString& input)
{
    auto result = MIXED;
    const auto len = input.length();
    UErrorCode icuError = U_ZERO_ERROR;
    bool ok = true;

    // Allocate ICU BiDi context
    const auto pContext = ubidi_openSized(len, 0, &icuError);
    if (pContext == nullptr || U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        return result;
    }

    // Configure context to reorder from logical to visual
    ubidi_setReorderingMode(pContext, UBIDI_REORDER_DEFAULT);

    ubidi_setPara(pContext, reinterpret_cast<const UChar*>(input.unicode()), len, UBIDI_DEFAULT_LTR, nullptr, &icuError);
    ok = U_SUCCESS(icuError);
    if (ok)
    {
        auto const dir = ubidi_getDirection(pContext);
        result = dir == UBIDI_LTR ? LTR : (dir == UBIDI_RTL ? RTL : (dir == UBIDI_MIXED ? MIXED : NEUTRAL));
    }
    else
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);

    // Release context
    ubidi_close(pContext);

    return result;
}

OSMAND_CORE_API QString OSMAND_CORE_CALL OsmAnd::ICU::transliterateToLatin(
    const QString& input,
    const bool keepAccentsAndDiacriticsInInput /*= true*/,
    const bool keepAccentsAndDiacriticsInOutput /*= true*/)
{
    QReadLocker icuReadLocker(&icuResourcesLock);
    if (!initThreadLocalTransliteratorsLocked()) {
        return input;
    }

    icu::UnicodeString icuString(reinterpret_cast<const UChar*>(input.constData()), input.length());

    // Step 1: Any → Latin
    tl_anyToLatin->transliterate(icuString);

    QString output(reinterpret_cast<const QChar*>(icuString.getBuffer()), icuString.length());

    // Step 2: Optionally strip accents/diacritics
    if ((input.compare(output, Qt::CaseInsensitive) != 0 || !keepAccentsAndDiacriticsInInput) &&
        !keepAccentsAndDiacriticsInOutput)
    {
        if (tl_accentsConverter) {
            tl_accentsConverter->transliterate(icuString);
            output = QString(reinterpret_cast<const QChar*>(icuString.getBuffer()), icuString.length());
        }
    }

    return output;
}


OSMAND_CORE_API QVector<int> OSMAND_CORE_CALL OsmAnd::ICU::getTextWrapping(
    const QString& input,
    const int maxCharsPerLine,
    const int maxLines /*= 0*/)
{
    assert(maxCharsPerLine > 0);
    QVector<int> result;
    UErrorCode icuError = U_ZERO_ERROR;
    bool ok = true;
    QReadLocker icuReadLocker(&icuResourcesLock);
    if (!ensureIcuInitializedLocked() || g_pIcuLineBreakIterator == nullptr)
        return (result << 0);

    // Create break iterator
    const auto pBreakIterator = g_pIcuLineBreakIterator->clone();
    if (pBreakIterator == nullptr || U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        if (pBreakIterator != nullptr)
            delete pBreakIterator;
        return (result << 0);
    }

    // Set text for breaking
    pBreakIterator->setText(UnicodeString(reinterpret_cast<const UChar*>(input.unicode()), input.length()));

    auto cursor = 0;
    while(ok && cursor < input.length())
    {
        if (maxLines > 0 && result.size() > maxLines)
            break;
        // Get next desired breaking position
        auto lookAheadCursor = cursor + maxCharsPerLine;
        if (lookAheadCursor >= input.length())
            break;

        // If look-ahead cursor is still in bounds of input, and is pointing to:
        //  - control character
        //  - space character
        //  - non-spacing mark
        // then move forward until a valuable character is found
        while(lookAheadCursor < input.length())
        {
            const auto c = static_cast<UChar>(input[lookAheadCursor].unicode());
            if (!u_isspace(c) && u_charType(c) != U_CONTROL_CHAR && u_charType(c) != U_NON_SPACING_MARK)
                break;
            lookAheadCursor++;
        }

        // Now locate last legal line-break at or before the look-ahead cursor
        const auto lastBreak = pBreakIterator->preceding(lookAheadCursor + 1);

        // If last legal line-break wasn't found since current cursor, perform a hard-break
        if (lastBreak <= cursor)
        {
            result.push_back(lookAheadCursor);
            cursor = lookAheadCursor;
            continue;
        }

        // Otherwise a legal line-break was found, so move there and find next valuable character
        // and place line start there
        cursor = lastBreak;
        while(cursor < input.length())
        {
            const auto c = static_cast<UChar>(input[cursor].unicode());
            if (!u_isspace(c) && u_charType(c) != U_CONTROL_CHAR && u_charType(c) != U_NON_SPACING_MARK)
                break;
            cursor++;
        }
        result.push_back(cursor);
    }
    if (result.isEmpty() || result.first() != 0)
        result.prepend(0);

    if (pBreakIterator != nullptr)
        delete pBreakIterator;

    if (!ok)
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        return (result << 0);
    }
    return result;
}

OSMAND_CORE_API QVector<QStringRef> OSMAND_CORE_CALL OsmAnd::ICU::getTextWrappingRefs(
    const QString& input,
    const int maxCharsPerLine,
    const int maxLines /*= 0*/)
{
    QVector<QStringRef> result;
    const auto lineStartIndices = getTextWrapping(input, maxCharsPerLine, maxLines);
    result.reserve(lineStartIndices.size());
    auto lastStartIndex = lineStartIndices[0];
    int linesCount = 0;
    for(auto idx = 1, count = lineStartIndices.size(); idx < count; idx++)
    {
        auto startIndex = lineStartIndices[idx];
        result.push_back(input.midRef(lastStartIndex, startIndex - lastStartIndex));
        lastStartIndex = startIndex;
        linesCount++;
        if (maxLines > 0 && linesCount == maxLines)
            break;
    }
    if (maxLines <= 0 || linesCount < maxLines)
        result.push_back(input.midRef(lastStartIndex));
    
    return result;
}

OSMAND_CORE_API QStringList OSMAND_CORE_CALL OsmAnd::ICU::wrapText(
    const QString& input,
    const int maxCharsPerLine,
    const int maxLines /*= 0*/)
{
    QStringList result;
    const auto lineStartIndices = getTextWrapping(input, maxCharsPerLine, maxLines);
    auto lastStartIndex = lineStartIndices[0];
    int linesCount = 0;
    for(auto idx = 1, count = lineStartIndices.size(); idx < count; idx++)
    {
        auto startIndex = lineStartIndices[idx];
        result.push_back(input.mid(lastStartIndex, startIndex - lastStartIndex));
        lastStartIndex = startIndex;
        linesCount++;
        if (maxLines > 0 && linesCount == maxLines)
            break;
    }
    if (maxLines <= 0 || linesCount < maxLines)
        result.push_back(input.mid(lastStartIndex));

    return result;
}

OSMAND_CORE_API QString OSMAND_CORE_CALL OsmAnd::ICU::stripAccentsAndDiacritics(const QString& input)
{
    // WARNING ! Very slow. Use only for single call
    QReadLocker icuReadLocker(&icuResourcesLock);
    if (!initThreadLocalTransliteratorsLocked() || !tl_accentsConverter)
        return input;

    // Remove accents and diacritics
    UnicodeString icuString(reinterpret_cast<const UChar*>(input.unicode()), input.length());
    tl_accentsConverter->transliterate(icuString);
    return QString(reinterpret_cast<const QChar*>(icuString.getBuffer()), icuString.length());
}

OSMAND_CORE_API QString OSMAND_CORE_CALL OsmAnd::ICU::stripDiacritics(const QString& input)
{
    if (input.isEmpty())
        return input;

    const int len = input.length();
    bool needsProcessing = false;
    for (int i = 0; i < len; )
    {
        const auto currentChar = input.at(i);
        uint codePoint = currentChar.unicode();
        int step = 1;
        if (currentChar.isHighSurrogate() && i + 1 < len && input.at(i + 1).isLowSurrogate())
        {
            codePoint = QChar::surrogateToUcs4(currentChar, input.at(i + 1));
            step = 2;
        }

        if ((codePoint <= 0xFFFF && s_isUnsafeChar.test(codePoint)) ||
            (codePoint > 0xFFFF && QChar::category(codePoint) == QChar::Mark_NonSpacing))
        {
            needsProcessing = true;
            break;
        }

        i += step;
    }

    if (!needsProcessing)
    {
        return input;
    }

    QReadLocker icuReadLocker(&icuResourcesLock);
    if (!g_icuInitialized || !g_pIcuNFDNormalizer || !g_pIcuNFCNormalizer)
        return input;

    UErrorCode status = U_ZERO_ERROR;
    UnicodeString source(reinterpret_cast<const UChar*>(input.unicode()), input.length());
    tl_stripDiacriticsDecomposed.remove();
    g_pIcuNFDNormalizer->normalize(source, tl_stripDiacriticsDecomposed, status);
    if (U_FAILURE(status))
        return input;

    tl_stripDiacriticsFiltered.remove();
    for (int32_t i = 0; i < tl_stripDiacriticsDecomposed.length(); )
    {
        const auto codePoint = tl_stripDiacriticsDecomposed.char32At(i);
        i += U16_LENGTH(codePoint);
        if (QChar::category(static_cast<uint>(codePoint)) != QChar::Mark_NonSpacing)
            tl_stripDiacriticsFiltered.append(codePoint);
    }

    status = U_ZERO_ERROR;
    tl_stripDiacriticsComposed.remove();
    g_pIcuNFCNormalizer->normalize(tl_stripDiacriticsFiltered, tl_stripDiacriticsComposed, status);
    if (U_FAILURE(status))
        return input;

    return QString(reinterpret_cast<const QChar*>(tl_stripDiacriticsComposed.getBuffer()), tl_stripDiacriticsComposed.length());
}

UnicodeString qStrToUniStr(QString input)
{
    UnicodeString icuString(reinterpret_cast<const UChar*>(input.unicode()), input.length());
    return icuString;
}

bool isSpace(UChar c)
{
    return !u_isalnum(c);
}

OSMAND_CORE_API bool OSMAND_CORE_CALL OsmAnd::ICU::cmatches(const QString& _base, const QString& _part, StringMatcherMode _mode)
{
    switch (_mode)
    {
        case StringMatcherMode::CHECK_CONTAINS:
            return ccontains(_base, _part);
        case StringMatcherMode::CHECK_EQUALS_FROM_SPACE:
            return cstartsWith(_base, _part, true, true, true);
        case StringMatcherMode::CHECK_STARTS_FROM_SPACE:
            return cstartsWith(_base, _part, true, true, false);
        case StringMatcherMode::CHECK_STARTS_FROM_SPACE_NOT_BEGINNING:
            return cstartsWith(_base, _part, false, true, false);
        case StringMatcherMode::CHECK_ONLY_STARTS_WITH:
            return cstartsWith(_base, _part, true, false, false);
        case StringMatcherMode::CHECK_EQUALS:
            return cstartsWith(_base, _part, false, false, true);
        default:
            return false;
    }
}
OSMAND_CORE_API bool OSMAND_CORE_CALL OsmAnd::ICU::ccontains(const QString& _base, const QString& _part)
{
    UErrorCode icuError = U_ZERO_ERROR;
    bool result = false;
    QReadLocker icuReadLocker(&icuResourcesLock);
    if (!ensureIcuInitializedLocked())
        return false;
    const auto collator = getThreadSafeCollator();
    if (collator == nullptr || U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        return false;
    }
    else
    {
        UnicodeString baseString = qStrToUniStr(_base);
        UnicodeString partString = qStrToUniStr(_part);
        
        if (baseString.length() <= partString.length())
            return collator->equals(baseString, partString);
        
        for (int pos = 0; pos <= baseString.length() - partString.length() + 1; pos++)
        {
            UnicodeString temp = baseString.tempSubString(pos, baseString.length());
            
            for (int length = temp.length(); length >= 0; length--)
            {
                UnicodeString temp2 = temp.tempSubString(0, length);
                if (collator->equals(temp2, partString))
                {
                    result = true;
                    break;
                }
            }
            if (result)
                break;
        }
    }
    return result;
}
OSMAND_CORE_API bool OSMAND_CORE_CALL OsmAnd::ICU::cstartsWith(const QString& _searchInParam, const QString& _theStart,
                                                  bool checkBeginning, bool checkSpaces, bool equals)
{
    UErrorCode icuError = U_ZERO_ERROR;
    bool result = false;
    QReadLocker icuReadLocker(&icuResourcesLock);
    if (!ensureIcuInitializedLocked())
        return false;
    const auto collator = getThreadSafeCollator();
    if (collator == nullptr || U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        return false;
    }
    else
    {
        // FUTURE: This is not effective code, it runs on each comparision
        // It would be more efficient to normalize all strings in file and normalize search string before collator
        UnicodeString searchIn = qStrToUniStr(OsmAnd::CollatorStringMatcher::lowercaseAndAlignChars(_searchInParam));
        QString theStartAligned = OsmAnd::CollatorStringMatcher::alignChars(_theStart);
        UnicodeString theStart = qStrToUniStr(theStartAligned);

        int startLength = theStart.length();
        int serchInLength = searchIn.length();
        
        if (startLength == 0)
        {
            result = true;
        }
        // this is not correct without (lowercaseAndAlignChars) because of Auhofstrasse != Auhofstraße
        if (startLength > serchInLength)
        {
            result = false;
        }
        
        if (checkBeginning)
        {
            bool starts = collator->equals(searchIn.tempSubString(0, startLength), theStart);
            if (starts)
            {
                if (equals)
                {
                    if (startLength == serchInLength || isSpace(searchIn.charAt(startLength)))
                    {
                        result = true;
                    }
                }
                else
                {
                    result = true;
                }
            }
        }
        
        if (!result && checkSpaces)
        {
            for (int i = 1; i <= serchInLength - startLength; i++)
            {
                if (isSpace(searchIn.charAt(i - 1)) && !isSpace(searchIn.charAt(i)))
                {
                    if (collator->equals(searchIn.tempSubString(i, startLength), theStart))
                    {
                        if (equals)
                        {
                            if (i + startLength == serchInLength || isSpace(searchIn.charAt(i + startLength)))
                            {
                                result = true;
                                break;
                            }
                        }
                        else
                        {
                            result = true;
                            break;
                        }
                    }
                }
            }
        }
        if (!checkBeginning && !checkSpaces && equals)
            result = collator->equals(searchIn, theStart);
    }
    
    return result;
}

OSMAND_CORE_API int OSMAND_CORE_CALL OsmAnd::ICU::ccompare(const QString& _s1, const QString& _s2)
{
    UErrorCode icuError = U_ZERO_ERROR;
    int result = 0;
    QReadLocker icuReadLocker(&icuResourcesLock);
    if (!ensureIcuInitializedLocked())
        return result;
    const auto collator = getThreadSafeCollator();
    if (collator == nullptr || U_FAILURE(icuError))
    {
        LogPrintf(LogSeverityLevel::Error, "ICU error: %d", icuError);
        return result;
    }
    else
    {
        UnicodeString s1 = qStrToUniStr(_s1);
        UnicodeString s2 = qStrToUniStr(_s2);
        result = collator->compare(s1, s2);
    }
    return result;
}
