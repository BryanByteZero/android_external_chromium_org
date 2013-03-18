# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'cc_source_files': [
      'animation/animation.cc',
      'animation/animation.h',
      'animation/animation_curve.cc',
      'animation/animation_curve.h',
      'animation/animation_events.h',
      'animation/animation_id_provider.cc',
      'animation/animation_id_provider.h',
      'animation/animation_registrar.cc',
      'animation/animation_registrar.h',
      'append_quads_data.h',
      'resources/bitmap_content_layer_updater.cc',
      'resources/bitmap_content_layer_updater.h',
      'resources/bitmap_skpicture_content_layer_updater.cc',
      'resources/bitmap_skpicture_content_layer_updater.h',
      'resources/caching_bitmap_content_layer_updater.cc',
      'resources/caching_bitmap_content_layer_updater.h',
      'quads/checkerboard_draw_quad.cc',
      'quads/checkerboard_draw_quad.h',
      'base/completion_event.h',
      'output/compositor_frame.cc',
      'output/compositor_frame.h',
      'output/compositor_frame_ack.cc',
      'output/compositor_frame_ack.h',
      'output/compositor_frame_metadata.cc',
      'output/compositor_frame_metadata.h',
      'content_layer.cc',
      'content_layer.h',
      'content_layer_client.h',
      'resources/content_layer_updater.cc',
      'resources/content_layer_updater.h',
      'contents_scaling_layer.cc',
      'contents_scaling_layer.h',
      'output/context_provider.h',
      'trees/damage_tracker.cc',
      'trees/damage_tracker.h',
      'quads/debug_border_draw_quad.cc',
      'quads/debug_border_draw_quad.h',
      'debug/debug_colors.cc',
      'debug/debug_colors.h',
      'debug/debug_rect_history.cc',
      'debug/debug_rect_history.h',
      'delay_based_time_source.cc',
      'delay_based_time_source.h',
      'output/delegated_frame_data.h',
      'output/delegated_frame_data.cc',
      'delegated_renderer_layer.cc',
      'delegated_renderer_layer.h',
      'delegated_renderer_layer_impl.cc',
      'delegated_renderer_layer_impl.h',
      'output/delegating_renderer.cc',
      'output/delegating_renderer.h',
      'debug/devtools_instrumentation.h',
      'output/direct_renderer.cc',
      'output/direct_renderer.h',
      'draw_properties.h',
      'quads/draw_quad.cc',
      'quads/draw_quad.h',
      'debug/fake_web_graphics_context_3d.cc',
      'debug/fake_web_graphics_context_3d.h',
      'frame_rate_controller.cc',
      'frame_rate_controller.h',
      'debug/frame_rate_counter.cc',
      'debug/frame_rate_counter.h',
      'output/geometry_binding.cc',
      'output/geometry_binding.h',
      'output/gl_frame_data.h',
      'output/gl_frame_data.cc',
      'output/gl_renderer.cc',
      'output/gl_renderer.h',
      'output/gl_renderer_draw_cache.cc',
      'output/gl_renderer_draw_cache.h',
      'base/hash_pair.h',
      'heads_up_display_layer.cc',
      'heads_up_display_layer.h',
      'heads_up_display_layer_impl.cc',
      'heads_up_display_layer_impl.h',
      'resources/image_layer_updater.cc',
      'resources/image_layer_updater.h',
      'image_layer.cc',
      'image_layer.h',
      'input/input_handler.h',
      'quads/io_surface_draw_quad.cc',
      'quads/io_surface_draw_quad.h',
      'io_surface_layer.cc',
      'io_surface_layer.h',
      'io_surface_layer_impl.cc',
      'io_surface_layer_impl.h',
      'animation/keyframed_animation_curve.cc',
      'animation/keyframed_animation_curve.h',
      'latency_info.h',
      'layer.cc',
      'layer.h',
      'animation/layer_animation_controller.cc',
      'animation/layer_animation_controller.h',
      'animation/layer_animation_event_observer.h',
      'animation/layer_animation_value_observer.h',
      'layer_impl.cc',
      'layer_impl.h',
      'layer_iterator.cc',
      'layer_iterator.h',
      'resources/layer_painter.h',
      'resources/layer_quad.cc',
      'resources/layer_quad.h',
      'trees/layer_sorter.cc',
      'trees/layer_sorter.h',
      'resources/layer_tiling_data.cc',
      'resources/layer_tiling_data.h',
      'debug/layer_tree_debug_state.cc',
      'debug/layer_tree_debug_state.h',
      'trees/layer_tree_host.cc',
      'trees/layer_tree_host.h',
      'trees/layer_tree_host_client.h',
      'trees/layer_tree_host_common.cc',
      'trees/layer_tree_host_common.h',
      'trees/layer_tree_host_impl.cc',
      'trees/layer_tree_host_impl.h',
      'trees/layer_tree_impl.cc',
      'trees/layer_tree_impl.h',
      'trees/layer_tree_settings.cc',
      'trees/layer_tree_settings.h',
      'resources/layer_updater.cc',
      'resources/layer_updater.h',
      'resources/managed_memory_policy.cc',
      'resources/managed_memory_policy.h',
      'managed_tile_state.cc',
      'managed_tile_state.h',
      'base/math_util.cc',
      'base/math_util.h',
      'resources/memory_history.cc',
      'resources/memory_history.h',
      'nine_patch_layer.cc',
      'nine_patch_layer.h',
      'nine_patch_layer_impl.cc',
      'nine_patch_layer_impl.h',
      'trees/occlusion_tracker.cc',
      'trees/occlusion_tracker.h',
      'output/output_surface.cc',
      'output/output_surface.h',
      'output/output_surface_client.h',
      'debug/overdraw_metrics.cc',
      'debug/overdraw_metrics.h',
      'input/page_scale_animation.cc',
      'input/page_scale_animation.h',
      'debug/paint_time_counter.cc',
      'debug/paint_time_counter.h',
      'resources/picture.cc',
      'resources/picture.h',
      'picture_image_layer.cc',
      'picture_image_layer.h',
      'picture_image_layer_impl.cc',
      'picture_image_layer_impl.h',
      'picture_layer.cc',
      'picture_layer.h',
      'picture_layer_impl.cc',
      'picture_layer_impl.h',
      'resources/picture_layer_tiling.cc',
      'resources/picture_layer_tiling.h',
      'resources/picture_layer_tiling_set.cc',
      'resources/picture_layer_tiling_set.h',
      'resources/picture_pile.cc',
      'resources/picture_pile.h',
      'resources/picture_pile_base.cc',
      'resources/picture_pile_base.h',
      'resources/picture_pile_impl.cc',
      'resources/picture_pile_impl.h',
      'pinch_zoom_scrollbar.cc',
      'pinch_zoom_scrollbar.h',
      'pinch_zoom_scrollbar_geometry.cc',
      'pinch_zoom_scrollbar_geometry.h',
      'pinch_zoom_scrollbar_painter.cc',
      'pinch_zoom_scrollbar_painter.h',
      'resources/platform_color.h',
      'resources/prioritized_resource.cc',
      'resources/prioritized_resource.h',
      'resources/prioritized_resource_manager.cc',
      'resources/prioritized_resource_manager.h',
      'resources/priority_calculator.cc',
      'resources/priority_calculator.h',
      'output/program_binding.cc',
      'output/program_binding.h',
      'trees/proxy.cc',
      'trees/proxy.h',
      'trees/quad_culler.cc',
      'trees/quad_culler.h',
      'quad_sink.h',
      'resources/raster_worker_pool.cc',
      'resources/raster_worker_pool.h',
      'rate_limiter.cc',
      'rate_limiter.h',
      'base/region.cc',
      'base/region.h',
      'quads/render_pass.cc',
      'quads/render_pass.h',
      'quads/render_pass_draw_quad.cc',
      'quads/render_pass_draw_quad.h',
      'render_pass_sink.h',
      'render_surface.cc',
      'render_surface.h',
      'output/render_surface_filters.cc',
      'output/render_surface_filters.h',
      'render_surface_impl.cc',
      'render_surface_impl.h',
      'output/renderer.cc',
      'output/renderer.h',
      'debug/rendering_stats.cc',
      'debug/rendering_stats.h',
      'resources/resource.cc',
      'resources/resource.h',
      'resources/resource_pool.cc',
      'resources/resource_pool.h',
      'resources/resource_provider.cc',
      'resources/resource_provider.h',
      'resources/resource_update.cc',
      'resources/resource_update.h',
      'resources/resource_update_controller.cc',
      'resources/resource_update_controller.h',
      'resources/resource_update_queue.cc',
      'resources/resource_update_queue.h',
      'debug/ring_buffer.h',
      'scheduler.cc',
      'scheduler.h',
      'scheduler_settings.cc',
      'scheduler_settings.h',
      'scheduler_state_machine.cc',
      'scheduler_state_machine.h',
      'base/scoped_ptr_algorithm.h',
      'base/scoped_ptr_deque.h',
      'base/scoped_ptr_hash_map.h',
      'base/scoped_ptr_vector.h',
      'resources/scoped_resource.cc',
      'resources/scoped_resource.h',
      'animation/scrollbar_animation_controller.h',
      'animation/scrollbar_animation_controller_linear_fade.cc',
      'animation/scrollbar_animation_controller_linear_fade.h',
      'scrollbar_geometry_fixed_thumb.cc',
      'scrollbar_geometry_fixed_thumb.h',
      'scrollbar_geometry_stub.cc',
      'scrollbar_geometry_stub.h',
      'scrollbar_layer.cc',
      'scrollbar_layer.h',
      'scrollbar_layer_impl.cc',
      'scrollbar_layer_impl.h',
      'scrollbar_layer_impl_base.h',
      'output/shader.cc',
      'output/shader.h',
      'quads/shared_quad_state.cc',
      'quads/shared_quad_state.h',
      'trees/single_thread_proxy.cc',
      'trees/single_thread_proxy.h',
      'resources/skpicture_content_layer_updater.cc',
      'resources/skpicture_content_layer_updater.h',
      'output/software_frame_data.cc',
      'output/software_frame_data.h',
      'output/software_output_device.cc',
      'output/software_output_device.h',
      'output/software_renderer.cc',
      'output/software_renderer.h',
      'quads/solid_color_draw_quad.cc',
      'quads/solid_color_draw_quad.h',
      'solid_color_layer.cc',
      'solid_color_layer.h',
      'solid_color_layer_impl.cc',
      'solid_color_layer_impl.h',
      'quads/stream_video_draw_quad.cc',
      'quads/stream_video_draw_quad.h',
      'base/switches.cc',
      'base/switches.h',
      'output/texture_copier.cc',
      'output/texture_copier.h',
      'quads/texture_draw_quad.cc',
      'quads/texture_draw_quad.h',
      'texture_layer.cc',
      'texture_layer.h',
      'texture_layer_client.h',
      'texture_layer_impl.cc',
      'texture_layer_impl.h',
      'resources/texture_mailbox.cc',
      'resources/texture_mailbox.h',
      'texture_uploader.cc',
      'texture_uploader.h',
      'base/thread.h',
      'base/thread_impl.cc',
      'base/thread_impl.h',
      'trees/thread_proxy.cc',
      'trees/thread_proxy.h',
      'resources/tile.cc',
      'resources/tile.h',
      'quads/tile_draw_quad.cc',
      'quads/tile_draw_quad.h',
      'resources/tile_manager.cc',
      'resources/tile_manager.h',
      'resources/tile_priority.cc',
      'resources/tile_priority.h',
      'tiled_layer.cc',
      'tiled_layer.h',
      'tiled_layer_impl.cc',
      'tiled_layer_impl.h',
      'base/tiling_data.cc',
      'base/tiling_data.h',
      'time_source.h',
      'timing_function.cc',
      'timing_function.h',
      'input/top_controls_manager.cc',
      'input/top_controls_manager.h',
      'input/top_controls_manager_client.h',
      'resources/transferable_resource.cc',
      'resources/transferable_resource.h',
      'transform_operation.cc',
      'transform_operation.h',
      'transform_operations.cc',
      'transform_operations.h',
      'trees/tree_synchronizer.cc',
      'trees/tree_synchronizer.h',
      'base/util.h',
      'video_frame_provider.h',
      'video_frame_provider_client_impl.cc',
      'video_frame_provider_client_impl.h',
      'video_layer.cc',
      'video_layer.h',
      'video_layer_impl.cc',
      'video_layer_impl.h',
      'vsync_time_source.cc',
      'vsync_time_source.h',
      'base/worker_pool.cc',
      'base/worker_pool.h',
      'quads/yuv_video_draw_quad.cc',
      'quads/yuv_video_draw_quad.h',
    ],
    'conditions': [
      ['inside_chromium_build==1', {
        'webkit_src_dir': '<(DEPTH)/third_party/WebKit',
      }, {
        'webkit_src_dir': '<(DEPTH)/../../..',
      }],
    ],
  },
  'conditions': [
    ['inside_chromium_build==0', {
      'defines': [
        'INSIDE_WEBKIT_BUILD=1',
      ],
    }],
  ],
  'targets': [
    {
      'target_name': 'cc',
      'type': '<(component)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '<(DEPTH)/gpu/gpu.gyp:gpu',
        '<(DEPTH)/skia/skia.gyp:skia',
        '<(DEPTH)/media/media.gyp:media',
        '<(DEPTH)/ui/gl/gl.gyp:gl',
        '<(DEPTH)/ui/surface/surface.gyp:surface',
        '<(DEPTH)/ui/ui.gyp:ui',
        '<(webkit_src_dir)/Source/WebKit/chromium/WebKit.gyp:webkit',
      ],
      'defines': [
        'CC_IMPLEMENTATION=1',
      ],
      'sources': [
        '<@(cc_source_files)',
      ],
      # TODO(jschuh): crbug.com/167187 fix size_t to int truncations.
      'msvs_disabled_warnings': [ 4267, ],
    },
  ],
}
