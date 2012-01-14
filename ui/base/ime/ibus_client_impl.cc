// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ime/ibus_client_impl.h"

#include <ibus.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#undef FocusIn
#undef FocusOut

#include "base/basictypes.h"
#include "base/i18n/char_iterator.h"
#include "base/logging.h"
#include "base/utf_string_conversions.h"
#include "ui/base/ime/composition_text.h"

// input_method_ibus.cc assumes X and IBus use the same mask for Lock, Control,
// Shift, Alt, and buttons. Check if the assumption is really correct.
COMPILE_ASSERT(IBUS_LOCK_MASK == LockMask, test_mask);
COMPILE_ASSERT(IBUS_CONTROL_MASK == ControlMask, test_mask);
COMPILE_ASSERT(IBUS_SHIFT_MASK == ShiftMask, test_mask);
COMPILE_ASSERT(IBUS_MOD1_MASK == Mod1Mask, test_mask);
COMPILE_ASSERT(IBUS_BUTTON1_MASK == Button1Mask, test_mask);
COMPILE_ASSERT(IBUS_BUTTON2_MASK == Button2Mask, test_mask);
COMPILE_ASSERT(IBUS_BUTTON3_MASK == Button3Mask, test_mask);
COMPILE_ASSERT(IBUS_RELEASE_MASK == ui::kIBusReleaseMask, test_mask);

