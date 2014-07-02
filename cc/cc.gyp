# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'cc',
      'type': '<(component)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '<(DEPTH)/gpu/gpu.gyp:gpu',
        '<(DEPTH)/media/media.gyp:media',
        '<(DEPTH)/skia/skia.gyp:skia',
        '<(DEPTH)/ui/events/events.gyp:events_base',
        '<(DEPTH)/ui/gfx/gfx.gyp:gfx',
        '<(DEPTH)/ui/gfx/gfx.gyp:gfx_geometry',
        '<(DEPTH)/ui/gl/gl.gyp:gl',
      ],
      'variables': {
        'optimize': 'max',
      },
      'export_dependent_settings': [
        '<(DEPTH)/skia/skia.gyp:skia',
      ],
      'defines': [
        'CC_IMPLEMENTATION=1',
      ],
      'sources': [
        'animation/animation.cc',
        'animation/animation.h',
        'animation/animation_curve.cc',
        'animation/animation_curve.h',
        'animation/animation_delegate.h',
        'animation/animation_events.cc',
        'animation/animation_events.h',
        'animation/animation_id_provider.cc',
        'animation/animation_id_provider.h',
        'animation/animation_registrar.cc',
        'animation/animation_registrar.h',
        'animation/keyframed_animation_curve.cc',
        'animation/keyframed_animation_curve.h',
        'animation/layer_animation_controller.cc',
        'animation/layer_animation_controller.h',
        'animation/layer_animation_event_observer.h',
        'animation/layer_animation_value_observer.h',
        'animation/layer_animation_value_provider.h',
        'animation/scroll_offset_animation_curve.cc',
        'animation/scroll_offset_animation_curve.h',
        'animation/scrollbar_animation_controller.h',
        'animation/scrollbar_animation_controller.cc',
        'animation/scrollbar_animation_controller_linear_fade.cc',
        'animation/scrollbar_animation_controller_linear_fade.h',
        'animation/scrollbar_animation_controller_thinning.cc',
        'animation/scrollbar_animation_controller_thinning.h',
        'animation/timing_function.cc',
        'animation/timing_function.h',
        'animation/transform_operation.cc',
        'animation/transform_operation.h',
        'animation/transform_operations.cc',
        'animation/transform_operations.h',
        'base/completion_event.h',
        'base/delayed_unique_notifier.cc',
        'base/delayed_unique_notifier.h',
        'base/invalidation_region.cc',
        'base/invalidation_region.h',
        'base/latency_info_swap_promise.cc',
        'base/latency_info_swap_promise.h',
        'base/latency_info_swap_promise_monitor.cc',
        'base/latency_info_swap_promise_monitor.h',
        'base/math_util.cc',
        'base/math_util.h',
        'base/ref_counted_managed.h',
        'base/region.cc',
        'base/region.h',
        'base/rolling_time_delta_history.cc',
        'base/rolling_time_delta_history.h',
        'base/scoped_ptr_algorithm.h',
        'base/scoped_ptr_deque.h',
        'base/scoped_ptr_vector.h',
        'base/swap_promise.h',
        'base/swap_promise_monitor.cc',
        'base/swap_promise_monitor.h',
        'base/switches.cc',
        'base/switches.h',
        'base/tiling_data.cc',
        'base/tiling_data.h',
        'base/unique_notifier.cc',
        'base/unique_notifier.h',
        'base/util.h',
        'debug/benchmark_instrumentation.cc',
        'debug/benchmark_instrumentation.h',
        'debug/debug_colors.cc',
        'debug/debug_colors.h',
        'debug/debug_rect_history.cc',
        'debug/debug_rect_history.h',
        'debug/devtools_instrumentation.h',
        'debug/frame_rate_counter.cc',
        'debug/frame_rate_counter.h',
        'debug/frame_viewer_instrumentation.h',
        'debug/invalidation_benchmark.cc',
        'debug/invalidation_benchmark.h',
        'debug/lap_timer.cc',
        'debug/lap_timer.h',
        'debug/layer_tree_debug_state.cc',
        'debug/layer_tree_debug_state.h',
        'debug/micro_benchmark.cc',
        'debug/micro_benchmark.h',
        'debug/micro_benchmark_impl.cc',
        'debug/micro_benchmark_impl.h',
        'debug/micro_benchmark_controller.cc',
        'debug/micro_benchmark_controller.h',
        'debug/micro_benchmark_controller_impl.cc',
        'debug/micro_benchmark_controller_impl.h',
        'debug/paint_time_counter.cc',
        'debug/paint_time_counter.h',
        'debug/picture_record_benchmark.cc',
        'debug/picture_record_benchmark.h',
        'debug/rasterize_and_record_benchmark.cc',
        'debug/rasterize_and_record_benchmark.h',
        'debug/rasterize_and_record_benchmark_impl.cc',
        'debug/rasterize_and_record_benchmark_impl.h',
        'debug/rendering_stats.cc',
        'debug/rendering_stats.h',
        'debug/rendering_stats_instrumentation.cc',
        'debug/rendering_stats_instrumentation.h',
        'debug/ring_buffer.h',
        'debug/traced_picture.cc',
        'debug/traced_picture.h',
        'debug/traced_value.cc',
        'debug/traced_value.h',
        'debug/unittest_only_benchmark.cc',
        'debug/unittest_only_benchmark.h',
        'debug/unittest_only_benchmark_impl.cc',
        'debug/unittest_only_benchmark_impl.h',
        'input/input_handler.h',
        'input/page_scale_animation.cc',
        'input/page_scale_animation.h',
        'input/layer_selection_bound.cc',
        'input/layer_selection_bound.h',
        'input/selection_bound_type.h',
        'input/top_controls_manager.cc',
        'input/top_controls_manager.h',
        'input/top_controls_manager_client.h',
        'layers/append_quads_data.h',
        'layers/content_layer.cc',
        'layers/content_layer.h',
        'layers/content_layer_client.h',
        'layers/contents_scaling_layer.cc',
        'layers/contents_scaling_layer.h',
        'layers/delegated_frame_provider.cc',
        'layers/delegated_frame_provider.h',
        'layers/delegated_frame_resource_collection.cc',
        'layers/delegated_frame_resource_collection.h',
        'layers/delegated_renderer_layer.cc',
        'layers/delegated_renderer_layer.h',
        'layers/delegated_renderer_layer_impl.cc',
        'layers/delegated_renderer_layer_impl.h',
        'layers/draw_properties.h',
        'layers/heads_up_display_layer.cc',
        'layers/heads_up_display_layer.h',
        'layers/heads_up_display_layer_impl.cc',
        'layers/heads_up_display_layer_impl.h',
        'layers/image_layer.cc',
        'layers/image_layer.h',
        'layers/io_surface_layer.cc',
        'layers/io_surface_layer.h',
        'layers/io_surface_layer_impl.cc',
        'layers/io_surface_layer_impl.h',
        'layers/layer.cc',
        'layers/layer.h',
        'layers/layer_client.h',
        'layers/layer_impl.cc',
        'layers/layer_impl.h',
        'layers/layer_iterator.h',
        'layers/layer_lists.cc',
        'layers/layer_lists.h',
        'layers/layer_position_constraint.cc',
        'layers/layer_position_constraint.h',
        'layers/layer_utils.cc',
        'layers/layer_utils.h',
        'layers/nine_patch_layer.cc',
        'layers/nine_patch_layer.h',
        'layers/nine_patch_layer_impl.cc',
        'layers/nine_patch_layer_impl.h',
        'layers/paint_properties.h',
        'layers/painted_scrollbar_layer.cc',
        'layers/painted_scrollbar_layer.h',
        'layers/painted_scrollbar_layer_impl.cc',
        'layers/painted_scrollbar_layer_impl.h',
        'layers/picture_image_layer.cc',
        'layers/picture_image_layer.h',
        'layers/picture_image_layer_impl.cc',
        'layers/picture_image_layer_impl.h',
        'layers/picture_layer.cc',
        'layers/picture_layer.h',
        'layers/picture_layer_impl.cc',
        'layers/picture_layer_impl.h',
        'layers/render_pass_sink.h',
        'layers/render_surface.cc',
        'layers/render_surface.h',
        'layers/render_surface_impl.cc',
        'layers/render_surface_impl.h',
        'layers/scrollbar_layer_impl_base.cc',
        'layers/scrollbar_layer_impl_base.h',
        'layers/scrollbar_layer_interface.h',
        'layers/solid_color_layer.cc',
        'layers/solid_color_layer.h',
        'layers/solid_color_layer_impl.cc',
        'layers/solid_color_layer_impl.h',
        'layers/solid_color_scrollbar_layer.cc',
        'layers/solid_color_scrollbar_layer.h',
        'layers/solid_color_scrollbar_layer_impl.cc',
        'layers/solid_color_scrollbar_layer_impl.h',
        'layers/surface_layer.cc',
        'layers/surface_layer.h',
        'layers/surface_layer_impl.cc',
        'layers/surface_layer_impl.h',
        'layers/texture_layer.cc',
        'layers/texture_layer.h',
        'layers/texture_layer_client.h',
        'layers/texture_layer_impl.cc',
        'layers/texture_layer_impl.h',
        'layers/tiled_layer.cc',
        'layers/tiled_layer.h',
        'layers/tiled_layer_impl.cc',
        'layers/tiled_layer_impl.h',
        'layers/ui_resource_layer.cc',
        'layers/ui_resource_layer.h',
        'layers/ui_resource_layer_impl.cc',
        'layers/ui_resource_layer_impl.h',
        'layers/video_frame_provider.h',
        'layers/video_frame_provider_client_impl.cc',
        'layers/video_frame_provider_client_impl.h',
        'layers/video_layer.cc',
        'layers/video_layer.h',
        'layers/video_layer_impl.cc',
        'layers/video_layer_impl.h',
        'output/begin_frame_args.cc',
        'output/begin_frame_args.h',
        'output/compositor_frame.cc',
        'output/compositor_frame.h',
        'output/compositor_frame_ack.cc',
        'output/compositor_frame_ack.h',
        'output/compositor_frame_metadata.cc',
        'output/compositor_frame_metadata.h',
        'output/context_provider.cc',
        'output/context_provider.h',
        'output/copy_output_request.cc',
        'output/copy_output_request.h',
        'output/copy_output_result.cc',
        'output/copy_output_result.h',
        'output/delegated_frame_data.h',
        'output/delegated_frame_data.cc',
        'output/delegating_renderer.cc',
        'output/delegating_renderer.h',
        'output/direct_renderer.cc',
        'output/direct_renderer.h',
        'output/filter_operation.cc',
        'output/filter_operation.h',
        'output/filter_operations.cc',
        'output/filter_operations.h',
        'output/geometry_binding.cc',
        'output/geometry_binding.h',
        'output/gl_frame_data.h',
        'output/gl_frame_data.cc',
        'output/gl_renderer.cc',
        'output/gl_renderer.h',
        'output/gl_renderer_draw_cache.cc',
        'output/gl_renderer_draw_cache.h',
        'output/managed_memory_policy.cc',
        'output/managed_memory_policy.h',
        'output/output_surface.cc',
        'output/output_surface.h',
        'output/output_surface_client.h',
        'output/overlay_candidate.cc',
        'output/overlay_candidate.h',
        'output/overlay_candidate_validator.h',
        'output/overlay_processor.cc',
        'output/overlay_processor.h',
        'output/overlay_strategy_single_on_top.cc',
        'output/overlay_strategy_single_on_top.h',
        'output/program_binding.cc',
        'output/program_binding.h',
        'output/render_surface_filters.cc',
        'output/render_surface_filters.h',
        'output/renderer.cc',
        'output/renderer.h',
        'output/shader.cc',
        'output/shader.h',
        'output/software_frame_data.cc',
        'output/software_frame_data.h',
        'output/software_output_device.cc',
        'output/software_output_device.h',
        'output/software_renderer.cc',
        'output/software_renderer.h',
        'output/viewport_selection_bound.cc',
        'output/viewport_selection_bound.h',
        'quads/checkerboard_draw_quad.cc',
        'quads/checkerboard_draw_quad.h',
        'quads/content_draw_quad_base.cc',
        'quads/content_draw_quad_base.h',
        'quads/debug_border_draw_quad.cc',
        'quads/debug_border_draw_quad.h',
        'quads/draw_quad.cc',
        'quads/draw_quad.h',
        'quads/io_surface_draw_quad.cc',
        'quads/io_surface_draw_quad.h',
        'quads/picture_draw_quad.cc',
        'quads/picture_draw_quad.h',
        'quads/render_pass.cc',
        'quads/render_pass.h',
        'quads/render_pass_draw_quad.cc',
        'quads/render_pass_draw_quad.h',
        'quads/shared_quad_state.cc',
        'quads/shared_quad_state.h',
        'quads/solid_color_draw_quad.cc',
        'quads/solid_color_draw_quad.h',
        'quads/stream_video_draw_quad.cc',
        'quads/stream_video_draw_quad.h',
        'quads/surface_draw_quad.cc',
        'quads/surface_draw_quad.h',
        'quads/texture_draw_quad.cc',
        'quads/texture_draw_quad.h',
        'quads/tile_draw_quad.cc',
        'quads/tile_draw_quad.h',
        'quads/yuv_video_draw_quad.cc',
        'quads/yuv_video_draw_quad.h',
        'resources/bitmap_content_layer_updater.cc',
        'resources/bitmap_content_layer_updater.h',
        'resources/bitmap_skpicture_content_layer_updater.cc',
        'resources/bitmap_skpicture_content_layer_updater.h',
        'resources/content_layer_updater.cc',
        'resources/content_layer_updater.h',
        'resources/gpu_raster_worker_pool.cc',
        'resources/gpu_raster_worker_pool.h',
        'resources/image_layer_updater.cc',
        'resources/image_layer_updater.h',
        'resources/image_raster_worker_pool.cc',
        'resources/image_raster_worker_pool.h',
        'resources/image_copy_raster_worker_pool.cc',
        'resources/image_copy_raster_worker_pool.h',
        'resources/layer_painter.h',
        'resources/layer_quad.cc',
        'resources/layer_quad.h',
        'resources/layer_tiling_data.cc',
        'resources/layer_tiling_data.h',
        'resources/layer_updater.cc',
        'resources/layer_updater.h',
        'resources/managed_tile_state.cc',
        'resources/managed_tile_state.h',
        'resources/memory_history.cc',
        'resources/memory_history.h',
        'resources/picture.cc',
        'resources/picture.h',
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
        'resources/pixel_buffer_raster_worker_pool.cc',
        'resources/pixel_buffer_raster_worker_pool.h',
        'resources/platform_color.h',
        'resources/prioritized_resource.cc',
        'resources/prioritized_resource.h',
        'resources/prioritized_resource_manager.cc',
        'resources/prioritized_resource_manager.h',
        'resources/prioritized_tile_set.cc',
        'resources/prioritized_tile_set.h',
        'resources/priority_calculator.cc',
        'resources/priority_calculator.h',
        'resources/raster_mode.cc',
        'resources/raster_mode.h',
        'resources/raster_worker_pool.cc',
        'resources/raster_worker_pool.h',
        'resources/rasterizer.cc',
        'resources/rasterizer.h',
        'resources/release_callback.h',
        'resources/resource.cc',
        'resources/resource.h',
        'resources/resource_format.h',
        'resources/resource_format.cc',
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
        'resources/returned_resource.h',
        'resources/scoped_resource.cc',
        'resources/scoped_resource.h',
        'resources/scoped_ui_resource.cc',
        'resources/scoped_ui_resource.h',
        'resources/shared_bitmap.cc',
        'resources/shared_bitmap.h',
        'resources/shared_bitmap_manager.h',
        'resources/single_release_callback.cc',
        'resources/single_release_callback.h',
        'resources/skpicture_content_layer_updater.cc',
        'resources/skpicture_content_layer_updater.h',
        'resources/task_graph_runner.cc',
        'resources/task_graph_runner.h',
        'resources/texture_mailbox.cc',
        'resources/texture_mailbox.h',
        'resources/texture_mailbox_deleter.cc',
        'resources/texture_mailbox_deleter.h',
        'resources/texture_uploader.cc',
        'resources/texture_uploader.h',
        'resources/tile.cc',
        'resources/tile.h',
        'resources/tile_manager.cc',
        'resources/tile_manager.h',
        'resources/tile_priority.cc',
        'resources/tile_priority.h',
        'resources/transferable_resource.cc',
        'resources/transferable_resource.h',
        'resources/ui_resource_bitmap.cc',
        'resources/ui_resource_bitmap.h',
        'resources/ui_resource_client.h',
        'resources/ui_resource_request.cc',
        'resources/ui_resource_request.h',
        'resources/video_resource_updater.cc',
        'resources/video_resource_updater.h',
        'scheduler/delay_based_time_source.cc',
        'scheduler/delay_based_time_source.h',
        'scheduler/draw_result.h',
        'scheduler/scheduler.cc',
        'scheduler/scheduler.h',
        'scheduler/scheduler_settings.cc',
        'scheduler/scheduler_settings.h',
        'scheduler/scheduler_state_machine.cc',
        'scheduler/scheduler_state_machine.h',
        'scheduler/time_source.h',
        'test/mock_occlusion_tracker.h',
        'trees/blocking_task_runner.cc',
        'trees/blocking_task_runner.h',
        'trees/damage_tracker.cc',
        'trees/damage_tracker.h',
        'trees/layer_sorter.cc',
        'trees/layer_sorter.h',
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
        'trees/occlusion_tracker.cc',
        'trees/occlusion_tracker.h',
        'trees/proxy.cc',
        'trees/proxy.h',
        'trees/proxy_timing_history.cc',
        'trees/proxy_timing_history.h',
        'trees/single_thread_proxy.cc',
        'trees/single_thread_proxy.h',
        'trees/thread_proxy.cc',
        'trees/thread_proxy.h',
        'trees/tree_synchronizer.cc',
        'trees/tree_synchronizer.h',
      ],
      # TODO(jschuh): crbug.com/167187 fix size_t to int truncations.
      'msvs_disabled_warnings': [ 4267, ],
    },
    {
      'target_name': 'cc_surfaces',
      'type': '<(component)',
      'dependencies': [
        'cc',
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '<(DEPTH)/skia/skia.gyp:skia',
        '<(DEPTH)/ui/gfx/gfx.gyp:gfx',
        '<(DEPTH)/ui/gfx/gfx.gyp:gfx_geometry',
      ],
      'defines': [
        'CC_SURFACES_IMPLEMENTATION=1',
      ],
      'sources': [
        'surfaces/display.cc',
        'surfaces/display.h',
        'surfaces/display_client.h',
        'surfaces/surface.cc',
        'surfaces/surface.h',
        'surfaces/surface_aggregator.cc',
        'surfaces/surface_aggregator.h',
        'surfaces/surface_factory.cc',
        'surfaces/surface_factory.h',
        'surfaces/surface_factory_client.h',
        'surfaces/surface_id.h',
        'surfaces/surface_id_allocator.cc',
        'surfaces/surface_id_allocator.h',
        'surfaces/surface_manager.cc',
        'surfaces/surface_manager.h',
        'surfaces/surface_resource_holder.cc',
        'surfaces/surface_resource_holder.h',
        'surfaces/surfaces_export.h',
      ],
    },
  ],
}
