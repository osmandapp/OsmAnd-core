#if defined(OSMAND_CORE_INTERNAL)
#   if defined(OSMAND_CORE_EXTERNAL_INCLUDES_WARNINGS_IGNORED)
#       error Unpaired "ignore_warnings_on_external_includes.h" and "restore_internal_warnings.h"
#   endif // defined(OSMAND_CORE_EXTERNAL_INCLUDES_WARNINGS_IGNORED)

#   if defined(_MSC_VER)
#       pragma warning(push, 3)
#   endif

#   define OSMAND_CORE_EXTERNAL_INCLUDES_WARNINGS_IGNORED
#endif // defined(OSMAND_CORE_INTERNAL)
