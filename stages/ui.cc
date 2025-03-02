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
//
// -----------------------------------------------------------------------------
//
// User interface

#include <algorithm>

#include "stages/chain_state.h"
#include "stages/drivers/leds.h"
#include "stages/envelope_mode.h"
#include "stages/settings.h"
#include "stages/ui.h"

#include "stmlib/system/system_clock.h"

using namespace std;
using namespace stmlib;

const int32_t kLongPressDuration = 500;

namespace stages {

/* static */
const MultiMode Ui::multimodes_[6] = {
  MULTI_MODE_STAGES, // Mode enabled by long pressing the left-most button
  MULTI_MODE_STAGES_ADVANCED,
  MULTI_MODE_SIX_INDEPENDENT_EGS,
  MULTI_MODE_SIX_IDENTICAL_EGS,
  MULTI_MODE_OUROBOROS,
  MULTI_MODE_OUROBOROS_ALTERNATE, // Mode enabled by long pressing the right-most button
};

/* static */
const LedColor Ui::palette_[4] = {
  LED_COLOR_GREEN,
  LED_COLOR_YELLOW,
  LED_COLOR_RED,
  LED_COLOR_OFF,
};

void Ui::Init(Settings* settings, ChainState* chain_state, CvReader* cv_reader, EnvelopeMode* eg_mode) {
  leds_.Init();
  switches_.Init();

  system_clock.Init();
  fill(&press_time_[0], &press_time_[kNumSwitches], 0);
  fill(&press_time_multimode_toggle_[0], &press_time_multimode_toggle_[kNumSwitches], 0);

  settings_ = settings;
  mode_ = UI_MODE_NORMAL;
  chain_state_ = chain_state;
  cv_reader_ = cv_reader;
  eg_mode_ = eg_mode;

  if (switches_.pressed_immediate(0)) {
    State* state = settings_->mutable_state();
    if (state->color_blind == 1) {
      state->color_blind = 0;
    } else {
      state->color_blind = 1;
    }
    settings_->SaveStateWithDebounce();
  }

  fill(&slider_led_counter_[0], &slider_led_counter_[kNumLEDs], 0);
}

void Ui::Poll() {
  system_clock.Tick();
  UpdateLEDs();

  switches_.Debounce();

  MultiMode multimode = (MultiMode) settings_->state().multimode;


  // Forward presses information to chain state
  ChainState::ChannelBitmask pressed = 0;
  for (int i = 0; i < kNumSwitches; ++i) {
    if (switches_.pressed(i)) {
      pressed |= 1 << i;
    }
  }
  // This should get overwritten by SuspendSwitches if a mode switch or local
  // prop change is happening, so must happen first.
  chain_state_->set_local_switch_pressed(pressed);

  // TODO: This is gross. Each mode should have its own UI handler, with a
  // generic system for changing segment properties that each can leverage.
  bool changing_prop = false;
  if (pressed || //changing_pot_prop_ || changing_slider_prop_ ||
      cv_reader_->any_locked()) {
    uint16_t* seg_config = settings_->mutable_state()->segment_configuration;
    for (uint8_t i = 0; i < kNumChannels; ++i) {
      if (switches_.pressed(i)) {
        cv_reader_->Lock(i);
        float slider = cv_reader_->lp_slider(i);
        float pot = cv_reader_->lp_pot(i);
        float locked_slider = cv_reader_->locked_slider(i);
        float locked_pot = cv_reader_->locked_pot(i);
        bool input_patched = chain_state_->input_patched(i);
        not_patched_when_pressed_ |= (!input_patched) << i;

        uint16_t old_flags = seg_config[i];

        if (changing_slider_prop_ >> i & 1 // in the middle of change, so keep changing
            || fabsf(slider - locked_slider) > 0.05f) {

          changing_slider_prop_ |= 1 << i;

          if (settings_->in_seg_gen_mode()) {
            bool change_range = false;
            bool change_scale = false;
            bool has_audio_rate = false;
            switch (old_flags & 0x3) {
              case 0: // ramp
                change_range = true;
                has_audio_rate = chain_state_->loop_status(i) ==
                                 ChainState::LOOP_STATUS_SELF;
                break;
              case 3: // random
                if (chain_state_->loop_status(i) == ChainState::LOOP_STATUS_SELF
                    || !chain_state_->input_patched(i)) {
                  // Random LFO or chaotic
                  change_range = true;
                } else {
                  change_scale = true;
                }
                break;
              case 1: // step
              case 2: // hold
                change_scale = true;
                break;
              default: break;
            }

            if (change_range) {
              seg_config[i] &= ~0x0300; // reset range bits
              if (slider < 0.25f) {
                seg_config[i] |= 0x0100;
              } else if (has_audio_rate && slider > 0.999f) {
                seg_config[i] |= 0x0300;
              } else if (slider > 0.75f) {
                seg_config[i] |= 0x0200;
              }
              // default middle range is 0, so no else
            } else if (change_scale) {
              seg_config[i] &= ~0x3000; // reset quant scale bits
              seg_config[i] |= static_cast<uint8_t>(4 *  slider) << 12;
            }

          } else if (settings_->in_ouroboros_mode()) {

            seg_config[i] &= ~0x0c00; // reset range bits
            if (slider < 0.25f) {
              seg_config[i] |= 0x0800;
            } else if ((i == 0 && slider < 0.75f) || (i > 0 && slider > 0.75f)) {
              // high is default (0) in ouroboros root; otherwise middle is default
              seg_config[i] |= 0x0400;
            }
            if (i == 0 && seg_config[i] != old_flags) {
              for (uint8_t j=1; j < kNumChannels; j++) {
                // Reset all channels if root range changes
                seg_config[j] &= ~0x0c00;
              }
            }
          }
        }

        if( !(changing_pot_prop_ >> i & 1) && fabs(pot - locked_pot) > 0.05f) {
          // This is a toggle, so don't change if we've changed.
          changing_pot_prop_ |= 1 << i;
          switch (multimode) {
            case MULTI_MODE_STAGES:
            case MULTI_MODE_STAGES_ADVANCED:
            case MULTI_MODE_STAGES_SLOW_LFO:
              // toggle polarity
              seg_config[i] ^= 0b00001000;
              break;
            default:
              break;
          }
        }

        if (input_patched && ((not_patched_when_pressed_ >> i) & 1) &&
            settings_->in_seg_gen_mode()) {
          changing_gate_prop_ |= 1 << i;
          seg_config[i] |= 0x0080;
        }
        dirty_ = dirty_ || seg_config[i] != old_flags;
      } else if (cv_reader_->is_locked(i)) {
        changing_pot_prop_ &= ~(1 << i);
        changing_slider_prop_ &= ~(1 << i);

        float locked_slider = cv_reader_->locked_slider(i);
        float slider = cv_reader_->lp_slider(i);
        float locked_pot = cv_reader_->locked_pot(i);
        float pot = cv_reader_->lp_pot(i);

        bool unlock = true;
        if (fabsf(locked_slider - slider) > 0.01f) {
          unlock = false;
          // This should run about every ms, so 0.001f causes return to value
          // within, at most, 1 sec.
          float delta = 0.001f * (2.0f * (locked_slider < slider) - 0.5f);
          cv_reader_->set_locked_slider(i, locked_slider + delta);
        }
        if (fabsf(locked_pot - pot) > 0.01f) {
          unlock = false;
          float delta = 0.001f * (2.0f * (locked_pot < pot) - 0.5f);
          cv_reader_->set_locked_pot(i, locked_pot + delta);
        }
        if (unlock) {
          cv_reader_->Unlock(i);
        }
      }
    }
    changing_prop =
        changing_pot_prop_ || changing_slider_prop_ || changing_gate_prop_;
    // We're changing prop parameters, so ignore buttons for segment type
    // changes
    if (changing_prop) {
      chain_state_->SuspendSwitches();
    }
  } else {
    changing_gate_prop_ = 0;
    changing_slider_prop_ = 0;
    changing_pot_prop_ = 0;
    not_patched_when_pressed_ = 0;
    uint16_t* seg_config = settings_->mutable_state()->segment_configuration;
    for (uint8_t i = 0; i < kNumChannels; ++i) {
      if ((seg_config[i] & 0b10000000) && !chain_state_->input_patched(i) &&
          system_clock.milliseconds() > 5000) { // give time for gate detection to occur 
        seg_config[i] &= ~0b10000000;
        dirty_ = true;
      }
    }
  }
  if (!pressed && dirty_) {
    dirty_ = false;
    settings_->SaveStateWithDebounce();
  }

  if (settings_->in_ouroboros_mode()) {

    State* s = settings_->mutable_state();
    for (int i = 0; i < kNumSwitches; ++i) {
      if (changing_prop) {
        press_time_[i] = 0;
      } else if (switches_.pressed(i)) {
        if (press_time_[i] != -1) {
          ++press_time_[i];
        }
      } else {
        if (press_time_[i] > kLongPressDuration) { // Long-press
          if (press_time_[i] < kLongPressDurationForMultiModeToggle) { // But not long enough for multi-mode toggle
            s->segment_configuration[i] ^= 0b01000000; // Toggle waveshape MSB
            settings_->SaveStateWithDebounce();
          }
        } else if (press_time_[i] > 0) {
          uint8_t type_bits = (s->segment_configuration[i] & 0b00110000) >> 4; // Get current waveshape LSB number
          s->segment_configuration[i] &= ~0b00110000; // Reset waveshape LSB bits
          s->segment_configuration[i] |= (((type_bits + 1) % 3) << 4); // Cycle through 0,1,2 and set LSB bits
          settings_->SaveStateWithDebounce();
        }
        press_time_[i] = 0;
      }
    }
  }


  // Detect very long presses for multi-mode toggle (using a negative counter)
  if (tracking_multimode_ || pressed) {
    tracking_multimode_ = 0;
    for (uint8_t i = 0; i < kNumSwitches; ++i) {
      if (switches_.pressed(i) & !changing_prop) {
        if (press_time_multimode_toggle_[i] != -1) {
          ++press_time_multimode_toggle_[i];
          ++tracking_multimode_;
        }
        if (press_time_multimode_toggle_[i] > kLongPressDurationForMultiModeToggle) {
          MultiModeToggle(i);
          press_time_multimode_toggle_[i] = -1;
        }
      } else {
        press_time_multimode_toggle_[i] = 0;
      }
    }
  }
  if (settings_->PollSave()) {
    save_blink_counter_ = kSaveBlinkMs;
  }
}

void Ui::MultiModeToggle(const uint8_t i) {

  // Save the toggle value into permanent settings (if necessary)
  State* state = settings_->mutable_state();
  if (state->multimode != (uint8_t) multimodes_[i]) {
    for (int j = 0; j < kNumSwitches; ++j) {
      press_time_[j] = -1; // Don't consider Ouroboros button presses while changing mode
    }
    chain_state_->SuspendSwitches(); // Don't consider chain button presses while changing mode
    state->multimode = (uint8_t) multimodes_[i];
    settings_->SaveStateWithDebounce();
    chain_state_->start_reinit();
    eg_mode_->ReInit();
  }
}

inline uint8_t Ui::FadePattern(uint8_t shift, uint8_t phase) const {
  uint8_t x = system_clock.milliseconds() >> shift;
  x += phase;
  x &= 0x1f;
  return x <= 0x10 ? x : 0x1f - x;
}

inline uint8_t Ui::RampPattern(uint8_t shift, uint8_t phase) const {
  uint8_t x = system_clock.milliseconds() >> shift;
  x += phase;
  x &= 0x1f;
  return x > 0x0f ? 0x0f : 0x0f - x;
}


void Ui::UpdateLEDs() {
  leds_.Clear();

  MultiMode multimode = (MultiMode) settings_->state().multimode;
  ChainState::ChainStateStatus status = chain_state_->status();
  const uint32_t ms = system_clock.milliseconds();

  if (save_blink_counter_ > 0 ) {
    save_blink_counter_--; // LEDs already cleared, so don't need to do anything
  } else if (mode_ == UI_MODE_FACTORY_TEST) {

    size_t counter = (ms >> 8) % 3;
    for (size_t i = 0; i < kNumChannels; ++i) {
      if (slider_led_counter_[i] == 0) {
        leds_.set(LED_GROUP_UI + i, palette_[counter]);
        leds_.set(LED_GROUP_SLIDER + i,
            counter == 0 ? LED_COLOR_GREEN : LED_COLOR_OFF);
      } else if (slider_led_counter_[i] == 1) {
        leds_.set(LED_GROUP_UI + i, LED_COLOR_GREEN);
        leds_.set(LED_GROUP_SLIDER + i, LED_COLOR_OFF);
      } else {
        leds_.set(LED_GROUP_UI + i, LED_COLOR_GREEN);
        leds_.set(LED_GROUP_SLIDER + i, LED_COLOR_GREEN);
      }
    }

  } else if (status == ChainState::CHAIN_READY) {

    // LEDs update for original Stage modes (Stages, advanced, slow LFO variant and Ouroboros)
    if (settings_->in_ouroboros_mode() || settings_->in_seg_gen_mode()) {
      uint8_t pwm = ms & 0xf;
      uint8_t fade_patterns[4] = {
        0xf,  // NONE
        FadePattern(4, 0),  // START
        FadePattern(4, 0x0f),  // END
        FadePattern(4, 0x08),  // SELF
      };

      uint8_t lfo_patterns[4] = {
        FadePattern(4, 0x08), // Default, middle
        FadePattern(6, 0x08), // slow
        FadePattern(2, 0x08), // fast
        FadePattern(3, 0x08) // audio
      };

      for (size_t i = 0; i < kNumChannels; ++i) {
        uint16_t configuration = settings_->state().segment_configuration[i];
        int brightness = 0xf;
        if (settings_->in_ouroboros_mode()) {
          configuration = configuration >> 4; // slide to ouroboros bits
          brightness = fade_patterns[configuration & 0x4 ? 3 : 0];
        }
        uint8_t type = configuration & 0x3;
        bool self_loop = chain_state_->loop_status(i) == ChainState::LOOP_STATUS_SELF;
        LedColor color = palette_[type];
        if (settings_->in_seg_gen_mode()) {
          uint8_t speed = configuration >> 8 & 0x3;
          if (self_loop) {
            brightness = lfo_patterns[speed];
          } else {
            brightness = fade_patterns[chain_state_->loop_status(i)];
            if (type == 0) {
              if (speed == 1) {
                brightness = brightness * (RampPattern(5, 0x08) + 1) >> 5;
              } else if (speed == 2) {
                brightness = brightness * (RampPattern(7, 0x08) + 1) >> 5;
              }
            }
          }
          if (speed == segment::RANGE_AUDIO) {
            if ((ms >> 8) & 1) {
               color = LED_COLOR_RED;
               brightness >>= 2;
            }
          } else if (is_bipolar(configuration) && (((ms >> 8) & 0b11) == 0)) {
            color = LED_COLOR_RED;
            brightness = 0x1;
          }

          if ((changing_slider_prop_ & (1 << i)) && (
              type == 1
              || type == 2
              || (type == 3 && !self_loop))) {
            uint8_t scale = 3 - ((configuration >> 12) & 0x3);
            color = ((ms >> 6) & 0x1) == 0
              ? palette_[scale] : LED_COLOR_OFF;
          } else if (type == 3) {
            uint8_t proportion = (ms >> 7) & 15;
            proportion = proportion > 7 ? 15 - proportion : proportion;
            if ((ms & 7) < proportion) {
              color = LED_COLOR_GREEN;
            } else {
              color = LED_COLOR_RED;
            }
          }
        }
        if (settings_->state().color_blind == 1) {
          if (type == 0) {
            uint8_t modulation = FadePattern(6, 13 - (2 * i)) >> 1;
            brightness = brightness * (7 + modulation) >> 4;
          } else if (type == 1) {
            brightness = brightness >= 0x8 ? 0xf : 0;
          } else if (type == 2) {
            brightness = brightness >= 0xc ? 0x1 : 0;
          } else if (type == 3) {
            // Not sure how to make it distinct.
          }
        }
        uint32_t discrete_state_change_dur = ms - discrete_change_time_[i];

        if (discrete_state_change_dur <= kDiscreteStateBrightDur) {
          brightness = 0xf * (kDiscreteStateBrightDur - discrete_state_change_dur) / kDiscreteStateBrightDur
            + brightness * discrete_state_change_dur / kDiscreteStateBrightDur;
          if (discrete_state_change_dur <= kDiscreteStateBlinkDur + kDiscreteStatePreBlinkDur
              && discrete_state_change_dur > kDiscreteStatePreBlinkDur) {
            color = LED_COLOR_OFF;
          }
        }
        leds_.set(
            LED_GROUP_UI + i,
            (brightness >= pwm && brightness != 0) ? color : LED_COLOR_OFF);
        leds_.set(
            LED_GROUP_SLIDER + i,
            slider_led_counter_[i] ? LED_COLOR_GREEN : LED_COLOR_OFF);
      }

    } else if (multimode == MULTI_MODE_SIX_IDENTICAL_EGS ||
               multimode == MULTI_MODE_SIX_INDEPENDENT_EGS) {
      // LEDs update for 6EG and 6IEG modes
      for (size_t i = 0; i < kNumChannels; ++i) {
        leds_.set(LED_GROUP_UI + i, led_color_[i]);
        leds_.set(LED_GROUP_SLIDER + i, slider_led_counter_[i] ? LED_COLOR_GREEN : LED_COLOR_OFF);
      }

    } else {

      // Invalid mode, turn all off
      for (size_t i = 0; i < kNumChannels; ++i) {
        leds_.set(LED_GROUP_UI + i, LED_COLOR_OFF);
        leds_.set(LED_GROUP_SLIDER + i, LED_COLOR_OFF);
      }

    }

    // For any multi-mode, update slider LEDs counters
    for (size_t i = 0; i < kNumChannels; ++i) {
      if (slider_led_counter_[i]) {
        --slider_led_counter_[i];
      }
    }

    if (cv_reader_->any_locked()) {
      for (size_t i = 0; i < kNumChannels; ++i) {
        // Turn off LEDs if in limbo
        if (cv_reader_->is_locked(i)) {
          uint8_t slider_dimness = static_cast<uint8_t>(
              8 * fabs(cv_reader_->locked_slider(i) - cv_reader_->lp_slider(i)));
          leds_.set(LED_GROUP_SLIDER + i,
              (ms & 0x07) < slider_dimness ? LED_COLOR_OFF : LED_COLOR_GREEN );

          uint8_t pot_dimness = static_cast<uint8_t>(
              8 * fabs(cv_reader_->locked_pot(i) - cv_reader_->lp_pot(i)));

          if ((ms & 0x07) < pot_dimness) {
            leds_.set(LED_GROUP_UI + i, LED_COLOR_OFF);
          }
        }
      }
    }
  } else if (chain_state_->status() == ChainState::CHAIN_REINITIALIZING) {
    show_mode();
  } else if (chain_state_-> status() == ChainState::CHAIN_DISCOVERING_NEIGHBORS) {
    size_t counter = ms >> 5;
    size_t n = chain_state_->size() * kNumChannels;
    counter = counter % (2 * n - 2);
    if (counter >= n) {
      counter = 2 * n - 2 - counter;
    }
    if (counter >= chain_state_->index() * kNumChannels) {
      counter -= chain_state_->index() * kNumChannels;
      if (counter < kNumChannels) {
        leds_.set(LED_GROUP_UI + counter, LED_COLOR_YELLOW);
        leds_.set(LED_GROUP_SLIDER + counter, LED_COLOR_GREEN);
      }
    }
    show_mode();
  }
  leds_.Write();

}

}  // namespace stages
