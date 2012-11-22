CC_LITE_SRC_FILES := \
    upstream.patched/src/google/protobuf/stubs/common.cc                              \
    upstream.patched/src/google/protobuf/stubs/once.cc                                \
    upstream.patched/src/google/protobuf/stubs/hash.cc                                \
    upstream.patched/src/google/protobuf/extension_set.cc                             \
    upstream.patched/src/google/protobuf/generated_message_util.cc                    \
    upstream.patched/src/google/protobuf/message_lite.cc                              \
    upstream.patched/src/google/protobuf/repeated_field.cc                            \
    upstream.patched/src/google/protobuf/wire_format_lite.cc                          \
    upstream.patched/src/google/protobuf/io/coded_stream.cc                           \
    upstream.patched/src/google/protobuf/io/zero_copy_stream.cc                       \
    upstream.patched/src/google/protobuf/io/zero_copy_stream_impl_lite.cc    

LOCAL_SRC_FILES := $(CC_LITE_SRC_FILES) \
 	upstream.patched/src/google/protobuf/io/zero_copy_stream_impl.cc
