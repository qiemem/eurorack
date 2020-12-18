// Copyright 2017 Emilie Gillet.
//
// Author: Emilie Gillet (emilie.o.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// See http://creativecommons.org/licenses/MIT/ for more information.

#include <stm32f37x_conf.h>

#include "stmlib/dsp/dsp.h"

#include "stmlib/dsp/dsp.h"
#include "stmlib/dsp/units.h"
#include "stages/chain_state.h"
#include "stages/drivers/dac.h"
#include "stages/drivers/gate_inputs.h"
#include "stages/drivers/leds.h"
#include "stages/drivers/serial_link.h"
#include "stages/drivers/system.h"
#include "stages/cv_reader.h"
#include "stages/factory_test.h"
#include "stages/io_buffer.h"
#include "stages/oscillator.h"
#include "stages/resources.h"
#include "stages/envelope.h"
#include "stages/segment_generator.h"
#include "stages/settings.h"
#include "stages/ui.h"

using namespace stages;
using namespace std;
using namespace stmlib;

const bool skip_factory_test = false;
const bool test_adc_noise = false;

ChainState chain_state;
CvReader cv_reader;
Dac dac;
FactoryTest factory_test;
GateFlags no_gate[kBlockSize];
GateInputs gate_inputs;
SegmentGenerator segment_generator[kNumChannels];
Oscillator oscillator[kNumChannels];
IOBuffer io_buffer;
Envelope eg[kNumChannels];
SerialLink left_link;
SerialLink right_link;
Settings settings;
Ui ui;

// Default interrupt handlers.
extern "C" {

  void NMI_Handler() { }
  void HardFault_Handler() { while (1); }
  void MemManage_Handler() { while (1); }
  void BusFault_Handler() { while (1); }
  void UsageFault_Handler() { while (1); }
  void SVC_Handler() { }
  void DebugMon_Handler() { }
  void PendSV_Handler() { }

}

// SysTick and 32kHz handles
extern "C" {

  void SysTick_Handler() {
    IWDG_ReloadCounter();
    ui.Poll();
    if (!skip_factory_test) {
      factory_test.Poll();
    }
  }

}

IOBuffer::Slice FillBuffer(size_t size) {
  IOBuffer::Slice s = io_buffer.NextSlice(size);
  gate_inputs.Read(s, size);
  if (io_buffer.new_block()) {
    cv_reader.Read(s.block);
    gate_inputs.ReadNormalization(s.block);
  }
  return s;
}

SegmentGenerator::Output out[kBlockSize];

static float note_lp[kNumChannels] = { 0, 0, 0, 0, 0, 0 };

void Process(IOBuffer::Block* block, size_t size) {
  chain_state.Update(
      *block,
      &settings,
      &segment_generator[0],
      out);
  for (size_t channel = 0; channel < kNumChannels; ++channel) {
    bool led_state = segment_generator[channel].Process(
        block->input_patched[channel] ? block->input[channel] : no_gate,
        out,
        size);
    ui.set_slider_led(channel, led_state, 5);

    if (test_adc_noise) {
      float note = block->cv_slider[channel];
      ONE_POLE(note_lp[channel], note, 0.0001f);
      float cents = (note - note_lp[channel]) * 1200.0f * 0.5f;
      CONSTRAIN(cents, -1.0f, +1.0f)
      for (size_t i = 0; i < size; ++i) {
        out[i].value = cents;
      }
    }

    for (size_t i = 0; i < size; ++i) {
      block->output[channel][i] = settings.dac_code(channel, out[i].value);
    }
  }
}

void ProcessSixEg(IOBuffer::Block* block, size_t size) {

  // Slider LEDs
  ui.set_slider_led(0, eg[0].HasDelay  (), 1);
  ui.set_slider_led(1, eg[0].HasAttack (), 1);
  ui.set_slider_led(2, eg[0].HasHold   (), 1);
  ui.set_slider_led(3, eg[0].HasDecay  (), 1);
  ui.set_slider_led(4, eg[0].HasSustain(), 1);
  ui.set_slider_led(5, eg[0].HasRelease(), 1);

  // Wait 1sec at boot before checking gates
  static int egGateWarmTime = 4000;
  if (egGateWarmTime > 0) egGateWarmTime--;

  for (size_t ch = 0; ch < kNumChannels; ch++) {

    // Set pots params
    eg[ch].SetAttackCurve (block->pot[1]);
    eg[ch].SetDecayCurve  (block->pot[3]);
    eg[ch].SetReleaseCurve(block->pot[5]);

    // Set slider params
    eg[ch].SetDelayLength  (block->cv_slider[0]);
    eg[ch].SetAttackLength (block->cv_slider[1]);
    eg[ch].SetHoldLength   (block->cv_slider[2]);
    eg[ch].SetDecayLength  (block->cv_slider[3]);
    eg[ch].SetSustainLevel (block->cv_slider[4]);
    eg[ch].SetReleaseLength(block->cv_slider[5]);

    // Gate or button?
    bool gate = ui.switches().pressed(ch);
    if (!gate && egGateWarmTime == 0 && block->input_patched[ch]) {
      for (size_t i = 0; i < size; i++) {
        if (block->input[ch][i] & GATE_FLAG_HIGH) {
          gate = true;
          break;
        }
      }
    }
    eg[ch].Gate(gate);
    ui.set_led(ch, gate ? LED_COLOR_RED : LED_COLOR_OFF);

    // Compute value and set as output
    float value = eg[ch].Value();
    for (size_t i = 0; i < size; i++) {
      block->output[ch][i] = settings.dac_code(ch, value);
    }

    // Display current stage
    switch (eg[ch].CurrentStage()) {
      case DELAY:
      case ATTACK:
      case HOLD:
      case DECAY:
        ui.set_led(ch, LED_COLOR_GREEN);
        break;
      case SUSTAIN:
        ui.set_led(ch, LED_COLOR_YELLOW);
        break;
      case RELEASE:
        ui.set_led(ch, LED_COLOR_RED);
        break;
      default:
        ui.set_led(ch, LED_COLOR_OFF);
        break;
    }

  }

}

