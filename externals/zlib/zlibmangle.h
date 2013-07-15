#ifndef _OSMAND_ZLIB_MANGLE_
#define _OSMAND_ZLIB_MANGLE_

// Make sure that we'll use prefixes
#ifndef Z_PREFIX
#    define z_Z_PREFIX
#endif

// Override default prefixes with our own
#define z__dist_code            osmand_zlib__dist_code
#define z__length_code          osmand_zlib__length_code
#define z__tr_align             osmand_zlib__tr_align
#define z__tr_flush_block       osmand_zlib__tr_flush_block
#define z__tr_init              osmand_zlib__tr_init
#define z__tr_stored_block      osmand_zlib__tr_stored_block
#define z__tr_tally             osmand_zlib__tr_tally
#define z_adler32               osmand_zlib_adler32
#define z_adler32_combine       osmand_zlib_adler32_combine
#define z_adler32_combine64     osmand_zlib_adler32_combine64
#  ifndef Z_SOLO
#    define z_compress              osmand_zlib_compress
#    define z_compress2             osmand_zlib_compress2
#    define z_compressBound         osmand_zlib_compressBound
#  endif
#define z_crc32                 osmand_zlib_crc32
#define z_crc32_combine         osmand_zlib_crc32_combine
#define z_crc32_combine64       osmand_zlib_crc32_combine64
#define z_deflate               osmand_zlib_deflate
#define z_deflateBound          osmand_zlib_deflateBound
#define z_deflateCopy           osmand_zlib_deflateCopy
#define z_deflateEnd            osmand_zlib_deflateEnd
#define z_deflateInit2_         osmand_zlib_deflateInit2_
#define z_deflateInit_          osmand_zlib_deflateInit_
#define z_deflateParams         osmand_zlib_deflateParams
#define z_deflatePending        osmand_zlib_deflatePending
#define z_deflatePrime          osmand_zlib_deflatePrime
#define z_deflateReset          osmand_zlib_deflateReset
#define z_deflateResetKeep      osmand_zlib_deflateResetKeep
#define z_deflateSetDictionary  osmand_zlib_deflateSetDictionary
#define z_deflateSetHeader      osmand_zlib_deflateSetHeader
#define z_deflateTune           osmand_zlib_deflateTune
#define z_deflate_copyright     osmand_zlib_deflate_copyright
#define z_get_crc_table         osmand_zlib_get_crc_table
#  ifndef Z_SOLO
#    define z_gz_error              osmand_zlib_gz_error
#    define z_gz_intmax             osmand_zlib_gz_intmax
#    define z_gz_strwinerror        osmand_zlib_gz_strwinerror
#    define z_gzbuffer              osmand_zlib_gzbuffer
#    define z_gzclearerr            osmand_zlib_gzclearerr
#    define z_gzclose               osmand_zlib_gzclose
#    define z_gzclose_r             osmand_zlib_gzclose_r
#    define z_gzclose_w             osmand_zlib_gzclose_w
#    define z_gzdirect              osmand_zlib_gzdirect
#    define z_gzdopen               osmand_zlib_gzdopen
#    define z_gzeof                 osmand_zlib_gzeof
#    define z_gzerror               osmand_zlib_gzerror
#    define z_gzflush               osmand_zlib_gzflush
#    define z_gzgetc                osmand_zlib_gzgetc
#    define z_gzgetc_               osmand_zlib_gzgetc_
#    define z_gzgets                osmand_zlib_gzgets
#    define z_gzoffset              osmand_zlib_gzoffset
#    define z_gzoffset64            osmand_zlib_gzoffset64
#    define z_gzopen                osmand_zlib_gzopen
#    define z_gzopen64              osmand_zlib_gzopen64
#    ifdef _WIN32
#      define z_gzopen_w              osmand_zlib_gzopen_w
#    endif
#    define z_gzprintf              osmand_zlib_gzprintf
#    define z_gzputc                osmand_zlib_gzputc
#    define z_gzputs                osmand_zlib_gzputs
#    define z_gzread                osmand_zlib_gzread
#    define z_gzrewind              osmand_zlib_gzrewind
#    define z_gzseek                osmand_zlib_gzseek
#    define z_gzseek64              osmand_zlib_gzseek64
#    define z_gzsetparams           osmand_zlib_gzsetparams
#    define z_gztell                osmand_zlib_gztell
#    define z_gztell64              osmand_zlib_gztell64
#    define z_gzungetc              osmand_zlib_gzungetc
#    define z_gzwrite               osmand_zlib_gzwrite
#  endif
#define z_inflate               osmand_zlib_inflate
#define z_inflateBack           osmand_zlib_inflateBack
#define z_inflateBackEnd        osmand_zlib_inflateBackEnd
#define z_inflateBackInit_      osmand_zlib_inflateBackInit_
#define z_inflateCopy           osmand_zlib_inflateCopy
#define z_inflateEnd            osmand_zlib_inflateEnd
#define z_inflateGetHeader      osmand_zlib_inflateGetHeader
#define z_inflateInit2_         osmand_zlib_inflateInit2_
#define z_inflateInit_          osmand_zlib_inflateInit_
#define z_inflateMark           osmand_zlib_inflateMark
#define z_inflatePrime          osmand_zlib_inflatePrime
#define z_inflateReset          osmand_zlib_inflateReset
#define z_inflateReset2         osmand_zlib_inflateReset2
#define z_inflateSetDictionary  osmand_zlib_inflateSetDictionary
#define z_inflateSync           osmand_zlib_inflateSync
#define z_inflateSyncPoint      osmand_zlib_inflateSyncPoint
#define z_inflateUndermine      osmand_zlib_inflateUndermine
#define z_inflateResetKeep      osmand_zlib_inflateResetKeep
#define z_inflate_copyright     osmand_zlib_inflate_copyright
#define z_inflate_fast          osmand_zlib_inflate_fast
#define z_inflate_table         osmand_zlib_inflate_table
#  ifndef Z_SOLO
#    define z_uncompress            osmand_zlib_uncompress
#  endif
#define z_zError                osmand_zlib_zError
#  ifndef Z_SOLO
#    define z_zcalloc               osmand_zlib_zcalloc
#    define z_zcfree                osmand_zlib_zcfree
#  endif
#define z_zlibCompileFlags      osmand_zlib_zlibCompileFlags
#define z_zlibVersion           osmand_zlib_zlibVersion
#define z_errmsg                osmand_zlib_errmsg

