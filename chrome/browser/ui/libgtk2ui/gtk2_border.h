// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LIBGTK2UI_GTK2_BORDER_H_
#define CHROME_BROWSER_UI_LIBGTK2UI_GTK2_BORDER_H_

#include "ui/gfx/image/image_skia.h"
#include "ui/views/border.h"

namespace gfx {
class Canvas;
}

namespace views {
class CustomButton;
}

namespace libgtk2ui {
class Gtk2UI;

// Draws a gtk button border, and manages the memory of the resulting pixbufs.
class Gtk2Border : public views::Border {
 public:
  Gtk2Border(Gtk2UI* gtk2_ui,
             views::CustomButton* owning_button,
             scoped_ptr<views::Border> border);
  virtual ~Gtk2Border();

  // Called on theme changes. We invalidate the layout, drop our cached images,
  // and update our GTK state.
  void InvalidateAndSetUsesGtk(bool use_gtk);

  // Overridden from views::Border:
  virtual void Paint(const views::View& view, gfx::Canvas* canvas) OVERRIDE;
  virtual gfx::Insets GetInsets() const OVERRIDE;
  virtual gfx::Size GetMinimumSize() const OVERRIDE;

 private:
  Gtk2UI* gtk2_ui_;
  bool use_gtk_;

  gfx::ImageSkia hovered_;
  gfx::ImageSkia pressed_;

  // The view to which we are a border. We keep track of this so that we can
  // force invalidate the layout on theme changes.
  views::CustomButton* owning_button_;

  // Since we don't want to expose the concept of whether we're using a GTK
  // theme down to the cross platform views layer, we keep a normal Border and
  // delegate to it whenever we aren't in GTK theme mode.
  scoped_ptr<views::Border> border_;

  DISALLOW_COPY_AND_ASSIGN(Gtk2Border);
};

}  // namespace libgtk2ui

#endif  // CHROME_BROWSER_UI_LIBGTK2UI_GTK2_BORDER_H_
