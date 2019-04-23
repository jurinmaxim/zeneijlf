/*
  ==============================================================================

	This file was auto-generated!

	It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginParameter.h"

//==============================================================================

MultiEffectDelayAudioProcessor::MultiEffectDelayAudioProcessor() :
#ifndef JucePlugin_PreferredChannelConfigurations
	AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
		.withInput("Input", AudioChannelSet::stereo(), true)
#endif
		.withOutput("Output", AudioChannelSet::stereo(), true)
#endif
	),
#endif
	parameters(*this)
	, paramDepth(parameters, "Depth/Wet", "", 0.0f, 1.0f, 1.0f)
	, paramDelay(parameters, "Delay", "ms", 0.0f, 1000.0f, 50.0f, [](float value) { return value * 0.001f; })
	, paramWidth(parameters, "Width", "ms", 0.0f, 50.0f, 10.0f, [](float value) { return value * 0.001f; })
	, paramFeedback(parameters, "Feedback", "", 0.0f, 1.0f, 0.0f)
	, paramFrequency(parameters, "LFO Frequency", "Hz", 0.1f, 1.0f, 0.2f)
	, paramWaveform(parameters, "Modulation", { "Sine","White Noise" }, 0)
	, paramMultiTapOne(parameters, "Multi-tap One", "1/n", 1.0f, 10.0f, 2.0f)
	, paramMultiTapTwo(parameters, "Multi-tap Two", "1/n", 1.0f, 10.0f, 3.0f)
	, paramMultiTapGain(parameters, "Multi-tap Gain", "", 0.0f, 1.0f, 0.0f)
	, lowPassFilter(dsp::IIR::Coefficients<float>::makeLowPass(44100, 20000.0f, 0.1f))
{
	parameters.valueTreeState.state = ValueTree(Identifier(getName().removeCharacters("- ")));

}

MultiEffectDelayAudioProcessor::~MultiEffectDelayAudioProcessor()
{
}

//==============================================================================

void MultiEffectDelayAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	//reseteljuk a parametereket
	const double smoothTime = 1e-3;
	paramDelay.reset(sampleRate, smoothTime);
	paramWidth.reset(sampleRate, smoothTime);
	paramDepth.reset(sampleRate, smoothTime);
	paramFeedback.reset(sampleRate, smoothTime);
	paramFrequency.reset(sampleRate, smoothTime);
	paramWaveform.reset(sampleRate, smoothTime);
	paramMultiTapOne.reset(sampleRate, smoothTime);
	paramMultiTapTwo.reset(sampleRate, smoothTime);
	paramMultiTapGain.reset(sampleRate, smoothTime);

	//inicializaljuk a delayBuffert
	float maxDelayTime = paramDelay.maxValue + paramWidth.maxValue;
	delayBufferSamples = (int)(maxDelayTime * (float)sampleRate) + 1;
	if (delayBufferSamples < 1) delayBufferSamples = 1;
	const int delayBufferChannels = getTotalNumInputChannels();
	delayBuffer.setSize(delayBufferChannels, delayBufferSamples);
	delayBuffer.clear();

	//inicializajuk a filterBuffert
	const int filterBufferChannels = getTotalNumInputChannels();
	filterBuffer.setSize(filterBufferChannels, samplesPerBlock);
	filterBuffer.clear();

	//a delaybuffer irasi poziciojat, illetve az lfo fazisat nulazzuk
	delayWritePosition = 0;
	lfoPhase = 0.0f;

	//inicializaljuk a low pass filter parametereit
	dsp::ProcessSpec spec;
	spec.sampleRate = sampleRate;
	spec.maximumBlockSize = samplesPerBlock;
	spec.numChannels = getTotalNumOutputChannels();
	lowPassFilter.prepare(spec);
	lowPassFilter.reset();
	*lowPassFilter.state = *dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 15.0f, 1.0f);
}

void MultiEffectDelayAudioProcessor::releaseResources()
{
}

void MultiEffectDelayAudioProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	ScopedNoDenormals noDenormals;

	const int numInputChannels = getTotalNumInputChannels();
	const int numOutputChannels = getTotalNumOutputChannels();
	const int numSamples = buffer.getNumSamples();

	//kiolvassuk a parameterek allasait
	float currentDelay = paramDelay.getNextValue();
	float currentWidth = paramWidth.getNextValue();
	float currentDepth = paramDepth.getNextValue();
	float currentFeedback = paramFeedback.getNextValue();
	float currentFrequency = paramFrequency.getNextValue();
	float currentMultiTapOne = paramMultiTapOne.getNextValue();
	float currentMultiTapTwo = paramMultiTapTwo.getNextValue();
	float currentMultiTapGain = paramMultiTapGain.getNextValue();

	int localWritePosition;
	float phase;

	//blokkba foglaljuk a filterBufferunket
	dsp::AudioBlock<float> block(filterBuffer);

	//ha nincs delay, akkor ne lehessen modulalni sem azt
	if (currentDelay == 0)
		currentWidth = 0;

	for (int channel = 0; channel < numInputChannels; ++channel) {
		float* channelData = buffer.getWritePointer(channel);
		float* delayData = delayBuffer.getWritePointer(channel);
		float* filterData = filterBuffer.getWritePointer(channel);

		//a kor-korosen valtozo parameterek elozo blokkbol mentett ertekeit olvassuk ki
		localWritePosition = delayWritePosition;
		phase = lfoPhase;

		//feltoltjuk a filterBufferunket zajjal
		for (int sample = 0; sample < filterBuffer.getNumSamples(); ++sample)
			filterData[sample] = random.nextFloat();

		//szurjuk a filterBuffert low pass filterrel
		lowPassFilter.process(dsp::ProcessContextReplacing<float>(block));

		for (int sample = 0; sample < numSamples; ++sample) {
			//dry signal tarolasa
			const float in = channelData[sample];

			float out = 0.0f, sOut = 0.0f, tOut = 0.0f;
				
			int64 readPosition = 0, sReadPosition = 0, tReadPosition = 0;

			//ezen az agon tortenik a chorus, flanger, es a sima delay megvalositasa
			if (currentMultiTapGain == 0) {
				if ((int)paramWaveform.getTargetValue())
					//chorus, a width-et a filterBufferbol kiolvasott adattal modulaljuk
					readPosition = static_cast<int64> (localWritePosition - (currentDelay + currentWidth * filterData[sample]) * getSampleRate() + delayBufferSamples) % delayBufferSamples;
				else
					//flanger, a width-et egy 0-1 kozotti szinusz jellel modulaljuk, sima delay eseten a width-et lehuzzuk 0-ra
					readPosition = static_cast<int64> (localWritePosition - (currentDelay + currentWidth * (0.5f + 0.5f * sinf(2.0f * M_PI * phase))) * getSampleRate() + delayBufferSamples) % delayBufferSamples;

				//kimeneti erteket kiolvassuk a kiszamolt olvaso pointerunk segitsegevel a delay bufferbol
				out = delayData[readPosition % delayBufferSamples];
				
				//visszairjuk a bufferbe az eredeti jelet es a delaybufferbol az olvaso pointerunk altal mutatott samplet
				channelData[sample] = in + out * currentDepth;

				//feltoltjuk a delay buffert az eredeti jellel es feedback eseten a kesleltetett jellel
				delayData[localWritePosition] = in + out * currentFeedback;

			}
			//ezen az agon a multi-tap delay valosul meg a fentihez hasonlo modon, ebben az esetben 3 pointerrel
			else {
				//kiszamoljuk az eredeti olvaso pointert es a masik ketto pointert, melyek kesleltetesi ideje le van osztva a multi-tap egyutthatoval (1/n)
				readPosition = static_cast<int64> (localWritePosition - currentDelay * getSampleRate() + delayBufferSamples) % delayBufferSamples;
				sReadPosition = static_cast<int64> (localWritePosition - currentDelay / currentMultiTapOne * getSampleRate() + delayBufferSamples) % delayBufferSamples;
				tReadPosition = static_cast<int64> (localWritePosition - currentDelay / currentMultiTapTwo * getSampleRate() + delayBufferSamples) % delayBufferSamples;

				//kiszamoljuk a kimeneteket a fenti aggal analog modon
				out = delayData[readPosition % delayBufferSamples];
				sOut = delayData[sReadPosition % delayBufferSamples];
				tOut = delayData[tReadPosition % delayBufferSamples];

				//visszairjuk a bufferbe az osszeget
				channelData[sample] = in + (out + (sOut + tOut) * currentMultiTapGain) * currentDepth;

				//feltoltjuk a delaybuffert ugyanugy
				delayData[localWritePosition] = in + (out + (sOut + tOut) * currentMultiTapGain) / 3 * currentFeedback;
			}

			//megyunk tovabb az iropointerrel
			if (++localWritePosition >= delayBufferSamples)
				localWritePosition -= delayBufferSamples;

			//noveljuk a modulalo fazist
			phase += currentFrequency / (float)getSampleRate();
			if (phase >= 1.0f) phase -= 1.0f;
		}
	}

	//blokk feldolgozasa utan kimentjuk az iropointert es a fazist
	delayWritePosition = localWritePosition;
	lfoPhase = phase;

	for (int channel = numInputChannels; channel < numOutputChannels; ++channel) {
		buffer.clear(channel, 0, numSamples);
		filterBuffer.clear(channel, 0, filterBuffer.getNumSamples());
	}
}

//==============================================================================

void MultiEffectDelayAudioProcessor::getStateInformation(MemoryBlock& destData)
{
	ScopedPointer<XmlElement> xml(parameters.valueTreeState.state.createXml());
	copyXmlToBinary(*xml, destData);
}

void MultiEffectDelayAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
	ScopedPointer<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
	if (xmlState != nullptr)
		if (xmlState->hasTagName(parameters.valueTreeState.state.getType()))
			parameters.valueTreeState.state = ValueTree::fromXml(*xmlState);
}

//==============================================================================

bool MultiEffectDelayAudioProcessor::hasEditor() const
{
	return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* MultiEffectDelayAudioProcessor::createEditor()
{
	return new MultiEffectDelayAudioProcessorEditor(*this);
}

//==============================================================================

#ifndef JucePlugin_PreferredChannelConfigurations
bool MultiEffectDelayAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
	ignoreUnused(layouts);
	return true;
#else
	// This is the place where you check if the layout is supported.
	// In this template code we only support mono or stereo.
	if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
		&& layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
		return false;

	// This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;
#endif

	return true;
#endif
}
#endif

//==============================================================================

const String MultiEffectDelayAudioProcessor::getName() const
{
	return JucePlugin_Name;
}

bool MultiEffectDelayAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
	return true;
#else
	return false;
#endif
}

bool MultiEffectDelayAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
	return true;
#else
	return false;
#endif
}

bool MultiEffectDelayAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
	return true;
#else
	return false;
#endif
}

double MultiEffectDelayAudioProcessor::getTailLengthSeconds() const
{
	return 0.0;
}

int MultiEffectDelayAudioProcessor::getNumPrograms()
{
	return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
				// so this should be at least 1, even if you're not really implementing programs.
}

int MultiEffectDelayAudioProcessor::getCurrentProgram()
{
	return 0;
}

void MultiEffectDelayAudioProcessor::setCurrentProgram(int index)
{
}

const String MultiEffectDelayAudioProcessor::getProgramName(int index)
{
	return {};
}

void MultiEffectDelayAudioProcessor::changeProgramName(int index, const String& newName)
{
}

//==============================================================================

// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new MultiEffectDelayAudioProcessor();
}

//==============================================================================

