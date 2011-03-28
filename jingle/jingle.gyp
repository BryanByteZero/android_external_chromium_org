# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },  # variables
  'targets': [
    # A library of various utils for integration with libjingle.
    {
      'target_name': 'jingle_glue',
      'type': '<(library)',
      'sources': [
        'glue/thread_wrapper.cc',
        'glue/thread_wrapper.h',
      ],
      'dependencies': [
        '../base/base.gyp:base',
        '../third_party/libjingle/libjingle.gyp:libjingle',
      ],
      'export_dependent_settings': [
        '../third_party/libjingle/libjingle.gyp:libjingle',
      ],
    },
    # A library for sending and receiving peer-issued notifications.
    #
    # TODO(akalin): Separate out the XMPP stuff from this library into
    # its own library.
    {
      'target_name': 'notifier',
      'type': '<(library)',
      'sources': [
        'notifier/base/chrome_async_socket.cc',
        'notifier/base/chrome_async_socket.h',
        'notifier/base/const_communicator.h',
        'notifier/base/fake_ssl_client_socket.cc',
        'notifier/base/fake_ssl_client_socket.h',
        'notifier/base/gaia_token_pre_xmpp_auth.cc',
        'notifier/base/gaia_token_pre_xmpp_auth.h',
        'notifier/base/notification_method.h',
        'notifier/base/notification_method.cc',
        'notifier/base/notifier_options.cc',
        'notifier/base/notifier_options.h',
        'notifier/base/notifier_options_util.cc',
        'notifier/base/notifier_options_util.h',
        'notifier/base/server_information.cc',
        'notifier/base/server_information.h',
        'notifier/base/task_pump.cc',
        'notifier/base/task_pump.h',
        'notifier/base/weak_xmpp_client.cc',
        'notifier/base/weak_xmpp_client.h',
        'notifier/base/xmpp_client_socket_factory.cc',
        'notifier/base/xmpp_client_socket_factory.h',
        'notifier/base/xmpp_connection.cc',
        'notifier/base/xmpp_connection.h',
        'notifier/communicator/connection_options.cc',
        'notifier/communicator/connection_options.h',
        'notifier/communicator/connection_settings.cc',
        'notifier/communicator/connection_settings.h',
        'notifier/communicator/login.cc',
        'notifier/communicator/login.h',
        'notifier/communicator/login_settings.cc',
        'notifier/communicator/login_settings.h',
        'notifier/communicator/single_login_attempt.cc',
        'notifier/communicator/single_login_attempt.h',
        'notifier/communicator/xmpp_connection_generator.cc',
        'notifier/communicator/xmpp_connection_generator.h',
        'notifier/listener/mediator_thread.h',
        'notifier/listener/mediator_thread_impl.cc',
        'notifier/listener/mediator_thread_impl.h',
        'notifier/listener/notification_constants.cc',
        'notifier/listener/notification_constants.h',
        'notifier/listener/notification_defines.h',
        'notifier/listener/push_notifications_listen_task.cc',
        'notifier/listener/push_notifications_listen_task.h',
        'notifier/listener/push_notifications_send_update_task.cc',
        'notifier/listener/push_notifications_send_update_task.h',
        'notifier/listener/push_notifications_subscribe_task.cc',
        'notifier/listener/push_notifications_subscribe_task.h',
        'notifier/listener/talk_mediator.h',
        'notifier/listener/talk_mediator_impl.cc',
        'notifier/listener/talk_mediator_impl.h',
        'notifier/listener/xml_element_util.cc',
        'notifier/listener/xml_element_util.h',
      ],
      'defines' : [
        '_CRT_SECURE_NO_WARNINGS',
        '_USE_32BIT_TIME_T',
      ],
      'dependencies': [
        '../base/base.gyp:base',
        '../net/net.gyp:net',
        '../third_party/expat/expat.gyp:expat',
        '../third_party/libjingle/libjingle.gyp:libjingle',
      ],
      'export_dependent_settings': [
        '../third_party/libjingle/libjingle.gyp:libjingle',
      ],
      'conditions': [
        ['OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris"', {
          'dependencies': [
            '../build/linux/system.gyp:gtk'
          ],
        }],
      ],
    },
    {
      'target_name': 'notifier_test_util',
      'type': '<(library)',
      'sources': [
        'notifier/base/fake_base_task.cc',
        'notifier/base/fake_base_task.h',
      ],
      'dependencies': [
        'notifier',
        '../base/base.gyp:base',
        '../testing/gmock.gyp:gmock',
      ],
    },
    {
      'target_name': 'jingle_unittests',
      'type': 'executable',
      'sources': [
        'glue/thread_wrapper_unittest.cc',
        'notifier/base/chrome_async_socket_unittest.cc',
        'notifier/base/fake_ssl_client_socket_unittest.cc',
        'notifier/base/xmpp_connection_unittest.cc',
        'notifier/base/weak_xmpp_client_unittest.cc',
        'notifier/communicator/xmpp_connection_generator_unittest.cc',
        'notifier/listener/mediator_thread_mock.cc',
        'notifier/listener/mediator_thread_mock.h',
        'notifier/listener/push_notifications_send_update_task_unittest.cc',
        'notifier/listener/push_notifications_subscribe_task_unittest.cc',
        'notifier/listener/talk_mediator_unittest.cc',
        'notifier/listener/xml_element_util_unittest.cc',
        'run_all_unittests.cc',
      ],
      'include_dirs': [
        '..',
      ],
      'dependencies': [
        'jingle_glue',
        'notifier',
        'notifier_test_util',
        '../base/base.gyp:base',
        '../base/base.gyp:test_support_base',
        '../net/net.gyp:net',
        '../net/net.gyp:net_test_support',
        '../testing/gmock.gyp:gmock',
        '../testing/gtest.gyp:gtest',
        '../third_party/libjingle/libjingle.gyp:libjingle',
      ],
    },
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
