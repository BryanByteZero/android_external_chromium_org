# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'fileapi',
      'type': '<(component)',
      'variables': { 'enable_wexit_time_destructors': 1, },
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/build/temp_gyp/googleurl.gyp:googleurl',
        '<(DEPTH)/net/net.gyp:net',
        '<(DEPTH)/third_party/leveldatabase/leveldatabase.gyp:leveldatabase',
        '<(DEPTH)/webkit/support/webkit_support.gyp:blob',
        '<(DEPTH)/webkit/support/webkit_support.gyp:quota',
        '<(webkit_src_dir)/Source/WebKit/chromium/WebKit.gyp:webkit',
      ],
      'defines': ['FILEAPI_IMPLEMENTATION'],
      'sources': [
        'file_stream_writer.h',
        'file_system_callback_dispatcher.cc',
        'file_system_callback_dispatcher.h',
        'file_system_context.cc',
        'file_system_context.h',
        'file_system_dir_url_request_job.cc',
        'file_system_dir_url_request_job.h',
        'file_system_directory_database.cc',
        'file_system_directory_database.h',
        'file_system_file_stream_reader.cc',
        'file_system_file_stream_reader.h',
        'file_system_file_util.h',
        'file_system_file_util_proxy.cc',
        'file_system_file_util_proxy.h',
        'file_system_mount_point_provider.h',
        'file_system_operation_context.cc',
        'file_system_operation_context.h',
        'file_system_operation_interface.h',
        'file_system_options.cc',
        'file_system_options.h',
        'file_system_origin_database.cc',
        'file_system_origin_database.h',
        'file_system_quota_client.cc',
        'file_system_quota_client.h',
        'file_system_quota_util.cc',
        'file_system_quota_util.h',
        'file_system_types.h',
        'file_system_url.cc',
        'file_system_url.h',
        'file_system_url_request_job.cc',
        'file_system_url_request_job.h',
        'file_system_url_request_job_factory.cc',
        'file_system_url_request_job_factory.h',
        'file_system_usage_cache.cc',
        'file_system_usage_cache.h',
        'file_system_util.cc',
        'file_system_util.h',
        'file_util_helper.cc',
        'file_util_helper.h',
        'file_writer_delegate.cc',
        'file_writer_delegate.h',
        'isolated_context.cc',
        'isolated_context.h',
        'isolated_file_util.cc',
        'isolated_file_util.h',
        'isolated_mount_point_provider.cc',
        'isolated_mount_point_provider.h',
        'local_file_util.cc',
        'local_file_util.h',
        'local_file_stream_writer.cc',
        'local_file_stream_writer.h',
        'local_file_system_operation.cc',
        'local_file_system_operation.h',
        'media/media_path_filter.cc',
        'media/media_path_filter.h',
        'media/native_media_file_util.cc',
        'media/native_media_file_util.h',
        'native_file_util.cc',
        'native_file_util.h',
        'obfuscated_file_util.cc',
        'obfuscated_file_util.h',
        'sandbox_file_stream_writer.cc',
        'sandbox_file_stream_writer.h',
        'sandbox_mount_point_provider.cc',
        'sandbox_mount_point_provider.h',
        'test_mount_point_provider.cc',
        'test_mount_point_provider.h',
        'webfilewriter_base.cc',
        'webfilewriter_base.h',
      ],
      'conditions': [
        ['inside_chromium_build==0', {
          'dependencies': [
            '<(DEPTH)/webkit/support/setup_third_party.gyp:third_party_headers',
          ],
        }],
        ['chromeos==1', {
          'sources': [
            '../chromeos/fileapi/async_file_stream.h',
            '../chromeos/fileapi/cros_mount_point_provider.cc',
            '../chromeos/fileapi/cros_mount_point_provider.h',
            '../chromeos/fileapi/file_access_permissions.cc',
            '../chromeos/fileapi/file_access_permissions.h',
            '../chromeos/fileapi/file_util_async.h',
            '../chromeos/fileapi/remote_file_system_operation.cc',
            '../chromeos/fileapi/remote_file_system_operation.h',
            '../chromeos/fileapi/remote_file_system_proxy.h',
            '../chromeos/fileapi/remote_file_stream_writer.cc',
            '../chromeos/fileapi/remote_file_stream_writer.h',
          ],
        }],
      ],
    },
  ],
}
