#pragma once

#include <juce_core/juce_core.h>

namespace BabyGirl
{
    // One Theme object is simply a named color palette.
    struct Theme
    {
        const char* name;
        uint32_t bg;
        uint32_t panel;
        uint32_t accent;
        uint32_t text;
    };

    // Beginner shortcut:
    // Copy one of these lines, change the five values, and you have a theme.
    #define BB_THEME(NAME, BG, PANEL, ACCENT, TEXT) { NAME, BG, PANEL, ACCENT, TEXT },

    inline constexpr Theme kThemes[] =
    {
        BB_THEME("Midnight Lab",   0xff07090c, 0xff11171e, 0xff54d6ff, 0xffedf3f8)
        BB_THEME("Amber Console", 0xff090807, 0xff1a1510, 0xffffb84d, 0xfffff4df)
        BB_THEME("Violet Machine", 0xff09070d, 0xff17111f, 0xffb889ff, 0xfff4ecff)
        BB_THEME("Acid Workshop",  0xff070c08, 0xff111b14, 0xffb8ff4d, 0xffefffe3)
        BB_THEME("Cold Rack",      0xff070b10, 0xff101923, 0xff78b7ff, 0xffe8f3ff)
        BB_THEME("Tape Room",      0xff0c0907, 0xff1b1510, 0xffd99b5f, 0xfffff0df)
    };

    #undef BB_THEME

    inline constexpr int kThemeCount = static_cast<int>(sizeof(kThemes) / sizeof(kThemes[0]));

    // Find a theme by name. If it does not exist, use the first theme.
    inline int findTheme(const juce::String& name)
    {
        for (int i = 0; i < kThemeCount; ++i)
        {
            if (name.equalsIgnoreCase(kThemes[i].name))
                return i;
        }

        return 0;
    }
}
