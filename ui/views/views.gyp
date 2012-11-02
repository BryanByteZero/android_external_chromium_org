# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
{
  'variables': {
    'chromium_code': 1,
  },

  'target_defaults': {
    'conditions': [
      ['use_aura==1', {
        'sources/': [ ['exclude', '_win\\.(h|cc)$'] ],
        'dependencies': [ '../aura/aura.gyp:aura', ],
      }],
      ['OS!="linux" or chromeos==1', {
        'sources/': [ ['exclude', '_linux\\.(h|cc)$'] ],
      }],
    ],
  },
  'targets': [
    {
      'target_name': 'views',
      'type': '<(component)',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../base/base.gyp:base_i18n',
        '../../base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '../../build/temp_gyp/googleurl.gyp:googleurl',
        '../../skia/skia.gyp:skia',
        '../../third_party/icu/icu.gyp:icui18n',
        '../../third_party/icu/icu.gyp:icuuc',
        '../base/strings/ui_strings.gyp:ui_strings',
        '../compositor/compositor.gyp:compositor',
        '../ui.gyp:ui',
        '../ui.gyp:ui_resources',
      ],
      'defines': [
        'VIEWS_IMPLEMENTATION',
      ],
      'sources': [
        # All .cc, .h under views, except unittests
        'accessibility/native_view_accessibility_win.cc',
        'accessibility/native_view_accessibility_win.h',
        'accessible_pane_view.cc',
        'accessible_pane_view.h',
        'animation/bounds_animator.cc',
        'animation/bounds_animator.h',
        'animation/scroll_animator.cc',
        'animation/scroll_animator.h',
        'background.cc',
        'background.h',
        'border.cc',
        'border.h',
        'bubble/bubble_border.cc',
        'bubble/bubble_border.h',
        'bubble/bubble_delegate.cc',
        'bubble/bubble_delegate.h',
        'bubble/bubble_frame_view.cc',
        'bubble/bubble_frame_view.h',
        'bubble/tray_bubble_view.cc',
        'bubble/tray_bubble_view.h',
        'button_drag_utils.cc',
        'button_drag_utils.h',
        'color_chooser/color_chooser_listener.h',
        'color_chooser/color_chooser_view.cc',
        'color_chooser/color_chooser_view.h',
        'color_constants.cc',
        'color_constants.h',
        'context_menu_controller.h',
        'controls/button/border_images.cc',
        'controls/button/border_images.h',
        'controls/button/button.cc',
        'controls/button/button.h',
        'controls/button/button_dropdown.cc',
        'controls/button/button_dropdown.h',
        'controls/button/checkbox.cc',
        'controls/button/checkbox.h',
        'controls/button/custom_button.cc',
        'controls/button/custom_button.h',
        'controls/button/image_button.cc',
        'controls/button/image_button.h',
        'controls/button/label_button.cc',
        'controls/button/label_button.h',
        'controls/button/label_button_border.cc',
        'controls/button/label_button_border.h',
        'controls/button/menu_button.cc',
        'controls/button/menu_button.h',
        'controls/button/menu_button_listener.h',
        'controls/button/radio_button.cc',
        'controls/button/radio_button.h',
        'controls/button/text_button.cc',
        'controls/button/text_button.h',
        'controls/button/chrome_style.cc',
        'controls/button/chrome_style.h',
        'controls/combobox/combobox.cc',
        'controls/combobox/combobox.h',
        'controls/combobox/combobox_listener.h',
        'controls/combobox/native_combobox_views.cc',
        'controls/combobox/native_combobox_views.h',
        'controls/combobox/native_combobox_win.cc',
        'controls/combobox/native_combobox_win.h',
        'controls/combobox/native_combobox_wrapper.h',
        'controls/focusable_border.cc',
        'controls/focusable_border.h',
        'controls/glow_hover_controller.cc',
        'controls/glow_hover_controller.h',
        'controls/image_view.cc',
        'controls/image_view.h',
        'controls/label.cc',
        'controls/label.h',
        'controls/link.cc',
        'controls/link.h',
        'controls/link_listener.h',
        'controls/menu/display_change_listener_aura.cc',
        'controls/menu/menu.cc',
        'controls/menu/menu.h',
        'controls/menu/menu_2.cc',
        'controls/menu/menu_2.h',
        'controls/menu/menu_config.cc',
        'controls/menu/menu_config.h',
        'controls/menu/menu_config_views.cc',
        'controls/menu/menu_config_win.cc',
        'controls/menu/menu_controller.cc',
        'controls/menu/menu_controller.h',
        'controls/menu/menu_controller_delegate.h',
        'controls/menu/menu_delegate.cc',
        'controls/menu/menu_delegate.h',
        'controls/menu/menu_host.cc',
        'controls/menu/menu_host.h',
        'controls/menu/menu_host_root_view.cc',
        'controls/menu/menu_host_root_view.h',
        'controls/menu/menu_item_view.cc',
        'controls/menu/menu_item_view.h',
        'controls/menu/menu_item_view_views.cc',
        'controls/menu/menu_item_view_win.cc',
        'controls/menu/menu_listener.cc',
        'controls/menu/menu_listener.h',
        'controls/menu/menu_model_adapter.cc',
        'controls/menu/menu_model_adapter.h',
        'controls/menu/menu_runner.cc',
        'controls/menu/menu_runner.h',
        'controls/menu/menu_scroll_view_container.cc',
        'controls/menu/menu_scroll_view_container.h',
        'controls/menu/menu_separator.h',
        'controls/menu/menu_separator_views.cc',
        'controls/menu/menu_separator_win.cc',
        'controls/menu/menu_win.cc',
        'controls/menu/menu_win.h',
        'controls/menu/menu_wrapper.h',
        'controls/menu/native_menu_host.h',
        'controls/menu/native_menu_host_delegate.h',
        'controls/menu/native_menu_win.cc',
        'controls/menu/native_menu_win.h',
        'controls/menu/menu_image_util.cc',
        'controls/menu/menu_image_util.h',
        'controls/menu/submenu_view.cc',
        'controls/menu/submenu_view.h',
        'controls/message_box_view.cc',
        'controls/message_box_view.h',
        'controls/native_control.cc',
        'controls/native_control.h',
        'controls/native_control_win.cc',
        'controls/native_control_win.h',
        'controls/native/native_view_host.cc',
        'controls/native/native_view_host.h',
        'controls/native/native_view_host_aura.cc',
        'controls/native/native_view_host_aura.h',
        'controls/native/native_view_host_win.cc',
        'controls/native/native_view_host_win.h',
        'controls/progress_bar.cc',
        'controls/progress_bar.h',
        'controls/resize_area.cc',
        'controls/resize_area.h',
        'controls/resize_area_delegate.h',
        'controls/scroll_view.cc',
        'controls/scroll_view.h',
        'controls/scrollbar/base_scroll_bar.cc',
        'controls/scrollbar/base_scroll_bar.h',
        'controls/scrollbar/base_scroll_bar_button.cc',
        'controls/scrollbar/base_scroll_bar_button.h',
        'controls/scrollbar/base_scroll_bar_thumb.cc',
        'controls/scrollbar/base_scroll_bar_thumb.h',
        'controls/scrollbar/bitmap_scroll_bar.cc',
        'controls/scrollbar/bitmap_scroll_bar.h',
        'controls/scrollbar/native_scroll_bar_views.cc',
        'controls/scrollbar/native_scroll_bar_views.h',
        'controls/scrollbar/native_scroll_bar_win.cc',
        'controls/scrollbar/native_scroll_bar_win.h',
        'controls/scrollbar/native_scroll_bar_wrapper.h',
        'controls/scrollbar/native_scroll_bar.cc',
        'controls/scrollbar/native_scroll_bar.h',
        'controls/scrollbar/scroll_bar.cc',
        'controls/scrollbar/scroll_bar.h',
        'controls/separator.cc',
        'controls/separator.h',
        'controls/single_split_view.cc',
        'controls/single_split_view.h',
        'controls/single_split_view_listener.h',
        'controls/slider.cc',
        'controls/slider.h',
        'controls/tabbed_pane/native_tabbed_pane_views.cc',
        'controls/tabbed_pane/native_tabbed_pane_views.h',
        'controls/tabbed_pane/native_tabbed_pane_win.cc',
        'controls/tabbed_pane/native_tabbed_pane_win.h',
        'controls/tabbed_pane/native_tabbed_pane_wrapper.h',
        'controls/tabbed_pane/tabbed_pane.cc',
        'controls/tabbed_pane/tabbed_pane.h',
        'controls/tabbed_pane/tabbed_pane_listener.h',
        'controls/table/group_table_model.h',
        'controls/table/group_table_view.h',
        'controls/table/group_table_view_views.cc',
        'controls/table/group_table_view_views.h',
        'controls/table/group_table_view_win.cc',
        'controls/table/group_table_view_win.h',
        'controls/table/table_view.h',
        'controls/table/table_view_observer.h',
        'controls/table/table_view_views.cc',
        'controls/table/table_view_views.h',
        'controls/table/table_view_win.cc',
        'controls/table/table_view_win.h',
        'controls/textfield/native_textfield_views.cc',
        'controls/textfield/native_textfield_views.h',
        'controls/textfield/native_textfield_win.cc',
        'controls/textfield/native_textfield_win.h',
        'controls/textfield/native_textfield_wrapper.h',
        'controls/textfield/textfield.cc',
        'controls/textfield/textfield.h',
        'controls/textfield/textfield_controller.cc',
        'controls/textfield/textfield_controller.h',
        'controls/textfield/textfield_views_model.cc',
        'controls/textfield/textfield_views_model.h',
        'controls/throbber.cc',
        'controls/throbber.h',
        'controls/tree/tree_view.h',
        'controls/tree/tree_view_controller.cc',
        'controls/tree/tree_view_controller.h',
        'controls/tree/tree_view_views.cc',
        'controls/tree/tree_view_views.h',
        'controls/tree/tree_view_win.cc',
        'controls/tree/tree_view_win.h',
        # TODO(beng): rename to 'corewm/' hence this sort order:
        '../aura/shared/compound_event_filter.cc',
        '../aura/shared/compound_event_filter.h',
        '../aura/shared/input_method_event_filter.cc',
        '../aura/shared/input_method_event_filter.h',
        'debug_utils.cc',
        'debug_utils.h',
        'drag_controller.h',
        'drag_utils.cc',
        'drag_utils.h',
        'focus/accelerator_handler.h',
        'focus/accelerator_handler_aura.cc',
        'focus/accelerator_handler_win.cc',
        'focus/external_focus_tracker.cc',
        'focus/external_focus_tracker.h',
        'focus/focus_manager.cc',
        'focus/focus_manager.h',
        'focus/focus_manager_delegate.h',
        'focus/focus_manager_factory.cc',
        'focus/focus_manager_factory.h',
        'focus/focus_search.cc',
        'focus/focus_search.h',
        'focus/view_storage.cc',
        'focus/view_storage.h',
        'focus/widget_focus_manager.cc',
        'focus/widget_focus_manager.h',
        'focus_border.cc',
        'focus_border.h',
        'ime/input_method_base.cc',
        'ime/input_method_base.h',
        'ime/input_method_bridge.cc',
        'ime/input_method_bridge.h',
        'ime/input_method_delegate.h',
        'ime/input_method.h',
        'ime/input_method_win.cc',
        'ime/input_method_win.h',
        'ime/mock_input_method.cc',
        'ime/mock_input_method.h',
        'layout/box_layout.cc',
        'layout/box_layout.h',
        'layout/fill_layout.cc',
        'layout/fill_layout.h',
        'layout/grid_layout.cc',
        'layout/grid_layout.h',
        'layout/layout_constants.h',
        'layout/layout_manager.cc',
        'layout/layout_manager.h',
        'metrics.cc',
        'metrics.h',
        'metrics_aura.cc',
        'metrics_win.cc',
        'mouse_watcher.cc',
        'mouse_watcher.h',
        'mouse_watcher_view_host.cc',
        'mouse_watcher_view_host.h',
        'native_theme_delegate.h',
        'native_theme_painter.cc',
        'native_theme_painter.h',
        'painter.cc',
        'painter.h',
        'repeat_controller.cc',
        'repeat_controller.h',
        'touchui/touch_selection_controller.cc',
        'touchui/touch_selection_controller.h',
        'view.cc',
        'view.h',
        'view_constants.cc',
        'view_constants.h',
        'view_aura.cc',
        'view_model.cc',
        'view_model.h',
        'view_model_utils.cc',
        'view_model_utils.h',
        'view_text_utils.cc',
        'view_text_utils.h',
        'view_win.cc',
        'views_delegate.h',
        'widget/aero_tooltip_manager.cc',
        'widget/aero_tooltip_manager.h',
        'widget/child_window_message_processor.cc',
        'widget/child_window_message_processor.h',
        'widget/default_theme_provider.cc',
        'widget/default_theme_provider.h',
        # TODO(beng): rename to 'widget/desktop_aura/' hence this sort order:
        '../aura/desktop/desktop_activation_client.cc',
        '../aura/desktop/desktop_activation_client.h',
        '../aura/desktop/desktop_cursor_client.cc',
        '../aura/desktop/desktop_cursor_client.h',
        '../aura/desktop/desktop_dispatcher_client.cc',
        '../aura/desktop/desktop_dispatcher_client.h',
        '../aura/desktop/desktop_screen.h',
        '../aura/desktop/desktop_screen_win.cc',
        '../aura/desktop/desktop_screen_win.h',
        '../aura/desktop/desktop_screen_x11.cc',
        '../aura/desktop/desktop_stacking_client.cc',
        '../aura/desktop/desktop_stacking_client.h',
        'widget/desktop_layout_manager.cc',
        'widget/desktop_layout_manager.h',
        'widget/desktop_native_widget_aura.cc',
        'widget/desktop_native_widget_aura.h',
        'widget/desktop_root_window_host.h',
        'widget/desktop_root_window_host_linux.cc',
        'widget/desktop_root_window_host_linux.h',
        'widget/desktop_root_window_host_win.cc',
        'widget/desktop_root_window_host_win.h',
        'widget/desktop_screen_position_client.cc',
        'widget/desktop_screen_position_client.h',
        'widget/drop_helper.cc',
        'widget/drop_helper.h',
        'widget/drop_target_win.cc',
        'widget/drop_target_win.h',
        'widget/root_view.cc',
        'widget/root_view.h',
        'widget/tooltip_manager_aura.cc',
        'widget/tooltip_manager_aura.h',
        'widget/tooltip_manager_win.cc',
        'widget/tooltip_manager_win.h',
        'widget/tooltip_manager.cc',
        'widget/tooltip_manager.h',
        'widget/monitor_win.cc',
        'widget/monitor_win.h',
        'widget/native_widget.h',
        'widget/native_widget_aura.cc',
        'widget/native_widget_aura.h',
        'widget/native_widget_aura_window_observer.cc',
        'widget/native_widget_aura_window_observer.h',
        'widget/native_widget_delegate.h',
        'widget/native_widget_private.h',
        'widget/native_widget_win.cc',
        'widget/native_widget_win.h',
        'widget/widget.cc',
        'widget/widget.h',
        'widget/widget_aura_utils.cc',
        'widget/widget_aura_utils.h',
        'widget/widget_delegate.cc',
        'widget/widget_delegate.h',
        'widget/widget_hwnd_utils.cc',
        'widget/widget_hwnd_utils.h',
        'widget/widget_message_filter.cc',
        'widget/widget_message_filter.h',
        'widget/widget_observer.h',
        'widget/x11_desktop_handler.cc',
        'widget/x11_desktop_handler.h',
        'widget/x11_desktop_window_move_client.cc',
        'widget/x11_desktop_window_move_client.h',
        'widget/x11_window_event_filter.cc',
        'widget/x11_window_event_filter.h',
        'win/fullscreen_handler.cc',
        'win/fullscreen_handler.h',
        'win/hwnd_message_handler.cc',
        'win/hwnd_message_handler.h',
        'win/hwnd_message_handler_delegate.h',
        'win/scoped_fullscreen_visibility.cc',
        'win/scoped_fullscreen_visibility.h',
        'window/client_view.cc',
        'window/client_view.h',
        'window/custom_frame_view.cc',
        'window/custom_frame_view.h',
        'window/dialog_client_view.cc',
        'window/dialog_client_view.h',
        'window/dialog_delegate.cc',
        'window/dialog_delegate.h',
        'window/frame_background.cc',
        'window/frame_background.h',
        'window/native_frame_view.cc',
        'window/native_frame_view.h',
        'window/non_client_view.cc',
        'window/non_client_view.h',
        'window/window_resources.h',
        'window/window_shape.cc',
        'window/window_shape.h',
      ],
      'include_dirs': [
        '../../third_party/wtl/include',
      ],
      'conditions': [
        ['use_aura==1', {
          'sources/': [
            ['exclude', 'controls/menu/menu_2.*'],
          ],
          'sources!': [
            'controls/native_control.cc',
            'controls/native_control.h',
            'controls/scrollbar/bitmap_scroll_bar.cc',
            'controls/scrollbar/bitmap_scroll_bar.h',
            'controls/table/group_table_model.h',
            'controls/table/group_table_view.cc',
            'controls/table/group_table_view.h',
            'controls/table/table_view_observer.h',
            'widget/aero_tooltip_manager.cc',
            'widget/aero_tooltip_manager.h',
            'widget/child_window_message_processor.cc',
            'widget/child_window_message_processor.h',
          ],
          'conditions': [
            ['OS=="mac"', {
              'sources/': [
                ['exclude', 'mouse_watcher.cc'],
                ['exclude', 'controls/menu/'],
                ['exclude', 'controls/scrollbar/'],
                ['exclude', 'focus/accelerator_handler_aura.cc'],
              ],
            }],
            ['OS=="win"', {
              'sources/': [
                ['include', '../aura/desktop/desktop_screen_win.cc'],
                ['include', '../aura/desktop/desktop_screen_win.h'],
                ['include', 'ime/input_method_win.cc'],
                ['include', 'ime/input_method_win.h'],
                ['include', 'widget/desktop_root_window_host_win.cc'],
                ['include', 'widget/desktop_root_window_host_win.h'],
              ],
            }],
          ],
        }],
        ['use_aura==0', {
          'sources/': [
            ['exclude', '../aura/shared/'],
            ['exclude', '../aura/desktop/'],
            ['exclude', '/desktop_[^/]*\\.cc$'],
            ['exclude', '/x11_[^/]*\\.cc$'],
            ['exclude', 'widget/native_widget_aura_window_observer.cc'],
            ['exclude', 'widget/native_widget_aura_window_observer.h'],
            ['exclude', 'widget/widget_aura_utils.cc'],
            ['exclude', 'widget/widget_aura_utils.h'],
          ],
        }],
        ['chromeos==1', {
          'sources/': [
            ['exclude', '/desktop_[^/]*\\.cc$'],
            ['exclude', 'widget/x11_*'],
            ['exclude', '../aura/desktop/'],
          ],
        }],
        ['use_aura==0 and OS=="win"', {
          'sources!': [
            'controls/menu/menu_config_views.cc',
            'controls/menu/menu_item_view_views.cc',
            'controls/menu/menu_separator_views.cc',
            'controls/table/group_table_view_views.cc',
            'controls/table/group_table_view_views.h',
            'controls/table/table_view_views.cc',
            'controls/table/table_view_views.h',
            'controls/tree/tree_view_views.cc',
            'controls/tree/tree_view_views.h',
          ],
        }],
        ['OS=="win"', {
          'dependencies': [
            # For accessibility
            '../../third_party/iaccessible2/iaccessible2.gyp:iaccessible2',
          ],
          'include_dirs': [
            '../../third_party/wtl/include',
          ],
          'link_settings': {
            'msvs_settings': {
              'VCLinkerTool': {
                'DelayLoadDLLs': [
                  'user32.dll',
                ],
              },
            },
          },
        }],
        ['use_aura==0 or OS!="win"', {
          'sources!': [
            'widget/widget_message_filter.cc',
            'widget/widget_message_filter.h',
          ],
        }],
        ['OS!="win"', {
          'sources!': [
            'win/fullscreen_handler.cc',
            'win/fullscreen_handler.h',
            'win/hwnd_message_handler.cc',
            'win/hwnd_message_handler.h',
            'win/hwnd_message_handler_delegate.h',
            'win/scoped_fullscreen_visibility.cc',
            'win/scoped_fullscreen_visibility.h',
            'widget/widget_hwnd_utils.cc',
            'widget/widget_hwnd_utils.h',
          ],
        }],
      ],
    }, # target_name: views
    {
      'target_name': 'views_test_support',
      'type': 'static_library',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../ipc/ipc.gyp:test_support_ipc',
        '../../skia/skia.gyp:skia',
        '../../testing/gtest.gyp:gtest',
        '../ui.gyp:ui',
        'views',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        'test/desktop_test_views_delegate.cc',
        'test/desktop_test_views_delegate.h',
        'test/test_views_delegate.cc',
        'test/test_views_delegate.h',
        'test/views_test_base.cc',
        'test/views_test_base.h',
      ],
    },  # target_name: views_test_support
    {
      'target_name': 'views_with_content_test_support',
      'type': 'static_library',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../content/content.gyp:content',
        '../../content/content.gyp:test_support_content',
        '../../ipc/ipc.gyp:test_support_ipc',
        '../../skia/skia.gyp:skia',
        '../../testing/gtest.gyp:gtest',
        '../ui.gyp:ui',
        'controls/webview/webview.gyp:webview',
        'views_test_support',
        'views',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        'test/webview_test_helper.cc',
        'test/webview_test_helper.h',
      ],
    },  # target_name: views_with_content_test_support
    {
      'target_name': 'views_unittests',
      'type': 'executable',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../base/base.gyp:test_support_base',
        # TODO(jcivelli): ideally the resource needed by views would be
        #                 factored out. (for some reason it pulls in a bunch
        #                 unrelated things like v8, sqlite nss...).
        '../../chrome/chrome_resources.gyp:packed_resources',
        '../../build/temp_gyp/googleurl.gyp:googleurl',
        '../../skia/skia.gyp:skia',
        '../../testing/gmock.gyp:gmock',
        '../../testing/gtest.gyp:gtest',
        '../../third_party/icu/icu.gyp:icui18n',
        '../../third_party/icu/icu.gyp:icuuc',
        '../base/strings/ui_strings.gyp:ui_strings',
        '../compositor/compositor.gyp:compositor',
        '../compositor/compositor.gyp:compositor_test_support',
        '../ui.gyp:ui',
        '../ui.gyp:ui_resources',
        '../ui.gyp:ui_test_support',
        'views',
        'views_test_support',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        'accessible_pane_view_unittest.cc',
        'animation/bounds_animator_unittest.cc',
        'bubble/bubble_border_unittest.cc',
        'bubble/bubble_delegate_unittest.cc',
        'bubble/bubble_frame_view_unittest.cc',
        'controls/button/image_button_unittest.cc',
        'controls/button/label_button_unittest.cc',
        'controls/combobox/native_combobox_views_unittest.cc',
        'controls/label_unittest.cc',
        'controls/menu/menu_model_adapter_unittest.cc',
        'controls/native/native_view_host_unittest.cc',
        'controls/progress_bar_unittest.cc',
        'controls/scrollbar/scrollbar_unittest.cc',
        'controls/single_split_view_unittest.cc',
        'controls/slider_unittest.cc',
        'controls/tabbed_pane/tabbed_pane_unittest.cc',
        'controls/table/table_view_unittest.cc',
        'controls/textfield/native_textfield_views_unittest.cc',
        'controls/textfield/textfield_views_model_unittest.cc',
        'controls/tree/tree_view_views_unittest.cc',
        # TODO(beng): rename 'corewm/' hence this sort order
        '../aura/shared/compound_event_filter_unittest.cc',
        '../aura/shared/input_method_event_filter_unittest.cc',
        'focus/focus_manager_test.h',
        'focus/focus_manager_test.cc',
        'focus/focus_manager_unittest.cc',
        'focus/focus_manager_unittest_win.cc',
        'focus/focus_traversal_unittest.cc',
        'layout/box_layout_unittest.cc',
        'layout/grid_layout_unittest.cc',
        'view_model_unittest.cc',
        'view_model_utils_unittest.cc',
        'view_unittest.cc',
        'widget/native_widget_aura_unittest.cc',
        'widget/native_widget_test_utils.h',
        'widget/native_widget_test_utils_aura.cc',
        'widget/native_widget_test_utils_win.cc',
        'widget/native_widget_unittest.cc',
        'widget/native_widget_win_unittest.cc',
        'widget/widget_unittest.cc',
        'run_all_unittests.cc',
      ],
      'conditions': [
        ['OS=="win"', {
          'link_settings': {
            'libraries': [
              '-limm32.lib',
              '-loleacc.lib',
            ]
          },
          'include_dirs': [
            '../third_party/wtl/include',
          ],
        }],
        ['OS=="win" and win_use_allocator_shim==1', {
          'dependencies': [
            '../../base/allocator/allocator.gyp:allocator',
          ],
        }],
        ['use_aura==0 and OS=="win"', {
          'sources/': [
            ['exclude', 'controls/combobox/native_combobox_views_unittest.cc'],
            ['exclude', 'controls/tree/tree_view_views_unittest.cc'],
          ],
        }],
        [ 'use_aura==1', {
          'dependencies': [
            '../aura/aura.gyp:test_support_aura',
          ],
          'sources/': [
            ['exclude', 'controls/table/table_view_unittest.cc'],
            ['exclude', 'widget/native_widget_win_unittest.cc'],
          ],
        }, {
          'sources/': [
            ['exclude', '../aura/shared/'],
            ['exclude', '../aura/test/test_desktop_delegate.cc'],
            ['exclude', '../aura/test/test_desktop_delegate.h'],
            ['exclude', 'widget/native_widget_aura_unittest.cc'],
          ],
        }],
      ],
    },  # target_name: views_unittests
    {
      'target_name': 'views_examples_lib',
      'type': '<(component)',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../chrome/chrome_resources.gyp:packed_resources',
        '../../skia/skia.gyp:skia',
        '../../third_party/icu/icu.gyp:icui18n',
        '../../third_party/icu/icu.gyp:icuuc',
        '../ui.gyp:ui',
        '../ui.gyp:ui_resources',
        'views',
      ],
      'include_dirs': [
        '..',
      ],
      'defines': [
        'VIEWS_EXAMPLES_IMPLEMENTATION',
      ],
      'sources': [
        'examples/bubble_example.cc',
        'examples/bubble_example.h',
        'examples/button_example.cc',
        'examples/button_example.h',
        'examples/combobox_example.cc',
        'examples/combobox_example.h',
        'examples/double_split_view_example.cc',
        'examples/double_split_view_example.h',
        'examples/example_base.cc',
        'examples/example_base.h',
        'examples/example_combobox_model.cc',
        'examples/example_combobox_model.h',
        'examples/examples_window.cc',
        'examples/examples_window.h',
        'examples/label_example.cc',
        'examples/label_example.h',
        'examples/link_example.cc',
        'examples/link_example.h',
        'examples/message_box_example.cc',
        'examples/message_box_example.h',
        'examples/menu_example.cc',
        'examples/menu_example.h',
        'examples/native_theme_button_example.cc',
        'examples/native_theme_button_example.h',
        'examples/native_theme_checkbox_example.cc',
        'examples/native_theme_checkbox_example.h',
        'examples/progress_bar_example.cc',
        'examples/progress_bar_example.h',
        'examples/radio_button_example.cc',
        'examples/radio_button_example.h',
        'examples/scroll_view_example.cc',
        'examples/scroll_view_example.h',
        'examples/single_split_view_example.cc',
        'examples/single_split_view_example.h',
        'examples/slider_example.cc',
        'examples/slider_example.h',
        'examples/tabbed_pane_example.cc',
        'examples/tabbed_pane_example.h',
        'examples/table_example.cc',
        'examples/table_example.h',
        'examples/text_example.cc',
        'examples/text_example.h',
        'examples/textfield_example.cc',
        'examples/textfield_example.h',
        'examples/throbber_example.cc',
        'examples/throbber_example.h',
        'examples/tree_view_example.cc',
        'examples/tree_view_example.h',
        'examples/views_examples_export.h',
        'examples/widget_example.cc',
        'examples/widget_example.h',
      ],
      'conditions': [
        ['OS=="win"', {
          'include_dirs': [
            '../third_party/wtl/include',
          ],
        }],
      ],
    },  # target_name: views_examples_lib
    {
      'target_name': 'views_examples_with_content_lib',
      'type': '<(component)',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../base/base.gyp:base_i18n',
        '../../build/temp_gyp/googleurl.gyp:googleurl',
        '../../chrome/chrome_resources.gyp:packed_resources',
        '../../content/content.gyp:content',
        '../../skia/skia.gyp:skia',
        '../../third_party/icu/icu.gyp:icui18n',
        '../../third_party/icu/icu.gyp:icuuc',
        '../ui.gyp:ui',
        '../ui.gyp:ui_resources',
        'controls/webview/webview.gyp:webview',
        'views',
      ],
      'include_dirs': [
        '..',
      ],
      'defines': [
        'VIEWS_EXAMPLES_WITH_CONTENT_IMPLEMENTATION',
      ],
      'sources': [
        'examples/bubble_example.cc',
        'examples/bubble_example.h',
        'examples/button_example.cc',
        'examples/button_example.h',
        'examples/combobox_example.cc',
        'examples/combobox_example.h',
        'examples/double_split_view_example.cc',
        'examples/double_split_view_example.h',
        'examples/example_base.cc',
        'examples/example_base.h',
        'examples/example_combobox_model.cc',
        'examples/example_combobox_model.h',
        'examples/examples_window_with_content.cc',
        'examples/examples_window_with_content.h',
        'examples/label_example.cc',
        'examples/label_example.h',
        'examples/link_example.cc',
        'examples/link_example.h',
        'examples/message_box_example.cc',
        'examples/message_box_example.h',
        'examples/menu_example.cc',
        'examples/menu_example.h',
        'examples/native_theme_button_example.cc',
        'examples/native_theme_button_example.h',
        'examples/native_theme_checkbox_example.cc',
        'examples/native_theme_checkbox_example.h',
        'examples/progress_bar_example.cc',
        'examples/progress_bar_example.h',
        'examples/radio_button_example.cc',
        'examples/radio_button_example.h',
        'examples/scroll_view_example.cc',
        'examples/scroll_view_example.h',
        'examples/single_split_view_example.cc',
        'examples/single_split_view_example.h',
        'examples/slider_example.cc',
        'examples/slider_example.h',
        'examples/tabbed_pane_example.cc',
        'examples/tabbed_pane_example.h',
        'examples/table_example.cc',
        'examples/table_example.h',
        'examples/text_example.cc',
        'examples/text_example.h',
        'examples/textfield_example.cc',
        'examples/textfield_example.h',
        'examples/throbber_example.cc',
        'examples/throbber_example.h',
        'examples/tree_view_example.cc',
        'examples/tree_view_example.h',
        'examples/views_examples_with_content_export.h',
        'examples/webview_example.cc',
        'examples/webview_example.h',
        'examples/widget_example.cc',
        'examples/widget_example.h',
      ],
      'conditions': [
        ['OS=="win"', {
          'include_dirs': [
            '../third_party/wtl/include',
          ],
        }],
      ],
    },  # target_name: views_examples_with_content_lib
    {
      'target_name': 'views_examples_with_content_exe',
      'type': 'executable',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../base/base.gyp:base_i18n',
        '../../chrome/chrome_resources.gyp:packed_resources',
        '../../content/content.gyp:content_shell_lib',
        '../../content/content.gyp:content',
        '../../content/content.gyp:test_support_content',
        '../../skia/skia.gyp:skia',
        '../../third_party/icu/icu.gyp:icui18n',
        '../../third_party/icu/icu.gyp:icuuc',
        '../ui.gyp:ui',
        '../ui.gyp:ui_resources',
        'views',
        'views_examples_with_content_lib',
        'views_test_support'
      ],
      'include_dirs': [
        '../..',
      ],
      'sources': [
        '../../content/app/startup_helper_win.cc',
        'examples/content_client/examples_browser_main_parts.cc',
        'examples/content_client/examples_browser_main_parts.h',
        'examples/content_client/examples_content_browser_client.cc',
        'examples/content_client/examples_content_browser_client.h',
        'examples/content_client/examples_main_delegate.cc',
        'examples/content_client/examples_main_delegate.h',
        'examples/examples_main.cc',
      ],
      'conditions': [
        ['OS=="win"', {
          'link_settings': {
            'libraries': [
              '-limm32.lib',
              '-loleacc.lib',
            ]
          },
          'msvs_settings': {
            'VCManifestTool': {
              'AdditionalManifestFiles': 'examples\\views_examples.exe.manifest',
            },
            'VCLinkerTool': {
              'SubSystem': '2',  # Set /SUBSYSTEM:WINDOWS
            },
          },
          'dependencies': [
            '../../sandbox/sandbox.gyp:sandbox',
          ],
        }],
        ['use_aura==1', {
          'dependencies': [
            '../compositor/compositor.gyp:compositor',
            '../compositor/compositor.gyp:compositor_test_support',
          ],
        }],
        ['OS=="win"', {
          'sources/': [
            # This is needed because the aura rule strips it from the default
            # sources list.
            ['include', '^../../content/app/startup_helper_win.cc'],
          ],
        }],
      ],
    },  # target_name: views_examples_with_content_exe
  ],
}
