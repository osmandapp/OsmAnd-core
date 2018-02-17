project(xsb)

prepare_upstream()
build_upstream()

set(XSB_CONFIG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/upstream.patched/XSB/config)
set(XSB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/upstream.patched/XSB/emu)
set(XSB_CONFIG_INCLUDE_DIR ${XSB_CONFIG_DIR}/arm-apple-darwin)

file(GLOB xsb_emu_headers "${XSB_INCLUDE_DIR}/*.h")

add_library(XSB_STATIC
    ${xsb_emu_headers}

    ${XSB_INCLUDE_DIR}/auxlry.c
    ${XSB_INCLUDE_DIR}/biassert.c 
    ${XSB_INCLUDE_DIR}/builtin.c 
    ${XSB_INCLUDE_DIR}/call_xsb.c 
    ${XSB_INCLUDE_DIR}/cinterf.c 
    ${XSB_INCLUDE_DIR}/conc_compl.c 
    ${XSB_INCLUDE_DIR}/debug_xsb.c 
    ${XSB_INCLUDE_DIR}/dis.c 
    ${XSB_INCLUDE_DIR}/dynload.c 
    ${XSB_INCLUDE_DIR}/dynamic_stack.c 
    ${XSB_INCLUDE_DIR}/deadlock.c 
    ${XSB_INCLUDE_DIR}/emuloop.c 
    ${XSB_INCLUDE_DIR}/error_xsb.c 
    ${XSB_INCLUDE_DIR}/findall.c 
    ${XSB_INCLUDE_DIR}/function.c 
    ${XSB_INCLUDE_DIR}/hash_xsb.c 
    ${XSB_INCLUDE_DIR}/hashtable_xsb.c
    ${XSB_INCLUDE_DIR}/heap_xsb.c 
    ${XSB_INCLUDE_DIR}/init_xsb.c 
    ${XSB_INCLUDE_DIR}/inst_xsb.c 
    ${XSB_INCLUDE_DIR}/io_builtins_xsb.c 
    ${XSB_INCLUDE_DIR}/loader_xsb.c 
    ${XSB_INCLUDE_DIR}/main_xsb.c 
    ${XSB_INCLUDE_DIR}/memory_xsb.c 
    ${XSB_INCLUDE_DIR}/orient_xsb.c
    ${XSB_INCLUDE_DIR}/pathname_xsb.c 
    ${XSB_INCLUDE_DIR}/psc_xsb.c 
    ${XSB_INCLUDE_DIR}/random_xsb.c 
    ${XSB_INCLUDE_DIR}/remove_unf.c 
    ${XSB_INCLUDE_DIR}/residual.c 
    ${XSB_INCLUDE_DIR}/rw_lock.c
    ${XSB_INCLUDE_DIR}/scc_xsb.c
    ${XSB_INCLUDE_DIR}/slgdelay.c 
    ${XSB_INCLUDE_DIR}/socket_xsb.c 
    ${XSB_INCLUDE_DIR}/string_xsb.c 
    ${XSB_INCLUDE_DIR}/storage_xsb.c 
    ${XSB_INCLUDE_DIR}/struct_manager.c 
    ${XSB_INCLUDE_DIR}/sub_delete.c 
    ${XSB_INCLUDE_DIR}/subp.c 
    ${XSB_INCLUDE_DIR}/system_xsb.c 
    ${XSB_INCLUDE_DIR}/table_stats.c 
    ${XSB_INCLUDE_DIR}/tables.c 
    ${XSB_INCLUDE_DIR}/thread_xsb.c 
    ${XSB_INCLUDE_DIR}/timer_xsb.c 
    ${XSB_INCLUDE_DIR}/token_xsb.c 
    ${XSB_INCLUDE_DIR}/tr_utils.c 
    ${XSB_INCLUDE_DIR}/trace_xsb.c 
    ${XSB_INCLUDE_DIR}/trie_lookup.c 
    ${XSB_INCLUDE_DIR}/trie_search.c 
    ${XSB_INCLUDE_DIR}/tries.c 
    ${XSB_INCLUDE_DIR}/tst_insert.c 
    ${XSB_INCLUDE_DIR}/tst_retrv.c 
    ${XSB_INCLUDE_DIR}/tst_unify.c 
    ${XSB_INCLUDE_DIR}/tst_utils.c 
    ${XSB_INCLUDE_DIR}/varstring.c 
    ${XSB_INCLUDE_DIR}/ubi_BinTree.c 
    ${XSB_INCLUDE_DIR}/ubi_SplayTree.c 
    ${XSB_INCLUDE_DIR}/hashtable.c 
    ${XSB_INCLUDE_DIR}/hashtable_itr.c 
    ${XSB_INCLUDE_DIR}/sha1.c 
    ${XSB_INCLUDE_DIR}/md5.c 
    ${XSB_INCLUDE_DIR}/url_encode.c 
    ${XSB_INCLUDE_DIR}/getMemorySize.c 
    ${XSB_INCLUDE_DIR}/incr_xsb.c 
    ${XSB_INCLUDE_DIR}/call_graph_xsb.c
    ${XSB_INCLUDE_DIR}/struct_intern.c
)

target_include_directories(XSB_STATIC 
    PRIVATE
        ${XSB_CONFIG_INCLUDE_DIR}
    PUBLIC 
        ${XSB_INCLUDE_DIR}
)

SET_TARGET_PROPERTIES(XSB_STATIC PROPERTIES LINKER_LANGUAGE C11)
