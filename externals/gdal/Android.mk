include $(CLEAR_VARS)

LOCAL_PATH := $(call my-dir)

ifneq ($(OSMAND_BUILDING_NEON_LIBRARY),true)
    LOCAL_MODULE := osmand_gdal
else
    LOCAL_MODULE := osmand_gdal_neon
    LOCAL_ARM_NEON := true
endif

LOCAL_EXPORT_CFLAGS :=
LOCAL_EXPORT_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/upstream.patched/gcore \
    $(LOCAL_PATH)/upstream.patched/ogr \
    $(LOCAL_PATH)/upstream.patched/alg \
    $(LOCAL_PATH)/upstream.patched/port

LOCAL_SHARED_LIBRARIES := \
    libz

ifneq ($(OSMAND_USE_PREBUILT),true)
    LOCAL_CFLAGS := \
        $(LOCAL_EXPORT_CFLAGS) \
        -DFRMT_gtiff

    LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/upstream.patched/frmts/gtiff \
        $(LOCAL_PATH)/upstream.patched/frmts/gtiff/libtiff \
        $(LOCAL_PATH)/upstream.patched/frmts/gtiff/libgeotiff \
        $(LOCAL_PATH)/upstream.patched/ogr/ogrsf_frmts \
        $(LOCAL_PATH)/upstream.patched/ogr/ogrsf_frmts/mem

    SRC_FILES := \
        $(wildcard $(LOCAL_PATH)/upstream.patched/gcore/*.cpp) \
        $(wildcard $(LOCAL_PATH)/upstream.patched/alg/*.cpp) \
        $(wildcard $(LOCAL_PATH)/upstream.patched/ogr/*.cpp) \
        $(wildcard $(LOCAL_PATH)/upstream.patched/ogr/ogrsf_frmts/mem/*.cpp) \
        $(wildcard $(LOCAL_PATH)/upstream.patched/frmts/*.c*) \
        $(wildcard $(LOCAL_PATH)/upstream.patched/frmts/gtiff/*.c*) \
        $(wildcard $(LOCAL_PATH)/upstream.patched/frmts/gtiff/libtiff/*.c*) \
        $(wildcard $(LOCAL_PATH)/upstream.patched/frmts/gtiff/libgeotiff/*.c*)
    LOCAL_SRC_FILES := \
        $(SRC_FILES:$(LOCAL_PATH)/%=%) \
        upstream.patched/alg/gdal_crs.c \
        upstream.patched/alg/gdalwarpkernel_opencl.c \
        upstream.patched/frmts/hfa/hfaband.cpp \
        upstream.patched/frmts/hfa/hfadataset.cpp \
        upstream.patched/frmts/hfa/hfadictionary.cpp \
        upstream.patched/frmts/hfa/hfaentry.cpp \
        upstream.patched/frmts/hfa/hfafield.cpp \
        upstream.patched/frmts/hfa/hfaopen.cpp \
        upstream.patched/frmts/hfa/hfatype.cpp \
        upstream.patched/frmts/hfa/hfacompress.cpp \
        upstream.patched/frmts/hfa/hfa_overviews.cpp \
        upstream.patched/port/cpl_atomic_ops.cpp \
        upstream.patched/port/cpl_base64.cpp \
        upstream.patched/port/cpl_conv.cpp \
        upstream.patched/port/cpl_csv.cpp \
        upstream.patched/port/cpl_error.cpp \
        upstream.patched/port/cpl_findfile.cpp \
        upstream.patched/port/cpl_getexecpath.cpp \
        upstream.patched/port/cpl_google_oauth2.cpp \
        upstream.patched/port/cpl_hash_set.cpp \
        upstream.patched/port/cpl_http.cpp \
        upstream.patched/port/cpl_list.cpp \
        upstream.patched/port/cpl_minixml.cpp \
        upstream.patched/port/cpl_minizip_ioapi.cpp \
        upstream.patched/port/cpl_minizip_unzip.cpp \
        upstream.patched/port/cpl_minizip_zip.cpp \
        upstream.patched/port/cpl_multiproc.cpp \
        upstream.patched/port/cpl_path.cpp \
        upstream.patched/port/cpl_progress.cpp \
        upstream.patched/port/cpl_quad_tree.cpp \
        upstream.patched/port/cpl_recode.cpp \
        upstream.patched/port/cpl_recode_iconv.cpp \
        upstream.patched/port/cpl_recode_stub.cpp \
        upstream.patched/port/cpl_spawn.cpp \
        upstream.patched/port/cpl_string.cpp \
        upstream.patched/port/cpl_strtod.cpp \
        upstream.patched/port/cpl_time.cpp \
        upstream.patched/port/cpl_vsi_mem.cpp \
        upstream.patched/port/cpl_vsil.cpp \
        upstream.patched/port/cpl_vsil_abstract_archive.cpp \
        upstream.patched/port/cpl_vsil_buffered_reader.cpp \
        upstream.patched/port/cpl_vsil_cache.cpp \
        upstream.patched/port/cpl_vsil_curl.cpp \
        upstream.patched/port/cpl_vsil_curl_streaming.cpp \
        upstream.patched/port/cpl_vsil_gzip.cpp \
        upstream.patched/port/cpl_vsil_sparsefile.cpp \
        upstream.patched/port/cpl_vsil_stdin.cpp \
        upstream.patched/port/cpl_vsil_stdout.cpp \
        upstream.patched/port/cpl_vsil_subfile.cpp \
        upstream.patched/port/cpl_vsil_tar.cpp \
        upstream.patched/port/cpl_vsisimple.cpp \
        upstream.patched/port/cpl_xml_validate.cpp \
        upstream.patched/port/cplgetsymbol.cpp \
        upstream.patched/port/cplkeywordparser.cpp \
        upstream.patched/port/cplstring.cpp \
        upstream.patched/port/cplstringlist.cpp \
        upstream.patched/port/cpl_vsil_unix_stdio_64.cpp

    include $(BUILD_STATIC_LIBRARY)
else
    LOCAL_SRC_FILES := \
        $(OSMAND_ANDROID_PREBUILT_ROOT)/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE).a
    include $(PREBUILT_STATIC_LIBRARY)
endif
