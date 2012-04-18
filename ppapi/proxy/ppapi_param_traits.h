// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_PPAPI_PARAM_TRAITS_H_
#define PPAPI_PROXY_PPAPI_PARAM_TRAITS_H_

#include <string>
#include <vector>

#include "ipc/ipc_message_utils.h"
#include "ipc/ipc_platform_file.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_rect.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/proxy/ppapi_proxy_export.h"
#include "ppapi/shared_impl/ppb_file_ref_shared.h"

struct PP_FileInfo;
struct PP_ObjectProperty;
struct PP_NetAddress_Private;

namespace ppapi {

class HostResource;
class PPB_X509Certificate_Fields;

namespace proxy {

struct PPBFlash_DrawGlyphs_Params;
struct PPBURLLoader_UpdateProgress_Params;
struct SerializedDirEntry;
struct SerializedFontDescription;
class SerializedFlashMenu;
class SerializedVar;

}  // namespace proxy
}  // namespace ppapi

namespace IPC {

template<>
struct PPAPI_PROXY_EXPORT ParamTraits<PP_Bool> {
  typedef PP_Bool param_type;
  static void Write(Message* m, const param_type& p);
  static bool Read(const Message* m, PickleIterator* iter, param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template<>
struct PPAPI_PROXY_EXPORT ParamTraits<PP_FileInfo> {
  typedef PP_FileInfo param_type;
  static void Write(Message* m, const param_type& p);
  static bool Read(const Message* m, PickleIterator* iter, param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template <>
struct PPAPI_PROXY_EXPORT ParamTraits<PP_NetAddress_Private> {
  typedef PP_NetAddress_Private param_type;
  static void Write(Message* m, const param_type& p);
  static bool Read(const Message* m, PickleIterator* iter, param_type* p);
  static void Log(const param_type& p, std::string* l);
};

template<>
struct PPAPI_PROXY_EXPORT ParamTraits<PP_ObjectProperty> {
  typedef PP_ObjectProperty param_type;
  static void Write(Message* m, const param_type& p);
  static bool Read(const Message* m, PickleIterator* iter, param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template<>
struct PPAPI_PROXY_EXPORT ParamTraits<
    ppapi::proxy::PPBFlash_DrawGlyphs_Params> {
  typedef ppapi::proxy::PPBFlash_DrawGlyphs_Params param_type;
  static void Write(Message* m, const param_type& p);
  static bool Read(const Message* m, PickleIterator* iter, param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template<>
struct PPAPI_PROXY_EXPORT ParamTraits<ppapi::PPB_FileRef_CreateInfo> {
  typedef ppapi::PPB_FileRef_CreateInfo param_type;
  static void Write(Message* m, const param_type& p);
  static bool Read(const Message* m, PickleIterator* iter, param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template<>
struct PPAPI_PROXY_EXPORT ParamTraits<
    ppapi::proxy::PPBURLLoader_UpdateProgress_Params> {
  typedef ppapi::proxy::PPBURLLoader_UpdateProgress_Params param_type;
  static void Write(Message* m, const param_type& p);
  static bool Read(const Message* m, PickleIterator* iter, param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template<>
struct PPAPI_PROXY_EXPORT ParamTraits<ppapi::proxy::SerializedDirEntry> {
  typedef ppapi::proxy::SerializedDirEntry param_type;
  static void Write(Message* m, const param_type& p);
  static bool Read(const Message* m, PickleIterator* iter, param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template<>
struct PPAPI_PROXY_EXPORT ParamTraits<ppapi::proxy::SerializedFontDescription> {
  typedef ppapi::proxy::SerializedFontDescription param_type;
  static void Write(Message* m, const param_type& p);
  static bool Read(const Message* m, PickleIterator* iter, param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template<>
struct PPAPI_PROXY_EXPORT ParamTraits<ppapi::HostResource> {
  typedef ppapi::HostResource param_type;
  static void Write(Message* m, const param_type& p);
  static bool Read(const Message* m, PickleIterator* iter, param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template<>
struct PPAPI_PROXY_EXPORT ParamTraits<ppapi::proxy::SerializedVar> {
  typedef ppapi::proxy::SerializedVar param_type;
  static void Write(Message* m, const param_type& p);
  static bool Read(const Message* m, PickleIterator* iter, param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template<>
struct PPAPI_PROXY_EXPORT ParamTraits<
    std::vector<ppapi::proxy::SerializedVar> > {
  typedef std::vector<ppapi::proxy::SerializedVar> param_type;
  static void Write(Message* m, const param_type& p);
  static bool Read(const Message* m, PickleIterator* iter, param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template<>
struct PPAPI_PROXY_EXPORT ParamTraits< std::vector<
    ppapi::PPB_FileRef_CreateInfo> > {
  typedef std::vector<ppapi::PPB_FileRef_CreateInfo> param_type;
  static void Write(Message* m, const param_type& p);
  static bool Read(const Message* m, PickleIterator* iter, param_type* r);
  static void Log(const param_type& p, std::string* l);
};

#if !defined(OS_NACL)
template<>
struct PPAPI_PROXY_EXPORT ParamTraits<ppapi::proxy::SerializedFlashMenu> {
  typedef ppapi::proxy::SerializedFlashMenu param_type;
  static void Write(Message* m, const param_type& p);
  static bool Read(const Message* m, PickleIterator* iter, param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template<>
struct PPAPI_PROXY_EXPORT ParamTraits<ppapi::PPB_X509Certificate_Fields> {
  typedef ppapi::PPB_X509Certificate_Fields param_type;
  static void Write(Message* m, const param_type& p);
  static bool Read(const Message* m, PickleIterator* iter, param_type* r);
  static void Log(const param_type& p, std::string* l);
};
#endif  // !defined(OS_NACL)

}  // namespace IPC

#endif  // PPAPI_PROXY_PPAPI_PARAM_TRAITS_H_
