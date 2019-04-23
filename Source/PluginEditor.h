/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"

//==============================================================================

class MultiEffectDelayAudioProcessorEditor : public AudioProcessorEditor
{
public:
	//==============================================================================

	MultiEffectDelayAudioProcessorEditor(MultiEffectDelayAudioProcessor&);
	~MultiEffectDelayAudioProcessorEditor();

	//==============================================================================

	void paint(Graphics&) override;
	void resized() override;

private:
	//==============================================================================

	MultiEffectDelayAudioProcessor& processor;

	enum {
		editorWidth = 500,
		editorMargin = 10,
		editorPadding = 10,

		sliderTextEntryBoxWidth = 100,
		sliderTextEntryBoxHeight = 25,
		sliderHeight = 25,
		buttonHeight = 25,
		comboBoxHeight = 25,
		labelWidth = 100,
	};

	//======================================

	OwnedArray<Slider> sliders;
	OwnedArray<ToggleButton> toggles;
	OwnedArray<ComboBox> comboBoxes;

	OwnedArray<Label> labels;
	Array<Component*> components;

	typedef AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
	typedef AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;
	typedef AudioProcessorValueTreeState::ComboBoxAttachment ComboBoxAttachment;

	OwnedArray<SliderAttachment> sliderAttachments;
	OwnedArray<ButtonAttachment> buttonAttachments;
	OwnedArray<ComboBoxAttachment> comboBoxAttachments;

	//==============================================================================

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiEffectDelayAudioProcessorEditor)
};
