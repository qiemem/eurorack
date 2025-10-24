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
// Settings storage.

#ifndef STAGES_SETTINGS_H_
#define STAGES_SETTINGS_H_

#include "stmlib/stmlib.h"
#include "stmlib/system/storage.h"

#include "stages/io_buffer.h"
#include "stages/modes.h"

namespace stages {

const int kSaveDebounceMs = 1000;

struct ChannelCalibrationData {
  float adc_offset;
  float adc_scale;
  float dac_offset;
  float dac_scale;

  inline uint16_t dac_code(float level) const {
    int32_t value = level * dac_scale + dac_offset;
    CONSTRAIN(value, 0, 65531);
    return static_cast<uint16_t>(value);
  }
};

struct PersistentData {
  ChannelCalibrationData channel_calibration_data[kNumChannels];
  uint8_t padding[16];

  enum { tag = 0x494C4143 };  // CALI
};

// Segment configuration is 8 bits:
//  - b00000011 (0x03) -> segment type bits
//  - b00000100 (0x04) -> segment loop bit
//  - b01110000 (0x70) -> ouroboros waveshape (8 values)
//
// New:
//  - b00001000 (0x08) -> bipolar bit
//  - b10000000 (0x80) -> alt gate behavior (reset for LFOs)
//
// Other new segment properties occupy the first 8 bits:
//  - b00000011 (0x0300) (8)  ->  stages range
//  - b00001100 (0x0600) (10) ->  ouroboros range
//  - b00110000 (0x0c00) (12) ->  quantization scale
//

// Independent EGs state is 12 bytes per envelope (there is one envelope per
// channel). Each pair of bytes corresponds to slider and pot values. three of
// the bytes are not used but are kept as padding to keep the bytes in pairs.
// - byte 0: delay length
// - byte 1: -- padding --
// - byte 2: attack length
// - byte 3: attack curve
// - byte 4: hold length
// - byte 5: -- padding --
// - byte 6: decay length
// - byte 7: decay curve
// - byte 8: sustain level
// - byte 9: min sustain length
// - byte 10: release level
// - byte 11: release curve
enum IEGParam : uint8_t {
  IEG_DELAY_LENGTH = 0,
  IEG_ATTACK_LENGTH = 2,
  IEG_ATTACK_CURVE = 3,
  IEG_HOLD_LENGTH = 4,
  IEG_DECAY_LENGTH = 6,
  IEG_DECAY_CURVE = 7,
  IEG_SUSTAIN_LEVEL = 8,
  IEG_SUSTAIN_LENGTH = 9,
  IEG_RELEASE_LENGTH = 10,
  IEG_RELEASE_CURVE = 11,
};

#define is_bipolar(seg_config) seg_config & 0x08

// Should be okay to use bitfields here as arm uses consistent ordering of
// fields (first in lowest address)
struct State {
  uint16_t segment_configuration[kNumChannels];
  uint8_t color_blind : 1;
  uint8_t multimode : 3;
  uint8_t : 0;
  uint8_t independent_eg_looping : kNumChannels;
  uint8_t : 0;
  uint8_t independent_eg_state[kNumChannels][12];
  enum { tag = 0x54415453 }; // STAT
};
// ensure padding and alignment are as expected
static_assert(
  sizeof(State) == 6 * 2 + 1 + 1 + 12 * 6,
  "State struct size has changed - this will break saved settings in flash!"
);

class Settings {
 public:
  Settings() { }
  ~Settings() { }

  bool Init();

  void SavePersistentData();
  // Schedules a state save after kSaveDebounceMs.
  void SaveStateWithDebounce();
  // Saves state if sufficient time has passed since the last change.
  // Should be called exactly once per sys tick in the sys tick interrupt.
  bool PollSave();

  inline ChannelCalibrationData* mutable_calibration_data(int channel) {
    return &persistent_data_.channel_calibration_data[channel];
  }

  inline const ChannelCalibrationData& calibration_data(int channel) const {
    return persistent_data_.channel_calibration_data[channel];
  }

  inline State* mutable_state() {
    return &state_;
  }

  inline const State& state() const {
    return state_;
  }

  inline bool in_seg_gen_mode() const {
    return state_.multimode == MULTI_MODE_STAGES
      || state_.multimode == MULTI_MODE_STAGES_ADVANCED
      || state_.multimode == MULTI_MODE_STAGES_SLOW_LFO;
  }

  inline bool in_ouroboros_mode() const {
    return state_.multimode == MULTI_MODE_OUROBOROS
      || state_.multimode == MULTI_MODE_OUROBOROS_ALTERNATE;
  }


  inline uint16_t dac_code(int index, float level) const {
    return calibration_data(index).dac_code(level);
  }

 private:
  PersistentData persistent_data_;
  State state_;
  int save_counter_ = 0;

  stmlib::ChunkStorage<
      0x08004000,
      0x08008000,
      PersistentData,
      State> chunk_storage_;

  DISALLOW_COPY_AND_ASSIGN(Settings);
};

}  // namespace stages

#endif  // STAGES_SETTINGS_H_
