project(archive)

prepare_upstream()

if (CMAKE_TARGET_OS STREQUAL "ios" AND CMAKE_TARGET_BUILD_TOOL STREQUAL "xcode")
	configure_file("config.h.ios" "${CMAKE_CURRENT_BINARY_DIR}/config.h" COPYONLY)
else()
	save_and_set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${CMAKE_C_FLAGS}")
	include(CheckIncludeFile)
	include(CheckIncludeFiles)
	include(CheckFunctionExists)
	include(CheckSymbolExists)
	include(CheckLibraryExists)
	include(CheckTypeSize)
	include(TestBigEndian)

	check_function_exists("acl_create_entry" HAVE_ACL_CREATE_ENTRY)
	check_function_exists("acl_get_link" HAVE_ACL_GET_LINK)
	check_function_exists("acl_get_link_np" HAVE_ACL_GET_LINK_NP)
	check_function_exists("acl_get_perm" HAVE_ACL_GET_PERM)
	check_function_exists("acl_get_perm_np" HAVE_ACL_GET_PERM_NP)
	check_function_exists("acl_init" HAVE_ACL_INIT)
	check_include_file("acl/libacl.h" HAVE_ACL_LIBACL_H)
	check_symbol_exists("acl_permset_t" "sys/acl.h" HAVE_ACL_PERMSET_T)
	check_function_exists("acl_set_fd" HAVE_ACL_SET_FD)
	check_function_exists("acl_set_fd_np" HAVE_ACL_SET_FD_NP)
	check_function_exists("acl_set_file" HAVE_ACL_SET_FILE)
	unset(HAVE_ACL_USER) # True for systems with POSIX ACL support
	check_include_file("attr/xattr.h" HAVE_ATTR_XATTR_H)
	check_include_file("bzlib.h" HAVE_BZLIB_H)
	if (HAVE_BZLIB_H)
		find_package(BZip2)
		if (NOT BZIP2_FOUND)
			set(HAVE_BZLIB_H OFF)
		else()
			include_directories(${BZIP_INCLUDE_DIRS})
		endif()
	endif()
	check_function_exists("chflags" HAVE_CHFLAGS)
	check_function_exists("chown" HAVE_CHOWN)
	check_function_exists("chroot" HAVE_CHROOT)
	check_include_file("copyfile.h" HAVE_COPYFILE_H)
	check_function_exists("ctime_r" HAVE_CTIME_R)
	check_include_file("ctype.h" HAVE_CTYPE_H)
	check_function_exists("cygwin_conv_path" HAVE_CYGWIN_CONV_PATH)
	set(HAVE_DECL_EXTATTR_NAMESPACE_USER "EXTATTR_NAMESPACE_USER")
	set(HAVE_DECL_INT64_MAX "INT64_MAX")
	set(HAVE_DECL_INT64_MIN "INT64_MIN")
	set(HAVE_DECL_SIZE_MAX "SIZE_MAX")
	set(HAVE_DECL_SSIZE_MAX "SSIZE_MAX")
	check_symbol_exists("strerror_r" "string.h" HAVE_DECL_STRERROR_R)
	set(HAVE_DECL_UINT32_MAX "UINT32_MAX")
	set(HAVE_DECL_UINT64_MAX "UINT64_MAX")
	check_include_file("dirent.h" HAVE_DIRENT_H)
	check_function_exists("dirfd" HAVE_DIRFD)
	check_include_file("dlfcn.h" HAVE_DLFCN_H)
	check_function_exists("_doprnt" HAVE_DOPRNT)
	check_symbol_exists("D_MD_ORDER" "langinfo.h" HAVE_D_MD_ORDER)
	check_symbol_exists("EFTYPE" "sys/errno.h" HAVE_EFTYPE)
	check_symbol_exists("EILSEQ" "sys/errno.h" HAVE_EILSEQ)
	check_include_file("errno.h" HAVE_ERRNO_H)
	check_include_file("expat.h" HAVE_EXPAT_H) #NOTE: It's present, if needed
	check_include_file("ext2fs/ext2_fs.h" HAVE_EXT2FS_EXT2_FS_H)
	check_function_exists("extattr_get_file" HAVE_EXTATTR_GET_FILE)
	check_function_exists("extattr_list_file" HAVE_EXTATTR_LIST_FILE)
	check_function_exists("extattr_set_fd" HAVE_EXTATTR_SET_FD)
	check_function_exists("extattr_set_file" HAVE_EXTATTR_SET_FILE)
	check_function_exists("fchdir" HAVE_FCHDIR)
	check_function_exists("fchflags" HAVE_FCHFLAGS)
	check_function_exists("fchmod" HAVE_FCHMOD)
	check_function_exists("fchown" HAVE_FCHOWN)
	check_function_exists("fcntl" HAVE_FCNTL)
	check_include_file("fcntl.h" HAVE_FCNTL_H)
	check_function_exists("fdopendir" HAVE_FDOPENDIR)
	check_function_exists("fgetea" HAVE_FGETEA)
	check_function_exists("fgetxattr" HAVE_FGETXATTR)
	check_function_exists("flistea" HAVE_FLISTEA)
	check_function_exists("flistxattr" HAVE_FLISTXATTR)
	check_function_exists("fork" HAVE_FORK)
	check_function_exists("fseeko" HAVE_FSEEKO)
	check_function_exists("fsetea" HAVE_FSETEA)
	check_function_exists("fsetxattr" HAVE_FSETXATTR)
	check_function_exists("fstat" HAVE_FSTAT)
	check_function_exists("fstatat" HAVE_FSTATAT)
	check_function_exists("fstatfs" HAVE_FSTATFS)
	check_function_exists("fstatvfs" HAVE_FSTATVFS)
	check_function_exists("ftruncate" HAVE_FTRUNCATE)
	check_function_exists("futimens" HAVE_FUTIMENS)
	check_function_exists("futimes" HAVE_FUTIMES)
	check_function_exists("futimesat" HAVE_FUTIMESAT)
	check_function_exists("getea" HAVE_GETEA)
	check_function_exists("geteuid" HAVE_GETEUID)
	check_function_exists("getgrgid_r" HAVE_GETGRGID_R)
	check_function_exists("getgrnam_r" HAVE_GETGRNAM_R)
	check_function_exists("getpid" HAVE_GETPID)
	check_function_exists("getpwnam_r" HAVE_GETPWNAM_R)
	check_function_exists("getpwuid_r" HAVE_GETPWUID_R)
	check_function_exists("getvfsbyname" HAVE_GETVFSBYNAME)
	check_function_exists("getxattr" HAVE_GETXATTR)
	check_function_exists("gmtime_r" HAVE_GMTIME_R)
	check_include_file("grp.h" HAVE_GRP_H)
	check_function_exists("iconv" HAVE_ICONV)
	check_include_file("iconv.h" HAVE_ICONV_H)
	check_type_size("intmax_t" SIZEOF_INTMAX_T)
	if (SIZEOF_INTMAX_T)
		set(HAVE_INTMAX_T ON)
	else()
		set(HAVE_INTMAX_T OFF)
	endif()
	check_include_file("inttypes.h" HAVE_INTTYPES_H)
	check_include_file("io.h" HAVE_IO_H)
	check_include_file("langinfo.h" HAVE_LANGINFO_H)
	check_function_exists("lchflags" HAVE_LCHFLAGS)
	check_function_exists("lchmod" HAVE_LCHMOD)
	check_function_exists("lchown" HAVE_LCHOWN)
	check_function_exists("lgetea" HAVE_LGETEA)
	check_function_exists("lgetxattr" HAVE_LGETXATTR)
	#cmakedefine HAVE_LIBACL 1
	#cmakedefine HAVE_LIBATTR 1
	#cmakedefine HAVE_LIBBZ2 1
	#cmakedefine HAVE_LIBCHARSET 1
	#cmakedefine HAVE_LIBCRYPTO 1
	#cmakedefine HAVE_LIBEAY32 1
	#cmakedefine HAVE_LIBEAY64 1
	#cmakedefine HAVE_LIBEXPAT 1
	#cmakedefine HAVE_LIBLZMA 1
	#cmakedefine HAVE_LIBLZMADEC 1
	#cmakedefine HAVE_LIBLZO2 1
	#cmakedefine HAVE_LIBMD 1
	#cmakedefine HAVE_LIBNETTLE 1
	#cmakedefine HAVE_LIBPCRE 1
	#cmakedefine HAVE_LIBPCREPOSIX 1
	#cmakedefine HAVE_LIBREGEX 1
	#cmakedefine HAVE_LIBXML2 1
	check_include_file("libxml/xmlreader.h" HAVE_LIBXML_XMLREADER_H)
	check_include_file("libxml/xmlwriter.h" HAVE_LIBXML_XMLWRITER_H)
	set(HAVE_LIBZ ON)
	check_include_file("limits.h" HAVE_LIMITS_H)
	check_function_exists("link" HAVE_LINK)
	check_include_file("linux/fiemap.h" HAVE_LINUX_FIEMAP_H)
	check_include_file("linux/fs.h" HAVE_LINUX_FS_H)
	check_include_file("linux/magic.h" HAVE_LINUX_MAGIC_H)
	check_include_file("linux/types.h" HAVE_LINUX_TYPES_H)
	check_function_exists("listea" HAVE_LISTEA)
	check_function_exists("listxattr" HAVE_LISTXATTR)
	check_function_exists("llistea" HAVE_LLISTEA)
	check_function_exists("llistxattr" HAVE_LLISTXATTR)
	check_include_file("localcharset.h" HAVE_LOCALCHARSET_H)
	check_function_exists("locale_charset" HAVE_LOCALE_CHARSET)
	check_include_file("locale.h" HAVE_LOCALE_H)
	check_function_exists("localtime_r" HAVE_LOCALTIME_R)
	check_type_size("long long int" SIZEOF_LONG_LONG_INT)
	if (SIZEOF_LONG_LONG_INT)
		set(HAVE_LONG_LONG_INT ON)
	else()
		set(HAVE_LONG_LONG_INT OFF)
	endif()
	check_function_exists("lsetea" HAVE_LSETEA)
	check_function_exists("lsetxattr" HAVE_LSETXATTR)
	check_function_exists("lstat" HAVE_LSTAT)
	unset(HAVE_LSTAT_EMPTY_STRING_BUG)
	check_function_exists("lutimes" HAVE_LUTIMES)
	check_include_file("lzmadec.h" HAVE_LZMADEC_H)
	check_include_file("lzma.h" HAVE_LZMA_H)
	check_include_file("lzo/lzo1x.h" HAVE_LZO_LZO1X_H)
	check_include_file("lzo/lzoconf.h" HAVE_LZO_LZOCONF_H)
	check_function_exists("mbrtowc" HAVE_MBRTOWC)
	check_include_file("md5.h" HAVE_MD5_H)
	check_function_exists("memmove" HAVE_MEMMOVE)
	check_include_file("memory.h" HAVE_MEMORY_H)
	check_function_exists("memset" HAVE_MEMSET)
	check_function_exists("mkdir" HAVE_MKDIR)
	check_function_exists("mkfifo" HAVE_MKFIFO)
	check_function_exists("mknod" HAVE_MKNOD)
	check_function_exists("mkstemp" HAVE_MKSTEMP)
	check_symbol_exists("DIR" "ndir.h" HAVE_NDIR_H)
	check_include_file("nettle/md5.h" HAVE_NETTLE_MD5_H)
	check_include_file("nettle/ripemd160.h" HAVE_NETTLE_RIPEMD160_H)
	check_include_file("nettle/sha.h" HAVE_NETTLE_SHA_H)
	check_function_exists("nl_langinfo" HAVE_NL_LANGINFO)
	check_function_exists("openat" HAVE_OPENAT)
	check_include_file("openssl/evp.h" HAVE_OPENSSL_EVP_H)
	check_include_file("paths.h" HAVE_PATHS_H)
	check_include_file("pcreposix.h" HAVE_PCREPOSIX_H)
	check_function_exists("pipe" HAVE_PIPE)
	check_function_exists("poll" HAVE_POLL)
	check_include_file("poll.h" HAVE_POLL_H)
	check_function_exists("posix_spawnp" HAVE_POSIX_SPAWNP)
	check_include_file("pwd.h" HAVE_PWD_H)
	#cmakedefine HAVE_READDIR_R 1
	check_function_exists("readlink" HAVE_READLINK)
	check_function_exists("readlinkat" HAVE_READLINKAT)
	check_include_file("regex.h" HAVE_REGEX_H)
	check_include_file("ripemd.h" HAVE_RIPEMD_H)
	check_function_exists("select" HAVE_SELECT)
	check_function_exists("setenv" HAVE_SETENV)
	check_function_exists("setlocale" HAVE_SETLOCALE)
	check_include_file("sha256.h" HAVE_SHA256_H)
	check_include_file("sha512.h" HAVE_SHA512_H)
	check_include_file("sha.h" HAVE_SHA_H)
	check_function_exists("sigaction" HAVE_SIGACTION)
	check_include_file("signal.h" HAVE_SIGNAL_H)
	check_include_file("spawn.h" HAVE_SPAWN_H)
	check_function_exists("statfs" HAVE_STATFS)
	check_function_exists("statvfs" HAVE_STATVFS)
	#cmakedefine HAVE_STAT_EMPTY_STRING_BUG 1
	check_include_file("stdarg.h" HAVE_STDARG_H)
	check_include_file("stdint.h" HAVE_STDINT_H)
	check_include_file("stdlib.h" HAVE_STDLIB_H)
	check_function_exists("strchr" HAVE_STRCHR)
	check_function_exists("strdup" HAVE_STRDUP)
	check_function_exists("strerror" HAVE_STRERROR)
	check_function_exists("strerror_r" HAVE_STRERROR_R)
	check_function_exists("strftime" HAVE_STRFTIME)
	check_include_file("strings.h" HAVE_STRINGS_H)
	check_include_file("string.h" HAVE_STRING_H)
	check_function_exists("strncpy_s" HAVE_STRNCPY_S)
	check_function_exists("strrchr" HAVE_STRRCHR)
	#cmakedefine HAVE_STRUCT_STATFS_F_NAMEMAX 1
	#cmakedefine HAVE_STRUCT_STATVFS_F_IOSIZE 1
	#cmakedefine HAVE_STRUCT_STAT_ST_BIRTHTIME 1
	#cmakedefine HAVE_STRUCT_STAT_ST_BIRTHTIMESPEC_TV_NSEC 1
	#cmakedefine HAVE_STRUCT_STAT_ST_BLKSIZE 1
	#cmakedefine HAVE_STRUCT_STAT_ST_FLAGS 1
	#cmakedefine HAVE_STRUCT_STAT_ST_MTIMESPEC_TV_NSEC 1
	#cmakedefine HAVE_STRUCT_STAT_ST_MTIME_N 1
	#cmakedefine HAVE_STRUCT_STAT_ST_MTIME_USEC 1
	#cmakedefine HAVE_STRUCT_STAT_ST_MTIM_TV_NSEC 1
	#cmakedefine HAVE_STRUCT_STAT_ST_UMTIME 1
	#cmakedefine HAVE_STRUCT_TM_TM_GMTOFF 1
	#cmakedefine HAVE_STRUCT_TM___TM_GMTOFF 1
	check_function_exists("symlink" HAVE_SYMLINK)
	if (CMAKE_TARGET_OS STREQUAL "macosx")
		# MacOSX has sys/acl.h file, but it's not POSIX-compatible and
		# requires "sys/types.h" to be included manually
		check_include_files("sys/types.h;sys/acl.h" HAVE_SYS_ACL_H)
	else()
		check_include_file("sys/acl.h" HAVE_SYS_ACL_H)
	endif()
	check_include_file("sys/cdefs.h" HAVE_SYS_CDEFS_H)
	check_symbol_exists("DIR" "sys/dir.h" HAVE_SYS_DIR_H)
	check_include_file("sys/ea.h" HAVE_SYS_EA_H)
	check_include_file("sys/extattr.h" HAVE_SYS_EXTATTR_H)
	check_include_file("sys/ioctl.h" HAVE_SYS_IOCTL_H)
	check_include_file("sys/mkdev.h" HAVE_SYS_MKDEV_H)
	check_include_file("sys/mount.h" HAVE_SYS_MOUNT_H)
	check_symbol_exists("DIR" "sys/ndir.h" HAVE_SYS_DIR_H)
	check_include_file("sys/param.h" HAVE_SYS_PARAM_H)
	check_include_file("sys/poll.h" HAVE_SYS_POLL_H)
	check_include_file("sys/select.h" HAVE_SYS_SELECT_H)
	check_include_file("sys/statfs.h" HAVE_SYS_STATFS_H)
	check_include_file("sys/statvfs.h" HAVE_SYS_STATVFS_H)
	check_include_file("sys/stat.h" HAVE_SYS_STAT_H)
	check_include_file("sys/time.h" HAVE_SYS_TIME_H)
	check_include_file("sys/types.h" HAVE_SYS_TYPES_H)
	check_include_file("sys/utime.h" HAVE_SYS_UTIME_H)
	check_include_file("sys/utsname.h" HAVE_SYS_UTSNAME_H)
	check_include_file("sys/vfs.h" HAVE_SYS_VFS_H)
	check_include_file("sys/wait.h" HAVE_SYS_WAIT_H)
	check_include_file("sys/xattr.h" HAVE_SYS_XATTR_H)
	check_function_exists("timegm" HAVE_TIMEGM)
	check_include_file("time.h" HAVE_TIME_H)
	check_function_exists("tzset" HAVE_TZSET)
	set(HAVE_UINTMAX_T ON)
	check_include_file("unistd.h" HAVE_UNISTD_H)
	check_function_exists("unsetenv" HAVE_UNSETENV)
	check_type_size("unsigned long long" SIZEOF_UNSIGNED_LONG_LONG)
	if (SIZEOF_UNSIGNED_LONG_LONG)
		set(HAVE_UNSIGNED_LONG_LONG ON)
	else()
		set(HAVE_UNSIGNED_LONG_LONG OFF)
	endif()
	check_type_size("unsigned long long int" SIZEOF_UNSIGNED_LONG_LONG_INT)
	if (SIZEOF_UNSIGNED_LONG_LONG_INT)
		set(HAVE_UNSIGNED_LONG_LONG_INT ON)
	else()
		set(HAVE_UNSIGNED_LONG_LONG_INT OFF)
	endif()
	check_function_exists("utime" HAVE_UTIME)
	check_function_exists("utimensat" HAVE_UTIMENSAT)
	check_function_exists("utimes" HAVE_UTIMES)
	check_include_file("utime.h" HAVE_UTIME_H)
	check_function_exists("vfork" HAVE_VFORK)
	check_function_exists("vprintf" HAVE_VPRINTF)
	check_include_file("wchar.h" HAVE_WCHAR_H)
	check_type_size("wchar_t" SIZEOF_WCHAR_T)
	if (SIZEOF_WCHAR_T)
		set(HAVE_WCHAR_T ON)
	else()
		set(HAVE_WCHAR_T OFF)
	endif()
	check_function_exists("wcrtomb" HAVE_WCRTOMB)
	check_function_exists("wcscmp" HAVE_WCSCMP)
	check_function_exists("wcscpy" HAVE_WCSCPY)
	check_function_exists("wcslen" HAVE_WCSLEN)
	check_function_exists("wctomb" HAVE_WCTOMB)
	check_include_file("wctype.h" HAVE_WCTYPE_H)
	check_include_file("wincrypt.h" HAVE_WINCRYPT_H)
	check_include_file("windows.h" HAVE_WINDOWS_H)
	check_include_file("winioctl.h" HAVE_WINIOCTL_H)
	check_function_exists("wmemcmp" HAVE_WMEMCMP)
	check_function_exists("wmemcpy" HAVE_WMEMCPY)
	#cmakedefine HAVE_WORKING_EXT2_IOC_GETFLAGS 1
	set(HAVE_ZLIB_H ON)
	check_function_exists("_ctime64_s" HAVE__CTIME64_S)
	check_function_exists("_fseeki64" HAVE__FSEEKI64)
	check_function_exists("_get_timezone" HAVE__GET_TIMEZONE)
	check_function_exists("_localtime64_s" HAVE__LOCALTIME64_S)
	check_function_exists("_mkgmtime64" HAVE__MKGMTIME64)
	if (HAVE_ICONV)
		# Check if iconv() takes const char ** (C)
		check_c_source_compiles("
			#include <iconv.h>
			int main(int argc, char ** argv)
			{ return iconv(0, (const char **)0, 0, (char**)0, 0); }
			" ICONV_NEEDS_C_CONST)
		# Check if iconv() takes char ** (C)
		check_c_source_compiles("
			#include <iconv.h>
			int main(int argc, char ** argv)
			{ return iconv(0, (char **)0, 0, (char**)0, 0); }
			" ICONV_NEEDS_C_NON_CONST)
		if (ICONV_NEEDS_C_CONST AND NOT ICONV_NEEDS_C_NON_CONST)
			set(ICONV_CONST "const")
		elseif (NOT ICONV_NEEDS_C_CONST AND ICONV_NEEDS_C_NON_CONST)
			set(ICONV_CONST "")
		elseif (ICONV_NEEDS_C_CONST AND ICONV_NEEDS_C_NON_CONST)
			# Doesn't matter
			set(ICONV_CONST "")
		else()
			message(FATAL_ERROR "iconv.h: Take a look on iconv() declaration")
		endif()
	else()
		unset(ICONV_CONST)
	endif()
	unset(LIBARCHIVE_VERSION_NUMBER)
	unset(LIBARCHIVE_VERSION_STRING)
	unset(LSTAT_FOLLOWS_SLASHED_SYMLINK)
	unset(LT_OBJDIR)
	unset(MAJOR_IN_MKDEV)
	unset(MAJOR_IN_SYSMACROS)
	unset(PACKAGE)
	unset(PACKAGE_BUGREPORT)
	unset(PACKAGE_NAME)
	unset(PACKAGE_STRING)
	unset(PACKAGE_TARNAME)
	unset(PACKAGE_URL)
	unset(PACKAGE_VERSION)
	#cmakedefine PCRE_STATIC 1
	check_include_files("stdlib.h;stdarg.h;string.h;float.h" STDC_HEADERS)
	#cmakedefine STRERROR_R_CHAR_P 1
	#cmakedefine TIME_WITH_SYS_TIME 1
	#unset(VERSION)
	unset(WINVER)
	unset(_FILE_OFFSET_BITS)
	set(_LARGEFILE_SOURCE ON)
	set(_LARGE_FILES ON)
	#cmakedefine _MINIX 1
	#cmakedefine _POSIX_1_SOURCE 2
	#cmakedefine _POSIX_SOURCE 1
	unset(_WIN32_WINNT)
	unset(const)
	check_type_size("gid_t" SIZEOF_GID_T)
	if (NOT SIZEOF_GID_T)
		set(gid_t "unsigned long")
	endif()
	check_type_size("id_t" SIZEOF_ID_T)
	if (NOT SIZEOF_ID_T)
		set(id_t "unsigned long")
	endif()
	unset(int16_t)
	unset(int32_t)
	unset(int64_t)
	unset(intmax_t)
	check_type_size("mode_t" SIZEOF_MODE_T)
	if (NOT SIZEOF_MODE_T)
		# Switch to plain 'int' for libarchive 4.0.  It's less broken than 'mode_t'
		set(mode_t "unsigned short")
	endif()
	check_type_size("off_t" SIZEOF_OFF_T)
	if (NOT SIZEOF_OFF_T)
		set(off_t "long long")
	endif()
	unset(size_t)
	check_type_size("uid_t" SIZEOF_UID_T)
	if (NOT SIZEOF_UID_T)
		set(uid_t "unsigned long")
	endif()
	unset(uint16_t)
	unset(uint32_t)
	unset(uint64_t)
	unset(uint8_t)
	unset(uintmax_t)
	unset(uintptr_t)
	check_type_size("pid_t" SIZEOF_PID_T)
	if (NOT SIZEOF_PID_T)
		set(pid_t "int")
	endif()

	if (CMAKE_TARGET_OS STREQUAL "windows")
		set(HAVE_WINCRYPT_H ON)
	endif()

	configure_file("config.h.cmake" "${CMAKE_CURRENT_BINARY_DIR}/config.h")

	restore(CMAKE_REQUIRED_FLAGS)
endif()

set(target_specific_sources "")
if (CMAKE_TARGET_OS STREQUAL "windows")
	set(target_specific_sources ${target_specific_sources}
		"upstream.patched/libarchive/archive_entry_copy_bhfi.c"
		"upstream.patched/libarchive/archive_read_disk_windows.c"
		"upstream.patched/libarchive/archive_windows.c"
		"upstream.patched/libarchive/archive_windows.h"
		"upstream.patched/libarchive/archive_write_disk_windows.c"
		"upstream.patched/libarchive/filter_fork_windows.c"
	)
endif()

add_library(archive_static STATIC
	"upstream.patched/libarchive/archive.h"
	"upstream.patched/libarchive/archive_entry.h"
	"upstream.patched/libarchive/archive_acl.c"
	"upstream.patched/libarchive/archive_check_magic.c"
	"upstream.patched/libarchive/archive_cmdline.c"
	"upstream.patched/libarchive/archive_cmdline_private.h"
	"upstream.patched/libarchive/archive_crc32.h"
	"upstream.patched/libarchive/archive_crypto.c"
	"upstream.patched/libarchive/archive_crypto_private.h"
	"upstream.patched/libarchive/archive_endian.h"
	"upstream.patched/libarchive/archive_entry.c"
	"upstream.patched/libarchive/archive_entry.h"
	"upstream.patched/libarchive/archive_entry_copy_stat.c"
	"upstream.patched/libarchive/archive_entry_link_resolver.c"
	"upstream.patched/libarchive/archive_entry_locale.h"
	"upstream.patched/libarchive/archive_entry_private.h"
	"upstream.patched/libarchive/archive_entry_sparse.c"
	"upstream.patched/libarchive/archive_entry_stat.c"
	"upstream.patched/libarchive/archive_entry_strmode.c"
	"upstream.patched/libarchive/archive_entry_xattr.c"
	"upstream.patched/libarchive/archive_getdate.c"
	"upstream.patched/libarchive/archive_match.c"
	"upstream.patched/libarchive/archive_options.c"
	"upstream.patched/libarchive/archive_options_private.h"
	"upstream.patched/libarchive/archive_pathmatch.c"
	"upstream.patched/libarchive/archive_pathmatch.h"
	"upstream.patched/libarchive/archive_platform.h"
	"upstream.patched/libarchive/archive_ppmd_private.h"
	"upstream.patched/libarchive/archive_ppmd7.c"
	"upstream.patched/libarchive/archive_ppmd7_private.h"
	"upstream.patched/libarchive/archive_private.h"
	"upstream.patched/libarchive/archive_rb.c"
	"upstream.patched/libarchive/archive_rb.h"
	"upstream.patched/libarchive/archive_read.c"
	"upstream.patched/libarchive/archive_read_append_filter.c"
	"upstream.patched/libarchive/archive_read_data_into_fd.c"
	"upstream.patched/libarchive/archive_read_disk_entry_from_file.c"
	"upstream.patched/libarchive/archive_read_disk_posix.c"
	"upstream.patched/libarchive/archive_read_disk_private.h"
	"upstream.patched/libarchive/archive_read_disk_set_standard_lookup.c"
	"upstream.patched/libarchive/archive_read_extract.c"
	"upstream.patched/libarchive/archive_read_open_fd.c"
	"upstream.patched/libarchive/archive_read_open_file.c"
	"upstream.patched/libarchive/archive_read_open_filename.c"
	"upstream.patched/libarchive/archive_read_open_memory.c"
	"upstream.patched/libarchive/archive_read_private.h"
	"upstream.patched/libarchive/archive_read_set_format.c"
	"upstream.patched/libarchive/archive_read_set_options.c"
	"upstream.patched/libarchive/archive_read_support_filter_all.c"
	"upstream.patched/libarchive/archive_read_support_filter_bzip2.c"
	"upstream.patched/libarchive/archive_read_support_filter_compress.c"
	"upstream.patched/libarchive/archive_read_support_filter_gzip.c"
	"upstream.patched/libarchive/archive_read_support_filter_grzip.c"
	"upstream.patched/libarchive/archive_read_support_filter_lrzip.c"
	"upstream.patched/libarchive/archive_read_support_filter_lzop.c"
	"upstream.patched/libarchive/archive_read_support_filter_none.c"
	"upstream.patched/libarchive/archive_read_support_filter_program.c"
	"upstream.patched/libarchive/archive_read_support_filter_rpm.c"
	"upstream.patched/libarchive/archive_read_support_filter_uu.c"
	"upstream.patched/libarchive/archive_read_support_filter_xz.c"
	"upstream.patched/libarchive/archive_read_support_format_7zip.c"
	"upstream.patched/libarchive/archive_read_support_format_all.c"
	"upstream.patched/libarchive/archive_read_support_format_ar.c"
	"upstream.patched/libarchive/archive_read_support_format_by_code.c"
	"upstream.patched/libarchive/archive_read_support_format_cab.c"
	"upstream.patched/libarchive/archive_read_support_format_cpio.c"
	"upstream.patched/libarchive/archive_read_support_format_empty.c"
	"upstream.patched/libarchive/archive_read_support_format_iso9660.c"
	"upstream.patched/libarchive/archive_read_support_format_lha.c"
	"upstream.patched/libarchive/archive_read_support_format_mtree.c"
	"upstream.patched/libarchive/archive_read_support_format_rar.c"
	"upstream.patched/libarchive/archive_read_support_format_raw.c"
	"upstream.patched/libarchive/archive_read_support_format_tar.c"
	"upstream.patched/libarchive/archive_read_support_format_xar.c"
	"upstream.patched/libarchive/archive_read_support_format_zip.c"
	"upstream.patched/libarchive/archive_string.c"
	"upstream.patched/libarchive/archive_string.h"
	"upstream.patched/libarchive/archive_string_composition.h"
	"upstream.patched/libarchive/archive_string_sprintf.c"
	"upstream.patched/libarchive/archive_util.c"
	"upstream.patched/libarchive/archive_virtual.c"
	"upstream.patched/libarchive/archive_write.c"
	"upstream.patched/libarchive/archive_write_disk_acl.c"
	"upstream.patched/libarchive/archive_write_disk_posix.c"
	"upstream.patched/libarchive/archive_write_disk_private.h"
	"upstream.patched/libarchive/archive_write_disk_set_standard_lookup.c"
	"upstream.patched/libarchive/archive_write_private.h"
	"upstream.patched/libarchive/archive_write_open_fd.c"
	"upstream.patched/libarchive/archive_write_open_file.c"
	"upstream.patched/libarchive/archive_write_open_filename.c"
	"upstream.patched/libarchive/archive_write_open_memory.c"
	"upstream.patched/libarchive/archive_write_add_filter.c"
	"upstream.patched/libarchive/archive_write_add_filter_b64encode.c"
	"upstream.patched/libarchive/archive_write_add_filter_by_name.c"
	"upstream.patched/libarchive/archive_write_add_filter_bzip2.c"
	"upstream.patched/libarchive/archive_write_add_filter_compress.c"
	"upstream.patched/libarchive/archive_write_add_filter_grzip.c"
	"upstream.patched/libarchive/archive_write_add_filter_gzip.c"
	"upstream.patched/libarchive/archive_write_add_filter_lrzip.c"
	"upstream.patched/libarchive/archive_write_add_filter_lzop.c"
	"upstream.patched/libarchive/archive_write_add_filter_none.c"
	"upstream.patched/libarchive/archive_write_add_filter_program.c"
	"upstream.patched/libarchive/archive_write_add_filter_uuencode.c"
	"upstream.patched/libarchive/archive_write_add_filter_xz.c"
	"upstream.patched/libarchive/archive_write_set_format.c"
	"upstream.patched/libarchive/archive_write_set_format_7zip.c"
	"upstream.patched/libarchive/archive_write_set_format_ar.c"
	"upstream.patched/libarchive/archive_write_set_format_by_name.c"
	"upstream.patched/libarchive/archive_write_set_format_cpio.c"
	"upstream.patched/libarchive/archive_write_set_format_cpio_newc.c"
	"upstream.patched/libarchive/archive_write_set_format_gnutar.c"
	"upstream.patched/libarchive/archive_write_set_format_iso9660.c"
	"upstream.patched/libarchive/archive_write_set_format_mtree.c"
	"upstream.patched/libarchive/archive_write_set_format_pax.c"
	"upstream.patched/libarchive/archive_write_set_format_shar.c"
	"upstream.patched/libarchive/archive_write_set_format_ustar.c"
	"upstream.patched/libarchive/archive_write_set_format_v7tar.c"
	"upstream.patched/libarchive/archive_write_set_format_xar.c"
	"upstream.patched/libarchive/archive_write_set_format_zip.c"
	"upstream.patched/libarchive/archive_write_set_options.c"
	"upstream.patched/libarchive/filter_fork_posix.c"
	"upstream.patched/libarchive/filter_fork.h"
	${target_specific_sources}
)

add_dependencies(archive_static
	z_static
)

set(target_specific_private_definitions "")
if (CMAKE_TARGET_OS STREQUAL "windows")
	set(target_specific_private_definitions ${target_specific_private_definitions}
		-D_SSIZE_T_DEFINED
		-Dssize_t=SSIZE_T
	)
	if (CMAKE_COMPILER_FAMILY STREQUAL "msvc")
		set(target_specific_private_definitions ${target_specific_private_definitions}
			-D_CRT_SECURE_NO_WARNINGS
		)
	endif()
endif()

target_compile_definitions(archive_static
	PRIVATE
		-DPLATFORM_CONFIG_H="config.h"
		${target_specific_private_definitions}
	PUBLIC
		-DLIBARCHIVE_STATIC
)
target_include_directories(archive_static
	PRIVATE
		"${CMAKE_CURRENT_BINARY_DIR}"
	PUBLIC
		"upstream.patched"
		"upstream.patched/libarchive"
)
target_link_libraries(archive_static
	LINK_PUBLIC
		z_static
		${BZIP2_LIBRARIES}
)
