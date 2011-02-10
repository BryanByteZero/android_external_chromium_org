// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VIEWS_EVENT_H_
#define VIEWS_EVENT_H_
#pragma once

#include "base/basictypes.h"
#include "base/time.h"
#include "ui/base/events.h"
#include "ui/base/keycodes/keyboard_codes.h"
#include "ui/gfx/point.h"
#include "views/native_types.h"

#if defined(OS_LINUX)
typedef struct _GdkEventKey GdkEventKey;
#endif

#if defined(TOUCH_UI)
typedef union _XEvent XEvent;
#endif

namespace ui {
class OSExchangeData;
}
using ui::OSExchangeData;

namespace views {

class View;

////////////////////////////////////////////////////////////////////////////////
//
// Event class
//
// An event encapsulates an input event that can be propagated into view
// hierarchies. An event has a type, some flags and a time stamp.
//
// Each major event type has a corresponding Event subclass.
//
// Events are immutable but support copy
//
////////////////////////////////////////////////////////////////////////////////
class Event {
 public:
  const NativeEvent& native_event() const { return native_event_; }
  ui::EventType type() const { return type_; }
  const base::Time& time_stamp() const { return time_stamp_; }
  int flags() const { return flags_; }
  void set_flags(int flags) { flags_ = flags; }

  // The following methods return true if the respective keys were pressed at
  // the time the event was created.
  bool IsShiftDown() const { return (flags_ & ui::EF_SHIFT_DOWN) != 0; }
  bool IsControlDown() const { return (flags_ & ui::EF_CONTROL_DOWN) != 0; }
  bool IsCapsLockDown() const { return (flags_ & ui::EF_CAPS_LOCK_DOWN) != 0; }
  bool IsAltDown() const { return (flags_ & ui::EF_ALT_DOWN) != 0; }

  bool IsMouseEvent() const {
    return type_ == ui::ET_MOUSE_PRESSED ||
           type_ == ui::ET_MOUSE_DRAGGED ||
           type_ == ui::ET_MOUSE_RELEASED ||
           type_ == ui::ET_MOUSE_MOVED ||
           type_ == ui::ET_MOUSE_ENTERED ||
           type_ == ui::ET_MOUSE_EXITED ||
           type_ == ui::ET_MOUSEWHEEL;
  }

#if defined(TOUCH_UI)
  bool IsTouchEvent() const {
    return type_ == ui::ET_TOUCH_RELEASED ||
           type_ == ui::ET_TOUCH_PRESSED ||
           type_ == ui::ET_TOUCH_MOVED ||
           type_ == ui::ET_TOUCH_STATIONARY ||
           type_ == ui::ET_TOUCH_CANCELLED;
  }
#endif

#if defined(OS_WIN)
  // Returns the EventFlags in terms of windows flags.
  int GetWindowsFlags() const;

  // Convert windows flags to views::Event flags
  static int ConvertWindowsFlags(uint32 win_flags);
#elif defined(OS_LINUX)
  // Convert the state member on a GdkEvent to views::Event flags
  static int GetFlagsFromGdkState(int state);
#endif

 protected:
  Event(ui::EventType type, int flags);

  Event(const Event& model)
      : native_event_(model.native_event()),
        type_(model.type()),
        time_stamp_(model.time_stamp()),
        flags_(model.flags()) {
  }

 private:
  void operator=(const Event&);

  NativeEvent native_event_;
  ui::EventType type_;
  base::Time time_stamp_;
  int flags_;
};

////////////////////////////////////////////////////////////////////////////////
//
// LocatedEvent class
//
// A generic event that is used for any events that is located at a specific
// position in the screen.
//
////////////////////////////////////////////////////////////////////////////////
class LocatedEvent : public Event {
 public:
  LocatedEvent(ui::EventType type, const gfx::Point& location, int flags)
      : Event(type, flags),
        location_(location) {
  }

  // Create a new LocatedEvent which is identical to the provided model.
  // If from / to views are provided, the model location will be converted
  // from 'from' coordinate system to 'to' coordinate system
  LocatedEvent(const LocatedEvent& model, View* from, View* to);

  int x() const { return location_.x(); }
  int y() const { return location_.y(); }
  const gfx::Point& location() const { return location_; }

 private:
  gfx::Point location_;
};

////////////////////////////////////////////////////////////////////////////////
//
// MouseEvent class
//
// A mouse event is used for any input event related to the mouse.
//
////////////////////////////////////////////////////////////////////////////////
class MouseEvent : public LocatedEvent {
 public:
  // Create a new mouse event
  MouseEvent(ui::EventType type, int x, int y, int flags)
      : LocatedEvent(type, gfx::Point(x, y), flags) {
  }

  // Create a new mouse event from a type and a point. If from / to views
  // are provided, the point will be converted from 'from' coordinate system to
  // 'to' coordinate system.
  MouseEvent(ui::EventType type,
             View* from,
             View* to,
             const gfx::Point &l,
             int flags);

  // Create a new MouseEvent which is identical to the provided model.
  // If from / to views are provided, the model location will be converted
  // from 'from' coordinate system to 'to' coordinate system
  MouseEvent(const MouseEvent& model, View* from, View* to);

#if defined(TOUCH_UI)
  // Create a mouse event from an X mouse event.
  explicit MouseEvent(XEvent* xevent);
#endif

  // Conveniences to quickly test what button is down
  bool IsOnlyLeftMouseButton() const {
    return (flags() & ui::EF_LEFT_BUTTON_DOWN) &&
      !(flags() & (ui::EF_MIDDLE_BUTTON_DOWN | ui::EF_RIGHT_BUTTON_DOWN));
  }

  bool IsLeftMouseButton() const {
    return (flags() & ui::EF_LEFT_BUTTON_DOWN) != 0;
  }

