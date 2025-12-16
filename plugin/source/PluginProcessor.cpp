#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <atomic>
#include <array>
#include <cstddef>
#include <cstdint>

namespace audio_plugin {
namespace {
std::atomic<std::uint32_t> gInstanceCounter{1u};

constexpr const char* kStateTreeId = "ParettoGateParams";

namespace ParamId {
constexpr const char* syncEnabled = "syncEnabled";
constexpr const char* rate = "rate";
constexpr const char* rateHz = "rateHz";
constexpr const char* steps = "steps";
constexpr const char* swing = "swing";
constexpr const char* phaseDeg = "phaseDeg";

constexpr const char* depth = "depth";
constexpr const char* smoothMs = "smoothMs";
constexpr const char* floorDb = "floorDb";
constexpr const char* prob = "prob";
constexpr const char* accent = "accent";

constexpr const char* toneDepth = "toneDepth";
constexpr const char* toneBaseHz = "toneBaseHz";
constexpr const char* toneSpanOct = "toneSpanOct";
constexpr const char* res = "res";

constexpr const char* morph = "morph";
constexpr const char* groove = "groove";

constexpr const char* mix = "mix";
constexpr const char* outputDb = "outputDb";
}  // namespace ParamId

inline float loadParam(const juce::AudioProcessorValueTreeState& apvts, const char* id) {
  if (auto* v = apvts.getRawParameterValue(id)) {
    return v->load();
  }
  return 0.0f;
}
}

juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{ParamId::syncEnabled, 1}, "Sync", true));

  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID{ParamId::rate, 1}, "Rate",
      juce::StringArray{"1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/1D", "1/2D", "1/4D", "1/8D", "1/16D",
                        "1/32D", "1/1T", "1/2T", "1/4T", "1/8T", "1/16T", "1/32T"},
      3));

  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ParamId::rateHz, 1}, "Rate Hz",
                                                         juce::NormalisableRange<float>(0.05f, 30.0f, 0.0f, 0.5f), 2.0f));

  layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ParamId::steps, 1}, "Steps",
                                                          juce::StringArray{"8", "16", "32"}, 1));

  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ParamId::swing, 1}, "Swing",
                                                         juce::NormalisableRange<float>(0.0f, 0.95f, 0.0f, 1.0f), 0.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ParamId::phaseDeg, 1}, "Phase",
                                                         juce::NormalisableRange<float>(0.0f, 360.0f, 0.0f, 1.0f), 0.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ParamId::depth, 1}, "Depth",
                                                         juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 1.0f), 1.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ParamId::smoothMs, 1}, "Smooth ms",
                                                         juce::NormalisableRange<float>(0.0f, 200.0f, 0.0f, 0.5f), 5.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ParamId::floorDb, 1}, "Floor dB",
                                                         juce::NormalisableRange<float>(-120.0f, 0.0f, 0.0f, 1.0f), -60.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ParamId::prob, 1}, "Probability",
                                                         juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 1.0f), 1.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ParamId::accent, 1}, "Accent",
                                                         juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 1.0f), 0.3f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ParamId::toneDepth, 1}, "Tone Depth",
                                                         juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 1.0f), 0.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{ParamId::toneBaseHz, 1}, "Tone Base Hz",
      juce::NormalisableRange<float>(20.0f, 20000.0f, 0.0f, 0.5f), 2000.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ParamId::toneSpanOct, 1}, "Tone Span (oct)",
                                                         juce::NormalisableRange<float>(0.0f, 8.0f, 0.0f, 1.0f), 2.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ParamId::res, 1}, "Resonance",
                                                         juce::NormalisableRange<float>(0.0f, 0.99f, 0.0f, 1.0f), 0.2f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ParamId::morph, 1}, "Morph",
                                                         juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 1.0f), 0.0f));

  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID{ParamId::groove, 1}, "Groove",
      juce::StringArray{"Straight", "Swing", "UKG", "Jersey", "DnB", "RandomMicro"}, 0));

  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ParamId::mix, 1}, "Mix",
                                                         juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 1.0f), 1.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ParamId::outputDb, 1}, "Output dB",
                                                         juce::NormalisableRange<float>(-60.0f, 24.0f, 0.0f, 1.0f), 0.0f));

  return layout;
}

