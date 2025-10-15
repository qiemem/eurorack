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
// CV reader.

#ifndef STAGES_CV_READER_H_
#define STAGES_CV_READER_H_

#include "stages/chain_state.h"
#include "stmlib/stmlib.h"

#include "stages/drivers/pots_adc.h"
#include "stages/drivers/cv_adc.h"
#include "stages/io_buffer.h"
#include <cmath>

namespace stages {

using namespace std;

class Settings;

class CvReader {
 public:
  CvReader() { }
  ~CvReader() { }

  void Init(Settings* settings);
  void Read(IOBuffer::Block* block);
  void Lock(int i);
  void Unlock(int i);

  void unlock_all() {
    locked_ = 0;
  }

  inline uint8_t raw_cv(int i) const {
    return (int32_t(cv_adc_.value(i)) + 32768) >> 8;
  }

  inline uint8_t raw_pot(int i) const {
    return pots_adc_.value(ADC_GROUP_POT, i) >> 8;
  }

  inline uint8_t raw_slider(int i) const {
    return pots_adc_.value(ADC_GROUP_SLIDER, i) >> 8;
  }

  inline float lp_cv(int i) const {
    return lp_cv_2_[i];
  }

  inline float lp_slider(int i) const {
    return lp_slider_[i];
  }

  inline float lp_pot(int i) const {
    return lp_pot_[i];
  }

  inline void set_locked_slider(int i, float x) {
    locked_slider_[i] = x;
  }

  inline float locked_slider(int i) const {
    return locked_slider_[i];
  }

  inline void set_locked_pot(int i, float x) {
    locked_pot_[i] = x;
  }

  inline bool any_locked() const {
    return locked_;
  }

  inline float locked_pot(int i) const {
    return locked_pot_[i];
  }

  inline bool is_locked(int i) const {
    return locked_ >> i & 1;
  }

 private:
  Settings* settings_;
  ChainState* chain_state_;
  CvAdc cv_adc_;
  PotsAdc pots_adc_;

  float lp_cv_[kNumChannels];
  float lp_cv_2_[kNumChannels];
  float lp_slider_[kNumChannels];
  float lp_pot_[kNumChannels];

  uint8_t locked_;
  float locked_slider_[kNumChannels];
  float locked_pot_[kNumChannels];


  DISALLOW_COPY_AND_ASSIGN(CvReader);
};

}  // namespace stages

#endif  // STAGES_CV_READER_H_