  bool IsOnlyMiddleMouseButton() const {
    return (flags() & ui::EF_MIDDLE_BUTTON_DOWN) &&
      !(flags() & (ui::EF_LEFT_BUTTON_DOWN | ui::EF_RIGHT_BUTTON_DOWN));
  }

  bool IsMiddleMouseButton() const {
    return (flags() & ui::EF_MIDDLE_BUTTON_DOWN) != 0;
  }

  bool IsOnlyRightMouseButton() const {
    return (flags() & ui::EF_RIGHT_BUTTON_DOWN) &&
      !(flags() & (ui::EF_LEFT_BUTTON_DOWN | ui::EF_MIDDLE_BUTTON_DOWN));
  }

  bool IsRightMouseButton() const {
    return (flags() & ui::EF_RIGHT_BUTTON_DOWN) != 0;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MouseEvent);
};

#if defined(TOUCH_UI)
////////////////////////////////////////////////////////////////////////////////
//
// TouchEvent class
//
// A touch event is generated by touch screen and advanced track
// pad devices. There is a deliberate direct correspondence between
// TouchEvent and PlatformTouchPoint.
//
////////////////////////////////////////////////////////////////////////////////
class TouchEvent : public LocatedEvent {
 public:
  // Create a new touch event.
  TouchEvent(ui::EventType type, int x, int y, int flags, int touch_id);

  // Create a new touch event from a type and a point. If from / to views
  // are provided, the point will be converted from 'from' coordinate system to
  // 'to' coordinate system.
  TouchEvent(ui::EventType type,
             View* from,
             View* to,
             const gfx::Point& l,
             int flags,
             int touch_id);

  // Create a new TouchEvent which is identical to the provided model.
  // If from / to views are provided, the model location will be converted
  // from 'from' coordinate system to 'to' coordinate system.
  TouchEvent(const TouchEvent& model, View* from, View* to);

#if defined(HAVE_XINPUT2)
  explicit TouchEvent(XEvent* xev);
#endif

  bool identity() const { return touch_id_; }

 private:
  // The identity (typically finger) of the touch starting at 0 and incrementing
  // for each separable additional touch that the hardware can detect.
  const int touch_id_;

  DISALLOW_COPY_AND_ASSIGN(TouchEvent);
};
#endif

////////////////////////////////////////////////////////////////////////////////
//
// KeyEvent class
//
// A key event is used for any input event related to the keyboard.
// Note: this event is about key pressed, not typed characters.
//
////////////////////////////////////////////////////////////////////////////////
class KeyEvent : public Event {
 public:
  // Create a new key event
  KeyEvent(ui::EventType type,
           ui::KeyboardCode key_code,
           int event_flags,
           int repeat_count,
           int message_flags);

#if defined(OS_WIN)
  KeyEvent(ui::EventType type,
           ui::KeyboardCode key_code,
           int event_flags,
           int repeat_count,
           int message_flags,
           UINT message);
#endif
#if defined(OS_LINUX)
  explicit KeyEvent(const GdkEventKey* event);

  const GdkEventKey* native_event() const { return native_event_; }
#endif
#if defined(TOUCH_UI)
  // Create a key event from an X key event.
  explicit KeyEvent(XEvent* xevent);
#endif

  ui::KeyboardCode key_code() const { return key_code_; }

#if defined(OS_WIN)
  bool IsExtendedKey() const;

  UINT message() const { return message_; }
#endif

  int repeat_count() const { return repeat_count_; }

#if defined(OS_WIN)
  static int GetKeyStateFlags();
#endif

 private:

  ui::KeyboardCode key_code_;
  int repeat_count_;
  int message_flags_;
#if defined(OS_WIN)
  UINT message_;
#elif defined(OS_LINUX)
  const GdkEventKey* native_event_;
#endif
  DISALLOW_COPY_AND_ASSIGN(KeyEvent);
};

////////////////////////////////////////////////////////////////////////////////
//
// MouseWheelEvent class
//
// A MouseWheelEvent is used to propagate mouse wheel user events.
// Note: e.GetOffset() > 0 means scroll up.
//
////////////////////////////////////////////////////////////////////////////////
class MouseWheelEvent : public LocatedEvent {
 public:
  // Create a new key event
  MouseWheelEvent(int offset, int x, int y, int flags)
      : LocatedEvent(ui::ET_MOUSEWHEEL, gfx::Point(x, y), flags),
        offset_(offset) {
  }

#if defined(TOUCH_UI)
  explicit MouseWheelEvent(XEvent* xev);
#endif

  int offset() const { return offset_; }

 private:
  int offset_;

  DISALLOW_COPY_AND_ASSIGN(MouseWheelEvent);
};

////////////////////////////////////////////////////////////////////////////////
//
// DropTargetEvent class
//
// A DropTargetEvent is sent to the view the mouse is over during a drag and
// drop operation.
//
////////////////////////////////////////////////////////////////////////////////
class DropTargetEvent : public LocatedEvent {
 public:
  DropTargetEvent(const OSExchangeData& data,
                  int x,
                  int y,
                  int source_operations)
      : LocatedEvent(ui::ET_DROP_TARGET_EVENT, gfx::Point(x, y), 0),
        data_(data),
        source_operations_(source_operations) {
  }

  const OSExchangeData& data() const { return data_; }
  int source_operations() const { return source_operations_; }

 private:
  // Data associated with the drag/drop session.
  const OSExchangeData& data_;

  // Bitmask of supported ui::DragDropTypes::DragOperation by the source.
  int source_operations_;

  DISALLOW_COPY_AND_ASSIGN(DropTargetEvent);
};

}  // namespace views

#endif  // VIEWS_EVENT_H_
