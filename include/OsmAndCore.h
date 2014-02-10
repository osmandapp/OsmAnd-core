#ifndef _OSMAND_CORE_OSMAND_CORE_H_
#define _OSMAND_CORE_OSMAND_CORE_H_

#if defined(OSMAND_CORE_EXPORTS)
#   if defined(_WIN32) || defined(__CYGWIN__)
#       define OSMAND_CORE_API \
            __declspec(dllexport)
#       define OSMAND_CORE_CALL \
            __stdcall
#   else
#       if __GNUC__ >= 4
#           define OSMAND_CORE_API \
                __attribute__ ((visibility ("default")))
#       else
#           define OSMAND_CORE_API
#       endif
#       define OSMAND_CORE_CALL
#   endif
#elif !defined(OSMAND_CORE_STATIC)
#   if defined(_WIN32) || defined(__CYGWIN__)
#       define OSMAND_CORE_API \
            __declspec(dllimport)
#       define OSMAND_CORE_API_FUNCTION \
            extern OSMAND_CORE_API
#       define OSMAND_CORE_CALL \
            __stdcall
#   else
#       if __GNUC__ >= 4
#           define OSMAND_CORE_API \
                __attribute__ ((visibility ("default")))
#       else
#           define OSMAND_CORE_API
#       endif
#       define OSMAND_CORE_CALL
#   endif
#else
#   define OSMAND_CORE_API
#   define OSMAND_CORE_CALL
#endif

#if !defined(OSMAND_DEBUG)
#   if defined(DEBUG) || defined(_DEBUG)
#       define OSMAND_DEBUG 1
#   endif
#endif
#if !defined(OSMAND_DEBUG)
#   define OSMAND_DEBUG 0
#endif

#if !defined(SWIG)
#   define STRONG_ENUM(name) enum class name
#   define STRONG_ENUM_EX(name, basetype) enum class name : basetype
#   define WEAK_ENUM(name) enum name
#   define WEAK_ENUM_EX(name, basetype) enum name : basetype
#else
#   define STRONG_ENUM(name) enum name
#   define STRONG_ENUM_EX(name, basetype) enum name
#   define WEAK_ENUM(name) enum name
#   define WEAK_ENUM_EX(name, basetype) enum name
#endif

#include <OsmAndCore/QtExtensions.h>
#include <QtGlobal>

#include <memory>

#define OSMAND_CLASS(name) \
    private: \
        Q_DISABLE_COPY(name) \
    public: \
        typedef name* Ptr; \
        typedef std::shared_ptr< name > Ref; \
        typedef std::weak_ptr< name > WeakRef; \
        typedef std::unique_ptr< name > UniqueRef; \
        static Ref NewRef(here should be args);

namespace OsmAnd
{
    OSMAND_CORE_API void OSMAND_CORE_CALL InitializeCore();
    OSMAND_CORE_API void OSMAND_CORE_CALL ReleaseCore();
}

#endif // !defined(_OSMAND_CORE_OSMAND_CORE_H_)
