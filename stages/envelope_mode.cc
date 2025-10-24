// Copyright 2025 Luke Zulauf.
//
// Author: Luke Zulauf (lzulauf@gmail.com)
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


#include "stages/envelope_mode.h"
#include "stages/ui.h"
#include "stages/settings.h"
#include "stages/macros.h"

using namespace stmlib;

namespace stages {

  const float kSliderMoveThreshold = 0.05f;
  const float kOneSecond = kSampleRate / kBlockSize;
  // Don't want to depend on chain_state (and also tick length is different here)
  // so just make our own that's also half a second
  const uint32_t kLongPressDuration = static_cast<uint32_t>(0.5f * kOneSecond);

  void EnvelopeMode::Init(Settings* settings) {
    settings_ = settings;
    envelope_manager_.Init(settings);
    ReInit();
  }

  void EnvelopeMode::ReInit() {
    // Don't process gates for half a second after boot or switching modes.
    // This is necessary because:
    // 1. There can be initial noise in the gate pins on startup.
    // 2. Sliders read 0 at first, causing erroneous detection of slider
    // movement.
    // Only necessary on startup, but simplifies code to apply on mode switching.
    warm_time_ = 2000;

    fill(switch_pressed_time_, switch_pressed_time_ + kNumChannels, 0);

    // The index of the currently selected envelope. 
    active_envelope_ = 0;

    // We can't read the slider positions directly in this method, so we record
    // that we need to set the initial slider positions upon next process step.
    need_to_set_initial_slider_positions_ = true;
    fill(&initial_slider_positions_[0], &initial_slider_positions_[kNumChannels], 0.0f);

    // Disable all sliders
    fill(&slider_enabled_[0], &slider_enabled_[kNumChannels], false);

    envelope_manager_.ReInit();
  }

  void EnvelopeMode::SetUI(Ui* ui) {
    ui_ = ui;
  }

  void EnvelopeMode::ProcessEnvelopes(IOBuffer::Block* block, size_t size) {
    switch (settings_->state().multimode) {
      case MULTI_MODE_SIX_INDEPENDENT_EGS:
        ProcessSixIndependentEgs(block, size);
        break;
      case MULTI_MODE_SIX_IDENTICAL_EGS:
        ProcessSixIdenticalEgs(block, size);
        break;
    }
  }  

  void EnvelopeMode::ProcessSixIndependentEgs(IOBuffer::Block* block, size_t size) {
    // Support six independant envelope generators
    //
    // Pressing a button corresponding to a non-active envelope generator sets
    // that envelope generator as the active envelope generator. Pressing the
    // button corresponding to the active envelope generator enables all
    // sliders.  Therefore, setting all sliders and then double tapping each
    // button will set six identical envelope generators.
    //
    // In general, switching egs does not immediately reflect the physical
    // position of sliders and pots. The user must move a slider sufficiently to
    // enable it. Activating a slider also activates its pot and cv inputs.
    // Slider leds will be green when that slider is enabled.
    //
    // If no input cable is plugged into an envelope generator's gate input, then
    // the previous envelope generator's gate input is used. This allows a single
    // input to trigger multiple envelope generators.

    // Don't do any processing during warmup.
    if (warm_time_ > 0) {
      --warm_time_;
      for (size_t ch = 0; ch < kNumChannels; ++ch) {
        for (size_t i = 0; i < size; ++i) {
          block->output[ch][i] = settings_->dac_code(ch, 0.f);
        }
      }
      return;
    }

    // If we need to set initial slider positions (from changing modes), record
    // the current slider positions.
    // Note that they will also be recorded when changing the active channel.
    if (need_to_set_initial_slider_positions_) {
      for (size_t ch = 0; ch < kNumChannels; ++ch) {
        initial_slider_positions_[ch] = block->slider[ch];
      }
      need_to_set_initial_slider_positions_ = false;
    }

    bool did_modify_state = false;

    // Handle for channel switch presses
    ForEachChannel(ch) {
      if (ui_->switches().pressed(ch)) {
        switch_pressed_time_[ch]++;
      } else {
        if (switch_pressed_time_[ch] > kLongPressDuration) {
          did_modify_state |= envelope_manager_.SetLooping(
            ch, !envelope_manager_.get_envelope(ch).IsLooping()
          );
        } else if (switch_pressed_time_[ch] > 0) {
          if (ch == active_envelope_) {
            // Pressing the active channel enables all sliders and pots
            fill(&slider_enabled_[0], &slider_enabled_[kNumChannels], true);
          } else {
            // Pressing an inactive channel switches to that channel
            // Record initial slider positions after switch and set all sliders
            // to inactive
            active_envelope_ = ch;
            fill(&slider_enabled_[0], &slider_enabled_[kNumChannels], false);
            ForEachChannel(slider) {
              initial_slider_positions_[slider] = block->slider[slider];
            }
          }
        }
        switch_pressed_time_[ch] = 0;
      }

      // Check if slider has moved sufficiently to enable the slider for the
      // active envelope.
      if (abs(block->slider[ch] - initial_slider_positions_[ch])
          > kSliderMoveThreshold) {
        slider_enabled_[ch] = true;
      }

      // Set Slider LED to indicate whether slider is active for curent
      // envelope.
      ui_->set_slider_led(ch, slider_enabled_[ch], 1);
    }

    if (slider_enabled_[0]) {
      did_modify_state |= envelope_manager_.SetDelayLength(active_envelope_, block->slider[0]);
    }
    if (slider_enabled_[1]) {
      did_modify_state |= envelope_manager_.SetAttackLength(active_envelope_, block->slider[1]);
      did_modify_state |= envelope_manager_.SetAttackCurve(active_envelope_, block->pot[1]);
    }
    if (slider_enabled_[2]) {
      did_modify_state |= envelope_manager_.SetHoldLength(active_envelope_, block->slider[2]);
    }
    if (slider_enabled_[3]) {
      did_modify_state |= envelope_manager_.SetDecayLength(active_envelope_, block->slider[3]);
      did_modify_state |= envelope_manager_.SetDecayCurve(active_envelope_, block->pot[3]);
    }
    if (slider_enabled_[4]) {
      did_modify_state |= envelope_manager_.SetSustainLevel(active_envelope_, block->slider[4]);
      did_modify_state |= envelope_manager_.SetSustainLength(active_envelope_, block->pot[4]);
    }
    if (slider_enabled_[5]) {
      did_modify_state |= envelope_manager_.SetReleaseLength(active_envelope_, block->slider[5]);
      did_modify_state |= envelope_manager_.SetReleaseCurve(active_envelope_, block->pot[5]);
    }
    // Start/Reset the save timer if state was modified
    if (did_modify_state) {
      settings_->SaveStateWithDebounce();
    }

    ProcessEGs(block, 0, size);
    if (envelope_manager_.get_envelope(active_envelope_).CurrentStage()
        == IDLE) {
      ui_->set_led(active_envelope_, LED_COLOR_YELLOW);
    }
  }

