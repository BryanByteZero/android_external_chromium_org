# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'api',
      'type': 'static_library',
      'sources': [
        '<@(schema_files)',
      ],
      # TODO(jschuh): http://crbug.com/167187 size_t -> int
      'msvs_disabled_warnings': [ 4267 ],
      'includes': [
        '../../../../build/json_schema_bundle_compile.gypi',
        '../../../../build/json_schema_compile.gypi',
      ],
      'variables': {
        'chromium_code': 1,
        'schema_files': [
          'alarms.idl',
          'app_current_window_internal.idl',
          'app_runtime.idl',
          'app_window.idl',
          'autotest_private.idl',
          'bluetooth.idl',
          'bookmark_manager_private.json',
          'bookmarks.json',
          'chromeos_info_private.json',
          'cloud_print_private.json',
          'content_settings.json',
          'context_menus.json',
          'cookies.json',
          'debugger.json',
          'developer_private.idl',
          'dial.idl',
          'downloads.idl',
          'echo_private.json',
          'downloads_internal.idl',
          'events.json',
          'experimental_accessibility.json',
          'experimental_discovery.idl',
          'experimental_dns.idl',
          'experimental_history.json',
          'experimental_identity.idl',
          'experimental_idltest.idl',
          'experimental_infobars.json',
          'experimental_media_galleries.idl',
          'experimental_record.json',
          'experimental_system_info_cpu.idl',
          'experimental_system_info_memory.idl',
          'experimental_system_info_storage.idl',
          'extension.json',
          'file_browser_handler_internal.json',
          'file_system.idl',
          'font_settings.json',
          'history.json',
          'i18n.json',
          'idle.json',
          'managed_mode_private.json',
          'management.json',
          'media_galleries.idl',
          'media_galleries_private.idl',
          'media_player_private.json',
          'metrics_private.json',
          'networking_private.json',
          'notifications.idl',
          'omnibox.json',
          'page_capture.json',
          'page_launcher.idl',
          'permissions.json',
          'power.idl',
          'push_messaging.idl',
          'rtc_private.idl',
          'serial.idl',
          'session_restore.json',
          'socket.idl',
          'storage.json',
          'sync_file_system.idl',
          'system_indicator.idl',
          'system_info_display.idl',
          'system_private.json',
          'tab_capture.idl',
          'tabs.json',
          'terminal_private.json',
          'test.json',
          'top_sites.json',
          'usb.idl',
          'wallpaper_private.json',
          'web_navigation.json',
          'web_request.json',
          'web_socket_proxy_private.json',
          'webview.json',
          'windows.json',
        ],
        'cc_dir': 'chrome/common/extensions/api',
        'root_namespace': 'extensions::api',
      },
      'dependencies': [
        '<(DEPTH)/skia/skia.gyp:skia',
        '<(DEPTH)/sync/sync.gyp:sync',
      ],
      'conditions': [
        ['OS=="android"', {
          'schema_files!': [
            'usb.idl',
          ],
        }],
        ['OS!="chromeos"', {
          'schema_files!': [
            'file_browser_handler_internal.json',
            'rtc_private.idl',
          ],
        }],
      ],
    },
  ],
}