/* all zlib typedefs in zlib.h and zconf.h */
#define z_Byte                  osmand_zlib_Byte
#define z_Bytef                 osmand_zlib_Bytef
#define z_alloc_func            osmand_zlib_alloc_func
#define z_charf                 osmand_zlib_charf
#define z_free_func             osmand_zlib_free_func
#  ifndef Z_SOLO
#    define z_gzFile                osmand_zlib_gzFile
#  endif
#define z_gz_header             osmand_zlib_gz_header
#define z_gz_headerp            osmand_zlib_gz_headerp
#define z_in_func               osmand_zlib_in_func
#define z_intf                  osmand_zlib_intf
#define z_out_func              osmand_zlib_out_func
#define z_uInt                  osmand_zlib_uInt
#define z_uIntf                 osmand_zlib_uIntf
#define z_uLong                 osmand_zlib_uLong
#define z_uLongf                osmand_zlib_uLongf
#define z_voidp                 osmand_zlib_voidp
#define z_voidpc                osmand_zlib_voidpc
#define z_voidpf                osmand_zlib_voidpf

/* all zlib structs in zlib.h and zconf.h */
#define z_gz_header_s           osmand_zlib_gz_header_s
#define z_internal_state        osmand_zlib_internal_state

#endif // _OSMAND_ZLIB_MANGLE_
