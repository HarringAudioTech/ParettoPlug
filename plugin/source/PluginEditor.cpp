#include "PluginEditor.h"
#include "PluginProcessor.h"

namespace audio_plugin {
namespace {
namespace ParamId {
constexpr const char* syncEnabled = "syncEnabled";
constexpr const char* rate = "rate";
constexpr const char* rateHz = "rateHz";
constexpr const char* steps = "steps";
constexpr const char* groove = "groove";
constexpr const char* depth = "depth";
constexpr const char* smoothMs = "smoothMs";
constexpr const char* floorDb = "floorDb";
constexpr const char* mix = "mix";
constexpr const char* outputDb = "outputDb";
}  // namespace ParamId

static void initSlider(juce::Slider& s) {
  s.setSliderStyle(juce::Slider::LinearHorizontal);
  s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 20);
}

static void initLabel(juce::Label& l, const juce::String& text, juce::Component& attachTo) {
  l.setText(text, juce::dontSendNotification);
  l.attachToComponent(&attachTo, true);
  l.setJustificationType(juce::Justification::centredLeft);
}
}  // namespace

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(
    AudioPluginAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p) {
  auto& apvts = processorRef.getAPVTS();

  syncToggle_.setButtonText("Sync");
  addAndMakeVisible(syncToggle_);

  rateChoice_.addItemList(
      juce::StringArray{"1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/1D", "1/2D", "1/4D", "1/8D", "1/16D",
                        "1/32D", "1/1T", "1/2T", "1/4T", "1/8T", "1/16T", "1/32T"},
      1);
  rateChoice_.setTextWhenNothingSelected("Rate");
  addAndMakeVisible(rateChoice_);

  stepsChoice_.addItemList(juce::StringArray{"8", "16", "32"}, 1);
  stepsChoice_.setTextWhenNothingSelected("Steps");
  addAndMakeVisible(stepsChoice_);

  grooveChoice_.addItemList(juce::StringArray{"Straight", "Swing", "UKG", "Jersey", "DnB", "RandomMicro"}, 1);
  grooveChoice_.setTextWhenNothingSelected("Groove");
  addAndMakeVisible(grooveChoice_);

  initSlider(rateHzSlider_);
  addAndMakeVisible(rateHzSlider_);
  initLabel(rateHzLabel_, "Rate Hz", rateHzSlider_);

  initSlider(depthSlider_);
  addAndMakeVisible(depthSlider_);
  initLabel(depthLabel_, "Depth", depthSlider_);

  initSlider(floorDbSlider_);
  addAndMakeVisible(floorDbSlider_);
  initLabel(floorDbLabel_, "Floor dB", floorDbSlider_);

  initSlider(smoothMsSlider_);
  addAndMakeVisible(smoothMsSlider_);
  initLabel(smoothMsLabel_, "Smooth ms", smoothMsSlider_);

  initSlider(mixSlider_);
  addAndMakeVisible(mixSlider_);
  initLabel(mixLabel_, "Mix", mixSlider_);

  initSlider(outputDbSlider_);
  addAndMakeVisible(outputDbSlider_);
  initLabel(outputDbLabel_, "Output dB", outputDbSlider_);

  syncAttachment_ = std::make_unique<APVTS::ButtonAttachment>(apvts, ParamId::syncEnabled, syncToggle_);
  rateAttachment_ = std::make_unique<APVTS::ComboBoxAttachment>(apvts, ParamId::rate, rateChoice_);
  stepsAttachment_ = std::make_unique<APVTS::ComboBoxAttachment>(apvts, ParamId::steps, stepsChoice_);
  grooveAttachment_ = std::make_unique<APVTS::ComboBoxAttachment>(apvts, ParamId::groove, grooveChoice_);
  rateHzAttachment_ = std::make_unique<APVTS::SliderAttachment>(apvts, ParamId::rateHz, rateHzSlider_);
  depthAttachment_ = std::make_unique<APVTS::SliderAttachment>(apvts, ParamId::depth, depthSlider_);
  floorDbAttachment_ = std::make_unique<APVTS::SliderAttachment>(apvts, ParamId::floorDb, floorDbSlider_);
  smoothMsAttachment_ = std::make_unique<APVTS::SliderAttachment>(apvts, ParamId::smoothMs, smoothMsSlider_);
  mixAttachment_ = std::make_unique<APVTS::SliderAttachment>(apvts, ParamId::mix, mixSlider_);
  outputDbAttachment_ = std::make_unique<APVTS::SliderAttachment>(apvts, ParamId::outputDb, outputDbSlider_);

  // Make sure that before the constructor has finished, you've set the
  // editor's size to whatever you need it to be.
  setSize(520, 260);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {}

void AudioPluginAudioProcessorEditor::paint(juce::Graphics& g) {
  // (Our component is opaque, so we must completely fill the background with a
  // solid colour)
  g.fillAll(
      getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

  g.setColour(juce::Colours::white);
  g.setFont(15.0f);
  auto header = getLocalBounds().removeFromTop(28);
  g.drawFittedText("ParettoGate", header, juce::Justification::centred, 1);
}

void AudioPluginAudioProcessorEditor::resized() {
  // This is generally where you'll want to lay out the positions of any
  // subcomponents in your editor..

  auto r = getLocalBounds().reduced(12);
  r.removeFromTop(28);
  r.removeFromTop(8);

  auto topRow = r.removeFromTop(28);
  syncToggle_.setBounds(topRow.removeFromLeft(90));
  topRow.removeFromLeft(10);
  rateChoice_.setBounds(topRow.removeFromLeft(160));
  topRow.removeFromLeft(10);
  stepsChoice_.setBounds(topRow.removeFromLeft(100));
  topRow.removeFromLeft(10);
  grooveChoice_.setBounds(topRow.removeFromLeft(140));

  r.removeFromTop(10);

  constexpr int labelW = 90;
  const int rowH = 26;
  const int rowGap = 6;

  auto placeRow = [&](juce::Slider& s) {
    auto row = r.removeFromTop(rowH);
    row.removeFromLeft(labelW);
    s.setBounds(row);
    r.removeFromTop(rowGap);
  };

  placeRow(rateHzSlider_);
  placeRow(depthSlider_);
  placeRow(floorDbSlider_);
  placeRow(smoothMsSlider_);
  placeRow(mixSlider_);
  placeRow(outputDbSlider_);
}
}  // namespace audio_plugin
