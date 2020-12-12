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

#include "stages/ui.h"
#include "stages/chain_state.h"

#include <algorithm>

#include "stages/drivers/leds.h"
#include "stages/settings.h"
#include "stmlib/system/system_clock.h"

using namespace std;
using namespace stmlib;

const int32_t kLongPressDuration = 500;

namespace stages {

/* static */
const MultiMode Ui::multimodes_[6] = {
  MULTI_MODE_STAGES, // Mode enabled by long pressing the left-most button
  MULTI_MODE_STAGES_ADVANCED,
  MULTI_MODE_STAGES_SLOW_LFO,
  MULTI_MODE_SIX_EG,
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

void Ui::Init(Settings* settings, ChainState* chain_state, CvReader* cv_reader) {
  leds_.Init();
  switches_.Init();

  system_clock.Init();
  fill(&press_time_[0], &press_time_[kNumSwitches], 0);
  fill(&press_time_multimode_toggle_[0], &press_time_multimode_toggle_[kNumSwitches], 0);

  settings_ = settings;
  mode_ = UI_MODE_NORMAL;
  chain_state_ = chain_state;
  cv_reader_ = cv_reader;

  if (switches_.pressed_immediate(0)) {
    State* state = settings_->mutable_state();
    if (state->color_blind == 1) {
      state->color_blind = 0;
    } else {
      state->color_blind = 1;
    }
    settings_->SaveState();
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
  //if (multimode == MULTI_MODE_STAGES || multimode == MULTI_MODE_STAGES_SLOW_LFO) {
    for (int i = 0; i < kNumSwitches; ++i) {
      if (switches_.pressed(i)) {
        pressed |= 1 << i;
      }
    }
  //}
  // This should get overwritten by SuspendSwitches if a mode switch or local
  // prop change is happening, so must happen first.
  chain_state_->set_local_switch_pressed(pressed);

  // TODO: This is gross. Each mode should have its own UI handler, with a
  // generic system for changing segment properties that each can leverage.
  uint8_t changing_prop = 0;
  bool dirty = false;
  uint16_t* seg_config = settings_->mutable_state()->segment_configuration;
  for (uint8_t i = 0; i < kNumChannels; ++i) {
    if (switches_.pressed(i)) {
      cv_reader_->Lock(i);
      float slider = cv_reader_->lp_slider(i);
      CONSTRAIN(slider, 0.0f, 0.9999f);
      float pot = cv_reader_->lp_pot(i);
      CONSTRAIN(pot, 0.0f, 0.9999f);

      float locked_slider = cv_reader_->locked_slider(i);
      float locked_pot = cv_reader_->locked_pot(i);

      uint16_t old_flags = seg_config[i];

      if (changing_slider_prop_ >> i & 1 // in the middle of change, so keep changing
          || fabs(slider - locked_slider) > 0.05f) {
        changing_slider_prop_ |= 1 << i;

        if (settings_->in_seg_gen_mode()) {
          switch (seg_config[i] & 0x3) {
            case 0: // ramp
              seg_config[i] &= ~0x0300; // reset range bits
              if (slider < 0.25) {
                seg_config[i] |= 0x0100;
              } else if (slider > 0.75) {
                seg_config[i] |= 0x0200;
              }
              // default middle range is 0, so no else
              break;
            case 3: // random
              if (chain_state_->loop_status(i) == ChainState::LOOP_STATUS_SELF) {
                seg_config[i] &= ~0x0300; // reset range bits
                if (slider < 0.25) {
                  seg_config[i] |= 0x0100;
                } else if (slider > 0.75) {
                  seg_config[i] |= 0x0200;
                }
                // default middle range is 0, so no else
              }
              break;
            case 1: // step
            case 2: // hold
              seg_config[i] &= ~0x3000; // reset quant scale bits
              seg_config[i] |= static_cast<uint8_t>(4 *  slider) << 12;
              break;
            default: break;
          }
        } else if (settings_->in_ouroboros_mode()) {
          seg_config[i] &= ~0x0c00; // reset range bits
          if (slider < 0.25) {
            seg_config[i] |= 0x0800;
          } else if (slider < 0.75) { // high is default in ouroboros
            seg_config[i] |= 0x0400;
          }
          break;
        }
      }

      if(
          !(changing_pot_prop_ >> i & 1) // This is a toggle, so don't change if we've changed.
          && fabs(pot - locked_pot) > 0.05f) {

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
      dirty = dirty || seg_config[i] != old_flags;
    } else {
      changing_pot_prop_ &= ~(1 << i);
      changing_slider_prop_ &= ~(1 << i);
      cv_reader_->Unlock(i);
    }
  }
  if (dirty) {
    settings_->SaveState();
  }
  changing_prop = changing_pot_prop_ | changing_slider_prop_;
  // We're changing prop parameters
  if (changing_prop) {
    chain_state_->SuspendSwitches();
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
            settings_->SaveState();
          }
        } else if (press_time_[i] > 0) {
          uint8_t type_bits = (s->segment_configuration[i] & 0b00110000) >> 4; // Get current waveshape LSB number
          s->segment_configuration[i] &= ~0b00110000; // Reset waveshape LSB bits
          s->segment_configuration[i] |= (((type_bits + 1) % 3) << 4); // Cycle through 0,1,2 and set LSB bits
          settings_->SaveState();
        }
        press_time_[i] = 0;
      }
    }
  }


