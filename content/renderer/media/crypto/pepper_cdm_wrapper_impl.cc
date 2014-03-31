// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(ENABLE_PEPPER_CDMS)
#include "content/renderer/media/crypto/pepper_cdm_wrapper_impl.h"

#include "content/renderer/pepper/pepper_plugin_instance_impl.h"
#include "content/renderer/pepper/pepper_webplugin_impl.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebHelperPlugin.h"
#include "third_party/WebKit/public/web/WebPlugin.h"
#include "third_party/WebKit/public/web/WebView.h"

namespace content {

void WebHelperPluginDeleter::operator()(blink::WebHelperPlugin* plugin) const {
  plugin->destroy();
}

scoped_ptr<PepperCdmWrapper> PepperCdmWrapperImpl::Create(
    blink::WebFrame* frame,
    const std::string& pluginType) {
  DCHECK(frame);
  ScopedHelperPlugin helper_plugin(blink::WebHelperPlugin::create(
      blink::WebString::fromUTF8(pluginType), frame));
  if (!helper_plugin)
    return scoped_ptr<PepperCdmWrapper>();

  blink::WebPlugin* plugin = helper_plugin->getPlugin();
  DCHECK(!plugin->isPlaceholder());  // Prevented by Blink.

  // Only Pepper plugins are supported, so it must ultimately be a ppapi object.
  PepperWebPluginImpl* ppapi_plugin = static_cast<PepperWebPluginImpl*>(plugin);
  scoped_refptr<PepperPluginInstanceImpl> plugin_instance =
      ppapi_plugin->instance();
  if (!plugin_instance)
    return scoped_ptr<PepperCdmWrapper>();

  if (!plugin_instance->GetContentDecryptorDelegate())
    return scoped_ptr<PepperCdmWrapper>();

  return scoped_ptr<PepperCdmWrapper>(
      new PepperCdmWrapperImpl(helper_plugin.Pass(), plugin_instance));
}

PepperCdmWrapperImpl::PepperCdmWrapperImpl(
    ScopedHelperPlugin helper_plugin,
    const scoped_refptr<PepperPluginInstanceImpl>& plugin_instance)
    : helper_plugin_(helper_plugin.Pass()),
      plugin_instance_(plugin_instance) {
  DCHECK(helper_plugin_);
  DCHECK(plugin_instance_);
  // Plugin must be a CDM.
  DCHECK(plugin_instance_->GetContentDecryptorDelegate());
}

PepperCdmWrapperImpl::~PepperCdmWrapperImpl() {
  // Destroy the nested objects in reverse order.
  plugin_instance_ = NULL;
  helper_plugin_.reset();
}

ContentDecryptorDelegate* PepperCdmWrapperImpl::GetCdmDelegate() {
  return plugin_instance_->GetContentDecryptorDelegate();
}

}  // namespace content

#endif  // defined(ENABLE_PEPPER_CDMS)
