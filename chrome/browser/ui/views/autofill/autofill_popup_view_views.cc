// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/autofill/autofill_popup_view_views.h"

#include "chrome/browser/ui/autofill/autofill_popup_controller.h"
#include "grit/ui_resources.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebAutofillClient.h"
#include "ui/base/keycodes/keyboard_codes.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/point.h"
#include "ui/gfx/rect.h"
#include "ui/views/border.h"
#include "ui/views/widget/widget.h"

using WebKit::WebAutofillClient;

namespace {

const SkColor kBorderColor = SkColorSetARGB(0xFF, 0xC7, 0xCA, 0xCE);
const SkColor kHoveredBackgroundColor = SkColorSetARGB(0xFF, 0xCD, 0xCD, 0xCD);
const SkColor kItemTextColor = SkColorSetARGB(0xFF, 0x7F, 0x7F, 0x7F);
const SkColor kPopupBackground = SkColorSetARGB(0xFF, 0xFF, 0xFF, 0xFF);
const SkColor kValueTextColor = SkColorSetARGB(0xFF, 0x00, 0x00, 0x00);

}  // namespace

AutofillPopupViewViews::AutofillPopupViewViews(
    AutofillPopupController* controller)
    : controller_(controller),
      observing_widget_(NULL) {}

AutofillPopupViewViews::~AutofillPopupViewViews() {
  if (observing_widget_)
    observing_widget_->RemoveObserver(this);

  controller_->ViewDestroyed();
}

void AutofillPopupViewViews::Hide() {
  if (GetWidget()) {
    // This deletes |this|.
    GetWidget()->Close();
  } else {
    delete this;
  }
}

void AutofillPopupViewViews::OnPaint(gfx::Canvas* canvas) {
  canvas->DrawColor(kPopupBackground);
  OnPaintBorder(canvas);

  for (size_t i = 0; i < controller_->names().size(); ++i) {
    gfx::Rect line_rect = controller_->GetRowBounds(i);

    if (controller_->identifiers()[i] ==
            WebAutofillClient::MenuItemIDSeparator) {
      canvas->DrawRect(line_rect, kItemTextColor);
    } else {
      DrawAutofillEntry(canvas, i, line_rect);
    }
  }
}

void AutofillPopupViewViews::OnMouseCaptureLost() {
  controller_->MouseExitedPopup();
}

bool AutofillPopupViewViews::OnMouseDragged(const ui::MouseEvent& event) {
  if (HitTestPoint(gfx::Point(event.x(), event.y()))) {
    controller_->MouseHovered(event.x(), event.y());

    // We must return true in order to get future OnMouseDragged and
    // OnMouseReleased events.
    return true;
  }

  // If we move off of the popup, we lose the selection.
  controller_->MouseExitedPopup();
  return false;
}

void AutofillPopupViewViews::OnMouseExited(const ui::MouseEvent& event) {
  controller_->MouseExitedPopup();
}

void AutofillPopupViewViews::OnMouseMoved(const ui::MouseEvent& event) {
  controller_->MouseHovered(event.x(), event.y());
}

bool AutofillPopupViewViews::OnMousePressed(const ui::MouseEvent& event) {
  // We must return true in order to get the OnMouseReleased event later.
  return true;
}

void AutofillPopupViewViews::OnMouseReleased(const ui::MouseEvent& event) {
  // We only care about the left click.
  if (event.IsOnlyLeftMouseButton() &&
      HitTestPoint(gfx::Point(event.x(), event.y())))
    controller_->MouseClicked(event.x(), event.y());
}

void AutofillPopupViewViews::OnWidgetBoundsChanged(
    views::Widget* widget,
    const gfx::Rect& new_bounds) {
  Hide();
}

void AutofillPopupViewViews::Show() {
  if (!GetWidget()) {
    // The widget is destroyed by the corresponding NativeWidget, so we use
    // a weak pointer to hold the reference and don't have to worry about
    // deletion.
    views::Widget* widget = new views::Widget;
    views::Widget::InitParams params(views::Widget::InitParams::TYPE_POPUP);
    params.delegate = this;
    params.transparent = true;
    // Note: since there is no parent specified, this popup must handle deleting
    // itself.
    params.context = controller_->container_view();
    widget->Init(params);
    widget->SetContentsView(this);
    widget->SetBounds(controller_->popup_bounds());
    widget->Show();

    // Setup an observer to check for when the browser moves or changes size,
    // since the popup should always be hidden in those cases.
    observing_widget_ = views::Widget::GetTopLevelWidgetForNativeView(
        controller_->container_view());
    observing_widget_->AddObserver(this);
  }

  set_border(views::Border::CreateSolidBorder(kBorderThickness, kBorderColor));

  UpdateBoundsAndRedrawPopup();
}

void AutofillPopupViewViews::InvalidateRow(size_t row) {
  SchedulePaintInRect(controller_->GetRowBounds(row));
}

void AutofillPopupViewViews::UpdateBoundsAndRedrawPopup() {
  GetWidget()->SetBounds(controller_->popup_bounds());
  SchedulePaint();
}

void AutofillPopupViewViews::DrawAutofillEntry(gfx::Canvas* canvas,
                                               int index,
                                               const gfx::Rect& entry_rect) {
  if (controller_->selected_line() == index)
    canvas->FillRect(entry_rect, kHoveredBackgroundColor);

  bool is_rtl = base::i18n::IsRTL();
  int value_text_width = controller_->GetNameFontForRow(index).GetStringWidth(
      controller_->names()[index]);
  int value_content_x = is_rtl ?
      entry_rect.width() - value_text_width - kEndPadding : kEndPadding;

  canvas->DrawStringInt(
      controller_->names()[index],
      controller_->GetNameFontForRow(index),
      kValueTextColor,
      value_content_x,
      entry_rect.y(),
      canvas->GetStringWidth(controller_->names()[index],
                             controller_->GetNameFontForRow(index)),
      entry_rect.height(),
      gfx::Canvas::TEXT_ALIGN_CENTER);

  // Use this to figure out where all the other Autofill items should be placed.
  int x_align_left = is_rtl ? kEndPadding : entry_rect.width() - kEndPadding;

  // Draw the Autofill icon, if one exists
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  int row_height = controller_->GetRowBounds(index).height();
  if (!controller_->icons()[index].empty()) {
    int icon = controller_->GetIconResourceID(controller_->icons()[index]);
    DCHECK_NE(-1, icon);
    int icon_y = entry_rect.y() + (row_height - kAutofillIconHeight) / 2;

    x_align_left += is_rtl ? 0 : -kAutofillIconWidth;

    canvas->DrawImageInt(*rb.GetImageSkiaNamed(icon), x_align_left, icon_y);

    x_align_left += is_rtl ? kAutofillIconWidth + kIconPadding : -kIconPadding;
  }

  // Draw the name text.
  if (!is_rtl) {
    x_align_left -= canvas->GetStringWidth(controller_->subtexts()[index],
                                           controller_->subtext_font());
  }

  canvas->DrawStringInt(
      controller_->subtexts()[index],
      controller_->subtext_font(),
      kItemTextColor,
      x_align_left + kEndPadding,
      entry_rect.y(),
      canvas->GetStringWidth(controller_->subtexts()[index],
                             controller_->subtext_font()),
      entry_rect.height(),
      gfx::Canvas::TEXT_ALIGN_CENTER);
}

AutofillPopupView* AutofillPopupView::Create(
    AutofillPopupController* controller) {
  return new AutofillPopupViewViews(controller);
}
