#include "epaper_ui/bitmap_font.h"

#include <algorithm>

namespace epaper_font {

namespace {

uint32_t NormalizeCodepoint(const BitmapFont& font, uint32_t codepoint) {
    if (codepoint < font.first_char || codepoint > font.last_char) {
        return static_cast<uint32_t>('?');
    }
    return codepoint;
}

}  // namespace

uint32_t DecodeUtf8Codepoint(std::string_view text, size_t& index) {
    const unsigned char lead = static_cast<unsigned char>(text[index]);

    // 1-byte sequence (ASCII).
    if (lead < 0x80U) {
        ++index;
        return lead;
    }

    // Determine expected continuation-byte count from the lead byte.
    int continuation_count = 0;
    uint32_t codepoint = 0;
    if ((lead & 0xE0U) == 0xC0U) {
        continuation_count = 1;
        codepoint = lead & 0x1FU;
    } else if ((lead & 0xF0U) == 0xE0U) {
        continuation_count = 2;
        codepoint = lead & 0x0FU;
    } else if ((lead & 0xF8U) == 0xF0U) {
        continuation_count = 3;
        codepoint = lead & 0x07U;
    } else {
        // Invalid lead byte (stray continuation byte or 0xF8-0xFF): consume
        // just this one byte so the loop keeps making progress.
        ++index;
        return static_cast<uint32_t>('?');
    }

    // Verify enough continuation bytes remain before consuming them, so a
    // truncated sequence at the end of the buffer doesn't read past it.
    if (index + static_cast<size_t>(continuation_count) >= text.size()) {
        ++index;
        return static_cast<uint32_t>('?');
    }

    for (int i = 1; i <= continuation_count; ++i) {
        const unsigned char cont = static_cast<unsigned char>(text[index + static_cast<size_t>(i)]);
        if ((cont & 0xC0U) != 0x80U) {
            // Malformed sequence; consume only the lead byte.
            ++index;
            return static_cast<uint32_t>('?');
        }
        codepoint = (codepoint << 6) | (cont & 0x3FU);
    }

    index += static_cast<size_t>(continuation_count) + 1;
    return codepoint;
}

const GlyphBitmap* FindGlyph(const BitmapFont& font, uint32_t codepoint) {
    const uint32_t normalized = NormalizeCodepoint(font, codepoint);
    if (normalized < font.first_char || normalized > font.last_char) {
        return nullptr;
    }

    const size_t index = static_cast<size_t>(normalized - font.first_char);
    return &font.glyphs[index];
}

int MeasureText(const BitmapFont& font, std::string_view text, int tracking) {
    int width = 0;
    bool first = true;

    size_t index = 0;
    while (index < text.size()) {
        const unsigned char lead = static_cast<unsigned char>(text[index]);
        if (lead == '\r' || lead == '\n') {
            break;
        }
        const uint32_t codepoint = DecodeUtf8Codepoint(text, index);
        const GlyphBitmap* glyph = FindGlyph(font, codepoint);
        if (glyph == nullptr) {
            continue;
        }
        if (!first) {
            width += std::max(tracking, 0);
        }
        width += glyph->advance;
        first = false;
    }

    return width;
}

}  // namespace epaper_font
