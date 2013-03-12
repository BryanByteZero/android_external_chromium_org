// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/tree_synchronizer.h"

#include "base/debug/trace_event.h"
#include "base/logging.h"
#include "cc/layer.h"
#include "cc/layer_impl.h"
#include "cc/scrollbar_animation_controller.h"
#include "cc/scrollbar_layer.h"
#include "cc/scrollbar_layer_impl.h"

namespace cc {

typedef ScopedPtrHashMap<int, LayerImpl> ScopedPtrLayerImplMap;
typedef base::hash_map<int, LayerImpl*> RawPtrLayerImplMap;

void collectExistingLayerImplRecursive(ScopedPtrLayerImplMap& oldLayers, scoped_ptr<LayerImpl> layerImpl)
{
    if (!layerImpl)
        return;

    ScopedPtrVector<LayerImpl>& children = layerImpl->children();
    for (ScopedPtrVector<LayerImpl>::iterator it = children.begin(); it != children.end(); ++it)
        collectExistingLayerImplRecursive(oldLayers, children.take(it));

    collectExistingLayerImplRecursive(oldLayers, layerImpl->TakeMaskLayer());
    collectExistingLayerImplRecursive(oldLayers, layerImpl->TakeReplicaLayer());

    int id = layerImpl->id();
    oldLayers.set(id, layerImpl.Pass());
}

template <typename LayerType>
scoped_ptr<LayerImpl> synchronizeTreesInternal(LayerType* layerRoot, scoped_ptr<LayerImpl> oldLayerImplRoot, LayerTreeImpl* treeImpl)
{
    DCHECK(treeImpl);

    TRACE_EVENT0("cc", "TreeSynchronizer::synchronizeTrees");
    ScopedPtrLayerImplMap oldLayers;
    RawPtrLayerImplMap newLayers;

    collectExistingLayerImplRecursive(oldLayers, oldLayerImplRoot.Pass());

    scoped_ptr<LayerImpl> newTree = synchronizeTreesRecursive(newLayers, oldLayers, layerRoot, treeImpl);

    updateScrollbarLayerPointersRecursive(newLayers, layerRoot);

    return newTree.Pass();
}

scoped_ptr<LayerImpl> TreeSynchronizer::synchronizeTrees(Layer* layerRoot, scoped_ptr<LayerImpl> oldLayerImplRoot, LayerTreeImpl* treeImpl)
{
    return synchronizeTreesInternal(layerRoot, oldLayerImplRoot.Pass(), treeImpl);
}

scoped_ptr<LayerImpl> TreeSynchronizer::synchronizeTrees(LayerImpl* layerRoot, scoped_ptr<LayerImpl> oldLayerImplRoot, LayerTreeImpl* treeImpl)
{
    return synchronizeTreesInternal(layerRoot, oldLayerImplRoot.Pass(), treeImpl);
}

template <typename LayerType>
scoped_ptr<LayerImpl> reuseOrCreateLayerImpl(RawPtrLayerImplMap& newLayers, ScopedPtrLayerImplMap& oldLayers, LayerType* layer, LayerTreeImpl* treeImpl)
{
    scoped_ptr<LayerImpl> layerImpl = oldLayers.take(layer->id());

    if (!layerImpl)
        layerImpl = layer->CreateLayerImpl(treeImpl);

    newLayers[layer->id()] = layerImpl.get();
    return layerImpl.Pass();
}

template <typename LayerType>
scoped_ptr<LayerImpl> synchronizeTreesRecursiveInternal(RawPtrLayerImplMap& newLayers, ScopedPtrLayerImplMap& oldLayers, LayerType* layer, LayerTreeImpl* treeImpl)
{
    if (!layer)
        return scoped_ptr<LayerImpl>();

    scoped_ptr<LayerImpl> layerImpl = reuseOrCreateLayerImpl(newLayers, oldLayers, layer, treeImpl);

    layerImpl->ClearChildList();
    for (size_t i = 0; i < layer->children().size(); ++i)
        layerImpl->AddChild(synchronizeTreesRecursiveInternal(newLayers, oldLayers, layer->child_at(i), treeImpl));

    layerImpl->SetMaskLayer(synchronizeTreesRecursiveInternal(newLayers, oldLayers, layer->mask_layer(), treeImpl));
    layerImpl->SetReplicaLayer(synchronizeTreesRecursiveInternal(newLayers, oldLayers, layer->replica_layer(), treeImpl));

    // Remove all dangling pointers. The pointers will be setup later in updateScrollbarLayerPointersRecursive phase
    layerImpl->SetHorizontalScrollbarLayer(NULL);
    layerImpl->SetVerticalScrollbarLayer(NULL);

    return layerImpl.Pass();
}

scoped_ptr<LayerImpl> synchronizeTreesRecursive(RawPtrLayerImplMap& newLayers, ScopedPtrLayerImplMap& oldLayers, Layer* layer, LayerTreeImpl* treeImpl)
{
    return synchronizeTreesRecursiveInternal(newLayers, oldLayers, layer, treeImpl);
}

scoped_ptr<LayerImpl> synchronizeTreesRecursive(RawPtrLayerImplMap& newLayers, ScopedPtrLayerImplMap& oldLayers, LayerImpl* layer, LayerTreeImpl* treeImpl)
{
    return synchronizeTreesRecursiveInternal(newLayers, oldLayers, layer, treeImpl);
}

template <typename LayerType, typename ScrollbarLayerType>
void updateScrollbarLayerPointersRecursiveInternal(const RawPtrLayerImplMap& newLayers, LayerType* layer)
{
    if (!layer)
        return;

    for (size_t i = 0; i < layer->children().size(); ++i)
        updateScrollbarLayerPointersRecursiveInternal<LayerType, ScrollbarLayerType>(newLayers, layer->child_at(i));

    ScrollbarLayerType* scrollbarLayer = layer->ToScrollbarLayer();
    if (!scrollbarLayer)
        return;

    RawPtrLayerImplMap::const_iterator iter = newLayers.find(scrollbarLayer->id());
    ScrollbarLayerImpl* scrollbarLayerImpl = iter != newLayers.end() ? static_cast<ScrollbarLayerImpl*>(iter->second) : NULL;
    iter = newLayers.find(scrollbarLayer->scroll_layer_id());
    LayerImpl* scrollLayerImpl = iter != newLayers.end() ? iter->second : NULL;

    DCHECK(scrollbarLayerImpl);
    DCHECK(scrollLayerImpl);

    if (scrollbarLayer->Orientation() == WebKit::WebScrollbar::Horizontal)
        scrollLayerImpl->SetHorizontalScrollbarLayer(scrollbarLayerImpl);
    else
        scrollLayerImpl->SetVerticalScrollbarLayer(scrollbarLayerImpl);
}

void updateScrollbarLayerPointersRecursive(const RawPtrLayerImplMap& newLayers, Layer* layer)
{
    updateScrollbarLayerPointersRecursiveInternal<Layer, ScrollbarLayer>(newLayers, layer);
}

void updateScrollbarLayerPointersRecursive(const RawPtrLayerImplMap& newLayers, LayerImpl* layer)
{
    updateScrollbarLayerPointersRecursiveInternal<LayerImpl, ScrollbarLayerImpl>(newLayers, layer);
}

template <typename LayerType>
void pushPropertiesInternal(LayerType* layer, LayerImpl* layerImpl)
{
    if (!layer) {
        DCHECK(!layerImpl);
        return;
    }

    DCHECK_EQ(layer->id(), layerImpl->id());
    layer->PushPropertiesTo(layerImpl);

    pushPropertiesInternal(layer->mask_layer(), layerImpl->mask_layer());
    pushPropertiesInternal(layer->replica_layer(), layerImpl->replica_layer());

    const ScopedPtrVector<LayerImpl>& implChildren = layerImpl->children();
    DCHECK_EQ(layer->children().size(), implChildren.size());

    for (size_t i = 0; i < layer->children().size(); ++i) {
        pushPropertiesInternal(layer->child_at(i), implChildren[i]);
    }
}

void TreeSynchronizer::pushProperties(Layer* layer, LayerImpl* layerImpl)
{
    pushPropertiesInternal(layer, layerImpl);
}

void TreeSynchronizer::pushProperties(LayerImpl* layer, LayerImpl* layerImpl)
{
    pushPropertiesInternal(layer, layerImpl);
}


}  // namespace cc
