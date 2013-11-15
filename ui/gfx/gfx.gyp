# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'gfx',
      'type': '<(component)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/base.gyp:base_i18n',
        '<(DEPTH)/base/base.gyp:base_static',
        '<(DEPTH)/base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '<(DEPTH)/net/net.gyp:net',
        '<(DEPTH)/skia/skia.gyp:skia',
        '<(DEPTH)/third_party/icu/icu.gyp:icui18n',
        '<(DEPTH)/third_party/icu/icu.gyp:icuuc',
        '<(DEPTH)/third_party/libpng/libpng.gyp:libpng',
        '<(DEPTH)/third_party/zlib/zlib.gyp:zlib',
        '<(DEPTH)/url/url.gyp:url_lib',
      ],
      # text_elider.h includes ICU headers.
      'export_dependent_settings': [
        '<(DEPTH)/skia/skia.gyp:skia',
        '<(DEPTH)/third_party/icu/icu.gyp:icui18n',
        '<(DEPTH)/third_party/icu/icu.gyp:icuuc',
      ],
      'defines': [
        'GFX_IMPLEMENTATION',
      ],
      'sources': [
        'android/device_display_info.cc',
        'android/device_display_info.h',
        'android/gfx_jni_registrar.cc',
        'android/gfx_jni_registrar.h',
        'android/java_bitmap.cc',
        'android/java_bitmap.h',
        'android/shared_device_display_info.cc',
        'android/shared_device_display_info.h',
        'animation/animation.cc',
        'animation/animation.h',
        'animation/animation_container.cc',
        'animation/animation_container.h',
        'animation/animation_container_element.h',
        'animation/animation_container_observer.h',
        'animation/animation_delegate.h',
        'animation/linear_animation.cc',
        'animation/linear_animation.h',
        'animation/multi_animation.cc',
        'animation/multi_animation.h',
        'animation/slide_animation.cc',
        'animation/slide_animation.h',
        'animation/throb_animation.cc',
        'animation/throb_animation.h',
        'animation/tween.cc',
        'animation/tween.h',
        'blit.cc',
        'blit.h',
        'box_f.cc',
        'box_f.h',
        'break_list.h',
        'canvas.cc',
        'canvas.h',
        'canvas_android.cc',
        'canvas_paint_gtk.cc',
        'canvas_paint_gtk.h',
        'canvas_paint_mac.h',
        'canvas_paint_mac.mm',
        'canvas_paint_win.cc',
        'canvas_paint_win.h',
        'canvas_skia.cc',
        'canvas_skia_paint.h',
        'codec/jpeg_codec.cc',
        'codec/jpeg_codec.h',
        'codec/png_codec.cc',
        'codec/png_codec.h',
        'color_analysis.cc',
        'color_analysis.h',
        'color_profile.cc',
        'color_profile.h',
        'color_profile_mac.cc',
        'color_profile_win.cc',
        'color_utils.cc',
        'color_utils.h',
        'display.cc',
        'display.h',
        'display_observer.cc',
        'display_observer.h',
        'favicon_size.cc',
        'favicon_size.h',
        'frame_time.h',
        'font.cc',
        'font.h',
        'font_fallback_win.cc',
        'font_fallback_win.h',
        'font_list.cc',
        'font_list.h',
        'font_render_params_android.cc',
        'font_render_params_linux.cc',
        'font_render_params_linux.h',
        'font_smoothing_win.cc',
        'font_smoothing_win.h',
        'gfx_export.h',
        'gfx_paths.cc',
        'gfx_paths.h',
        'gpu_memory_buffer.cc',
        'gpu_memory_buffer.h',
        'image/canvas_image_source.cc',
        'image/canvas_image_source.h',
        'image/image.cc',
        'image/image.h',
        'image/image_family.cc',
        'image/image_family.h',
        'image/image_ios.mm',
        'image/image_mac.mm',
        'image/image_png_rep.cc',
        'image/image_png_rep.h',
        'image/image_skia.cc',
        'image/image_skia.h',
        'image/image_skia_operations.cc',
        'image/image_skia_operations.h',
        'image/image_skia_rep.cc',
        'image/image_skia_rep.h',
        'image/image_skia_source.h',
        'image/image_skia_util_ios.h',
        'image/image_skia_util_ios.mm',
        'image/image_skia_util_mac.h',
        'image/image_skia_util_mac.mm',
        'image/image_util.cc',
        'image/image_util.h',
        'image/image_util_ios.mm',
        'insets.cc',
        'insets.h',
        'insets_base.h',
        'insets_f.cc',
        'insets_f.h',
        'interpolated_transform.cc',
        'interpolated_transform.h',
        'mac/scoped_ns_disable_screen_updates.h',
        'matrix3_f.cc',
        'matrix3_f.h',
        'native_widget_types.h',
        'ozone/dri/dri_skbitmap.cc',
        'ozone/dri/dri_skbitmap.h',
        'ozone/dri/dri_surface.cc',
        'ozone/dri/dri_surface.h',
        'ozone/dri/dri_surface_factory.cc',
        'ozone/dri/dri_surface_factory.h',
        'ozone/dri/dri_vsync_provider.cc',
        'ozone/dri/dri_vsync_provider.h',
        'ozone/dri/dri_wrapper.cc',
        'ozone/dri/dri_wrapper.h',
        'ozone/dri/hardware_display_controller.cc',
        'ozone/dri/hardware_display_controller.h',
        'ozone/impl/file_surface_factory.cc',
        'ozone/impl/file_surface_factory.h',
        'ozone/surface_factory_ozone.cc',
        'ozone/surface_factory_ozone.h',
        'pango_util.cc',
        'pango_util.h',
        'path.cc',
        'path.h',
        'path_aura.cc',
        'path_gtk.cc',
        'path_win.cc',
        'path_win.h',
        'path_x11.cc',
        'path_x11.h',
        'platform_font.h',
        'platform_font_android.cc',
        'platform_font_ios.h',
        'platform_font_ios.mm',
        'platform_font_mac.h',
        'platform_font_mac.mm',
        'platform_font_ozone.cc',
        'platform_font_pango.cc',
        'platform_font_pango.h',
        'platform_font_win.cc',
        'platform_font_win.h',
        'point.cc',
        'point.h',
        'point3_f.cc',
        'point3_f.h',
        'point_base.h',
        'point_conversions.cc',
        'point_conversions.h',
        'point_f.cc',
        'point_f.h',
        'quad_f.cc',
        'quad_f.h',
        'range/range.cc',
        'range/range.h',
        'range/range_mac.mm',
        'range/range_win.cc',
        'rect.cc',
        'rect.h',
        'rect_base.h',
        'rect_base_impl.h',
        'rect_conversions.cc',
        'rect_conversions.h',
        'rect_f.cc',
        'rect_f.h',
        'render_text.cc',
        'render_text.h',
        'render_text_mac.cc',
        'render_text_mac.h',
        'render_text_ozone.cc',
        'render_text_pango.cc',
        'render_text_pango.h',
        'render_text_win.cc',
        'render_text_win.h',
        'safe_integer_conversions.h',
        'scoped_canvas.h',
        'scoped_cg_context_save_gstate_mac.h',
        'scoped_ns_graphics_context_save_gstate_mac.h',
        'scoped_ns_graphics_context_save_gstate_mac.mm',
        'scoped_ui_graphics_push_context_ios.h',
        'scoped_ui_graphics_push_context_ios.mm',
        'screen.cc',
        'screen.h',
        'screen_android.cc',
        'screen_aura.cc',
        'screen_gtk.cc',
        'screen_ios.mm',
        'screen_mac.mm',
        'screen_win.cc',
        'screen_win.h',
        'scrollbar_size.cc',
        'scrollbar_size.h',
        'selection_model.cc',
        'selection_model.h',
        'sequential_id_generator.cc',
        'sequential_id_generator.h',
        'shadow_value.cc',
        'shadow_value.h',
        'size.cc',
        'size.h',
        'size_base.h',
        'size_conversions.cc',
        'size_conversions.h',
        'size_f.cc',
        'size_f.h',
        'skbitmap_operations.cc',
        'skbitmap_operations.h',
        'skia_util.cc',
        'skia_util.h',
        'skia_utils_gtk.cc',
        'skia_utils_gtk.h',
        'switches.cc',
        'switches.h',
        'sys_color_change_listener.cc',
        'sys_color_change_listener.h',
        'text_constants.h',
        'text_elider.cc',
        'text_elider.h',
        'text_utils.cc',
        'text_utils.h',
        'text_utils_android.cc',
        'text_utils_ios.mm',
        'text_utils_skia.cc',
        'transform.cc',
        'transform.h',
        'transform_util.cc',
        'transform_util.h',
        'ui_gfx_exports.cc',
        'utf16_indexing.cc',
        'utf16_indexing.h',
        'vector2d.cc',
        'vector2d.h',
        'vector2d_conversions.cc',
        'vector2d_conversions.h',
        'vector2d_f.cc',
        'vector2d_f.h',
        'vector3d_f.cc',
        'vector3d_f.h',
        'vsync_provider.h',
        'win/dpi.cc',
        'win/dpi.h',
        'win/hwnd_util.cc',
        'win/hwnd_util.h',
        'win/scoped_set_map_mode.h',
        'win/singleton_hwnd.cc',
        'win/singleton_hwnd.h',
        'win/window_impl.cc',
        'win/window_impl.h',
        'x/x11_atom_cache.cc',
        'x/x11_atom_cache.h',
        'x/x11_types.cc',
        'x/x11_types.h',
      ],
      'conditions': [
        ['OS=="ios"', {
          # iOS only uses a subset of UI.
          'sources/': [
            ['exclude', '^codec/jpeg_codec\\.cc$'],
          ],
        }, {
          'dependencies': [
            '<(libjpeg_gyp_path):libjpeg',
          ],
        }],
        # TODO(asvitkine): Switch all platforms to use canvas_skia.cc.
        #                  http://crbug.com/105550
        ['use_canvas_skia==1', {
          'sources!': [
            'canvas_android.cc',
          ],
        }, {  # use_canvas_skia!=1
          'sources!': [
            'canvas_skia.cc',
          ],
        }],
        ['toolkit_uses_gtk == 1', {
          'dependencies': [
            '<(DEPTH)/build/linux/system.gyp:gtk',
          ],
          'sources': [
            'gtk_native_view_id_manager.cc',
            'gtk_native_view_id_manager.h',
            'gtk_preserve_window.cc',
            'gtk_preserve_window.h',
            'gdk_compat.h',
            'gtk_compat.h',
            'gtk_util.cc',
            'gtk_util.h',
            'image/cairo_cached_surface.cc',
            'image/cairo_cached_surface.h',
            'scoped_gobject.h',
          ],
        }],
        ['OS=="win"', {
          'sources': [
            'gdi_util.cc',
            'gdi_util.h',
            'icon_util.cc',
            'icon_util.h',
          ],
          # TODO(jschuh): C4267: http://crbug.com/167187 size_t -> int
          # C4324 is structure was padded due to __declspec(align()), which is
          # uninteresting.
          'msvs_disabled_warnings': [ 4267, 4324 ],
        }],
        ['OS=="android"', {
          'sources!': [
            'animation/throb_animation.cc',
            'display_observer.cc',
            'path.cc',
            'selection_model.cc',
          ],
          'dependencies': [
            'gfx_jni_headers',
          ],
          'link_settings': {
            'libraries': [
              '-landroid',
              '-ljnigraphics',
            ],
          },
        }],
        ['OS=="android" and android_webview_build==0', {
          'dependencies': [
            '<(DEPTH)/base/base.gyp:base_java',
          ],
        }],
        ['OS=="android" or OS=="ios"', {
          'sources!': [
            'render_text.cc',
            'render_text.h',
            'text_utils_skia.cc',
          ],
        }],
        ['use_pango==1', {
          'dependencies': [
            '<(DEPTH)/build/linux/system.gyp:pangocairo',
          ],
          'sources!': [
            'platform_font_ozone.cc',
            'render_text_ozone.cc',
          ],
        }],
        ['ozone_platform_dri==1', {
          'dependencies': [
          '<(DEPTH)/build/linux/system.gyp:dridrm',
          ],
        }],
      ],
      'target_conditions': [
        # Need 'target_conditions' to override default filename_rules to include
        # the file on iOS.
        ['OS == "ios"', {
          'sources/': [
            ['include', '^scoped_cg_context_save_gstate_mac\\.h$'],
          ],
        }],
      ],
    }
  ],
  'conditions': [
    ['OS=="android"' , {
     'targets': [
       {
         'target_name': 'gfx_jni_headers',
         'type': 'none',
         'direct_dependent_settings': {
           'include_dirs': [
             '<(SHARED_INTERMEDIATE_DIR)/ui/gfx',
           ],
         },
         'sources': [
           '../android/java/src/org/chromium/ui/gfx/BitmapHelper.java',
           '../android/java/src/org/chromium/ui/gfx/DeviceDisplayInfo.java',
         ],
         'variables': {
           'jni_gen_package': 'ui/gfx',
           'jni_generator_ptr_type': 'long',
         },
         'includes': [ '../../build/jni_generator.gypi' ],
       },
     ],
    }],
  ],
}