const int kNumOuroborosRatiosHigh = 17;
float ouroboros_ratios_high[] = {
  1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f,
  10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 16.0f
};

const int kNumOuroborosRatiosLow = 17;
float ouroboros_ratios_low[] = {
  (1.0f / 16.0f), (1.0f / 15.0f), (1.0f / 14.0f), (1.0f / 13.0f),
  (1.0f / 12.0f), (1.0f / 11.0f), (1.0f / 10.0f), (1.0f / 9.0f),
  (1.0f / 8.0f), (1.0f / 7.0f), (1.0f / 6.0f), (1.0f / 5.0f),
  (1.0f / 4.0f), (1.0f / 3.0f), (1.0f / 2.0f), 1.0f, 1.0f
};

const int kNumOuroborosRatios = 11;
float ouroboros_ratios[] = {
  0.25f, 0.5f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 8.0f, 8.0f
};

float this_channel[kBlockSize];
float sum[kBlockSize];
float channel_amplitude[kNumChannels];
float previous_amplitude[kNumChannels];

void ProcessOuroboros(IOBuffer::Block* block, size_t size) {
  const float coarse = (block->cv_slider[0] - 0.5f) * 96.0f;
  const float fine = block->pot[0] * 2.0f - 1.0f;
  const uint16_t* config= settings.state().segment_configuration;
  const uint8_t range = (config[0] >> 10) & 0x3;
  const float range_mult =
    (range == 0x01) ? 1.0f / 128.0f :
    (range == 0x02) ? 1.0f / (128.0f * 16.0f) :
    1.0f;
  const bool lfo = range != 0x00;
  const float f0 = SemitonesToRatio(coarse + fine) * 261.6255f / kSampleRate * range_mult;

  std::fill(&sum[0], &sum[size], 0.0f);

  bool alternate = (MultiMode) settings.state().multimode == MULTI_MODE_OUROBOROS_ALTERNATE;
  float *blockHarmonic = alternate ? block->cv_slider : block->pot;
  float *blockAmplitude = alternate ? block->pot : block->cv_slider;

  for (int channel = kNumChannels - 1; channel >= 0; --channel) {

    float* ratios = ouroboros_ratios;
    int num_ratios = kNumOuroborosRatios;

    // TODO: This is too costly performance wise.  Could try switching to and
    // array that stores the ratios for each channel, which is set in ui.cc
    if (channel > 0) {
      const uint8_t r = (config[channel] >> 10) & 0x3;
      if (r == 0) {
        ratios = ouroboros_ratios_high;
        num_ratios = kNumOuroborosRatiosHigh;
      } else if (r == 2) {
        ratios = ouroboros_ratios_low;
        num_ratios = kNumOuroborosRatiosLow;
      }
    }
    const float harmonic = blockHarmonic[channel] * (num_ratios - 1.001f);
    MAKE_INTEGRAL_FRACTIONAL(harmonic);
    harmonic_fractional = 8.0f * (harmonic_fractional - 0.5f) + 0.5f;
    CONSTRAIN(harmonic_fractional, 0.0f, 1.0f);
    // harmonic_integral can go out of bounds with CV if harmonics set to cv_slider.
    CONSTRAIN(harmonic_integral, 0, num_ratios - 2);
    const float ratio = channel == 0 ? 1.0f : Crossfade(
        ratios[harmonic_integral],
        ratios[harmonic_integral + 1],
        harmonic_fractional);

    const float amplitude = channel == 0
        ? 1.0f
        : std::max(blockAmplitude[channel] - 0.01f, 0.0f);

    bool trigger = false;
    for (size_t i = 0; i < size; ++i) {
      trigger = trigger || (block->input[channel][i] & GATE_FLAG_RISING);
    }
    if (trigger || !block->input_patched[channel]) {
      channel_amplitude[channel] = 1.0f;
    } else {
      channel_amplitude[channel] *= 0.999f;
    }
    // For some reason, trigger can be true when not input is patched.
    if (block->input_patched[channel] && trigger && lfo) {
      oscillator[channel].Init();
    }
    ui.set_slider_led(
        channel, channel_amplitude[channel] * amplitude > 0.02f, 1);
    const float f = f0 * ratio;

    uint8_t waveshape = (settings.state().segment_configuration[channel] & 0b01110000) >> 4;
    switch (waveshape) {
      case 0:
        oscillator[channel].Render<OSCILLATOR_SHAPE_SINE>(
            f, 0.5f, this_channel, size);
        break;
      case 1:
        oscillator[channel].Render<OSCILLATOR_SHAPE_TRIANGLE>(
            f, 0.5f, this_channel, size);
        break;
      case 2:
      case 3:
        oscillator[channel].Render<OSCILLATOR_SHAPE_SQUARE>(
            f, 0.5f, this_channel, size);
        break;
      case 4:
        oscillator[channel].Render<OSCILLATOR_SHAPE_SAW>(
            f, 0.5f, this_channel, size);
        break;
      case 5:
        oscillator[channel].Render<OSCILLATOR_SHAPE_SQUARE>(
            f, 0.75f, this_channel, size);
        break;
      case 6:
      case 7:
        oscillator[channel].Render<OSCILLATOR_SHAPE_SQUARE>(
            f, 0.9f, this_channel, size);
        break;
    }

    ParameterInterpolator am(
        &previous_amplitude[channel],
        amplitude * amplitude * channel_amplitude[channel],
        size);
    for (size_t i = 0; i < size; ++i) {
      sum[i] += this_channel[i] * am.Next();
    }

    const float gain = channel == 0 ? 0.2f : 0.66f;
    // Don't bother interpolating over lfo amplitude as we don't apply pinging to the single LFO outs
    const float lfo_amp = lfo ? amplitude : 1.0f;
    const float* source = channel == 0 ? sum : this_channel;
    for (size_t i = 0; i < size; ++i) {
      block->output[channel][i] = settings.dac_code(channel, source[i] * gain * lfo_amp);
    }
  }
}

