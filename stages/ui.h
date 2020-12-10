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
// User interface.

#ifndef STAGES_UI_H_
#define STAGES_UI_H_

#include "stages/cv_reader.h"
#include "stmlib/stmlib.h"

#include "stmlib/ui/event_queue.h"

#include "stages/drivers/leds.h"
#include "stages/drivers/switches.h"

#include "stages/settings.h"

const int32_t kLongPressDurationForMultiModeToggle = 5000;

namespace stages {

enum UiMode {
  UI_MODE_NORMAL,
  UI_MODE_FACTORY_TEST
};

class ChainState;

class Ui {
 public:
  Ui() { }
  ~Ui() { }

  void Init(Settings* settings, ChainState* chain_state, CvReader* cv_reader);
  void Poll();
  void DoEvents();

  void set_led(int i, LedColor color) {
    led_color_[i] = color;
  }

  void set_slider_led(int i, bool value, int duration) {
    if (value) {
      slider_led_counter_[i] = duration;
    }
  }

  inline void set_factory_test(bool factory_test) {
    mode_ = factory_test ? UI_MODE_FACTORY_TEST : UI_MODE_NORMAL;
  }

  inline const Switches& switches() const { return switches_; }

 private:
  void OnSwitchPressed(const stmlib::Event& e);
  void OnSwitchReleased(const stmlib::Event& e);

  void MultiModeToggle(const uint8_t i);

  void UpdateLEDs();
  uint8_t FadePattern(uint8_t shift, uint8_t phase, bool ramp) const;

  void show_mode() {
    for (size_t i = 0; i < kNumChannels; ++i) {
      if (multimodes_[i] == settings_->state().multimode) {
        leds_.set(i, LED_COLOR_RED);
      }
    }
  }

  Leds leds_;
  Switches switches_;
  float lp_slider_[kNumChannels];
  uint8_t changing_slider_prop_;
  uint8_t changing_pot_prop_;

  LedColor led_color_[kNumLEDs];
  int slider_led_counter_[kNumLEDs];
  int press_time_[kNumSwitches];
  int press_time_multimode_toggle_[kNumSwitches];

  Settings* settings_;
  ChainState* chain_state_;
  CvReader* cv_reader_;

  UiMode mode_;

  static const MultiMode multimodes_[6];
  static const LedColor palette_[4];

  DISALLOW_COPY_AND_ASSIGN(Ui);
};

}  // namespace stages

#endif  // STAGES_UI_H_