namespace ui {
namespace internal {

namespace {

const char kClientName[] = "chrome";

XKeyEvent* GetKeyEvent(XEvent* event) {
  DCHECK(event && (event->type == KeyPress || event->type == KeyRelease));
  return &event->xkey;
}

void ProcessKeyEventDone(IBusInputContext* context,
                         GAsyncResult* res,
                         IBusClient::PendingKeyEvent* data) {
  DCHECK(context);
  DCHECK(res);
  DCHECK(data);
  const gboolean handled = ibus_input_context_process_key_event_async_finish(
      context, res, NULL);
  data->ProcessPostIME(handled);
  delete data;
}

void CreateInputContextDone(IBusBus* bus,
                            GAsyncResult* res,
                            IBusClient::PendingCreateICRequest* data) {
  DCHECK(bus);
  DCHECK(res);
  DCHECK(data);
  IBusInputContext* context =
      ibus_bus_create_input_context_async_finish(bus, res, NULL);
  if (context)
    data->StoreOrAbandonInputContext(context);
  delete data;
}

}  // namespace

IBusClientImpl::IBusClientImpl() {
}

IBusClientImpl::~IBusClientImpl() {
}

IBusBus* IBusClientImpl::GetConnection() {
  ibus_init();
  return ibus_bus_new();
}

bool IBusClientImpl::IsConnected(IBusBus* bus) {
  return ibus_bus_is_connected(bus) == TRUE;
}

void IBusClientImpl::CreateContext(IBusBus* bus,
                                   PendingCreateICRequest* request) {
  ibus_bus_create_input_context_async(
      bus,
      kClientName,
      -1,  // no timeout
      NULL,  // no cancellation object
      reinterpret_cast<GAsyncReadyCallback>(CreateInputContextDone),
      request);
}

void IBusClientImpl::DestroyProxy(IBusInputContext* context) {
  // ibus_proxy_destroy() will not really release the object, caller still need
  // to call g_object_unref() explicitly.
  ibus_proxy_destroy(IBUS_PROXY(context));
}

void IBusClientImpl::SetCapabilities(IBusInputContext* context,
                                     InlineCompositionCapability inline_type) {
  // TODO(penghuang): support surrounding text.
  guint32 capabilities =
      inline_type == INLINE_COMPOSITION ? IBUS_CAP_PREEDIT_TEXT | IBUS_CAP_FOCUS
                                        : IBUS_CAP_FOCUS;
  ibus_input_context_set_capabilities(context, capabilities);
}

void IBusClientImpl::FocusIn(IBusInputContext* context) {
  ibus_input_context_focus_in(context);
}

void IBusClientImpl::FocusOut(IBusInputContext* context) {
  ibus_input_context_focus_out(context);
}

void IBusClientImpl::Reset(IBusInputContext* context) {
  ibus_input_context_reset(context);
}

void IBusClientImpl::SetCursorLocation(IBusInputContext* context,
                                       int32 x,
                                       int32 y,
                                       int32 w,
                                       int32 h) {
  ibus_input_context_set_cursor_location(context, x, y, w, h);
}

void IBusClientImpl::SendKeyEvent(IBusInputContext* context,
                                  uint32 keyval,
                                  uint32 keycode,
                                  uint32 state,
                                  PendingKeyEvent* pending_key) {
  // Note:
  // 1. We currently set timeout to -1, because ibus doesn't have a mechanism to
  // associate input method results to corresponding key event, thus there is
  // actually no way to abandon results generated by a specific key event. So we
  // actually cannot abandon a specific key event and its result but accept
  // following key events and their results. So a timeout to abandon a key event
  // will not work.
  // 2. We set GCancellable to NULL, because the operation of cancelling a async
  // request also happens asynchronously, thus it's actually useless to us.
  //
  // The fundemental problem of ibus' async API is: it uses Glib's GIO API to
  // realize async communication, but in fact, GIO API is specially designed for
  // concurrent tasks, though it supports async communication as well, the API
  // is much more complicated than an ordinary message based async communication
  // API (such as Chrome's IPC).
  // Thus it's very complicated, if not impossible, to implement a client that
  // fully utilize asynchronous communication without potential problem.
  ibus_input_context_process_key_event_async(
      context,
      keyval,
      keycode,
      state,
      -1,  // no timeout
      NULL,  // no cancellation object
      reinterpret_cast<GAsyncReadyCallback>(ProcessKeyEventDone),
      pending_key);
}

// TODO(yusukes): Write a unit test for this function once build bots start
// supporting ibus-1.4.
void IBusClientImpl::ExtractCompositionText(IBusText* text,
                                            guint cursor_position,
                                            CompositionText* out_composition) {
  out_composition->Clear();
  out_composition->text = UTF8ToUTF16(text->text);

  if (out_composition->text.empty())
    return;

  // ibus uses character index for cursor position and attribute range, but we
  // use char16 offset for them. So we need to do conversion here.
  std::vector<size_t> char16_offsets;
  size_t length = out_composition->text.length();
  base::i18n::UTF16CharIterator char_iterator(&out_composition->text);
  do {
    char16_offsets.push_back(char_iterator.array_pos());
  } while (char_iterator.Advance());

  // The text length in Unicode characters.
  guint char_length = static_cast<guint>(char16_offsets.size());
  // Make sure we can convert the value of |char_length| as well.
  char16_offsets.push_back(length);

  size_t cursor_offset =
      char16_offsets[std::min(char_length, cursor_position)];

  out_composition->selection = Range(cursor_offset);
  if (text->attrs) {
    guint i = 0;
    while (true) {
      IBusAttribute* attr = ibus_attr_list_get(text->attrs, i++);
      if (!attr)
        break;
      if (attr->type != IBUS_ATTR_TYPE_UNDERLINE &&
          attr->type != IBUS_ATTR_TYPE_BACKGROUND) {
        continue;
      }
      guint start = std::min(char_length, attr->start_index);
      guint end = std::min(char_length, attr->end_index);
      if (start >= end)
        continue;
      CompositionUnderline underline(
          char16_offsets[start], char16_offsets[end],
          SK_ColorBLACK, false /* thick */);
      if (attr->type == IBUS_ATTR_TYPE_BACKGROUND) {
        underline.thick = true;
        // If the cursor is at start or end of this underline, then we treat
        // it as the selection range as well, but make sure to set the cursor
        // position to the selection end.
        if (underline.start_offset == cursor_offset) {
          out_composition->selection.set_start(underline.end_offset);
          out_composition->selection.set_end(cursor_offset);
        } else if (underline.end_offset == cursor_offset) {
          out_composition->selection.set_start(underline.start_offset);
          out_composition->selection.set_end(cursor_offset);
        }
      } else if (attr->type == IBUS_ATTR_TYPE_UNDERLINE) {
        if (attr->value == IBUS_ATTR_UNDERLINE_DOUBLE)
          underline.thick = true;
        else if (attr->value == IBUS_ATTR_UNDERLINE_ERROR)
          underline.color = SK_ColorRED;
      }
      out_composition->underlines.push_back(underline);
    }
  }

  // Use a black thin underline by default.
  if (out_composition->underlines.empty()) {
    out_composition->underlines.push_back(CompositionUnderline(
        0, length, SK_ColorBLACK, false /* thick */));
  }
}

string16 IBusClientImpl::ExtractCommitText(IBusText* text) {
  if (!text || !text->text)
    return WideToUTF16(L"");
  return UTF8ToUTF16(text->text);
}

}  // namespace internal
}  // namespace ui