  void EnvelopeMode::ProcessSixIdenticalEgs(IOBuffer::Block* block, size_t size) {

    // Don't do any processing during warmup.
    if (warm_time_ > 0) {
      --warm_time_;
      for (size_t ch = 0; ch < kNumChannels; ++ch) {
        for (size_t i = 0; i < size; ++i) {
          block->output[ch][i] = settings_->dac_code(ch, 0.f);
        }
      }
      return;
    }

    // Slider LEDs
    ui_->set_slider_led(0, envelope_manager_.get_envelope(0).HasDelay  (), 1);
    ui_->set_slider_led(1, envelope_manager_.get_envelope(0).HasAttack (), 1);
    ui_->set_slider_led(2, envelope_manager_.get_envelope(0).HasHold   (), 1);
    ui_->set_slider_led(3, envelope_manager_.get_envelope(0).HasDecay  (), 1);
    ui_->set_slider_led(4, envelope_manager_.get_envelope(0).HasSustain(), 1);
    ui_->set_slider_led(5, envelope_manager_.get_envelope(0).HasRelease(), 1);

    // Set pots params
    envelope_manager_.SetAllAttackCurve (block->pot[1]);
    envelope_manager_.SetAllDecayCurve  (block->pot[3]);
    envelope_manager_.SetAllSustainLength(block->pot[4]);
    envelope_manager_.SetAllReleaseCurve(block->pot[5]);

    // Set slider params
    envelope_manager_.SetAllDelayLength  (block->cv_slider[0]);
    envelope_manager_.SetAllAttackLength (block->cv_slider[1]);
    envelope_manager_.SetAllHoldLength   (block->cv_slider[2]);
    envelope_manager_.SetAllDecayLength  (block->cv_slider[3]);
    envelope_manager_.SetAllSustainLevel (block->cv_slider[4]);
    envelope_manager_.SetAllReleaseLength(block->cv_slider[5]);

    uint32_t manual_gates = 0;
    ForEachChannel(ch) manual_gates |= ui_->switches().pressed(ch) << ch;
    ProcessEGs(block, manual_gates, size);
  }

  void EnvelopeMode::ProcessEGs(IOBuffer::Block* block, uint32_t manual_gates, size_t size) {
    // Process each channel
    uint32_t gates = 0;
    ForEachChannel(ch) {

      // Check for gates. If the input is not patched or manually triggered, the previous channel's
      // gate status will be used.
      if ((manual_gates >> ch) & 1) {
        gates = 0xffffffff; // only need `size` bits, but this is fine.
      } else if (block->input_patched[ch]) {
        gates = 0;
        for (size_t i = 0; i < size; i++) {
          // != 0 so we're not relying on GATE_FLAG_HIGH being 1.
          gates |= ((block->input[ch][i] & GATE_FLAG_HIGH)!=0) << i;
        }
      }

      Envelope& envelope = envelope_manager_.get_envelope(ch);

      // Set LED to indicate stage of the channel's envelope. Active channel is lit rather than off when idle.
      switch (envelope.CurrentStage()) {
        case DELAY:
        case ATTACK:
        case HOLD:
        case DECAY:
          ui_->set_led(ch, LED_COLOR_GREEN);
          break;
        case SUSTAIN:
          ui_->set_led(ch, LED_COLOR_YELLOW);
          break;
        case RELEASE:
          ui_->set_led(ch, LED_COLOR_RED);
          break;
        default:
          ui_->set_led(ch, LED_COLOR_OFF);
          break;
      }

      // Compute output values for each envelope
      for (size_t i = 0; i < size; i++) {
        float value = envelope.Value((gates >> i) & 1);
        block->output[ch][i] = settings_->dac_code(ch, value);
      }
    }
  }

}  // namespace stages
