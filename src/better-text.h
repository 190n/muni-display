#pragma once

#include <Adafruit_HX8357.h>

typedef struct {
    unsigned int offset;
    uint16_t width;
    uint16_t height;
} Glyph;

typedef struct {
    const uint8_t* bitmapsFull;
    const uint8_t* bitmapsHalf;
    const Glyph* glyphs;
} Font;

uint16_t calculateHalfColor(uint16_t fg, uint16_t bg) {
    uint8_t fgR = (fg & 0xf800) >> 11,
        fgG = (fg & 0x07e0) >> 5,
        fgB = fg & 0x001f,
        bgR = (bg & 0xf800) >> 11,
        bgG = (bg & 0x07e0) >> 5,
        bgB = bg & 0x001f;

    uint16_t newR = (fgR + bgR) / 2,
        newG = (fgG + bgG) / 2,
        newB = (fgB + bgB) / 2;

    return (newR << 11) | (newG << 5) | newB;
}

int16_t drawText(Adafruit_HX8357 display, int16_t x, int16_t y, int16_t maxWidth, int16_t lineHeight, Font font, uint16_t fgColor, uint16_t bgColor, const char* text) {
    int16_t initialX = x;
    uint16_t halfColor = calculateHalfColor(fgColor, bgColor);
    size_t len = strlen(text);

    for (int i = 0; i < len; i++) {
        char c = text[i];
        Glyph g = font.glyphs[c];

        if (x + g.width > maxWidth) {
            x = initialX;
            y += lineHeight;
        }

        display.drawBitmap(x, y, &font.bitmapsFull[g.offset], g.width, g.height, fgColor);
        display.drawBitmap(x, y, &font.bitmapsHalf[g.offset], g.width, g.height, halfColor);
        x += g.width;
    }

    return x;
}