AudioPluginAudioProcessor::AudioPluginAudioProcessor()
    : AudioProcessor(
          BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
      )
    , apvts_(*this, nullptr, kStateTreeId, createParameterLayout()) {
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor() {}

const juce::String AudioPluginAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool AudioPluginAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const {
  return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms() {
  return 1;  // NB: some hosts don't cope very well if you tell them there are 0
             // programs, so this should be at least 1, even if you're not
             // really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram() {
  return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram(int index) {
  juce::ignoreUnused(index);
}

const juce::String AudioPluginAudioProcessor::getProgramName(int index) {
  juce::ignoreUnused(index);
  return {};
}

void AudioPluginAudioProcessor::changeProgramName(int index,
                                                  const juce::String& newName) {
  juce::ignoreUnused(index, newName);
}

void AudioPluginAudioProcessor::prepareToPlay(double sampleRate,
                                              int samplesPerBlock) {
  // Use this method as the place to do any pre-playback
  // initialisation that you need..
  juce::ignoreUnused(samplesPerBlock);

  ctx_.sampleRate = sampleRate;

  const auto counter = gInstanceCounter.fetch_add(1u, std::memory_order_relaxed);
  const auto ptrHash = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(this));
  seed_ = static_cast<std::uint32_t>(static_cast<std::uint32_t>(sampleRate) ^ ptrHash ^ counter);

  gateEngine_.prepare(sampleRate, seed_);

  gateParams_ = paretto::gate::GateParameters{};
  for (int i = 0; i < paretto::dsp::MaxSteps; ++i) {
    gateParams_.patternA.steps[static_cast<std::size_t>(i)].level = (i % 2 == 0) ? 1.0f : 0.0f;
    gateParams_.patternB.steps[static_cast<std::size_t>(i)].level = 1.0f;
  }
  gateEngine_.setParameters(gateParams_);
}

void AudioPluginAudioProcessor::releaseResources() {
  // When playback stops, you can use this as an opportunity to free up any
  // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const {
#if JucePlugin_IsMidiEffect
  juce::ignoreUnused(layouts);
  return true;
#else
  // This is the place where you check if the layout is supported.
  // In this template code we only support mono or stereo.
  // Some plugin hosts, such as certain GarageBand versions, will only
  // load plugins that support stereo bus layouts.
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;
#endif

  return true;
#endif
}

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages) {
  juce::ignoreUnused(midiMessages);

  juce::ScopedNoDenormals noDenormals;
  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  // In case we have more outputs than inputs, this code clears any output
  // channels that didn't contain input data, (because these aren't
  // guaranteed to be empty - they may contain garbage).
  // This is here to avoid people getting screaming feedback
  // when they first compile a plugin, but obviously you don't need to keep
  // this code if your algorithm always overwrites all the output channels.
  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  ctx_.transport.valid = false;
  if (auto* ph = getPlayHead()) {
    if (auto posInfo = ph->getPosition()) {
      ctx_.transport.valid = true;
      ctx_.transport.playing = posInfo->getIsPlaying();
      ctx_.transport.bpm = posInfo->getBpm().orFallback(120.0);
      ctx_.transport.ppqPosition = posInfo->getPpqPosition().orFallback(0.0);
      ctx_.transport.timeInSamples = posInfo->getTimeInSamples().orFallback(0);
    }
  }

  float* channelPtrs[2] = {nullptr, nullptr};
  const int numCh = std::min(2, totalNumInputChannels);
  for (int ch = 0; ch < numCh; ++ch) {
    channelPtrs[ch] = buffer.getWritePointer(ch);
  }

  gateParams_.syncEnabled = loadParam(apvts_, ParamId::syncEnabled) >= 0.5f;

  {
    constexpr std::array<paretto::dsp::SyncRate, 18> kRates = {
        paretto::dsp::SyncRate::R1_1,
        paretto::dsp::SyncRate::R1_2,
        paretto::dsp::SyncRate::R1_4,
        paretto::dsp::SyncRate::R1_8,
        paretto::dsp::SyncRate::R1_16,
        paretto::dsp::SyncRate::R1_32,
        paretto::dsp::SyncRate::R1_1_Dotted,
        paretto::dsp::SyncRate::R1_2_Dotted,
        paretto::dsp::SyncRate::R1_4_Dotted,
        paretto::dsp::SyncRate::R1_8_Dotted,
        paretto::dsp::SyncRate::R1_16_Dotted,
        paretto::dsp::SyncRate::R1_32_Dotted,
        paretto::dsp::SyncRate::R1_1_Triplet,
        paretto::dsp::SyncRate::R1_2_Triplet,
        paretto::dsp::SyncRate::R1_4_Triplet,
        paretto::dsp::SyncRate::R1_8_Triplet,
        paretto::dsp::SyncRate::R1_16_Triplet,
        paretto::dsp::SyncRate::R1_32_Triplet,
    };

    const int idx = static_cast<int>(loadParam(apvts_, ParamId::rate));
    const int safe = std::max(0, std::min(static_cast<int>(kRates.size()) - 1, idx));
    gateParams_.rate = kRates[static_cast<std::size_t>(safe)];
  }

  gateParams_.rateHz = loadParam(apvts_, ParamId::rateHz);

  {
    const int idx = static_cast<int>(loadParam(apvts_, ParamId::steps));
    gateParams_.steps = (idx == 0) ? paretto::gate::Steps::S8 : (idx == 2) ? paretto::gate::Steps::S32 : paretto::gate::Steps::S16;
  }

  gateParams_.swing = loadParam(apvts_, ParamId::swing);
  gateParams_.phaseDeg = loadParam(apvts_, ParamId::phaseDeg);

  gateParams_.depth = loadParam(apvts_, ParamId::depth);
  gateParams_.smoothMs = loadParam(apvts_, ParamId::smoothMs);
  gateParams_.floorDb = loadParam(apvts_, ParamId::floorDb);
  gateParams_.prob = loadParam(apvts_, ParamId::prob);
  gateParams_.accent = loadParam(apvts_, ParamId::accent);

  gateParams_.toneDepth = loadParam(apvts_, ParamId::toneDepth);
  gateParams_.toneBaseHz = loadParam(apvts_, ParamId::toneBaseHz);
  gateParams_.toneSpanOct = loadParam(apvts_, ParamId::toneSpanOct);
  gateParams_.res = loadParam(apvts_, ParamId::res);

  gateParams_.morph = loadParam(apvts_, ParamId::morph);

  {
    const int idx = static_cast<int>(loadParam(apvts_, ParamId::groove));
    const int safe = std::max(0, std::min(5, idx));
    gateParams_.groove = static_cast<paretto::gate::Groove>(safe);
  }

  gateParams_.mix = loadParam(apvts_, ParamId::mix);
  gateParams_.outputDb = loadParam(apvts_, ParamId::outputDb);

  gateEngine_.setParameters(gateParams_);
  gateEngine_.process(channelPtrs, numCh, buffer.getNumSamples(), ctx_);
}

bool AudioPluginAudioProcessor::hasEditor() const {
  return true;  // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor() {
  return new AudioPluginAudioProcessorEditor(*this);
}

void AudioPluginAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData) {
  auto state = apvts_.copyState();
  if (auto xml = state.createXml()) {
    copyXmlToBinary(*xml, destData);
  }
}

void AudioPluginAudioProcessor::setStateInformation(const void* data,
                                                    int sizeInBytes) {
  if (auto xml = getXmlFromBinary(data, sizeInBytes)) {
    apvts_.replaceState(juce::ValueTree::fromXml(*xml));
  }
}
}  // namespace audio_plugin

// This creates new instances of the plugin.
// This function definition must be in the global namespace.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new audio_plugin::AudioPluginAudioProcessor();
}