  // Detect very long presses for multi-mode toggle (using a negative counter)
  for (uint8_t i = 0; i < kNumSwitches; ++i) {
    if (switches_.pressed(i) & !changing_prop) {
      if (press_time_multimode_toggle_[i] != -1) {
        ++press_time_multimode_toggle_[i];
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

void Ui::MultiModeToggle(const uint8_t i) {

  // Save the toggle value into permanent settings (if necessary)
  State* state = settings_->mutable_state();
  if (state->multimode != (uint8_t) multimodes_[i]) {
    for (int j = 0; j < kNumSwitches; ++j) {
      press_time_[j] = -1; // Don't consider Ouroboros button presses while changing mode
    }
    chain_state_->SuspendSwitches(); // Don't consider chain button presses while changing mode
    state->multimode = (uint8_t) multimodes_[i];
    settings_->SaveState();
    chain_state_->start_reinit();
  }
}

inline uint8_t Ui::FadePattern(uint8_t shift, uint8_t phase, bool ramp) const {
  uint8_t x = system_clock.milliseconds() >> shift;
  x += phase;
  if (ramp) { // produce a downward ramp pattern with a delay
    x &= 0x1f;
    return x > 0x0f ? 0x0f : 0x0f - x;
  } else { // produce a triangular pattern
    x &= 0x1f;
    return x <= 0x10 ? x : 0x1f - x;
  }
}

void Ui::UpdateLEDs() {
  leds_.Clear();

  MultiMode multimode = (MultiMode) settings_->state().multimode;

  if (mode_ == UI_MODE_FACTORY_TEST) {

    size_t counter = (system_clock.milliseconds() >> 8) % 3;
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

  } else if (chain_state_->status() == ChainState::CHAIN_REINITIALIZING) {
    show_mode();
  } else if (chain_state_-> status() == ChainState::CHAIN_DISCOVERING_NEIGHBORS) {
    size_t counter = system_clock.milliseconds() >> 5;
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
  } else {

    // LEDs update for original Stage modes (Stages, advanced, slow LFO variant and Ouroboros)
    if (settings_->in_ouroboros_mode() || settings_->in_seg_gen_mode()) {
      uint8_t pwm = system_clock.milliseconds() & 0xf;
      uint8_t fade_patterns[4] = {
        0xf,  // NONE
        FadePattern(4, 0, false),  // START
        FadePattern(4, 0x0f, false),  // END
        FadePattern(4, 0x08, false),  // SELF
      };

      uint8_t lfo_patterns[3] = {
        FadePattern(4, 0x08, false), // Default, middle
        FadePattern(6, 0x08, false), // slow
        FadePattern(2, 0x08, false), // fast
      };

      uint8_t ramp_patterns[3] = {
        0xf,  // none
        FadePattern(5, 0x08, true), // fast ramp
        FadePattern(7, 0x08, true), // slow ramp
      };

      for (size_t i = 0; i < kNumChannels; ++i) {
        uint16_t configuration = settings_->state().segment_configuration[i];
        int brightness = 0xf;
        if (settings_->in_ouroboros_mode()) {
          configuration = configuration >> 4; // slide to ouroboros bits
          brightness = fade_patterns[configuration & 0x4 ? 3 : 0];
        }
        uint8_t type = configuration & 0x3;
        LedColor color = palette_[type];
        if (settings_->in_seg_gen_mode()) {
          if (chain_state_->loop_status(i) == ChainState::LOOP_STATUS_SELF) {
            brightness = lfo_patterns[configuration >> 8 & 0x3];
          } else {
            brightness = fade_patterns[chain_state_->loop_status(i)];
            if (type == 0) {
              brightness = brightness * (ramp_patterns[configuration >> 8 & 0x3] + 1) >> 5;
            }
          }
          if ((changing_slider_prop_ & (1 << i)) && (type == 1 || type == 2)) {
            uint8_t scale = 3 - ((configuration >> 12) & 0x3);
            color = (system_clock.milliseconds() >> 6) % 2 == 0 ?
              palette_[scale] : LED_COLOR_OFF;
          } else if (type == 3) {
            uint8_t proportion = (system_clock.milliseconds() >> 7) & 15;
            proportion = proportion > 7 ? 15 - proportion : proportion;
            if ((system_clock.milliseconds() & 7) < proportion) {
              color = LED_COLOR_GREEN;
            } else {
              color = LED_COLOR_RED;
            }
          }
        }
        if (settings_->state().color_blind == 1) {
          if (type == 0) {
            uint8_t modulation = FadePattern(6, 13 - (2 * i), false) >> 1;
            brightness = brightness * (7 + modulation) >> 4;
          } else if (type == 1) {
            brightness = brightness >= 0x8 ? 0xf : 0;
          } else if (type == 2) {
            brightness = brightness >= 0xc ? 0x1 : 0;
          } else if (type == 3) {
            // Not sure how to make it distinct.
          }
        }
        if (settings_->in_seg_gen_mode()
            && is_bipolar(configuration)
            && ((system_clock.milliseconds() >> 8) % 4 == 0)) {
          color = LED_COLOR_RED;
          brightness = 0x1;
        }
        leds_.set(
            LED_GROUP_UI + i,
            (brightness >= pwm && brightness != 0) ? color : LED_COLOR_OFF);
        leds_.set(
            LED_GROUP_SLIDER + i,
            slider_led_counter_[i] ? LED_COLOR_GREEN : LED_COLOR_OFF);
      }

    } else if (multimode == MULTI_MODE_SIX_EG) {

      // LEDs update for 6EG mode
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

    uint32_t ms = system_clock.milliseconds();
    // For any multi-mode, update slider LEDs counters
    for (size_t i = 0; i < kNumChannels; ++i) {
      if (slider_led_counter_[i]) {
        --slider_led_counter_[i];
      }

      // Turn off LEDs if in limbo
      if (cv_reader_->slider_in_limbo(i)) {
        uint8_t dimness = static_cast<uint8_t>(
            8 * fabs(cv_reader_->locked_slider(i) - cv_reader_->lp_slider(i)));
        leds_.set(LED_GROUP_SLIDER + i,
            (ms & 0x07) < dimness ? LED_COLOR_OFF : LED_COLOR_GREEN );
      }
      if (cv_reader_->pot_in_limbo(i)) {
        uint8_t dimness = static_cast<uint8_t>(
            8 * fabs(cv_reader_->locked_pot(i) - cv_reader_->lp_pot(i)));

        if ((ms & 0x07) < dimness) {
          leds_.set(LED_GROUP_UI + i, LED_COLOR_OFF);
        }
      }
    }
  }

  leds_.Write();

}

}  // namespace stages
