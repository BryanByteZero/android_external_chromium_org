// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CCFontAtlas_h
#define CCFontAtlas_h

#include <string>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "IntRect.h"
#include "SkBitmap.h"

class SkCanvas;

namespace gfx {
class Point;
}

namespace cc {

class Color;
class FontDescription;
class GraphicsContext;
class IntSize;

// This class provides basic ability to draw text onto the heads-up display.
class CCFontAtlas {
public:
    static scoped_ptr<CCFontAtlas> create(SkBitmap bitmap, IntRect asciiToRectTable[128], int fontHeight)
    {
        return make_scoped_ptr(new CCFontAtlas(bitmap, asciiToRectTable, fontHeight));
    }
    ~CCFontAtlas();

    // Draws multiple lines of text where each line of text is separated by '\n'.
    // - Correct glyphs will be drawn for ASCII codes in the range 32-127; any characters
    //   outside that range will be displayed as a default rectangle glyph.
    // - IntSize clip is used to avoid wasting time drawing things that are outside the
    //   target canvas bounds.
    // - Should only be called only on the impl thread.
    void drawText(SkCanvas*, const SkPaint&, const std::string& text, const gfx::Point& destPosition, const IntSize& clip) const;

    // Draws the entire atlas at the specified position, just for debugging purposes.
    void drawDebugAtlas(SkCanvas*, const gfx::Point& destPosition) const;

private:
    CCFontAtlas(SkBitmap, IntRect asciiToRectTable[128], int fontHeight);

    void drawOneLineOfTextInternal(SkCanvas*, const SkPaint&, const std::string&, const gfx::Point& destPosition) const;

    // The actual texture atlas containing all the pre-rendered glyphs.
    SkBitmap m_atlas;

    // The look-up tables mapping ascii characters to their IntRect locations on the atlas.
    IntRect m_asciiToRectTable[128];

    int m_fontHeight;

    DISALLOW_COPY_AND_ASSIGN(CCFontAtlas);
};

}  // namespace cc

#endif
