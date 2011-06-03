// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_JAVASCRIPT_DIALOG_DELEGATE_H_
#define CONTENT_BROWSER_JAVASCRIPT_DIALOG_DELEGATE_H_
#pragma once

#include "base/string16.h"
#include "ui/gfx/native_widget_types.h"

class ExtensionHost;
class GURL;
class Profile;
class TabContents;

namespace IPC {
class Message;
}

namespace content {

// A class that invokes a JavaScript dialog must implement this interface to
// allow the dialog implementation to get needed information and return results.
class JavaScriptDialogDelegate {
 public:
  // This callback is invoked when the dialog is closed.
  virtual void OnDialogClosed(IPC::Message* reply_msg,
                              bool success,
                              const string16& user_input) = 0;

  // Returns the root native window with which to associate the dialog.
  virtual gfx::NativeWindow GetDialogRootWindow() = 0;

  // Returns the TabContents implementing this delegate, or NULL if there is
  // none. TODO(avi): This breaks encapsulation and in general sucks; figure out
  // a better way of doing this.
  virtual TabContents* AsTabContents() = 0;

  // Returns the ExtensionHost implementing this delegate, or NULL if there is
  // none. TODO(avi): This is even suckier than AsTabContents above as it breaks
  // layering; figure out a better way of doing this. http://crbug.com/84604
  virtual ExtensionHost* AsExtensionHost() = 0;

 protected:
  virtual ~JavaScriptDialogDelegate() {}
};


// An interface consisting of methods that can be called to produce JavaScript
// dialogs.
class JavaScriptDialogCreator {
 public:
  // Displays a JavaScript dialog. |did_suppress_message| will not be nil; if
  // |true| is returned in it, the caller will handle faking the reply.
  // TODO(avi): Remove Profile from this call; http://crbug.com/84601
  virtual void RunJavaScriptDialog(JavaScriptDialogDelegate* delegate,
                                   const GURL& frame_url,
                                   int dialog_flags,
                                   const string16& message_text,
                                   const string16& default_prompt_text,
                                   IPC::Message* reply_message,
                                   bool* did_suppress_message,
                                   Profile* profile) = 0;

  // Displays a dialog asking the user if they want to leave a page.
  virtual void RunBeforeUnloadDialog(JavaScriptDialogDelegate* delegate,
                                     const string16& message_text,
                                     IPC::Message* reply_message) = 0;

  // Resets any saved JavaScript dialog state for the delegate.
  virtual void ResetJavaScriptState(JavaScriptDialogDelegate* delegate) = 0;

 protected:
  virtual ~JavaScriptDialogCreator() {}
};

}  // namespace content

#endif  // CONTENT_BROWSER_JAVASCRIPT_DIALOG_DELEGATE_H_
