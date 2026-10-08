# Themes — One-Line System

Add one line inside `Source/Themes.h`:

```cpp
BB_THEME("Theme Name", 0xffBACKGROUND, 0xffPANEL, 0xffACCENT, 0xffTEXT)
```

Example:

```cpp
BB_THEME("Cyber Mint", 0xff07100d, 0xff102018, 0xff62ffb0, 0xffedfff5)
```

**Background** = plugin base.

**Panel** = hardware surfaces.

**Accent** = knobs/highlights.

**Text** = readable text.

Keep each theme on one line.
