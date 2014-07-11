# Copyright 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'libwebviewchromium',
      'type': 'shared_library',
      'android_unmangled_name': 1,
      'dependencies': [
        'android_webview_common',
      ],
      'conditions': [
        # Avoid clashes between the versions of this library built with
        # android_webview_build==1 by using a different name prefix.
        [ 'android_webview_build==0', {
          'product_prefix': 'libstandalone',
        }],
        [ 'android_webview_build==1', {
          # When building inside the android tree we also need to depend on all
          # the java sources generated from templates which will be needed by
          # android_webview_java in android_webview/Android.mk.
          'dependencies': [
            '../base/base.gyp:base_java_application_state',
            '../base/base.gyp:base_java_memory_pressure_level_list',
            '../content/content.gyp:content_gamepad_mapping',
            '../content/content.gyp:gesture_event_type_java',
            '../content/content.gyp:page_transition_types_java',
            '../content/content.gyp:popup_item_type_java',
            '../content/content.gyp:result_codes_java',
            '../content/content.gyp:screen_orientation_values_java',
            '../content/content.gyp:speech_recognition_error_java',
            '../media/media.gyp:media_android_imageformat_list',
            '../net/net.gyp:cert_verify_status_android_java',
            '../net/net.gyp:certificate_mime_types_java',
            '../net/net.gyp:net_errors_java',
            '../net/net.gyp:private_key_types_java',
            '../ui/android/ui_android.gyp:bitmap_format_java',
            '../ui/android/ui_android.gyp:window_open_disposition_java',
          ],
        }],
        [ 'android_webview_build==1 and use_system_skia==0', {
          # When not using the system skia there are linker warnings about
          # overriden hidden symbols which there's no easy way to eliminate;
          # disable them. http://crbug.com/157326
          'ldflags': [
            '-Wl,--no-fatal-warnings',
          ],
          'ldflags!': [
            '-Wl,--fatal-warnings',
          ],
        }],
      ],
      'sources': [
        'lib/main/webview_entry_point.cc',
      ],
    },
    {
      'target_name': 'android_webview_pak',
      'type': 'none',
      'dependencies': [
        '<(DEPTH)/content/content_resources.gyp:content_resources',
        '<(DEPTH)/net/net.gyp:net_resources',
        '<(DEPTH)/ui/resources/ui_resources.gyp:ui_resources',
        '<(DEPTH)/webkit/webkit_resources.gyp:webkit_resources',
      ],
      'actions': [
        {
          'action_name': 'repack_android_webview_pack',
          'variables': {
            'pak_inputs': [
              '<(SHARED_INTERMEDIATE_DIR)/content/content_resources.pak',
              '<(SHARED_INTERMEDIATE_DIR)/net/net_resources.pak',
              '<(SHARED_INTERMEDIATE_DIR)/ui/resources/ui_resources_100_percent.pak',
              '<(SHARED_INTERMEDIATE_DIR)/webkit/blink_resources.pak',
              '<(SHARED_INTERMEDIATE_DIR)/webkit/webkit_resources_100_percent.pak',
            ],
            'pak_output': '<(PRODUCT_DIR)/android_webview_apk/assets/webviewchromium.pak',
          },
         'includes': [ '../build/repack_action.gypi' ],
        }
      ],
    },
    {
      'target_name': 'android_webview_common',
      'type': 'static_library',
      'dependencies': [
        '../android_webview/native/webview_native.gyp:webview_native',
        '../components/components.gyp:auto_login_parser',
        '../components/components.gyp:autofill_content_renderer',
        '../components/components.gyp:cdm_browser',
        '../components/components.gyp:cdm_renderer',
        '../components/components.gyp:data_reduction_proxy_browser',
        '../components/components.gyp:navigation_interception',
        '../components/components.gyp:visitedlink_browser',
        '../components/components.gyp:visitedlink_renderer',
        '../components/components.gyp:web_contents_delegate_android',
        '../content/content.gyp:content_app_both',
        '../gpu/gpu.gyp:command_buffer_service',
        '../gpu/gpu.gyp:gles2_implementation',
        '../gpu/gpu.gyp:gl_in_process_context',
        '../media/media.gyp:media',
        '../printing/printing.gyp:printing',
        '../skia/skia.gyp:skia',
        '../third_party/WebKit/public/blink.gyp:blink',
        '../v8/tools/gyp/v8.gyp:v8',
        '../ui/gl/gl.gyp:gl',
        '../ui/shell_dialogs/shell_dialogs.gyp:shell_dialogs',
        '../webkit/common/gpu/webkit_gpu.gyp:webkit_gpu',
        'android_webview_pak',
      ],
      'include_dirs': [
        '..',
        '../skia/config',
        '<(SHARED_INTERMEDIATE_DIR)/ui/resources/',
      ],
      'sources': [
        'browser/aw_browser_context.cc',
        'browser/aw_browser_context.h',
        'browser/aw_browser_main_parts.cc',
        'browser/aw_browser_main_parts.h',
        'browser/aw_browser_permission_request_delegate.h',
        'browser/aw_contents_client_bridge_base.cc',
        'browser/aw_contents_client_bridge_base.h',
        'browser/aw_content_browser_client.cc',
        'browser/aw_content_browser_client.h',
        'browser/aw_contents_io_thread_client.h',
        'browser/aw_cookie_access_policy.cc',
        'browser/aw_cookie_access_policy.h',
        'browser/aw_download_manager_delegate.cc',
        'browser/aw_download_manager_delegate.h',
        'browser/aw_form_database_service.cc',
        'browser/aw_form_database_service.h',
        'browser/aw_gl_surface.cc',
        'browser/aw_gl_surface.h',
        'browser/aw_http_auth_handler_base.cc',
        'browser/aw_http_auth_handler_base.h',
        'browser/aw_javascript_dialog_manager.cc',
        'browser/aw_javascript_dialog_manager.h',
        'browser/aw_login_delegate.cc',
        'browser/aw_login_delegate.h',
        'browser/aw_pref_store.cc',
        'browser/aw_pref_store.h',
        'browser/aw_quota_manager_bridge.cc',
        'browser/aw_quota_manager_bridge.h',
        'browser/aw_quota_permission_context.cc',
        'browser/aw_quota_permission_context.h',
        'browser/aw_request_interceptor.cc',
        'browser/aw_request_interceptor.h',
        'browser/aw_resource_context.cc',
        'browser/aw_resource_context.h',
        'browser/aw_result_codes.h',
        'browser/aw_web_preferences_populater.cc',
        'browser/aw_web_preferences_populater.h',
        'browser/aw_web_resource_response.cc',
        'browser/aw_web_resource_response.h',
        'browser/browser_view_renderer.cc',
        'browser/browser_view_renderer.h',
        'browser/browser_view_renderer_client.h',
        'browser/deferred_gpu_command_service.cc',
        'browser/deferred_gpu_command_service.h',
        'browser/find_helper.cc',
        'browser/find_helper.h',
        'browser/global_tile_manager.cc',
        'browser/global_tile_manager.h',
        'browser/global_tile_manager_client.h',
        'browser/gpu_memory_buffer_factory_impl.cc',
        'browser/gpu_memory_buffer_factory_impl.h',
        'browser/hardware_renderer.cc',
        'browser/hardware_renderer.h',
        'browser/icon_helper.cc',
        'browser/icon_helper.h',
        'browser/input_stream.h',
        'browser/jni_dependency_factory.h',
        'browser/gl_view_renderer_manager.cc',
        'browser/gl_view_renderer_manager.h',
        'browser/net/android_stream_reader_url_request_job.cc',
        'browser/net/android_stream_reader_url_request_job.h',
        'browser/net/aw_network_delegate.cc',
        'browser/net/aw_network_delegate.h',
        'browser/net/aw_url_request_context_getter.cc',
        'browser/net/aw_url_request_context_getter.h',
        'browser/net/aw_url_request_job_factory.cc',
        'browser/net/aw_url_request_job_factory.h',
        'browser/net_disk_cache_remover.cc',
        'browser/net_disk_cache_remover.h',
        'browser/net/init_native_callback.h',
        'browser/net/input_stream_reader.cc',
        'browser/net/input_stream_reader.h',
        'browser/parent_output_surface.cc',
        'browser/parent_output_surface.h',
        'browser/renderer_host/aw_render_view_host_ext.cc',
        'browser/renderer_host/aw_render_view_host_ext.h',
        'browser/renderer_host/aw_resource_dispatcher_host_delegate.cc',
        'browser/renderer_host/aw_resource_dispatcher_host_delegate.h',
        'browser/renderer_host/print_manager.cc',
        'browser/renderer_host/print_manager.h',
        'browser/scoped_allow_wait_for_legacy_web_view_api.h',
        'browser/scoped_app_gl_state_restore.cc',
        'browser/scoped_app_gl_state_restore.h',
        'browser/shared_renderer_state.cc',
        'browser/shared_renderer_state.h',
        'common/android_webview_message_generator.cc',
        'common/android_webview_message_generator.h',
        'common/aw_content_client.cc',
        'common/aw_content_client.h',
        'common/aw_hit_test_data.cc',
        'common/aw_hit_test_data.h',
        'common/aw_resource.h',
        'common/aw_switches.cc',
        'common/aw_switches.h',
        'common/devtools_instrumentation.h',
        'common/print_messages.cc',
        'common/print_messages.h',
        'common/render_view_messages.cc',
        'common/render_view_messages.h',
        'common/url_constants.cc',
        'common/url_constants.h',
        'lib/aw_browser_dependency_factory_impl.cc',
        'lib/aw_browser_dependency_factory_impl.h',
        'lib/main/aw_main_delegate.cc',
        'lib/main/aw_main_delegate.h',
        'public/browser/draw_gl.h',
        'renderer/aw_content_renderer_client.cc',
        'renderer/aw_content_renderer_client.h',
        'renderer/aw_execution_termination_filter.cc',
        'renderer/aw_execution_termination_filter.h',
        'renderer/aw_key_systems.cc',
        'renderer/aw_key_systems.h',
        'renderer/aw_permission_client.cc',
        'renderer/aw_permission_client.h',
        'renderer/aw_render_process_observer.cc',
        'renderer/aw_render_process_observer.h',
        'renderer/aw_render_frame_ext.cc',
        'renderer/aw_render_frame_ext.h',
        'renderer/aw_render_view_ext.cc',
        'renderer/aw_render_view_ext.h',
        'renderer/print_web_view_helper.cc',
        'renderer/print_web_view_helper.h',
        'renderer/print_web_view_helper_android.cc',
        'renderer/print_web_view_helper_linux.cc',
        'renderer/print_render_frame_observer.cc',
        'renderer/print_render_frame_observer.h',
      ],
    },
  ],
  'conditions': [
    ['android_webview_build==0', {
      'includes': [
        'android_webview_tests.gypi',
      ],
      'targets': [
        {
          'target_name': 'android_webview_java',
          'type': 'none',
          'dependencies': [
            '../components/components.gyp:navigation_interception_java',
            '../components/components.gyp:web_contents_delegate_android_java',
            '../content/content.gyp:content_java',
            '../ui/android/ui_android.gyp:ui_java',
          ],
          'variables': {
            'java_in_dir': '../android_webview/java',
          },
          'includes': [ '../build/java.gypi' ],
        },
      ],
     }, { # android_webview_build==1
      'targets': [
        {
          'target_name': 'android_webview_jarjar_ui_resources',
          'android_unmangled_name': 1,
          'type': 'none',
          'variables': {
            'res_dir': '../ui/android/java/res',
            'rules_file': '../android_webview/build/jarjar-rules.txt',
          },
          'includes': ['../android_webview/build/jarjar_resources.gypi'],
        },
        {
          'target_name': 'android_webview_jarjar_content_resources',
          'android_unmangled_name': 1,
          'type': 'none',
          'variables': {
            'res_dir': '../content/public/android/java/res',
            'rules_file': '../android_webview/build/jarjar-rules.txt',
          },
          'includes': ['../android_webview/build/jarjar_resources.gypi'],
        },
        {
          'target_name': 'android_webview_resources',
          'type': 'none',
          'android_unmangled_name': 1,
          'dependencies': [
            '../content/content.gyp:content_strings_grd',
            '../ui/android/ui_android.gyp:ui_strings_grd',
            'android_webview_jarjar_ui_resources',
            'android_webview_jarjar_content_resources'
          ],
        },
      ],
    }],
  ],
}
