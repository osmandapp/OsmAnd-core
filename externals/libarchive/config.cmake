include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckFunctionExists)
include(CheckTypeSize)
include(CheckCCompilerFlag)
include(CheckCSourceCompiles)
include(CheckCXXSymbolExists)
include(CheckCXXSourceCompiles)
include(CheckStructHasMember)

check_include_files("sys/types.h" HAVE_SYS_TYPES_H)

check_include_files("acl/libacl.h" HAVE_ACL_LIBACL_H)
check_include_files("attr/xattr.h" HAVE_ATTR_XATTR_H)
check_include_files("ctype.h" HAVE_CTYPE_H)
check_include_files("copyfile.h" HAVE_COPYFILE_H)
check_include_files("direct.h" HAVE_DIRECT_H)
check_include_files("dirent.h" HAVE_DIRENT_H)
check_include_files("dlfcn.h" HAVE_DLFCN_H)
check_include_files("errno.h" HAVE_ERRNO_H)
check_include_files("ext2fs/ext2_fs.h" HAVE_EXT2FS_EXT2_FS_H)

check_c_source_compiles("
	#include <sys/ioctl.h>
	#include <ext2fs/ext2_fs.h>
	int main(void) { return EXT2_IOC_GETFLAGS; }
" HAVE_WORKING_EXT2_IOC_GETFLAGS)

check_include_files("fcntl.h" HAVE_FCNTL_H)
check_include_files("grp.h" HAVE_GRP_H)
check_include_files("iconv.h" HAVE_ICONV_H)
check_include_files("inttypes.h" HAVE_INTTYPES_H)
check_include_files("io.h" HAVE_IO_H)
check_include_files("langinfo.h" HAVE_LANGINFO_H)
check_include_files("limits.h" HAVE_LIMITS_H)
check_include_files("linux/types.h" HAVE_LINUX_TYPES_H)
check_include_files("linux/fiemap.h" HAVE_LINUX_FIEMAP_H)
check_include_files("linux/fs.h" HAVE_LINUX_FS_H)

check_c_source_compiles("
	#include <sys/ioctl.h>
	#include <linux/fs.h>
	int main(void) { return FS_IOC_GETFLAGS; }
" HAVE_WORKING_FS_IOC_GETFLAGS)

check_include_files("linux/magic.h" HAVE_LINUX_MAGIC_H)
check_include_files("locale.h" HAVE_LOCALE_H)
check_include_files("membership.h" HAVE_MEMBERSHIP_H)
check_include_files("memory.h" HAVE_MEMORY_H)
check_include_files("paths.h" HAVE_PATHS_H)
check_include_files("poll.h" HAVE_POLL_H)
check_include_files("process.h" HAVE_PROCESS_H)
check_include_files("pthread.h" HAVE_PTHREAD_H)
check_include_files("pwd.h" HAVE_PWD_H)
check_include_files("readpassphrase.h" HAVE_READPASSPHRASE_H)
check_include_files("regex.h" HAVE_REGEX_H)
check_include_files("signal.h" HAVE_SIGNAL_H)
check_include_files("spawn.h" HAVE_SPAWN_H)
check_include_files("stdarg.h" HAVE_STDARG_H)
check_include_files("stdint.h" HAVE_STDINT_H)
check_include_files("stdlib.h" HAVE_STDLIB_H)
check_include_files("string.h" HAVE_STRING_H)
check_include_files("strings.h" HAVE_STRINGS_H)
check_include_files("sys/acl.h" HAVE_SYS_ACL_H)
check_include_files("sys/cdefs.h" HAVE_SYS_CDEFS_H)
check_include_files("sys/extattr.h" HAVE_SYS_EXTATTR_H)
check_include_files("sys/ioctl.h" HAVE_SYS_IOCTL_H)
check_include_files("sys/mkdev.h" HAVE_SYS_MKDEV_H)
check_include_files("sys/mount.h" HAVE_SYS_MOUNT_H)
check_include_files("sys/param.h" HAVE_SYS_PARAM_H)
check_include_files("sys/poll.h" HAVE_SYS_POLL_H)
check_include_files("sys/richacl.h" HAVE_SYS_RICHACL_H)
check_include_files("sys/select.h" HAVE_SYS_SELECT_H)
check_include_files("sys/stat.h" HAVE_SYS_STAT_H)
check_include_files("sys/statfs.h" HAVE_SYS_STATFS_H)
check_include_files("sys/statvfs.h" HAVE_SYS_STATVFS_H)
check_include_files("sys/sysmacros.h" HAVE_SYS_SYSMACROS_H)
check_include_files("sys/time.h" HAVE_SYS_TIME_H)
check_include_files("sys/utime.h" HAVE_SYS_UTIME_H)
check_include_files("sys/utsname.h" HAVE_SYS_UTSNAME_H)
check_include_files("sys/vfs.h" HAVE_SYS_VFS_H)
check_include_files("sys/wait.h" HAVE_SYS_WAIT_H)
check_include_files("sys/xattr.h" HAVE_SYS_XATTR_H)
check_include_files("time.h" HAVE_TIME_H)
check_include_files("unistd.h" HAVE_UNISTD_H)
check_include_files("utime.h" HAVE_UTIME_H)
check_include_files("wchar.h" HAVE_WCHAR_H)
check_include_files("wctype.h" HAVE_WCTYPE_H)
check_include_files("windows.h" HAVE_WINDOWS_H)

check_include_files("windows.h;wincrypt.h" HAVE_WINCRYPT_H)
check_include_files("windows.h;winioctl.h" HAVE_WINIOCTL_H)

set(SAFE_TO_DEFINE_EXTENSIONS ON)

check_symbol_exists("_CrtSetReportMode" "crtdbg.h" HAVE__CrtSetReportMode)
check_function_exists("arc4random_buf" HAVE_ARC4RANDOM_BUF)
check_function_exists("chflags" HAVE_CHFLAGS)
check_function_exists("chown" HAVE_CHOWN)
check_function_exists("chroot" HAVE_CHROOT)
check_function_exists("ctime_r" HAVE_CTIME_R)
check_function_exists("fchdir" HAVE_FCHDIR)
check_function_exists("fchflags" HAVE_FCHFLAGS)
check_function_exists("fchmod" HAVE_FCHMOD)
check_function_exists("fchown" HAVE_FCHOWN)
check_function_exists("fcntl" HAVE_FCNTL)
check_function_exists("fdopendir" HAVE_FDOPENDIR)
check_function_exists("fork" HAVE_FORK)
check_function_exists("fstat" HAVE_FSTAT)
check_function_exists("fstatat" HAVE_FSTATAT)
check_function_exists("fstatfs" HAVE_FSTATFS)
check_function_exists("fstatvfs" HAVE_FSTATVFS)
check_function_exists("ftruncate" HAVE_FTRUNCATE)
check_function_exists("futimens" HAVE_FUTIMENS)
check_function_exists("futimes" HAVE_FUTIMES)
check_function_exists("futimesat" HAVE_FUTIMESAT)
check_function_exists("geteuid" HAVE_GETEUID)
check_function_exists("getgrgid_r" HAVE_GETGRGID_R)
check_function_exists("getgrnam_r" HAVE_GETGRNAM_R)
check_function_exists("getpwnam_r" HAVE_GETPWNAM_R)
check_function_exists("getpwuid_r" HAVE_GETPWUID_R)
check_function_exists("getpid" HAVE_GETPID)
check_function_exists("getvfsbyname" HAVE_GETVFSBYNAME)
check_function_exists("gmtime_r" HAVE_GMTIME_R)
check_function_exists("iconv" HAVE_ICONV)
check_function_exists("lchflags" HAVE_LCHFLAGS)
check_function_exists("lchmod" HAVE_LCHMOD)
check_function_exists("lchown" HAVE_LCHOWN)
check_function_exists("link" HAVE_LINK)
check_function_exists("linkat" HAVE_LINKAT)
check_function_exists("localtime_r" HAVE_LOCALTIME_R)
check_function_exists("lstat" HAVE_LSTAT)
check_function_exists("lutimes" HAVE_LUTIMES)
check_function_exists("mbrtowc" HAVE_MBRTOWC)
check_function_exists("memmove" HAVE_MEMMOVE)
check_function_exists("mkdir" HAVE_MKDIR)
check_function_exists("mkfifo" HAVE_MKFIFO)
check_function_exists("mknod" HAVE_MKNOD)
check_function_exists("mkstemp" HAVE_MKSTEMP)
check_function_exists("nl_langinfo" HAVE_NL_LANGINFO)
check_function_exists("openat" HAVE_OPENAT)
check_function_exists("pipe" HAVE_PIPE)
check_function_exists("poll" HAVE_POLL)
check_function_exists("posix_spawnp" HAVE_POSIX_SPAWNP)
check_function_exists("readlink" HAVE_READLINK)
check_function_exists("readpassphrase" HAVE_READPASSPHRASE)
check_function_exists("select" HAVE_SELECT)
check_function_exists("setenv" HAVE_SETENV)
check_function_exists("setlocale" HAVE_SETLOCALE)
check_function_exists("sigaction" HAVE_SIGACTION)
check_function_exists("statfs" HAVE_STATFS)
check_function_exists("statvfs" HAVE_STATVFS)
check_function_exists("strchr" HAVE_STRCHR)
check_function_exists("strdup" HAVE_STRDUP)
check_function_exists("strerror" HAVE_STRERROR)
check_function_exists("strncpy_s" HAVE_STRNCPY_S)
check_function_exists("strnlen" HAVE_STRNLEN)
check_function_exists("strrchr" HAVE_STRRCHR)
check_function_exists("symlink" HAVE_SYMLINK)
check_function_exists("timegm" HAVE_TIMEGM)
check_function_exists("tzset" HAVE_TZSET)
check_function_exists("unlinkat" HAVE_UNLINKAT)
check_function_exists("unsetenv" HAVE_UNSETENV)
check_function_exists("utime" HAVE_UTIME)
check_function_exists("utimes" HAVE_UTIMES)
check_function_exists("utimensat" HAVE_UTIMENSAT)
check_function_exists("vfork" HAVE_VFORK)
check_function_exists("wcrtomb" HAVE_WCRTOMB)
check_function_exists("wcscmp" HAVE_WCSCMP)
check_function_exists("wcscpy" HAVE_WCSCPY)
check_function_exists("wcslen" HAVE_WCSLEN)
check_function_exists("wctomb" HAVE_WCTOMB)
check_function_exists("_ctime64_s" HAVE__CTIME64_S)
check_function_exists("_fseeki64" HAVE__FSEEKI64)
check_function_exists("_get_timezone" HAVE__GET_TIMEZONE)
check_function_exists("_gmtime64_s" HAVE__GMTIME64_S)
check_function_exists("_localtime64_s" HAVE__LOCALTIME64_S)
check_function_exists("_mkgmtime64" HAVE__MKGMTIME64)

check_function_exists("cygwin_conv_path" HAVE_CYGWIN_CONV_PATH)
check_function_exists("fseeko" HAVE_FSEEKO)
check_function_exists("strerror_r" HAVE_STRERROR_R)
check_function_exists("strftime" HAVE_STRFTIME)
check_function_exists("vprintf" HAVE_VPRINTF)
check_function_exists("wmemcmp" HAVE_WMEMCMP)
check_function_exists("wmemcpy" HAVE_WMEMCPY)
check_function_exists("wmemmove" HAVE_WMEMMOVE)

check_c_source_compiles("
	#include <sys/types.h>
	#include <sys/mount.h>
	int main(void) { struct vfsconf v; return sizeof(v);}
" HAVE_STRUCT_VFSCONF)

check_c_source_compiles("
	#include <sys/types.h>
	#include <sys/mount.h>
	int main(void) { struct xvfsconf v; return sizeof(v);}
" HAVE_STRUCT_XVFSCONF)

check_c_source_compiles("
	#include <sys/types.h>
	#include <sys/mount.h>
	int main(void) { struct statfs s; return sizeof(s);}
" HAVE_STRUCT_STATFS)

check_c_source_compiles("
	#include <dirent.h>
	int main() {DIR *d = opendir(\".\"); struct dirent e,*r; return readdir_r(d,&e,&r);}
" HAVE_READDIR_R)

check_c_source_compiles("
	#include <dirent.h>
	int main() {DIR *d = opendir(\".\"); return dirfd(d);}
" HAVE_DIRFD)

check_c_source_compiles("
	#include <fcntl.h>
	#include <unistd.h>
	int main() {char buf[10]; return readlinkat(AT_FDCWD, \"\", buf, 0);}
" HAVE_READLINKAT)

check_c_source_compiles("
	#include <sys/mkdev.h>
	int main() { return major(256); }
" MAJOR_IN_MKDEV)
check_c_source_compiles("
	#include <sys/sysmacros.h>
	int main() { return major(256); }
" MAJOR_IN_SYSMACROS)

set(HAVE_LZMA_STREAM_ENCODER_MT 0)

if (HAVE_STRERROR_R)
  set(HAVE_DECL_STRERROR_R 1)
endif()

check_symbol_exists(EFTYPE "errno.h" HAVE_EFTYPE)
check_symbol_exists(EILSEQ "errno.h" HAVE_EILSEQ)
check_symbol_exists(D_MD_ORDER "langinfo.h" HAVE_D_MD_ORDER)
check_symbol_exists(SSIZE_MAX "limits.h" HAVE_DECL_SSIZE_MAX)

set(int_limits_headers
	"limits.h"
)
if (HAVE_STDINT_H)
	list(APPEND int_limits_headers
		"stdint.h"
	)
endif()
if (HAVE_INTTYPES_H)
  list(APPEND int_limits_headers
  	"inttypes.h"
  )
endif()
check_symbol_exists(INT32_MAX "${int_limits_headers}" HAVE_DECL_INT32_MAX)
check_symbol_exists(INT32_MIN "${int_limits_headers}" HAVE_DECL_INT32_MIN)
check_symbol_exists(INT64_MAX "${int_limits_headers}" HAVE_DECL_INT64_MAX)
check_symbol_exists(INT64_MIN "${int_limits_headers}" HAVE_DECL_INT64_MIN)
check_symbol_exists(INTMAX_MAX "${int_limits_headers}" HAVE_DECL_INTMAX_MAX)
check_symbol_exists(INTMAX_MIN "${int_limits_headers}" HAVE_DECL_INTMAX_MIN)
check_symbol_exists(UINT32_MAX "${int_limits_headers}" HAVE_DECL_UINT32_MAX)
check_symbol_exists(UINT64_MAX "${int_limits_headers}" HAVE_DECL_UINT64_MAX)
check_symbol_exists(UINTMAX_MAX "${int_limits_headers}" HAVE_DECL_UINTMAX_MAX)
check_symbol_exists(SIZE_MAX "${int_limits_headers}" HAVE_DECL_SIZE_MAX)

check_struct_has_member("struct tm" "tm_gmtoff" "time.h" HAVE_STRUCT_TM_TM_GMTOFF)
check_struct_has_member("struct tm" "__tm_gmtoff" "time.h" HAVE_STRUCT_TM___TM_GMTOFF)

if (HAVE_STRUCT_STATFS)
	check_struct_has_member("struct statfs" "f_namemax" "sys/param.h;sys/mount.h" HAVE_STRUCT_STATFS_F_NAMEMAX)
	check_struct_has_member("struct statfs" "f_iosize" "sys/param.h;sys/mount.h" HAVE_STRUCT_STATFS_F_IOSIZE)
endif()

check_struct_has_member("struct stat" "st_birthtime" "sys/types.h;sys/stat.h" HAVE_STRUCT_STAT_ST_BIRTHTIME)

check_struct_has_member("struct stat" "st_birthtimespec.tv_nsec" "sys/types.h;sys/stat.h" HAVE_STRUCT_STAT_ST_BIRTHTIMESPEC_TV_NSEC)
check_struct_has_member("struct stat" "st_mtimespec.tv_nsec" "sys/types.h;sys/stat.h" HAVE_STRUCT_STAT_ST_MTIMESPEC_TV_NSEC)
check_struct_has_member("struct stat" "st_mtim.tv_nsec" "sys/types.h;sys/stat.h" HAVE_STRUCT_STAT_ST_MTIM_TV_NSEC)
check_struct_has_member("struct stat" "st_mtime_n" "sys/types.h;sys/stat.h" HAVE_STRUCT_STAT_ST_MTIME_N)
check_struct_has_member("struct stat" "st_umtime" "sys/types.h;sys/stat.h" HAVE_STRUCT_STAT_ST_UMTIME)
check_struct_has_member("struct stat" "st_mtime_usec" "sys/types.h;sys/stat.h" HAVE_STRUCT_STAT_ST_MTIME_USEC)
check_struct_has_member("struct stat" "st_blksize" "sys/types.h;sys/stat.h" HAVE_STRUCT_STAT_ST_BLKSIZE)
check_struct_has_member("struct stat" "st_flags" "sys/types.h;sys/stat.h" HAVE_STRUCT_STAT_ST_FLAGS)

if (HAVE_SYS_STATVFS_H)
  check_struct_has_member("struct statvfs" "f_iosize" "sys/types.h;sys/statvfs.h" HAVE_STRUCT_STATVFS_F_IOSIZE)
endif()

check_struct_has_member("struct tm" "tm_sec" "sys/types.h;sys/time.h;time.h" TIME_WITH_SYS_TIME)

check_type_size("short" SIZE_OF_SHORT)
check_type_size("int" SIZE_OF_INT)
check_type_size("long" SIZE_OF_LONG)
check_type_size("long long" SIZE_OF_LONG_LONG)

check_type_size("unsigned short" SIZE_OF_UNSIGNED_SHORT)
check_type_size("unsigned" SIZE_OF_UNSIGNED)
check_type_size("unsigned long" SIZE_OF_UNSIGNED_LONG)
check_type_size("unsigned long long" SIZE_OF_UNSIGNED_LONG_LONG)

check_type_size("__int64" __INT64)
check_type_size("unsigned __int64" UNSIGNED___INT64)

check_type_size(int16_t INT16_T)
check_type_size(int32_t INT32_T)
check_type_size(int64_t INT64_T)
check_type_size(intmax_t INTMAX_T)
check_type_size(uint8_t UINT8_T)
check_type_size(uint16_t UINT16_T)
check_type_size(uint32_t UINT32_T)
check_type_size(uint64_t UINT64_T)
check_type_size(uintmax_t UINTMAX_T)

set(const "const")

check_type_size(gid_t GID_T)
if (NOT HAVE_GID_T)
  if (CMAKE_TARGET_OS STREQUAL "windows")
    set(gid_t "short")
  else()
    set(gid_t "unsigned int")
  endif()
endif()

check_type_size(id_t ID_T)
if (NOT HAVE_ID_T)
  if (CMAKE_TARGET_OS STREQUAL "windows")
    set(id_t "short")
  else()
    set(id_t "unsigned int")
  endif()
endif()

check_type_size(mode_t MODE_T)
if (NOT HAVE_MODE_T)
  if (CMAKE_TARGET_OS STREQUAL "windows")
    set(mode_t "unsigned short")
  else()
    set(mode_t "int")
  endif()
endif()

check_type_size(off_t OFF_T)
if (NOT HAVE_OFF_T)
  set(off_t "__int64")
endif()

check_type_size(size_t SIZE_T)
if (NOT HAVE_SIZE_T)
  if ("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
    set(size_t "uint64_t")
  else()
    set(size_t "uint32_t")
  endif()
endif()

check_type_size(ssize_t SSIZE_T)
if (NOT HAVE_SSIZE_T)
  if ("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
    set(ssize_t "int64_t")
  else()
    set(ssize_t "long")
  endif()
endif()

check_type_size(uid_t UID_T)
if (NOT HAVE_UID_T)
  if (CMAKE_TARGET_OS STREQUAL "windows")
    set(uid_t "short")
  else()
    set(uid_t "unsigned int")
  endif()
endif()

check_type_size(pid_t PID_T)
if (NOT HAVE_PID_T)
  if (CMAKE_TARGET_OS STREQUAL "windows")
    SET(pid_t "int")
  else()
    message(FATAL_ERROR "pid_t doesn't exist on this platform?")
  endif()
endif()

check_type_size(intptr_t INTPTR_T)
if (NOT HAVE_INTPTR_T)
  if ("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
    set(intptr_t "int64_t")
  else()
    set(intptr_t "int32_t")
  endif()
endif()

check_type_size(uintptr_t UINTPTR_T)
if (NOT HAVE_UINTPTR_T)
  if ("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
    set(uintptr_t "uint64_t")
  else()
    set(uintptr_t "uint32_t")
  endif()
endif()

check_type_size(wchar_t SIZEOF_WCHAR_T)
if (HAVE_SIZEOF_WCHAR_T)
  set(HAVE_WCHAR_T 1)
endif()

configure_file("config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.h")