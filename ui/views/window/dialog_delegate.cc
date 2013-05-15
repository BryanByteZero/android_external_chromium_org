// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/window/dialog_delegate.h"

#include "base/logging.h"
#include "grit/ui_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/ui_base_switches_util.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"
#include "ui/views/window/dialog_client_view.h"

#if defined(USE_AURA)
#include "ui/views/corewm/shadow_types.h"
#endif

namespace views {

////////////////////////////////////////////////////////////////////////////////
// DialogDelegate:

DialogDelegate::~DialogDelegate() {
}

// static
bool DialogDelegate::UseNewStyle() {
  return switches::IsNewDialogStyleEnabled();
}

// static
Widget* DialogDelegate::CreateDialogWidget(DialogDelegate* dialog,
                                           gfx::NativeWindow context,
                                           gfx::NativeWindow parent) {
  views::Widget* widget = new views::Widget;
  views::Widget::InitParams params;
  params.delegate = dialog;
  if (DialogDelegate::UseNewStyle()) {
    // Note: Transparent widgets cannot host native Windows textfield controls.
    params.transparent = true;
    params.remove_standard_frame = true;
  }
  params.context = context;
  params.parent = parent;
  params.top_level = true;
  widget->Init(params);
  if (DialogDelegate::UseNewStyle()) {
#if defined(USE_AURA)
    // TODO(msw): Add a matching shadow type and remove the bubble frame border?
    corewm::SetShadowType(widget->GetNativeWindow(), corewm::SHADOW_TYPE_NONE);
#endif
  }
  return widget;
}

View* DialogDelegate::CreateExtraView() {
  return NULL;
}

View* DialogDelegate::CreateTitlebarExtraView() {
  return NULL;
}

View* DialogDelegate::CreateFootnoteView() {
  return NULL;
}

bool DialogDelegate::Cancel() {
  return true;
}

bool DialogDelegate::Accept(bool window_closing) {
  return Accept();
}

bool DialogDelegate::Accept() {
  return true;
}

base::string16 DialogDelegate::GetDialogLabel() const {
  return base::string16();
}

base::string16 DialogDelegate::GetDialogTitle() const {
  return base::string16();
}

int DialogDelegate::GetDialogButtons() const {
  return ui::DIALOG_BUTTON_OK | ui::DIALOG_BUTTON_CANCEL;
}

int DialogDelegate::GetDefaultDialogButton() const {
  if (GetDialogButtons() & ui::DIALOG_BUTTON_OK)
    return ui::DIALOG_BUTTON_OK;
  if (GetDialogButtons() & ui::DIALOG_BUTTON_CANCEL)
    return ui::DIALOG_BUTTON_CANCEL;
  return ui::DIALOG_BUTTON_NONE;
}

base::string16 DialogDelegate::GetDialogButtonLabel(
    ui::DialogButton button) const {
  if (button == ui::DIALOG_BUTTON_OK)
    return l10n_util::GetStringUTF16(IDS_APP_OK);
  if (button == ui::DIALOG_BUTTON_CANCEL) {
    if (GetDialogButtons() & ui::DIALOG_BUTTON_OK)
      return l10n_util::GetStringUTF16(IDS_APP_CANCEL);
    return l10n_util::GetStringUTF16(IDS_APP_CLOSE);
  }
  NOTREACHED();
  return base::string16();
}

bool DialogDelegate::IsDialogButtonEnabled(ui::DialogButton button) const {
  return true;
}

bool DialogDelegate::OnDialogButtonActivated(ui::DialogButton button) {
  if (button == ui::DIALOG_BUTTON_OK)
    return Accept();
  if (button == ui::DIALOG_BUTTON_CANCEL)
    return Cancel();
  return true;
}

View* DialogDelegate::GetInitiallyFocusedView() {
  // Focus the default button if any.
  const DialogClientView* dcv = GetDialogClientView();
  int default_button = GetDefaultDialogButton();
  if (default_button == ui::DIALOG_BUTTON_NONE)
    return NULL;

  if ((default_button & GetDialogButtons()) == 0) {
    // The default button is a button we don't have.
    NOTREACHED();
    return NULL;
  }

  if (default_button & ui::DIALOG_BUTTON_OK)
    return dcv->ok_button();
  if (default_button & ui::DIALOG_BUTTON_CANCEL)
    return dcv->cancel_button();
  return NULL;
}

DialogDelegate* DialogDelegate::AsDialogDelegate() {
  return this;
}

ClientView* DialogDelegate::CreateClientView(Widget* widget) {
  return new DialogClientView(widget, GetContentsView());
}

NonClientFrameView* DialogDelegate::CreateNonClientFrameView(Widget* widget) {
  return UseNewStyle() ? CreateNewStyleFrameView(widget) :
                         WidgetDelegate::CreateNonClientFrameView(widget);
}

// static
NonClientFrameView* DialogDelegate::CreateNewStyleFrameView(Widget* widget) {
  return CreateNewStyleFrameView(widget, false);
}

NonClientFrameView* DialogDelegate::CreateNewStyleFrameView(
    Widget* widget,
    bool force_opaque_border) {
  BubbleFrameView* frame = new BubbleFrameView(gfx::Insets());
  const SkColor color = widget->GetNativeTheme()->GetSystemColor(
      ui::NativeTheme::kColorId_DialogBackground);
  if (force_opaque_border) {
    frame->SetBubbleBorder(new BubbleBorder(
        BubbleBorder::NONE,
        BubbleBorder::NO_SHADOW_OPAQUE_BORDER,
        color));
  } else {
    frame->SetBubbleBorder(new BubbleBorder(BubbleBorder::FLOAT,
                                            BubbleBorder::SMALL_SHADOW,
                                            color));
  }
  frame->SetTitle(widget->widget_delegate()->GetWindowTitle());
  DialogDelegate* delegate = widget->widget_delegate()->AsDialogDelegate();
  if (delegate) {
    View* titlebar_view = delegate->CreateTitlebarExtraView();
    if (titlebar_view)
      frame->SetTitlebarExtraView(titlebar_view);
  }
  frame->SetShowCloseButton(widget->widget_delegate()->ShouldShowCloseButton());
  frame->set_can_drag(true);
  if (force_opaque_border)
    widget->set_frame_type(views::Widget::FRAME_TYPE_FORCE_CUSTOM);
  return frame;
}

const DialogClientView* DialogDelegate::GetDialogClientView() const {
  return GetWidget()->client_view()->AsDialogClientView();
}

DialogClientView* DialogDelegate::GetDialogClientView() {
  return GetWidget()->client_view()->AsDialogClientView();
}

ui::AccessibilityTypes::Role DialogDelegate::GetAccessibleWindowRole() const {
  return ui::AccessibilityTypes::ROLE_DIALOG;
}

////////////////////////////////////////////////////////////////////////////////
// DialogDelegateView:

DialogDelegateView::DialogDelegateView() {
  // A WidgetDelegate should be deleted on DeleteDelegate.
  set_owned_by_client();
}

DialogDelegateView::~DialogDelegateView() {}

void DialogDelegateView::DeleteDelegate() {
  delete this;
}

Widget* DialogDelegateView::GetWidget() {
  return View::GetWidget();
}

const Widget* DialogDelegateView::GetWidget() const {
  return View::GetWidget();
}

View* DialogDelegateView::GetContentsView() {
  return this;
}

}  // namespace views
