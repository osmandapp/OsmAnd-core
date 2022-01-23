project(archive)

prepare_upstream()

set(HAVE_LIBZ ON)
set(HAVE_ZLIB_H ON)
set(HAVE_LIBEXPAT ON)

include("config.cmake")

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
elseif (CMAKE_TARGET_OS STREQUAL "macosx" OR CMAKE_TARGET_OS STREQUAL "ios")
	set(target_specific_sources ${target_specific_sources}
		"upstream.patched/libarchive/archive_disk_acl_darwin.c"		
	)
elseif (CMAKE_TARGET_OS STREQUAL "linux" OR CMAKE_TARGET_OS STREQUAL "android")
	set(target_specific_sources ${target_specific_sources}
		"upstream.patched/libarchive/archive_disk_acl_linux.c"		
	)
endif()

set(target_specific_include_dirs_private "")
if (CMAKE_TARGET_OS STREQUAL "android")
	set(target_specific_include_dirs_private ${target_specific_include_dirs_private}
		"upstream.patched/contrib/android/include"
	)
endif()

add_library(archive_static STATIC
	"upstream.patched/libarchive/archive_acl.c"
	"upstream.patched/libarchive/archive_acl_private.h"
	"upstream.patched/libarchive/archive_check_magic.c"
	"upstream.patched/libarchive/archive_cmdline.c"
	"upstream.patched/libarchive/archive_cmdline_private.h"
	"upstream.patched/libarchive/archive_crc32.h"
	"upstream.patched/libarchive/archive_cryptor.c"
	"upstream.patched/libarchive/archive_cryptor_private.h"
	"upstream.patched/libarchive/archive_digest.c"
	"upstream.patched/libarchive/archive_digest_private.h"
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
	"upstream.patched/libarchive/archive_getdate.h"
	"upstream.patched/libarchive/archive_hmac.c"
	"upstream.patched/libarchive/archive_hmac_private.h"
	"upstream.patched/libarchive/archive_match.c"
	"upstream.patched/libarchive/archive_openssl_evp_private.h"
	"upstream.patched/libarchive/archive_openssl_hmac_private.h"
	"upstream.patched/libarchive/archive_options.c"
	"upstream.patched/libarchive/archive_options_private.h"
	"upstream.patched/libarchive/archive_pack_dev.h"
	"upstream.patched/libarchive/archive_pack_dev.c"
	"upstream.patched/libarchive/archive_pathmatch.c"
	"upstream.patched/libarchive/archive_pathmatch.h"
	"upstream.patched/libarchive/archive_platform.h"
	"upstream.patched/libarchive/archive_platform_acl.h"
	"upstream.patched/libarchive/archive_platform_xattr.h"
	"upstream.patched/libarchive/archive_ppmd_private.h"
	"upstream.patched/libarchive/archive_ppmd8.c"
	"upstream.patched/libarchive/archive_ppmd8_private.h"
	"upstream.patched/libarchive/archive_ppmd7.c"
	"upstream.patched/libarchive/archive_ppmd7_private.h"
	"upstream.patched/libarchive/archive_private.h"
	"upstream.patched/libarchive/archive_random.c"
	"upstream.patched/libarchive/archive_random_private.h"
	"upstream.patched/libarchive/archive_rb.c"
	"upstream.patched/libarchive/archive_rb.h"
	"upstream.patched/libarchive/archive_read.c"
	"upstream.patched/libarchive/archive_read_add_passphrase.c"
	"upstream.patched/libarchive/archive_read_append_filter.c"
	"upstream.patched/libarchive/archive_read_data_into_fd.c"
	"upstream.patched/libarchive/archive_read_disk_entry_from_file.c"
	"upstream.patched/libarchive/archive_read_disk_posix.c"
	"upstream.patched/libarchive/archive_read_disk_private.h"
	"upstream.patched/libarchive/archive_read_disk_set_standard_lookup.c"
	"upstream.patched/libarchive/archive_read_extract.c"
	"upstream.patched/libarchive/archive_read_extract2.c"
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
	"upstream.patched/libarchive/archive_read_support_filter_lz4.c"
	"upstream.patched/libarchive/archive_read_support_filter_lzop.c"
	"upstream.patched/libarchive/archive_read_support_filter_none.c"
	"upstream.patched/libarchive/archive_read_support_filter_program.c"
	"upstream.patched/libarchive/archive_read_support_filter_rpm.c"
	"upstream.patched/libarchive/archive_read_support_filter_uu.c"
	"upstream.patched/libarchive/archive_read_support_filter_xz.c"
	"upstream.patched/libarchive/archive_read_support_filter_zstd.c"
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
	"upstream.patched/libarchive/archive_read_support_format_rar5.c"
	"upstream.patched/libarchive/archive_read_support_format_raw.c"
	"upstream.patched/libarchive/archive_read_support_format_tar.c"
	"upstream.patched/libarchive/archive_read_support_format_warc.c"
	"upstream.patched/libarchive/archive_read_support_format_xar.c"
	"upstream.patched/libarchive/archive_read_support_format_zip.c"
	"upstream.patched/libarchive/archive_string.c"
	"upstream.patched/libarchive/archive_string.h"
	"upstream.patched/libarchive/archive_string_composition.h"
	"upstream.patched/libarchive/archive_string_sprintf.c"
	"upstream.patched/libarchive/archive_util.c"
	"upstream.patched/libarchive/archive_version_details.c"
	"upstream.patched/libarchive/archive_virtual.c"
	"upstream.patched/libarchive/archive_write.c"
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
	"upstream.patched/libarchive/archive_write_add_filter_lz4.c"
	"upstream.patched/libarchive/archive_write_add_filter_lzop.c"
	"upstream.patched/libarchive/archive_write_add_filter_none.c"
	"upstream.patched/libarchive/archive_write_add_filter_program.c"
	"upstream.patched/libarchive/archive_write_add_filter_uuencode.c"
	"upstream.patched/libarchive/archive_write_add_filter_xz.c"
	"upstream.patched/libarchive/archive_write_add_filter_zstd.c"
	"upstream.patched/libarchive/archive_write_set_format.c"
	"upstream.patched/libarchive/archive_write_set_format_7zip.c"
	"upstream.patched/libarchive/archive_write_set_format_ar.c"
	"upstream.patched/libarchive/archive_write_set_format_by_name.c"
	"upstream.patched/libarchive/archive_write_set_format_cpio.c"
	"upstream.patched/libarchive/archive_write_set_format_cpio_binary.c"
	"upstream.patched/libarchive/archive_write_set_format_cpio_newc.c"
	"upstream.patched/libarchive/archive_write_set_format_cpio_odc.c"
	"upstream.patched/libarchive/archive_write_set_format_filter_by_ext.c"
	"upstream.patched/libarchive/archive_write_set_format_gnutar.c"
	"upstream.patched/libarchive/archive_write_set_format_iso9660.c"
	"upstream.patched/libarchive/archive_write_set_format_mtree.c"
	"upstream.patched/libarchive/archive_write_set_format_pax.c"
	"upstream.patched/libarchive/archive_write_set_format_private.h"
	"upstream.patched/libarchive/archive_write_set_format_raw.c"
	"upstream.patched/libarchive/archive_write_set_format_shar.c"
	"upstream.patched/libarchive/archive_write_set_format_ustar.c"
	"upstream.patched/libarchive/archive_write_set_format_v7tar.c"
	"upstream.patched/libarchive/archive_write_set_format_warc.c"
	"upstream.patched/libarchive/archive_write_set_format_xar.c"
	"upstream.patched/libarchive/archive_write_set_format_zip.c"
	"upstream.patched/libarchive/archive_write_set_options.c"
	"upstream.patched/libarchive/archive_write_set_passphrase.c"
	"upstream.patched/libarchive/archive_xxhash.h"
	"upstream.patched/libarchive/filter_fork_posix.c"
	"upstream.patched/libarchive/filter_fork.h"
	"upstream.patched/libarchive/xxhash.c"
	"upstream.patched/libarchive/archive_blake2sp_ref.c"
  	"upstream.patched/libarchive/archive_blake2s_ref.c"
	${target_specific_sources}
)

add_dependencies(archive_static
	z_static
	expat_static
)

set(target_specific_private_definitions "")
if (CMAKE_TARGET_OS STREQUAL "windows")
	if (CMAKE_COMPILER_FAMILY STREQUAL "msvc")
		set(target_specific_private_definitions ${target_specific_private_definitions}
			-D_CRT_SECURE_NO_WARNINGS
		)
	endif()
endif()

target_compile_definitions(archive_static
	PRIVATE
		-DPLATFORM_CONFIG_H="config.h"
		-D__LIBARCHIVE_BUILD
		${target_specific_private_definitions}
	PUBLIC
		-DLIBARCHIVE_STATIC
)
target_include_directories(archive_static
	PRIVATE
		"${CMAKE_CURRENT_BINARY_DIR}"
		${target_specific_include_dirs_private}
	PUBLIC
		"upstream.patched"
		"upstream.patched/libarchive"
)
target_link_libraries(archive_static
	LINK_PUBLIC
		z_static
		expat_static
)
