// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_OUTPUT_FILTER_OPERATION_H_
#define CC_OUTPUT_FILTER_OPERATION_H_

#include "base/logging.h"
#include "cc/base/cc_export.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkScalar.h"
#include "ui/gfx/point.h"

namespace cc {

class CC_EXPORT FilterOperation {
 public:
  enum FilterType {
    GRAYSCALE,
    SEPIA,
    SATURATE,
    HUE_ROTATE,
    INVERT,
    BRIGHTNESS,
    CONTRAST,
    OPACITY,
    BLUR,
    DROP_SHADOW,
    COLOR_MATRIX,
    ZOOM,
    SATURATING_BRIGHTNESS,  // Not used in CSS/SVG.
  };

  FilterType type() const { return type_; }

  float amount() const {
    DCHECK_NE(type_, COLOR_MATRIX);
    return amount_;
  }

  gfx::Point drop_shadow_offset() const {
    DCHECK_EQ(type_, DROP_SHADOW);
    return drop_shadow_offset_;
  }

  SkColor drop_shadow_color() const {
    DCHECK_EQ(type_, DROP_SHADOW);
    return drop_shadow_color_;
  }

  const SkScalar* matrix() const {
    DCHECK_EQ(type_, COLOR_MATRIX);
    return matrix_;
  }

  int zoom_inset() const {
    DCHECK_EQ(type_, ZOOM);
    return zoom_inset_;
  }

  static FilterOperation CreateGrayscaleFilter(float amount) {
    return FilterOperation(GRAYSCALE, amount);
  }

  static FilterOperation CreateSepiaFilter(float amount) {
    return FilterOperation(SEPIA, amount);
  }

  static FilterOperation CreateSaturateFilter(float amount) {
    return FilterOperation(SATURATE, amount);
  }

  static FilterOperation CreateHueRotateFilter(float amount) {
    return FilterOperation(HUE_ROTATE, amount);
  }

  static FilterOperation CreateInvertFilter(float amount) {
    return FilterOperation(INVERT, amount);
  }

  static FilterOperation CreateBrightnessFilter(float amount) {
    return FilterOperation(BRIGHTNESS, amount);
  }

  static FilterOperation CreateContrastFilter(float amount) {
    return FilterOperation(CONTRAST, amount);
  }

  static FilterOperation CreateOpacityFilter(float amount) {
    return FilterOperation(OPACITY, amount);
  }

  static FilterOperation CreateBlurFilter(float amount) {
    return FilterOperation(BLUR, amount);
  }

  static FilterOperation CreateDropShadowFilter(gfx::Point offset,
                                                float std_deviation,
                                                SkColor color) {
    return FilterOperation(DROP_SHADOW, offset, std_deviation, color);
  }

  static FilterOperation CreateColorMatrixFilter(SkScalar matrix[20]) {
    return FilterOperation(COLOR_MATRIX, matrix);
  }

  static FilterOperation CreateZoomFilter(float amount, int inset) {
    return FilterOperation(ZOOM, amount, inset);
  }

  static FilterOperation CreateSaturatingBrightnessFilter(float amount) {
    return FilterOperation(SATURATING_BRIGHTNESS, amount);
  }

  bool operator==(const FilterOperation& other) const;

  bool operator!=(const FilterOperation& other) const {
    return !(*this == other);
  }

  // Methods for restoring a FilterOperation.
  static FilterOperation CreateEmptyFilter() {
    return FilterOperation(GRAYSCALE, 0.f);
  }

  void set_type(FilterType type) { type_ = type; }

  void set_amount(float amount) {
    DCHECK_NE(type_, COLOR_MATRIX);
    amount_ = amount;
  }

  void set_drop_shadow_offset(gfx::Point offset) {
    DCHECK_EQ(type_, DROP_SHADOW);
    drop_shadow_offset_ = offset;
  }

  void set_drop_shadow_color(SkColor color) {
    DCHECK_EQ(type_, DROP_SHADOW);
    drop_shadow_color_ = color;
  }

  void set_matrix(const SkScalar matrix[20]) {
    DCHECK_EQ(type_, COLOR_MATRIX);
    for (unsigned i = 0; i < 20; ++i)
      matrix_[i] = matrix[i];
  }

  void set_zoom_inset(int inset) {
    DCHECK_EQ(type_, ZOOM);
    zoom_inset_ = inset;
  }

 private:
  FilterOperation(FilterType type, float amount);

  FilterOperation(FilterType type,
                  gfx::Point offset,
                  float stdDeviation,
                  SkColor color);

  FilterOperation(FilterType, SkScalar matrix[20]);

  FilterOperation(FilterType type, float amount, int inset);

  FilterType type_;
  float amount_;
  gfx::Point drop_shadow_offset_;
  SkColor drop_shadow_color_;
  SkScalar matrix_[20];
  int zoom_inset_;
};

}  // namespace cc

#endif  // CC_OUTPUT_FILTER_OPERATION_H_
