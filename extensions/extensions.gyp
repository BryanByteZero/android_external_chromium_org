# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'extensions_common',
      'type': 'static_library',
      'dependencies': [
        # TODO(benwells): figure out what to do with the api target and
        # api resources compiled into the chrome resource bundle.
        # http://crbug.com/162530
        '../chrome/chrome_resources.gyp:chrome_resources',
        '../chrome/common/extensions/api/api.gyp:api',
        '../content/content.gyp:content_common',
        '../third_party/re2/re2.gyp:re2',
      ],
      'include_dirs': [
        '..',
        '<(INTERMEDIATE_DIR)',
      ],
      'sources': [
        'common/crx_file.cc',
        'common/crx_file.h',
        'common/draggable_region.cc',
        'common/draggable_region.h',
        'common/error_utils.cc',
        'common/error_utils.h',
        'common/event_filter.cc',
        'common/event_filter.h',
        'common/event_filtering_info.cc',
        'common/event_filtering_info.h',
        'common/event_matcher.cc',
        'common/event_matcher.h',
        'common/extension_api.cc',
        'common/extension_api.h',
        'common/extension_api_stub.cc',
        'common/extension_paths.cc',
        'common/extension_paths.h',
        'common/extension_resource.cc',
        'common/extension_resource.h',
        'common/extension_urls.cc',
        'common/extension_urls.h',
        'common/extensions_client.cc',
        'common/extensions_client.h',
        'common/feature_switch.cc',
        'common/feature_switch.h',
        'common/features/feature.cc',
        'common/features/feature.h',
        'common/features/feature_provider.cc',
        'common/features/feature_provider.h',
        'common/id_util.cc',
        'common/id_util.h',
        'common/install_warning.cc',
        'common/install_warning.h',
        'common/manifest.cc',
        'common/manifest.h',
        'common/manifest_constants.cc',
        'common/manifest_constants.h',
        'common/manifest_handler.cc',
        'common/manifest_handler.h',
        'common/manifest_handlers/incognito_info.cc',
        'common/manifest_handlers/incognito_info.h',
        'common/matcher/regex_set_matcher.cc',
        'common/matcher/regex_set_matcher.h',
        'common/matcher/string_pattern.cc',
        'common/matcher/string_pattern.h',
        'common/matcher/substring_set_matcher.cc',
        'common/matcher/substring_set_matcher.h',
        'common/matcher/url_matcher.cc',
        'common/matcher/url_matcher.h',
        'common/matcher/url_matcher_constants.cc',
        'common/matcher/url_matcher_constants.h',
        'common/matcher/url_matcher_factory.cc',
        'common/matcher/url_matcher_factory.h',
        'common/matcher/url_matcher_helpers.cc',
        'common/matcher/url_matcher_helpers.h',
        'common/one_shot_event.cc',
        'common/one_shot_event.h',
        'common/permissions/api_permission.cc',
        'common/permissions/api_permission.h',
        'common/permissions/api_permission_set.cc',
        'common/permissions/api_permission_set.h',
        'common/permissions/permission_message.cc',
        'common/permissions/permission_message.h',
        'common/permissions/permission_message_provider.cc',
        'common/permissions/permission_message_provider.h',
        'common/permissions/permission_set.cc',
        'common/permissions/permission_set.h',
        'common/permissions/permissions_info.cc',
        'common/permissions/permissions_info.h',
        'common/permissions/permissions_provider.h',
        'common/stack_frame.cc',
        'common/stack_frame.h',
        'common/switches.cc',
        'common/switches.h',
        'common/url_pattern.cc',
        'common/url_pattern.h',
        'common/url_pattern_set.cc',
        'common/url_pattern_set.h',
        'common/user_script.cc',
        'common/user_script.h',
        'common/view_type.cc',
        'common/view_type.h',
      ],
      # Disable c4267 warnings until we fix size_t to int truncations.
      'msvs_disabled_warnings': [ 4267, ],
      'conditions': [
        ['enable_extensions==1', {
          'sources!': [
            'common/extension_api_stub.cc',
          ],
        }, {  # enable_extensions == 0
          'sources!': [
            'common/extension_api.cc',
          ],
        }],
      ],
    },
    {
      'target_name': 'extensions_browser',
      'type': 'static_library',
      'dependencies': [
        'extensions_common',
        '../content/content.gyp:content_browser',
        '../skia/skia.gyp:skia',
      ],
      'include_dirs': [
        '..',
        '<(INTERMEDIATE_DIR)',
        '<(SHARED_INTERMEDIATE_DIR)/chrome',
      ],
      'sources': [
        'browser/admin_policy.cc',
        'browser/admin_policy.h',
        'browser/extension_prefs_scope.h',
        'browser/extension_error.cc',
        'browser/extension_error.h',
        'browser/extensions_browser_client.cc',
        'browser/extensions_browser_client.h',
        'browser/file_highlighter.cc',
        'browser/file_highlighter.h',
        'browser/file_reader.cc',
        'browser/file_reader.h',
        'browser/lazy_background_task_queue.cc',
        'browser/lazy_background_task_queue.h',
        'browser/pref_names.cc',
        'browser/pref_names.h',
        'browser/view_type_utils.cc',
        'browser/view_type_utils.h',
      ],
      # Disable c4267 warnings until we fix size_t to int truncations.
      'msvs_disabled_warnings': [ 4267, ],
    },
  ]
}
