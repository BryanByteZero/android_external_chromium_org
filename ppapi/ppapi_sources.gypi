# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'c_source_files': [
      'c/pp_bool.h',
      'c/pp_completion_callback.h',
      'c/pp_errors.h',
      'c/pp_file_info.h',
      'c/pp_graphics_3d.h',
      'c/pp_input_event.h',
      'c/pp_instance.h',
      'c/pp_macros.h',
      'c/pp_module.h',
      'c/pp_point.h',
      'c/pp_rect.h',
      'c/pp_resource.h',
      'c/pp_size.h',
      'c/pp_stdint.h',
      'c/pp_time.h',
      'c/pp_var.h',
      'c/ppb.h',
      'c/ppb_audio.h',
      'c/ppb_audio_config.h',
      'c/ppb_core.h',
      'c/ppb_file_io.h',
      'c/ppb_file_ref.h',
      'c/ppb_file_system.h',
      'c/ppb_fullscreen.h',
      'c/ppb_graphics_2d.h',
      'c/ppb_graphics_3d.h',
      'c/ppb_image_data.h',
      'c/ppb_input_event.h',
      'c/ppb_instance.h',
      'c/ppb_messaging.h',
      'c/ppb_mouse_lock.h',
      'c/ppb_opengles2.h',
      'c/ppb_url_loader.h',
      'c/ppb_url_request_info.h',
      'c/ppb_url_response_info.h',
      'c/ppb_var.h',
      'c/ppb_var_array_buffer.h',
      'c/ppb_view.h',
      'c/ppb_websocket.h',
      'c/ppp.h',
      'c/ppp_graphics_3d.h',
      'c/ppp_input_event.h',
      'c/ppp_instance.h',
      'c/ppp_messaging.h',
      'c/ppp_mouse_lock.h',

      # Dev interfaces.
      'c/dev/pp_cursor_type_dev.h',
      'c/dev/pp_video_dev.h',
      'c/dev/ppb_buffer_dev.h',
      'c/dev/ppb_char_set_dev.h',
      'c/dev/ppb_console_dev.h',
      'c/dev/ppb_cursor_control_dev.h',
      'c/dev/ppb_device_ref_dev.h',
      'c/dev/ppb_directory_reader_dev.h',
      'c/dev/ppb_file_chooser_dev.h',
      'c/dev/ppb_find_dev.h',
      'c/dev/ppb_font_dev.h',
      'c/dev/ppb_fullscreen_dev.h',
      'c/dev/ppb_gamepad_dev.h',
      'c/dev/ppb_ime_input_event_dev.h',
      'c/dev/ppb_memory_dev.h',
      'c/dev/ppb_message_loop_dev.h',
      'c/dev/ppb_resource_array_dev.h',
      'c/dev/ppb_scrollbar_dev.h',
      'c/dev/ppb_testing_dev.h',
      'c/dev/ppb_url_util_dev.h',
      'c/dev/ppb_video_decoder_dev.h',
      'c/dev/ppb_widget_dev.h',
      'c/dev/ppb_zoom_dev.h',
      'c/dev/ppp_cursor_control_dev.h',
      'c/dev/ppp_find_dev.h',
      'c/dev/ppp_network_state_dev.h',
      'c/dev/ppp_scrollbar_dev.h',
      'c/dev/ppp_selection_dev.h',
      'c/dev/ppb_text_input_dev.h',
      'c/dev/ppp_video_decoder_dev.h',
      'c/dev/ppp_widget_dev.h',
      'c/dev/ppp_zoom_dev.h',

      # Private interfaces.
      'c/private/ppb_flash.h',
      'c/private/ppb_flash_clipboard.h',
      'c/private/ppb_flash_file.h',
      'c/private/ppb_flash_fullscreen.h',
      'c/private/ppb_flash_menu.h',
      'c/private/ppb_flash_message_loop.h',
      'c/private/ppb_flash_net_connector.h',
      'c/private/ppb_flash_tcp_socket.h',
      'c/private/ppb_flash_udp_socket.h',
      'c/private/ppb_gpu_blacklist_private.h',
      'c/private/ppb_instance_private.h',
      'c/private/ppb_nacl_private.h',
      'c/private/ppb_net_address_private.h',
      'c/private/ppb_pdf.h',
      'c/private/ppb_proxy_private.h',
      'c/private/ppp_instance_private.h',
      'c/private/ppb_tcp_socket_private.h',
      'c/private/ppb_udp_socket_private.h',

      # Deprecated interfaces.
      'c/dev/deprecated_bool.h',
      'c/dev/ppb_var_deprecated.h',
      'c/dev/ppp_class_deprecated.h',

      # Trusted interfaces.
      'c/trusted/ppb_audio_trusted.h',
      'c/trusted/ppb_broker_trusted.h',
      'c/trusted/ppb_buffer_trusted.h',
      'c/trusted/ppb_file_chooser_trusted.h',
      'c/trusted/ppb_file_io_trusted.h',
      'c/trusted/ppb_graphics_3d_trusted.h',
      'c/trusted/ppb_image_data_trusted.h',
      'c/trusted/ppb_url_loader_trusted.h',
      'c/trusted/ppp_broker.h',
    ],
    'cpp_source_files': [
      'cpp/audio.cc',
      'cpp/audio.h',
      'cpp/audio_config.cc',
      'cpp/audio_config.h',
      'cpp/completion_callback.cc',
      'cpp/completion_callback.h',
      'cpp/core.cc',
      'cpp/core.h',
      'cpp/file_io.cc',
      'cpp/file_io.h',
      'cpp/file_ref.cc',
      'cpp/file_ref.h',
      'cpp/file_system.cc',
      'cpp/file_system.h',
      'cpp/fullscreen.cc',
      'cpp/fullscreen.h',
      'cpp/graphics_2d.cc',
      'cpp/graphics_2d.h',
      'cpp/graphics_3d.cc',
      'cpp/graphics_3d.h',
      'cpp/graphics_3d_client.cc',
      'cpp/graphics_3d_client.h',
      'cpp/image_data.cc',
      'cpp/image_data.h',
      'cpp/input_event.cc',
      'cpp/input_event.h',
      'cpp/instance.cc',
      'cpp/instance.h',
      'cpp/logging.h',
      'cpp/module.cc',
      'cpp/module.h',
      'cpp/module_impl.h',
      'cpp/mouse_lock.cc',
      'cpp/mouse_lock.h',
      'cpp/point.h',
      'cpp/rect.cc',
      'cpp/rect.h',
      'cpp/resource.cc',
      'cpp/resource.h',
      'cpp/size.h',
      'cpp/url_loader.cc',
      'cpp/url_loader.h',
      'cpp/url_request_info.cc',
      'cpp/url_request_info.h',
      'cpp/url_response_info.cc',
      'cpp/url_response_info.h',
      'cpp/var.cc',
      'cpp/var.h',
      'cpp/var_array_buffer.cc',
      'cpp/var_array_buffer.h',
      'cpp/view.cc',
      'cpp/view.h',
      'cpp/websocket.cc',
      'cpp/websocket.h',

      # Dev interfaces.
      'cpp/dev/audio_input_dev.cc',
      'cpp/dev/audio_input_dev.h',
      'cpp/dev/buffer_dev.cc',
      'cpp/dev/buffer_dev.h',
      'cpp/dev/device_ref_dev.cc',
      'cpp/dev/device_ref_dev.h',
      'cpp/dev/directory_entry_dev.cc',
      'cpp/dev/directory_entry_dev.h',
      'cpp/dev/directory_reader_dev.cc',
      'cpp/dev/directory_reader_dev.h',
      'cpp/dev/file_chooser_dev.cc',
      'cpp/dev/file_chooser_dev.h',
      'cpp/dev/find_dev.cc',
      'cpp/dev/find_dev.h',
      'cpp/dev/font_dev.cc',
      'cpp/dev/font_dev.h',
      'cpp/dev/fullscreen_dev.cc',
      'cpp/dev/fullscreen_dev.h',
      'cpp/dev/ime_input_event_dev.cc',
      'cpp/dev/ime_input_event_dev.h',
      'cpp/dev/memory_dev.cc',
      'cpp/dev/memory_dev.h',
      'cpp/dev/message_loop_dev.cc',
      'cpp/dev/message_loop_dev.h',
      'cpp/dev/printing_dev.cc',
      'cpp/dev/printing_dev.h',
      'cpp/dev/resource_array_dev.cc',
      'cpp/dev/resource_array_dev.h',
      'cpp/dev/scrollbar_dev.cc',
      'cpp/dev/scrollbar_dev.h',
      'cpp/dev/selection_dev.cc',
      'cpp/dev/selection_dev.h',
      'cpp/dev/text_input_dev.cc',
      'cpp/dev/text_input_dev.h',
      'cpp/dev/url_util_dev.cc',
      'cpp/dev/url_util_dev.h',
      'cpp/dev/video_capture_client_dev.cc',
      'cpp/dev/video_capture_client_dev.h',
      'cpp/dev/video_capture_dev.cc',
      'cpp/dev/video_capture_dev.h',
      'cpp/dev/video_decoder_client_dev.cc',
      'cpp/dev/video_decoder_client_dev.h',
      'cpp/dev/video_decoder_dev.cc',
      'cpp/dev/video_decoder_dev.h',
      'cpp/dev/widget_client_dev.cc',
      'cpp/dev/widget_client_dev.h',
      'cpp/dev/widget_dev.cc',
      'cpp/dev/widget_dev.h',
      'cpp/dev/zoom_dev.cc',
      'cpp/dev/zoom_dev.h',

      # Deprecated interfaces.
      'cpp/dev/scriptable_object_deprecated.h',
      'cpp/dev/scriptable_object_deprecated.cc',

      # Private interfaces.
      'cpp/private/flash.cc',
      'cpp/private/flash.h',
      'cpp/private/flash_clipboard.cc',
      'cpp/private/flash_clipboard.h',
      'cpp/private/flash_fullscreen.cc',
      'cpp/private/flash_fullscreen.h',
      'cpp/private/flash_menu.cc',
      'cpp/private/flash_menu.h',
      'cpp/private/flash_message_loop.cc',
      'cpp/private/flash_message_loop.h',
      'cpp/private/flash_net_connector.cc',
      'cpp/private/flash_net_connector.h',
      'cpp/private/instance_private.cc',
      'cpp/private/instance_private.h',
      'cpp/private/net_address_private.cc',
      'cpp/private/net_address_private.h',
      'cpp/private/tcp_socket_private.cc',
      'cpp/private/tcp_socket_private.h',
      'cpp/private/udp_socket_private.cc',
      'cpp/private/udp_socket_private.h',
      'cpp/private/var_private.cc',
      'cpp/private/var_private.h',

      # Trusted interfaces.
      'cpp/trusted/file_chooser_trusted.cc',
      'cpp/trusted/file_chooser_trusted.h',
      'cpp/trusted/file_io_trusted.cc',
      'cpp/trusted/file_io_trusted.h',

      # Utility sources.
      'utility/completion_callback_factory.h',
      'utility/non_thread_safe_ref_count.h',
      'utility/graphics/paint_aggregator.cc',
      'utility/graphics/paint_aggregator.h',
      'utility/graphics/paint_manager.cc',
      'utility/graphics/paint_manager.h',
      'utility/threading/simple_thread.cc',
      'utility/threading/simple_thread.h',
    ],
    #
    # Common Testing source for trusted and untrusted (NaCl) pugins.
    #
    'test_common_source_files': [
      # Common test files
      'tests/all_c_includes.h',
      'tests/all_cpp_includes.h',
      'tests/arch_dependent_sizes_32.h',
      'tests/arch_dependent_sizes_64.h',
      'tests/pp_thread.h',
      'tests/test_case.cc',
      'tests/test_case.h',
      'tests/test_net_address_private_untrusted.cc',
      'tests/test_net_address_private_untrusted.h',
      'tests/test_tcp_socket_private_shared.cc',
      'tests/test_tcp_socket_private_shared.h',
      'tests/test_udp_socket_private_shared.cc',
      'tests/test_udp_socket_private_shared.h',
      'tests/test_utils.cc',
      'tests/testing_instance.cc',
      'tests/testing_instance.h',

      # Compile-time tests
      'tests/test_c_includes.c',
      'tests/test_cpp_includes.cc',
      'tests/test_struct_sizes.c',
     ],
    #
    # Sources used in NaCl tests.
    #
    'test_nacl_source_files': [
      # Test cases (PLEASE KEEP THIS SECTION IN ALPHABETICAL ORDER)
      'tests/test_audio_config.cc',
      'tests/test_cursor_control.cc',
      'tests/test_directory_reader.cc',
      'tests/test_exception.cc',
      'tests/test_file_io.cc',
      'tests/test_file_ref.cc',
      'tests/test_file_system.cc',
      'tests/test_input_event.cc',
      'tests/test_memory.cc',
      'tests/test_graphics_2d.cc',
      'tests/test_image_data.cc',
      'tests/test_memory.cc',
      'tests/test_paint_aggregator.cc',
      'tests/test_post_message.cc',
      'tests/test_scrollbar.cc',
      'tests/test_tcp_socket_private_disallowed.cc',
      'tests/test_udp_socket_private_disallowed.cc',
      'tests/test_url_loader.cc',
      'tests/test_url_request.cc',
      'tests/test_var.cc',
      'tests/test_view.cc',
      'tests/test_websocket.cc',
    ],
    #
    # Sources used in trusted tests.
    #
    'test_trusted_source_files': [
      # Test cases (PLEASE KEEP THIS SECTION IN ALPHABETICAL ORDER)
      'tests/test_audio.cc',
      'tests/test_audio.h',
      'tests/test_audio_config.cc',
      'tests/test_audio_config.h',
      'tests/test_broker.cc',
      'tests/test_broker.h',
      'tests/test_buffer.cc',
      'tests/test_buffer.h',
      'tests/test_c_includes.c',
      'tests/test_char_set.cc',
      'tests/test_char_set.h',
      'tests/test_core.cc',
      'tests/test_core.h',
      'tests/test_cpp_includes.cc',
      'tests/test_crypto.cc',
      'tests/test_crypto.h',
      'tests/test_cursor_control.cc',
      'tests/test_cursor_control.h',
      'tests/test_directory_reader.cc',
      'tests/test_directory_reader.h',
      'tests/test_file_io.cc',
      'tests/test_file_io.h',
      'tests/test_file_ref.cc',
      'tests/test_file_ref.h',
      'tests/test_file_system.cc',
      'tests/test_file_system.h',
      'tests/test_flash.cc',
      'tests/test_flash.h',
      'tests/test_flash_clipboard.cc',
      'tests/test_flash_clipboard.h',
      'tests/test_flash_fullscreen.cc',
      'tests/test_flash_fullscreen.h',
      'tests/test_flash_message_loop.cc',
      'tests/test_flash_message_loop.h',
      'tests/test_fullscreen.cc',
      'tests/test_fullscreen.h',
      'tests/test_graphics_2d.cc',
      'tests/test_graphics_2d.h',
      'tests/test_graphics_3d.cc',
      'tests/test_graphics_3d.h',
      'tests/test_image_data.cc',
      'tests/test_image_data.h',
      'tests/test_input_event.cc',
      'tests/test_input_event.h',
      'tests/test_memory.cc',
      'tests/test_memory.h',
      'tests/test_net_address_private.cc',
      'tests/test_net_address_private.h',
      'tests/test_paint_aggregator.cc',
      'tests/test_paint_aggregator.h',
      'tests/test_post_message.cc',
      'tests/test_post_message.h',
      'tests/test_resource_array.cc',
      'tests/test_resource_array.h',
      'tests/test_scrollbar.cc',
      'tests/test_scrollbar.h',
      'tests/test_struct_sizes.c',
      'tests/test_tcp_socket_private.cc',
      'tests/test_tcp_socket_private.h',
      'tests/test_uma.cc',
      'tests/test_uma.h',
      'tests/test_url_loader.cc',
      'tests/test_url_loader.h',
      'tests/test_url_request.cc',
      'tests/test_url_request.h',
      'tests/test_url_util.cc',
      'tests/test_url_util.h',
      'tests/test_utils.cc',
      'tests/test_utils.h',
      'tests/test_var.cc',
      'tests/test_var.h',
      'tests/test_view.cc',
      'tests/test_view.h',
      'tests/test_video_decoder.cc',
      'tests/test_video_decoder.h',
      'tests/test_websocket.cc',
      'tests/test_websocket.h',

      # Deprecated test cases.
      'tests/test_instance_deprecated.cc',
      'tests/test_instance_deprecated.h',
      'tests/test_var_deprecated.cc',
      'tests/test_var_deprecated.h',
    ],
  },
  'conditions': [
    ['p2p_apis==1', {
      'variables': {
        'c_source_files': [
          'c/dev/ppb_transport_dev.h',
        ],
        'cpp_source_files': [
          'cpp/dev/transport_dev.cc',
          'cpp/dev/transport_dev.h',
        ],
      },
    }],
  ],
}