void ProcessTest(IOBuffer::Block* block, size_t size) {

  for (size_t channel = 0; channel < kNumChannels; channel++) {

    // Pot position affects LED color
    const float pot = block->pot[channel];
    ui.set_led(channel, pot > 0.5f ? LED_COLOR_GREEN : LED_COLOR_OFF);

    // Gete input and button turn the LED red
    bool gate = false;
    bool button = ui.switches().pressed(channel);
    if (block->input_patched[channel]) {
      for (size_t i = 0; i < size; i++) {
        gate = gate || (block->input[channel][i] & GATE_FLAG_HIGH);
      }
    }
    if (gate || button) {
      ui.set_led(channel, LED_COLOR_RED);
    }

    // Slider position (summed with input CV) affects output value
    const float output = (gate || button) ? 1.0f : block->cv_slider[channel];
    ui.set_slider_led(channel, output > 0.001f, 1);
    for (size_t i = 0; i < size; i++) {
      block->output[channel][i] = settings.dac_code(channel, output);
    }

  }

}

void Init() {
  System sys;
  sys.Init(true);
  dac.Init(int(kSampleRate), 2);
  gate_inputs.Init();
  io_buffer.Init();

  bool freshly_baked = !settings.Init();
  for (size_t i = 0; i < kNumChannels; ++i) {
    segment_generator[i].Init(&settings);
    eg[i].Init();
    oscillator[i].Init();
  }
  std::fill(&no_gate[0], &no_gate[kBlockSize], GATE_FLAG_LOW);

  cv_reader.Init(&settings, &chain_state);

  ui.Init(&settings, &chain_state, &cv_reader);

  if (freshly_baked && !skip_factory_test) {
    factory_test.Start(&settings, &cv_reader, &gate_inputs, &ui);
    ui.set_factory_test(true);
  } else {
    chain_state.Init(&left_link, &right_link, settings);
  }

  sys.StartTimers();
  dac.Start(&FillBuffer);
}

int main(void) {

  Init();

  while (1) {
    if (factory_test.running()) {
      io_buffer.Process(&FactoryTest::ProcessFn);
    } else if (
        chain_state.status() == stages::ChainState::CHAIN_DISCOVERING_NEIGHBORS
        || chain_state.status() == stages::ChainState::CHAIN_REINITIALIZING) {
      io_buffer.Process(&Process); // Still discovering neighbors, dont't process alternative multi-modes
    } else {
      switch ((MultiMode) settings.state().multimode) {
        case MULTI_MODE_SIX_EG:
          io_buffer.Process(&ProcessSixEg);
          break;
        case MULTI_MODE_OUROBOROS:
        case MULTI_MODE_OUROBOROS_ALTERNATE:
          io_buffer.Process(&ProcessOuroboros);
          break;
        default:
          io_buffer.Process(&Process);
          break;
      }
    }
  }

}
