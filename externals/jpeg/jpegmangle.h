#ifndef _OSMAND_JPEG_MANGLE_
#define _OSMAND_JPEG_MANGLE_

// Mangle names to avoid conflicts with system libraries
#define jcopy_block_row                                     osmand_jcopy_block_row
#define jcopy_sample_rows                                   osmand_jcopy_sample_rows
#define jdiv_round_up                                       osmand_jdiv_round_up
#define jinit_1pass_quantizer                               osmand_jinit_1pass_quantizer
#define jinit_2pass_quantizer                               osmand_jinit_2pass_quantizer
#define jinit_arith_decoder                                 osmand_jinit_arith_decoder
#define jinit_arith_encoder                                 osmand_jinit_arith_encoder
#define jinit_c_coef_controller                             osmand_jinit_c_coef_controller
#define jinit_c_main_controller                             osmand_jinit_c_main_controller
#define jinit_c_master_control                              osmand_jinit_c_master_control
#define jinit_c_prep_controller                             osmand_jinit_c_prep_controller
#define jinit_color_converter                               osmand_jinit_color_converter
#define jinit_color_deconverter                             osmand_jinit_color_deconverter
#define jinit_compress_master                               osmand_jinit_compress_master
#define jinit_d_coef_controller                             osmand_jinit_d_coef_controller
#define jinit_d_main_controller                             osmand_jinit_d_main_controller
#define jinit_d_post_controller                             osmand_jinit_d_post_controller
#define jinit_downsampler                                   osmand_jinit_downsampler
#define jinit_forward_dct                                   osmand_jinit_forward_dct
#define jinit_huff_decoder                                  osmand_jinit_huff_decoder
#define jinit_huff_encoder                                  osmand_jinit_huff_encoder
#define jinit_input_controller                              osmand_jinit_input_controller
#define jinit_inverse_dct                                   osmand_jinit_inverse_dct
#define jinit_marker_reader                                 osmand_jinit_marker_reader
#define jinit_marker_writer                                 osmand_jinit_marker_writer
#define jinit_master_decompress                             osmand_jinit_master_decompress
#define jinit_memory_mgr                                    osmand_jinit_memory_mgr
#define jinit_merged_upsampler                              osmand_jinit_merged_upsampler
#define jinit_upsampler                                     osmand_jinit_upsampler
#define jpeg_abort                                          osmand_jpeg_abort
#define jpeg_abort_compress                                 osmand_jpeg_abort_compress
#define jpeg_abort_decompress                               osmand_jpeg_abort_decompress
#define jpeg_add_quant_table                                osmand_jpeg_add_quant_table
#define jpeg_alloc_huff_table                               osmand_jpeg_alloc_huff_table
#define jpeg_alloc_quant_table                              osmand_jpeg_alloc_quant_table
#define jpeg_aritab                                         osmand_jpeg_aritab
#define jpeg_calc_jpeg_dimensions                           osmand_jpeg_calc_jpeg_dimensions
#define jpeg_calc_output_dimensions                         osmand_jpeg_calc_output_dimensions
#define jpeg_consume_input                                  osmand_jpeg_consume_input
#define jpeg_copy_critical_parameters                       osmand_jpeg_copy_critical_parameters
#define jpeg_core_output_dimensions                         osmand_jpeg_core_output_dimensions
#define jpeg_CreateCompress                                 osmand_jpeg_CreateCompress
#define jpeg_CreateDecompress                               osmand_jpeg_CreateDecompress
#define jpeg_default_colorspace                             osmand_jpeg_default_colorspace
#define jpeg_default_qtables                                osmand_jpeg_default_qtables
#define jpeg_destroy                                        osmand_jpeg_destroy
#define jpeg_destroy_compress                               osmand_jpeg_destroy_compress
#define jpeg_destroy_decompress                             osmand_jpeg_destroy_decompress
#define jpeg_fdct_10x10                                     osmand_jpeg_fdct_10x10
#define jpeg_fdct_10x5                                      osmand_jpeg_fdct_10x5
#define jpeg_fdct_11x11                                     osmand_jpeg_fdct_11x11
#define jpeg_fdct_12x12                                     osmand_jpeg_fdct_12x12
#define jpeg_fdct_12x6                                      osmand_jpeg_fdct_12x6
#define jpeg_fdct_13x13                                     osmand_jpeg_fdct_13x13
#define jpeg_fdct_14x14                                     osmand_jpeg_fdct_14x14
#define jpeg_fdct_14x7                                      osmand_jpeg_fdct_14x7
#define jpeg_fdct_15x15                                     osmand_jpeg_fdct_15x15
#define jpeg_fdct_16x16                                     osmand_jpeg_fdct_16x16
#define jpeg_fdct_16x8                                      osmand_jpeg_fdct_16x8
#define jpeg_fdct_1x1                                       osmand_jpeg_fdct_1x1
#define jpeg_fdct_1x2                                       osmand_jpeg_fdct_1x2
#define jpeg_fdct_2x1                                       osmand_jpeg_fdct_2x1
#define jpeg_fdct_2x2                                       osmand_jpeg_fdct_2x2
#define jpeg_fdct_2x4                                       osmand_jpeg_fdct_2x4
#define jpeg_fdct_3x3                                       osmand_jpeg_fdct_3x3
#define jpeg_fdct_3x6                                       osmand_jpeg_fdct_3x6
#define jpeg_fdct_4x2                                       osmand_jpeg_fdct_4x2
#define jpeg_fdct_4x4                                       osmand_jpeg_fdct_4x4
#define jpeg_fdct_4x8                                       osmand_jpeg_fdct_4x8
#define jpeg_fdct_5x10                                      osmand_jpeg_fdct_5x10
#define jpeg_fdct_5x5                                       osmand_jpeg_fdct_5x5
#define jpeg_fdct_6x12                                      osmand_jpeg_fdct_6x12
#define jpeg_fdct_6x3                                       osmand_jpeg_fdct_6x3
#define jpeg_fdct_6x6                                       osmand_jpeg_fdct_6x6
#define jpeg_fdct_7x14                                      osmand_jpeg_fdct_7x14
#define jpeg_fdct_7x7                                       osmand_jpeg_fdct_7x7
#define jpeg_fdct_8x16                                      osmand_jpeg_fdct_8x16
#define jpeg_fdct_8x4                                       osmand_jpeg_fdct_8x4
#define jpeg_fdct_9x9                                       osmand_jpeg_fdct_9x9
#define jpeg_fdct_float                                     osmand_jpeg_fdct_float
#define jpeg_fdct_ifast                                     osmand_jpeg_fdct_ifast
#define jpeg_fdct_islow                                     osmand_jpeg_fdct_islow
#define jpeg_finish_compress                                osmand_jpeg_finish_compress
#define jpeg_finish_decompress                              osmand_jpeg_finish_decompress
#define jpeg_finish_output                                  osmand_jpeg_finish_output
#define jpeg_free_large                                     osmand_jpeg_free_large
#define jpeg_free_small                                     osmand_jpeg_free_small
#define jpeg_get_large                                      osmand_jpeg_get_large
#define jpeg_get_small                                      osmand_jpeg_get_small
#define jpeg_has_multiple_scans                             osmand_jpeg_has_multiple_scans
#define jpeg_idct_10x10                                     osmand_jpeg_idct_10x10
#define jpeg_idct_10x5                                      osmand_jpeg_idct_10x5
#define jpeg_idct_11x11                                     osmand_jpeg_idct_11x11
#define jpeg_idct_12x12                                     osmand_jpeg_idct_12x12
#define jpeg_idct_12x6                                      osmand_jpeg_idct_12x6
#define jpeg_idct_13x13                                     osmand_jpeg_idct_13x13
#define jpeg_idct_14x14                                     osmand_jpeg_idct_14x14
#define jpeg_idct_14x7                                      osmand_jpeg_idct_14x7
#define jpeg_idct_15x15                                     osmand_jpeg_idct_15x15
#define jpeg_idct_16x16                                     osmand_jpeg_idct_16x16
#define jpeg_idct_16x8                                      osmand_jpeg_idct_16x8
#define jpeg_idct_1x1                                       osmand_jpeg_idct_1x1
#define jpeg_idct_1x2                                       osmand_jpeg_idct_1x2
#define jpeg_idct_2x1                                       osmand_jpeg_idct_2x1
#define jpeg_idct_2x2                                       osmand_jpeg_idct_2x2
#define jpeg_idct_2x4                                       osmand_jpeg_idct_2x4
#define jpeg_idct_3x3                                       osmand_jpeg_idct_3x3
#define jpeg_idct_3x6                                       osmand_jpeg_idct_3x6
#define jpeg_idct_4x2                                       osmand_jpeg_idct_4x2
#define jpeg_idct_4x4                                       osmand_jpeg_idct_4x4
#define jpeg_idct_4x8                                       osmand_jpeg_idct_4x8
#define jpeg_idct_5x10                                      osmand_jpeg_idct_5x10
#define jpeg_idct_5x5                                       osmand_jpeg_idct_5x5
#define jpeg_idct_6x12                                      osmand_jpeg_idct_6x12
#define jpeg_idct_6x3                                       osmand_jpeg_idct_6x3
#define jpeg_idct_6x6                                       osmand_jpeg_idct_6x6
#define jpeg_idct_7x14                                      osmand_jpeg_idct_7x14
#define jpeg_idct_7x7                                       osmand_jpeg_idct_7x7
#define jpeg_idct_8x16                                      osmand_jpeg_idct_8x16
#define jpeg_idct_8x4                                       osmand_jpeg_idct_8x4
#define jpeg_idct_9x9                                       osmand_jpeg_idct_9x9
#define jpeg_idct_float                                     osmand_jpeg_idct_float
#define jpeg_idct_ifast                                     osmand_jpeg_idct_ifast
#define jpeg_idct_islow                                     osmand_jpeg_idct_islow
#define jpeg_input_complete                                 osmand_jpeg_input_complete
#define jpeg_mem_available                                  osmand_jpeg_mem_available
#define jpeg_mem_dest                                       osmand_jpeg_mem_dest
#define jpeg_mem_init                                       osmand_jpeg_mem_init
#define jpeg_mem_src                                        osmand_jpeg_mem_src
#define jpeg_mem_term                                       osmand_jpeg_mem_term
#define jpeg_natural_order                                  osmand_jpeg_natural_order
#define jpeg_natural_order2                                 osmand_jpeg_natural_order2
#define jpeg_natural_order3                                 osmand_jpeg_natural_order3
#define jpeg_natural_order4                                 osmand_jpeg_natural_order4
#define jpeg_natural_order5                                 osmand_jpeg_natural_order5
#define jpeg_natural_order6                                 osmand_jpeg_natural_order6
#define jpeg_natural_order7                                 osmand_jpeg_natural_order7
#define jpeg_new_colormap                                   osmand_jpeg_new_colormap
#define jpeg_open_backing_store                             osmand_jpeg_open_backing_store
#define jpeg_quality_scaling                                osmand_jpeg_quality_scaling
#define jpeg_read_coefficients                              osmand_jpeg_read_coefficients
#define jpeg_read_header                                    osmand_jpeg_read_header
#define jpeg_read_raw_data                                  osmand_jpeg_read_raw_data
#define jpeg_read_scanlines                                 osmand_jpeg_read_scanlines
#define jpeg_resync_to_restart                              osmand_jpeg_resync_to_restart
#define jpeg_save_markers                                   osmand_jpeg_save_markers
#define jpeg_set_colorspace                                 osmand_jpeg_set_colorspace
#define jpeg_set_defaults                                   osmand_jpeg_set_defaults
#define jpeg_set_linear_quality                             osmand_jpeg_set_linear_quality
#define jpeg_set_marker_processor                           osmand_jpeg_set_marker_processor
#define jpeg_set_quality                                    osmand_jpeg_set_quality
#define jpeg_simple_progression                             osmand_jpeg_simple_progression
#define jpeg_start_compress                                 osmand_jpeg_start_compress
#define jpeg_start_decompress                               osmand_jpeg_start_decompress
#define jpeg_start_output                                   osmand_jpeg_start_output
#define jpeg_std_error                                      osmand_jpeg_std_error
#define jpeg_std_message_table                              osmand_jpeg_std_message_table
#define jpeg_stdio_dest                                     osmand_jpeg_stdio_dest
#define jpeg_stdio_src                                      osmand_jpeg_stdio_src
#define jpeg_suppress_tables                                osmand_jpeg_suppress_tables
#define jpeg_write_coefficients                             osmand_jpeg_write_coefficients
#define jpeg_write_m_byte                                   osmand_jpeg_write_m_byte
#define jpeg_write_m_header                                 osmand_jpeg_write_m_header
#define jpeg_write_marker                                   osmand_jpeg_write_marker
#define jpeg_write_raw_data                                 osmand_jpeg_write_raw_data
#define jpeg_write_scanlines                                osmand_jpeg_write_scanlines
#define jpeg_write_tables                                   osmand_jpeg_write_tables
#define jround_up                                           osmand_jround_up

#endif // _OSMAND_JPEG_MANGLE_