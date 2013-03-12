// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/plugin_var_tracker.h"

#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "ppapi/c/dev/ppp_class_deprecated.h"
#include "ppapi/c/ppb_var.h"
#include "ppapi/proxy/plugin_array_buffer_var.h"
#include "ppapi/proxy/plugin_dispatcher.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/proxy_object_var.h"
#include "ppapi/shared_impl/api_id.h"
#include "ppapi/shared_impl/var.h"

namespace ppapi {
namespace proxy {

PluginVarTracker::HostVar::HostVar(PluginDispatcher* d, int32 i)
    : dispatcher(d),
      host_object_id(i) {
}

bool PluginVarTracker::HostVar::operator<(const HostVar& other) const {
  if (dispatcher < other.dispatcher)
    return true;
  if (other.dispatcher < dispatcher)
    return false;
  return host_object_id < other.host_object_id;
}

PluginVarTracker::PluginVarTracker() : VarTracker(THREAD_SAFE) {
}

PluginVarTracker::~PluginVarTracker() {
}

PP_Var PluginVarTracker::ReceiveObjectPassRef(const PP_Var& host_var,
                                              PluginDispatcher* dispatcher) {
  CheckThreadingPreconditions();
  DCHECK(host_var.type == PP_VARTYPE_OBJECT);

  // Get the object.
  scoped_refptr<ProxyObjectVar> object(
      FindOrMakePluginVarFromHostVar(host_var, dispatcher));

  // Actually create the PP_Var, this will add all the tracking info but not
  // adjust any refcounts.
  PP_Var ret = GetOrCreateObjectVarID(object.get());

  VarInfo& info = GetLiveVar(ret)->second;
  if (info.ref_count > 0) {
    // We already had a reference to it before. That means the renderer now has
    // two references on our behalf. We want to transfer that extra reference
    // to our list. This means we addref in the plugin, and release the extra
    // one in the renderer.
    SendReleaseObjectMsg(*object);
  }
  info.ref_count++;
  return ret;
}

PP_Var PluginVarTracker::TrackObjectWithNoReference(
    const PP_Var& host_var,
    PluginDispatcher* dispatcher) {
  CheckThreadingPreconditions();
  DCHECK(host_var.type == PP_VARTYPE_OBJECT);

  // Get the object.
  scoped_refptr<ProxyObjectVar> object(
      FindOrMakePluginVarFromHostVar(host_var, dispatcher));

  // Actually create the PP_Var, this will add all the tracking info but not
  // adjust any refcounts.
  PP_Var ret = GetOrCreateObjectVarID(object.get());

  VarInfo& info = GetLiveVar(ret)->second;
  info.track_with_no_reference_count++;
  return ret;
}

void PluginVarTracker::StopTrackingObjectWithNoReference(
    const PP_Var& plugin_var) {
  CheckThreadingPreconditions();
  DCHECK(plugin_var.type == PP_VARTYPE_OBJECT);

  VarMap::iterator found = GetLiveVar(plugin_var);
  if (found == live_vars_.end()) {
    NOTREACHED();
    return;
  }

  DCHECK(found->second.track_with_no_reference_count > 0);
  found->second.track_with_no_reference_count--;
  DeleteObjectInfoIfNecessary(found);
}

PP_Var PluginVarTracker::GetHostObject(const PP_Var& plugin_object) const {
  CheckThreadingPreconditions();
  if (plugin_object.type != PP_VARTYPE_OBJECT) {
    NOTREACHED();
    return PP_MakeUndefined();
  }

  Var* var = GetVar(plugin_object);
  ProxyObjectVar* object = var->AsProxyObjectVar();
  if (!object) {
    NOTREACHED();
    return PP_MakeUndefined();
  }

  // Make a var with the host ID.
  PP_Var ret = { PP_VARTYPE_OBJECT };
  ret.value.as_id = object->host_var_id();
  return ret;
}

PluginDispatcher* PluginVarTracker::DispatcherForPluginObject(
    const PP_Var& plugin_object) const {
  CheckThreadingPreconditions();
  if (plugin_object.type != PP_VARTYPE_OBJECT)
    return NULL;

  VarMap::const_iterator found = GetLiveVar(plugin_object);
  if (found == live_vars_.end())
    return NULL;

  ProxyObjectVar* object = found->second.var->AsProxyObjectVar();
  if (!object)
    return NULL;
  return object->dispatcher();
}

void PluginVarTracker::ReleaseHostObject(PluginDispatcher* dispatcher,
                                         const PP_Var& host_object) {
  CheckThreadingPreconditions();
  DCHECK(host_object.type == PP_VARTYPE_OBJECT);

  // Convert the host object to a normal var valid in the plugin.
  HostVarToPluginVarMap::iterator found = host_var_to_plugin_var_.find(
      HostVar(dispatcher, static_cast<int32>(host_object.value.as_id)));
  if (found == host_var_to_plugin_var_.end()) {
    NOTREACHED();
    return;
  }

  // Now just release the object given the plugin var ID.
  ReleaseVar(found->second);
}

void PluginVarTracker::DidDeleteInstance(PP_Instance instance) {
  // Calling the destructors on plugin objects may in turn release other
  // objects which will mutate the map out from under us. So do a two-step
  // process of identifying the ones to delete, and then delete them.
  //
  // See the comment above user_data_to_plugin_ in the header file. We assume
  // there aren't that many objects so a brute-force search is reasonable.
  std::vector<void*> user_data_to_delete;
  for (UserDataToPluginImplementedVarMap::const_iterator i =
           user_data_to_plugin_.begin();
       i != user_data_to_plugin_.end();
       ++i) {
    if (i->second.instance == instance)
      user_data_to_delete.push_back(i->first);
  }

  for (size_t i = 0; i < user_data_to_delete.size(); i++) {
    UserDataToPluginImplementedVarMap::iterator found =
        user_data_to_plugin_.find(user_data_to_delete[i]);
    if (found == user_data_to_plugin_.end())
      continue;  // Object removed from list while we were iterating.

    if (!found->second.plugin_object_id) {
      // This object is for the freed instance and the plugin is not holding
      // any references to it. Deallocate immediately.
      found->second.ppp_class->Deallocate(found->first);
      user_data_to_plugin_.erase(found);
    } else {
      // The plugin is holding refs to this object. We don't want to call
      // Deallocate since the plugin may be depending on those refs to keep
      // its data alive. To avoid crashes in this case, just clear out the
      // instance to mark it and continue. When the plugin refs go to 0,
      // we'll notice there is no instance and call Deallocate.
      found->second.instance = 0;
    }
  }
}

ArrayBufferVar* PluginVarTracker::CreateArrayBuffer(uint32 size_in_bytes) {
  return new PluginArrayBufferVar(size_in_bytes);
}

void PluginVarTracker::PluginImplementedObjectCreated(
    PP_Instance instance,
    const PP_Var& created_var,
    const PPP_Class_Deprecated* ppp_class,
    void* ppp_class_data) {
  PluginImplementedVar p;
  p.ppp_class = ppp_class;
  p.instance = instance;
  p.plugin_object_id = created_var.value.as_id;
  user_data_to_plugin_[ppp_class_data] = p;

  // Link the user data to the object.
  ProxyObjectVar* object = GetVar(created_var)->AsProxyObjectVar();
  object->set_user_data(ppp_class_data);
}

void PluginVarTracker::PluginImplementedObjectDestroyed(void* user_data) {
  UserDataToPluginImplementedVarMap::iterator found =
      user_data_to_plugin_.find(user_data);
  if (found == user_data_to_plugin_.end()) {
    NOTREACHED();
    return;
  }
  user_data_to_plugin_.erase(found);
}

bool PluginVarTracker::IsPluginImplementedObjectAlive(void* user_data) {
  return user_data_to_plugin_.find(user_data) != user_data_to_plugin_.end();
}

bool PluginVarTracker::ValidatePluginObjectCall(
    const PPP_Class_Deprecated* ppp_class,
    void* user_data) {
  UserDataToPluginImplementedVarMap::iterator found =
      user_data_to_plugin_.find(user_data);
  if (found == user_data_to_plugin_.end())
    return false;
  return found->second.ppp_class == ppp_class;
}

int32 PluginVarTracker::AddVarInternal(Var* var, AddVarRefMode mode) {
  // Normal adding.
  int32 new_id = VarTracker::AddVarInternal(var, mode);

  // Need to add proxy objects to the host var map.
  ProxyObjectVar* proxy_object = var->AsProxyObjectVar();
  if (proxy_object) {
    HostVar host_var(proxy_object->dispatcher(), proxy_object->host_var_id());
    DCHECK(host_var_to_plugin_var_.find(host_var) ==
           host_var_to_plugin_var_.end());  // Adding an object twice, use
                                            // FindOrMakePluginVarFromHostVar.
    host_var_to_plugin_var_[host_var] = new_id;
  }
  return new_id;
}

void PluginVarTracker::TrackedObjectGettingOneRef(VarMap::const_iterator iter) {
  ProxyObjectVar* object = iter->second.var->AsProxyObjectVar();
  if (!object) {
    NOTREACHED();
    return;
  }

  DCHECK(iter->second.ref_count == 0);

  // Got an AddRef for an object we have no existing reference for.
  // We need to tell the browser we've taken a ref. This comes up when the
  // browser passes an object as an input param and holds a ref for us.
  // This must be a sync message since otherwise the "addref" will actually
  // occur after the return to the browser of the sync function that
  // presumably sent the object.
  SendAddRefObjectMsg(*object);
}

void PluginVarTracker::ObjectGettingZeroRef(VarMap::iterator iter) {
  ProxyObjectVar* object = iter->second.var->AsProxyObjectVar();
  if (!object) {
    NOTREACHED();
    return;
  }

  // Notify the host we're no longer holding our ref.
  DCHECK(iter->second.ref_count == 0);
  SendReleaseObjectMsg(*object);

  UserDataToPluginImplementedVarMap::iterator found =
      user_data_to_plugin_.find(object->user_data());
  if (found != user_data_to_plugin_.end()) {
    // This object is implemented in the plugin.
    if (found->second.instance == 0) {
      // Instance is destroyed. This means that we'll never get a Deallocate
      // call from the renderer and we should do so now.
      found->second.ppp_class->Deallocate(found->first);
      user_data_to_plugin_.erase(found);
    } else {
      // The plugin is releasing its last reference to an object it implements.
      // Clear the tracking data that links our "plugin implemented object" to
      // the var. If the instance is destroyed and there is no ID, we know that
      // we should just call Deallocate on the object data.
      //
      // See the plugin_object_id declaration for more info.
      found->second.plugin_object_id = 0;
    }
  }

  // This will optionally delete the info from live_vars_.
  VarTracker::ObjectGettingZeroRef(iter);
}

bool PluginVarTracker::DeleteObjectInfoIfNecessary(VarMap::iterator iter) {
  // Get the info before calling the base class's version of this function,
  // which may delete the object.
  ProxyObjectVar* object = iter->second.var->AsProxyObjectVar();
  HostVar host_var(object->dispatcher(), object->host_var_id());

  if (!VarTracker::DeleteObjectInfoIfNecessary(iter))
    return false;

  // Clean up the host var mapping.
  DCHECK(host_var_to_plugin_var_.find(host_var) !=
         host_var_to_plugin_var_.end());
  host_var_to_plugin_var_.erase(host_var);
  return true;
}

PP_Var PluginVarTracker::GetOrCreateObjectVarID(ProxyObjectVar* object) {
  // We can't use object->GetPPVar() because we don't want to affect the
  // refcount, so we have to add everything manually here.
  int32 var_id = object->GetExistingVarID();
  if (!var_id) {
    var_id = AddVarInternal(object, ADD_VAR_CREATE_WITH_NO_REFERENCE);
    object->AssignVarID(var_id);
  }

  PP_Var ret = { PP_VARTYPE_OBJECT };
  ret.value.as_id = var_id;
  return ret;
}

void PluginVarTracker::SendAddRefObjectMsg(
    const ProxyObjectVar& proxy_object) {
  int unused;
  proxy_object.dispatcher()->Send(new PpapiHostMsg_PPBVar_AddRefObject(
      API_ID_PPB_VAR_DEPRECATED, proxy_object.host_var_id(), &unused));
}

void PluginVarTracker::SendReleaseObjectMsg(
    const ProxyObjectVar& proxy_object) {
  proxy_object.dispatcher()->Send(new PpapiHostMsg_PPBVar_ReleaseObject(
      API_ID_PPB_VAR_DEPRECATED, proxy_object.host_var_id()));
}

scoped_refptr<ProxyObjectVar> PluginVarTracker::FindOrMakePluginVarFromHostVar(
    const PP_Var& var,
    PluginDispatcher* dispatcher) {
  DCHECK(var.type == PP_VARTYPE_OBJECT);
  HostVar host_var(dispatcher, var.value.as_id);

  HostVarToPluginVarMap::iterator found =
      host_var_to_plugin_var_.find(host_var);
  if (found == host_var_to_plugin_var_.end()) {
    // Create a new object.
    return scoped_refptr<ProxyObjectVar>(
        new ProxyObjectVar(dispatcher, static_cast<int32>(var.value.as_id)));
  }

  // Have this host var, look up the object.
  VarMap::iterator ret = live_vars_.find(found->second);
  DCHECK(ret != live_vars_.end());

  // All objects should be proxy objects.
  DCHECK(ret->second.var->AsProxyObjectVar());
  return scoped_refptr<ProxyObjectVar>(ret->second.var->AsProxyObjectVar());
}

}  // namesace proxy
}  // namespace ppapi
