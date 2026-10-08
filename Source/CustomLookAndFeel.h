// ============================================================================
// CUSTOM LOOK AND FEEL
// ============================================================================
// JUCE lets us replace the normal appearance of controls. This file mainly
// teaches the Slider how to draw a rotary knob.
//
// Nothing in this class creates audio. It is purely visual.
// ============================================================================

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Themes.h"

namespace BabyGirl
{
    /**
     * BabyGirl Custom LookAndFeel
     * Analog machined hardware knobs, knurled bezels, warm incandescent backlighting.
     */
    class BabyGirlLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        BabyGirlLookAndFeel()
        {
            setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xfff59e0b));
            setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff22242a));
            setColour(juce::Slider::thumbColourId, juce::Colour(0xffffffff));
            setColour(juce::TextButton::buttonColourId, juce::Colour(0xff202832));
            setColour(juce::TextButton::textColourOffId, juce::Colour(0xffdce7f2));
            setColour(juce::TextButton::textColourOnId, juce::Colour(0xff081016));
            setColour(juce::ToggleButton::textColourId, juce::Colour(0xffdce7f2));
            setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff10151b));
            setColour(juce::ComboBox::textColourId, juce::Colour(0xffdce7f2));
        }

        // JUCE calls this whenever one of our rotary sliders needs to be drawn.
        // sliderPosProportional is normally 0.0 -> 1.0. We turn that into an
        // angle and draw a track, active arc, metal body, and pointer.
        void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                              float sliderPosProportional, float rotaryStartAngle,
                              float rotaryEndAngle, juce::Slider& slider) override
        {
            // Work out a square drawing area that fits inside the component.
            auto radius = (float)juce::jmin(width / 2, height / 2) - 4.0f;
            auto centreX = (float)x + (float)width * 0.5f;
            auto centreY = (float)y + (float)height * 0.5f;
            auto rx = centreX - radius;
            auto ry = centreY - radius;
            auto rw = radius * 2.0f;
            auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

            // Knurled metal outer circle
            g.setColour(juce::Colour(0xff18191e));
            g.fillEllipse(rx - 2.0f, ry - 2.0f, rw + 4.0f, rw + 4.0f);

            // Dial track
            juce::Path trackPath;
            trackPath.addCentredArc(centreX, centreY, radius - 2.0f, radius - 2.0f,
                                    0.0f, rotaryStartAngle, rotaryEndAngle, true);
            g.setColour(juce::Colour(0xff262830));
            g.strokePath(trackPath, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Active glow arc
            juce::Path activePath;
            activePath.addCentredArc(centreX, centreY, radius - 2.0f, radius - 2.0f,
                                     0.0f, rotaryStartAngle, angle, true);
            g.setColour(slider.findColour(juce::Slider::rotarySliderFillColourId));
            g.strokePath(activePath, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Machined aluminum center body
            juce::ColourGradient grad(juce::Colour(0xff3a3c45), centreX - radius * 0.5f, centreY - radius * 0.5f,
                                      juce::Colour(0xff14151a), centreX + radius * 0.5f, centreY + radius * 0.5f, true);
            g.setGradientFill(grad);
            g.fillEllipse(rx + 4.0f, ry + 4.0f, rw - 8.0f, rw - 8.0f);

            // Pointer line
            juce::Path p;
            auto pointerLength = radius * 0.75f;
            auto pointerThickness = 2.5f;
            p.addRectangle(-pointerThickness * 0.5f, -radius + 4.0f, pointerThickness, pointerLength * 0.6f);
            p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));

            g.setColour(slider.findColour(juce::Slider::rotarySliderFillColourId));
            g.fillPath(p);

            // Center screw recess
            g.setColour(juce::Colour(0xff090a0d));
            g.fillEllipse(centreX - 3.0f, centreY - 3.0f, 6.0f, 6.0f);
        }
    };
}
