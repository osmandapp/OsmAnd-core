#ifndef _OSMAND_CORE_QT_EXTRA_DEFINITIONS_H_
#define _OSMAND_CORE_QT_EXTRA_DEFINITIONS_H_

#include <QtGlobal>

#if !defined(qPrintableRef)
#  define qPrintableRef(stringRef) stringRef.toLocal8Bit().constData()
#endif // !defined(qPrintableRef)

#if defined(_UNICODE) || defined(UNICODE)
#   define QStringToStlString(x) (x).toStdWString()
#else
#   define QStringToStlString(x) (x).toStdString()
#endif

#if !defined(Q_DISABLE_MOVE)
#   if defined(Q_COMPILER_RVALUE_REFS)
#       define Q_DISABLE_MOVE(Class)                                                        \
            Class(Class &&) Q_DECL_EQ_DELETE;                                               \
            Class &operator=(Class &&) Q_DECL_EQ_DELETE;
#   else
#       define Q_DISABLE_MOVE(Class)
#   endif
#endif

#if !defined(Q_DISABLE_COPY_AND_MOVE)
#   define Q_DISABLE_COPY_AND_MOVE(Class)                                                   \
        Q_DISABLE_COPY(Class);                                                              \
        Q_DISABLE_MOVE(Class);
#endif // !defined(Q_DISABLE_COPY_AND_MOVE)

#if !defined(Q_DISABLE_ASSIGN)
#   define Q_DISABLE_ASSIGN(Class)                                                          \
        Class &operator=(Class &) Q_DECL_EQ_DELETE;
#endif // !defined(Q_DISABLE_ASSIGN)

#endif // !defined(_OSMAND_CORE_QT_EXTRA_DEFINITIONS_H_)
