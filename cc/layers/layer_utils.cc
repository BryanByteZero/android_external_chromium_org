// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/layers/layer_utils.h"

#include "cc/layers/layer_impl.h"
#include "cc/trees/layer_tree_host_common.h"
#include "ui/gfx/box_f.h"

namespace cc {

namespace {

bool HasAnimationThatInflatesBounds(const LayerImpl& layer) {
  return layer.layer_animation_controller()->HasAnimationThatInflatesBounds();
}

bool HasFilterAnimationThatInflatesBounds(const LayerImpl& layer) {
  return layer.layer_animation_controller()
      ->HasFilterAnimationThatInflatesBounds();
}

bool HasTransformAnimationThatInflatesBounds(const LayerImpl& layer) {
  return layer.layer_animation_controller()
      ->HasTransformAnimationThatInflatesBounds();
}

inline bool HasAncestorTransformAnimation(const LayerImpl& layer) {
  return layer.screen_space_transform_is_animating();
}

inline bool HasAncestorFilterAnimation(const LayerImpl& layer) {
  for (const LayerImpl* current = &layer; current;
       current = current->parent()) {
    if (HasFilterAnimationThatInflatesBounds(*current))
        return true;
  }

  return false;
}

}  // namespace

bool LayerUtils::GetAnimationBounds(const LayerImpl& layer_in, gfx::BoxF* out) {
  // We don't care about animated bounds for invisible layers.
  if (!layer_in.DrawsContent())
    return false;

  // We also don't care for layers that are not animated or a child of an
  // animated layer.
  if (!HasAncestorTransformAnimation(layer_in) &&
      !HasAncestorFilterAnimation(layer_in))
    return false;

  // To compute the inflated bounds for a layer, we start by taking its bounds
  // and converting it to a 3d box, and then we transform or inflate it
  // repeatedly as we walk up the layer tree to the root.
  //
  // At each layer we apply the following transformations to the box:
  //   1) We translate so that the anchor point is the origin.
  //   2) We either apply the layer's transform or inflate if the layer's
  //      transform is animated.
  //   3) We undo the translation from step 1 and apply a second translation
  //      to account for the layer's position.
  //   4) We apply the sublayer transform from our parent (about the parent's
  //      anchor point).
  //
  gfx::BoxF box(layer_in.bounds().width(), layer_in.bounds().height(), 0.f);

  // We want to inflate/transform the box as few times as possible. Each time
  // we do this, we have to make the box axis aligned again, so if we make many
  // small adjustments to the box by transforming it repeatedly rather than
  // once by the product of all these matrices, we will accumulate a bunch of
  // unnecessary inflation because of the the many axis-alignment fixes. This
  // matrix stores said product.
  gfx::Transform coalesced_transform;

  for (const LayerImpl* layer = &layer_in; layer; layer = layer->parent()) {
    int anchor_x = layer->anchor_point().x() * layer->bounds().width();
    int anchor_y = layer->anchor_point().y() * layer->bounds().height();
    gfx::PointF position = layer->position();
    if (layer->parent() && !HasAnimationThatInflatesBounds(*layer)) {
      // |composite_layer_transform| contains 1 - 4 mentioned above. We compute
      // it separately and apply afterwards because it's a bit more efficient
      // because post-multiplication appears a bit more expensive, so we want
      // to do it only once.
      gfx::Transform composite_layer_transform;

      if (!layer->parent()->sublayer_transform().IsIdentity()) {
        LayerTreeHostCommon::ApplySublayerTransformAboutAnchor(
            *layer->parent(),
            layer->parent()->bounds(),
            &composite_layer_transform);
      }

      composite_layer_transform.Translate3d(anchor_x + position.x(),
                                            anchor_y + position.y(),
                                            layer->anchor_point_z());
      composite_layer_transform.PreconcatTransform(layer->transform());
      composite_layer_transform.Translate3d(
          -anchor_x, -anchor_y, -layer->anchor_point_z());

      // Add this layer's contributions to the |coalesced_transform|.
      coalesced_transform.ConcatTransform(composite_layer_transform);
      continue;
    }

    // First, apply coalesced transform we've been building and reset it.
    coalesced_transform.TransformBox(&box);
    coalesced_transform.MakeIdentity();

    // We need to apply the inflation about the layer's anchor point. Rather
    // than doing this via transforms, we'll just shift the box directly.
    box.set_origin(box.origin() + gfx::Vector3dF(-anchor_x,
                                                 -anchor_y,
                                                 -layer->anchor_point_z()));

    // Perform the inflation
    if (HasFilterAnimationThatInflatesBounds(*layer)) {
      gfx::BoxF inflated;
      if (!layer->layer_animation_controller()->FilterAnimationBoundsForBox(
              box, &inflated))
        return false;
      box = inflated;
    }

    if (HasTransformAnimationThatInflatesBounds(*layer)) {
      gfx::BoxF inflated;
      if (!layer->layer_animation_controller()->TransformAnimationBoundsForBox(
              box, &inflated))
        return false;
      box = inflated;
    }

    // Apply step 3) mentioned above.
    box.set_origin(box.origin() + gfx::Vector3dF(anchor_x + position.x(),
                                                 anchor_y + position.y(),
                                                 layer->anchor_point_z()));

    // Even for layers with animations, we have to tack in the sublayer
    // transform of our parent. *Every* layer is repsonsible for including the
    // sublayer transform of its parent (see step 4 above).
    if (layer->parent()) {
      LayerTreeHostCommon::ApplySublayerTransformAboutAnchor(
          *layer->parent(), layer->parent()->bounds(), &coalesced_transform);
    }
  }

  // If we've got an unapplied coalesced transform at this point, it must still
  // be applied.
  if (!coalesced_transform.IsIdentity())
    coalesced_transform.TransformBox(&box);

  *out = box;

  return true;
}

}  // namespace cc
