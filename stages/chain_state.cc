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
// Chain state.

#include "stages/chain_state.h"

#include <algorithm>

#include "stages/quantizer_scales.h"
#include "stages/drivers/serial_link.h"
#include "stages/segment_generator.h"
#include "stages/settings.h"
#include "stages/ui.h"

namespace stages {

using namespace std;

const uint32_t kSimpleLeftKey = stmlib::FourCC<'s', 'g', 's', 'l'>::value;
const uint32_t kSimpleRightKey = stmlib::FourCC<'s', 'g', 's', 'r'>::value;
const uint32_t kAdvancedLeftKey = stmlib::FourCC<'s', 'g', 'a', 'l'>::value;
const uint32_t kAdvancedRightKey = stmlib::FourCC<'s', 'g', 'a', 'r'>::value;

// How long before unpatching an input actually breaks the chain.
const uint32_t kUnpatchedInputDelay = 2000;
const int32_t kLongPressDuration = 500;

void ChainState::Init(SerialLink* left, SerialLink* right, const Settings& settings) {

  left_ = left;
  right_ = right;

  STATIC_ASSERT(sizeof(Packet) == kPacketSize, BAD_PACKET_SIZE);

  left_->Init(
      SERIAL_LINK_DIRECTION_LEFT,
      115200 * 8,
      left_rx_packet_[0].bytes,
      kPacketSize);
  right_->Init(
      SERIAL_LINK_DIRECTION_RIGHT,
      115200 * 8,
      right_rx_packet_[0].bytes,
      kPacketSize);

  Reinit(settings);
}

void ChainState::Reinit(const Settings& settings) {
  index_ = 0;
  size_ = 1;

  ChannelState c = { .flags = 0b11100000, .pot = 128, .cv_slider = 32768 };

  fill(&channel_state_[0], &channel_state_[kMaxNumChannels], c);
  fill(&last_local_config_[0], &last_local_config_[kNumChannels], 0);
  fill(&unpatch_counter_[0], &unpatch_counter_[kNumChannels], 0);
  fill(&loop_status_[0], &loop_status_[kNumChannels], LOOP_STATUS_NONE);
  fill(&switch_pressed_[0], &switch_pressed_[kMaxChainSize], 0);
  fill(&switch_press_time_[0], &switch_press_time_[kMaxNumChannels], 0);

  request_.request = REQUEST_NONE;

  status_ = CHAIN_DISCOVERING_NEIGHBORS;
  counter_ = 0;
  num_internal_bindings_ = 0;
  num_bindings_ = 0;

  if (settings.state().multimode == MULTI_MODE_STAGES) {
    leftKey = kSimpleLeftKey;
    rightKey = kSimpleRightKey;
  } else if (settings.in_seg_gen_mode()) {
    // Advanced and slow LFO are the same
    leftKey = kAdvancedLeftKey;
    rightKey = kAdvancedRightKey;
  } else {
    // Other modes don't use chaining, so just skip it
   status_ = CHAIN_READY;
  }

  for (uint8_t i=0; i<kNumChannels; i++) {
    quantizers_[i].Init();
    quantizers_[i].Configure(scales[0]);
  }
}

void ChainState::DiscoverNeighbors() {
  // Between t = 500ms and t = 1500ms, ping the neighbors every 50ms
  if (counter_ >= 2000 &&
      counter_ <= 6000 &&
      (counter_ % 200) == 0) {
    left_tx_packet_.discovery.key = leftKey;
    left_tx_packet_.discovery.counter = size_;
    left_->Transmit(left_tx_packet_);

    right_tx_packet_.discovery.key = rightKey;
    right_tx_packet_.discovery.counter = index_;
    right_->Transmit(right_tx_packet_);
  }

  const DiscoveryPacket* l = left_->available_rx_buffer<DiscoveryPacket>();
  if (l && l->key == rightKey) {
    index_ = size_t(l->counter) + 1;
    size_ = std::max(size_, index_ + 1);
  }

  const DiscoveryPacket* r = right_->available_rx_buffer<DiscoveryPacket>();
  if (r && r->key == leftKey) {
    size_ = std::max(size_, size_t(r->counter));
  }

  bool ouroboros_ = index_ >= kMaxChainSize || size_ > kMaxChainSize;

  // The discovery phase lasts 2000ms.
  status_ = counter_ < 8000 && !ouroboros_ ? CHAIN_DISCOVERING_NEIGHBORS : CHAIN_READY;
  if (status_ == CHAIN_DISCOVERING_NEIGHBORS) {
    ++counter_;
  } else {
    counter_ = 0;
  }
}

void ChainState::StartReinit(const Settings& settings) {
  // counter_ may have ticked up... do a couple times just to be safe
  if ((counter_ % 200) == 0) {
    left_tx_packet_.discovery.key = kReinitKey;
    left_tx_packet_.discovery.counter = kReinitCount;
    right_tx_packet_.discovery.key = kReinitKey;
    right_tx_packet_.discovery.counter = kReinitCount;
    left_->Transmit(left_tx_packet_);
    right_->Transmit(right_tx_packet_);
  } else if (counter_ >= 2000) {
    Reinit(settings);
  }
  ++counter_;
}

void ChainState::TransmitRight() {
  if (index_ == size_ - 1) {
    return;
  }

  LeftToRightPacket* p = &right_tx_packet_.to_right;
  p->phase = tx_last_sample_.phase;
  p->segment = tx_last_sample_.segment;
  p->last_patched_channel = tx_last_patched_channel_;
  p->last_loop = tx_last_loop_;

  copy(&input_patched_[0], &input_patched_[index_ + 1], &p->input_patched[0]);
  copy(&switch_pressed_[0], &switch_pressed_[index_ + 1],
       &p->switch_pressed[0]);
  right_->Transmit(right_tx_packet_);
}

void ChainState::ReceiveRight() {
  const RightToLeftPacket* p = right_->available_rx_buffer<RightToLeftPacket>();
  if (p && check_reinit<RightToLeftPacket>(p)) {
    start_reinit();
    return;
  } else if (index_ == size_ - 1) {
    return;
  }

  if (p) {
    size_t rx_index = p->channel[0].index();
    if (rx_index > index_ && rx_index < size_) {
      // This packet contains the state of a module on the right.
      // Check if some settings have been changed on the remote modules,
      // then update our local copy of its state.
      for (size_t i = 0; i < kNumChannels; ++i) {
        dirty_[remote_channel_index(rx_index, i)] = \
            remote_channel(rx_index, i)->flags != p->channel[i].flags;
      }
      copy(
          &p->channel[0],
          &p->channel[kNumChannels],
          remote_channel(rx_index, 0));
      request_.request = REQUEST_NONE;
    // rx_index is only 3 bits now, so check if 0x7 instead of 0xf
    } else if (rx_index == 0x7) {
      // This suspiciously looks like a state change request packet!
      // We will take care of it later.
      request_ = *(const RequestPacket*)(p);
    }
  }
}

void ChainState::TransmitLeft() {
  if (index_ == 0) {
    return;
  }

  if (request_.request != REQUEST_NONE) {
    // Forward the request to the left.
    left_tx_packet_.request = request_;
  } else {
    // Determine which module contains the last segment of the chain
    // starting at this module's last segment.
    //
    // For example:
    //
    // 0----- 1----- 2----- 3----- 4-----
    //
    // ----X- ------ ---X-- ------ ---X--
    //
    // last = 2
    //        last = 2
    //               last = 4
    //                      last = 4
    //                             last = 5
    size_t last = size_ - 1;
    for (size_t i = index_; i < size_; ++i) {
      for (size_t j = 0; j < kNumChannels; ++j) {
        if (remote_channel(i, j)->input_patched()) {
          last = i;
          goto found;
        }
      }
    }
  found:
    // In the example above, module 1 will alternate between sending to module
    // 0 its own state, and the state of module 2.
    size_t tx_index = index_ + ((counter_ >> 2) % (last - index_ + 1));
    copy(
        remote_channel(tx_index, 0),
        remote_channel(tx_index, kNumChannels),
        &left_tx_packet_.to_left.channel[0]);
  }
  left_->Transmit(left_tx_packet_);
}

void ChainState::ReceiveLeft() {
  const LeftToRightPacket* p = left_->available_rx_buffer<LeftToRightPacket>();
  if (p && check_reinit(p)) {
    start_reinit();
  } else if (index_ == 0) {
    rx_last_patched_channel_ = size_ * kNumChannels;
    rx_last_loop_.start = -1;
    rx_last_loop_.end = -1;
    return;
  }

  if (p) {
    rx_last_patched_channel_ = p->last_patched_channel;
    rx_last_loop_ = p->last_loop;
    rx_last_sample_.phase = p->phase;
    rx_last_sample_.segment = p->segment;
    copy(&p->switch_pressed[0], &p->switch_pressed[index_],
         &switch_pressed_[0]);
    copy(&p->input_patched[0], &p->input_patched[index_], &input_patched_[0]);
  }
}

void ChainState::Configure(
    SegmentGenerator* segment_generator, const Settings& settings) {
  size_t last_local_channel = local_channel_index(0) + kNumChannels;
  size_t last_channel = size_ * kNumChannels;
  size_t last_patched_channel = rx_last_patched_channel_;
  Loop last_loop = rx_last_loop_;

  num_internal_bindings_ = 0;
  num_bindings_ = 0;

  segment::Configuration configuration[kMaxNumChannels];

  attenuate_ = 0;

  for (size_t i = 0; i < kNumChannels; ++i) {
    size_t channel = local_channel_index(i);
    const uint16_t *local_configs = settings.state().segment_configuration;

    if (!local_channel(i)->input_patched()) {
      if (channel > last_patched_channel) {
        // Create a slave channel - we are just extending a chain of segments.
        size_t segment = channel - last_patched_channel;
        segment_generator[i].ConfigureSlave(segment);
        set_loop_status(i, segment, last_loop);
      } else {
        // Create a free-running channel.
        segment::Configuration c = local_channel(i)->configuration();
        c.range = segment::FreqRange(local_configs[i] >> 8 & 0x03);
        segment_generator[i].ConfigureSingleSegment(false, c);
        binding_[num_bindings_].generator = i;
        binding_[num_bindings_].source = i;
        binding_[num_bindings_].destination = 0;
        ++num_bindings_;
        ++num_internal_bindings_;
        loop_status_[i] = c.loop ? LOOP_STATUS_SELF : LOOP_STATUS_NONE;
      }
    } else {
      last_patched_channel = channel;

      // Create a normal channel, trying to extend it as far as possible.
      int num_segments = 0;
      bool add_more_segments = true;
      bool dirty = false;

      last_loop.start = -1;
      last_loop.end = -1;
      while (add_more_segments) {
        // Add an entry in the configuration array.
        segment::Configuration c = channel_state_[channel].configuration();
        configuration[num_segments] = c;
        dirty |= dirty_[channel];

        if (c.loop) {
          if (last_loop.start == -1) last_loop.start = num_segments;
          last_loop.end = num_segments;
        }

        // Add a binding in the binding array.
        binding_[num_bindings_].generator = i;
        binding_[num_bindings_].destination = num_segments;
        if (channel < last_local_channel) {
          // Bind local CV/pot to this segment's parameters.
          binding_[num_bindings_].source = i + num_segments;
          ++num_internal_bindings_;
          // Note: this will only have an effect on LFOs
          configuration[num_segments].range =
            segment::FreqRange((local_configs[i + num_segments] >> 8) & 0x03);
        } else {
          // Bind remote CV/pot to this segment's parameters.
          binding_[num_bindings_].source = channel;
        }
        ++num_bindings_;
        ++channel;
        ++num_segments;

        add_more_segments = channel < last_channel && \
             !channel_state_[channel].input_patched();
      }
      if (dirty || num_segments != segment_generator[i].num_segments()) {
        segment_generator[i].Configure(true, configuration, num_segments);
      }
      set_loop_status(i, 0, last_loop);
    }
    attenuate_ |= segment_generator[i].needs_attenuation() << i;
  }
  tx_last_loop_ = last_loop;
  tx_last_patched_channel_ = last_patched_channel;
}

inline void ChainState::UpdateLocalState(
    const IOBuffer::Block& block,
    const Settings& settings,
    const SegmentGenerator::Output& last_out) {
  tx_last_sample_ = last_out;

  ChannelBitmask input_patched_bitmask = 0;
  for (size_t i = 0; i < kNumChannels; ++i) {
    if (block.input_patched[i]) {
      unpatch_counter_[i] = 0;
    } else if (unpatch_counter_[i] < kUnpatchedInputDelay) {
      ++unpatch_counter_[i];
    }

    bool input_patched = unpatch_counter_[i] < kUnpatchedInputDelay;
    uint16_t config = settings.state().segment_configuration[i];
    size_t channel = local_channel_index(i);
    dirty_[channel] = local_channel(i)->UpdateFlags(
        index_,
        config,
        input_patched)
      || (config != last_local_config_[i]); // Check props that are not transmitted
    if (dirty_[channel]
        && ((config >> 12 & 0x03) != (last_local_config_[i] >> 12 & 0x03))) {
      quantizers_[i].Configure(scales[config >> 12 & 0x03]);
    }
    last_local_config_[i] = config;
    if (input_patched) {
      input_patched_bitmask |= 1 << i;
    }
  }
  input_patched_[index_] = input_patched_bitmask;
}

inline void ChainState::UpdateLocalPotCvSlider(
    const IOBuffer::Block& block, const Settings& settings) {
  const uint16_t *configs = settings.state().segment_configuration;
  for (size_t i = 0; i < kNumChannels; ++i) {
    ChannelState* s = local_channel(i);
    s->cv_slider = cv_slider(block, i, configs[i]) * 16384.0f + 32768.0f;
    s->pot = block.pot[i] * 256.0f;
  }
}

inline void ChainState::BindRemoteParameters(
    SegmentGenerator* segment_generator) {
  for (size_t i = num_internal_bindings_; i < num_bindings_; ++i) {
    const ParameterBinding& m = binding_[i];
    segment_generator[m.generator].set_segment_parameters(
        m.destination,
        channel_state_[m.source].cv_slider / 16384.0f - 2.0f,
        channel_state_[m.source].pot / 256.0f);
  }
}

inline void ChainState::BindLocalParameters(
    const IOBuffer::Block& block,
    SegmentGenerator* segment_generator,
    const Settings& settings) {
  const uint16_t *configs = settings.state().segment_configuration;
  for (size_t i = 0; i < num_internal_bindings_; ++i) {
    const ParameterBinding& m = binding_[i];
    segment_generator[m.generator].set_segment_parameters(
        m.destination,
        cv_slider(block, m.source, configs[i]),
        block.pot[m.source],
        block.cv[m.source]);
  }
}

ChainState::RequestPacket ChainState::MakeLoopChangeRequest(
    size_t loop_start, size_t loop_end) {
  size_t channel_index = 0;
  size_t group_start = 0;
  size_t group_end = size_ * kNumChannels;

  bool inconsistent_loop = false;

  // Fill group_start and group_end, which contain the tightest interval
  // of patched channels enclosing the loop.
  //
  // LOOP     ----S- ------ --E--- ------
  // PATCHED  -x---- ------ ----x- ------
  //           ^  ^           ^ ^
  //           |  |           | |
  //           |  |           | group_end
  //           |  |           loop_end
  //           |  loop_start
  //           group_start
  for (size_t i = 0; i < size_; ++i) {
    ChannelBitmask input_patched = input_patched_[i];
    for (size_t j = 0; j < kNumChannels; ++j) {
      if (input_patched & 1) {
        if (channel_index <= loop_start) {
          group_start = channel_index;
        } else if (channel_index >= loop_end) {
          group_end = min(group_end, channel_index);
        }
        // There shouldn't be a patched channel between the loop start
        // and the loop end.
        if (channel_index > loop_start && channel_index < loop_end) {
          // LOOP     ----S- ------ --E--- ------
          // PATCHED  -x---- ---x-- ----x- ------
          inconsistent_loop = true;
        }
      }
      input_patched >>= 1;
      ++channel_index;
    }
  }

  // There shouldn't be a loop spanning multiple channels among the first
  // group of unpatched channels.
  if (group_start == 0 && !(input_patched_[0] & 1)) {
    if (loop_start != loop_end) {
      // LOOP     -S-E-- ------
      // PATCHED  -----x ---x--
      inconsistent_loop = true;
    } else {
      group_start = group_end = loop_start = loop_end;
    }
  }

  // The only situation where a loop can end on a patched channel is when
  // we have a single-channel group.
  if (group_end == loop_end && group_start != group_end) {
    // Correct:
    // LOOP     ---S--
    //             E
    // PATCHED  ---xx-

    // Incorrect:
    // LOOP     ---S-E
    // PATCHED  ---x-x
    inconsistent_loop = true;
  }

  RequestPacket result;
  if (inconsistent_loop) {
    result.request = REQUEST_NONE;
  } else {
    result.request = REQUEST_SET_LOOP;
    result.argument[0] = group_start;
    result.argument[1] = loop_start;
    result.argument[2] = loop_end;
    result.argument[3] = group_end;
  }
  return result;
}

void ChainState::PollSwitches() {
  // The last module in the chain polls the states of the switches for the
  // entire chain. The state of the switches has been passed from left
  // to right.
  //
  // If a switch has been pressed, a Request packet is passed from right
  // to left. Each module is responsible from parsing the Request packet
  // and adjusting its internal state to simulate local changes. During
  // the next cycle (1ms later), the internal change will be propagated
  // from module to module through the usual mechanism (ChannelState
  // transmission).
  //
  // New property changes (polarity, frequency range, etc.) are handled locally
  // (transmitting what property should be changed is hard). If switches are
  // being handled locally, a module emits 0xff for its switch pressed bitmask
  // (0xff cannot naturally occur). Then, the rightmost module knows to suspend
  // switch processing for that module, setting all counts to -1 for it (so
  // that state changes aren't triggered on switch release if the count had
  // been sufficiently high prior to property change). Properties that must be
  // known by other modules are then trasmitted through ChannelState; this
  // includes polarity/retrigger control (which use the same bit).
  if (index_ == size_ - 1) {
    request_.request = REQUEST_NONE;
    size_t switch_index = 0;
    size_t first_pressed = kMaxNumChannels;

    for (size_t i = 0; i < size_; ++i) {
      ChannelBitmask switch_pressed = switch_pressed_[i];
      if (switch_pressed == 0xff) {
        // Switches are being locally processed; suspend
        fill(
            &switch_press_time_[switch_index],
            &switch_press_time_[switch_index + kNumChannels],
            -1);
        switch_index += kNumChannels;
      } else {
        for (size_t j = 0; j < kNumChannels; ++j) {
          if (switch_pressed & 1) {
            if (switch_press_time_[switch_index] != -1) {
              ++switch_press_time_[switch_index];
              if (first_pressed != kMaxNumChannels) {
                // Simultaneously pressing a pair of buttons.
                request_ = MakeLoopChangeRequest(first_pressed, switch_index);
                switch_press_time_[first_pressed] = -1;
                switch_press_time_[switch_index] = -1;
              } else {
                first_pressed = switch_index;
              }
            }
          } else {
            if (switch_press_time_[switch_index] > kLongPressDuration) { // Long-press
              if (switch_press_time_[switch_index] < kLongPressDurationForMultiModeToggle) { // But not long enough for multi-mode toggle
                request_ = MakeLoopChangeRequest(switch_index, switch_index);
                switch_press_time_[switch_index] = -1;
              }
            } else if (switch_press_time_[switch_index] > 5) {
              // A button has been released after having been held for a
              // sufficiently long time (5ms), but not for long enough to be
              // detected as a long press.
              request_.request = REQUEST_SET_SEGMENT_TYPE;
              request_.argument[0] = switch_index;
            }
            switch_press_time_[switch_index] = 0;
          }
          switch_pressed >>= 1;
          ++switch_index;
        }
      }
    }
  }
}

void ChainState::SuspendSwitches() {
  set_local_switch_pressed(0xff);
}

void ChainState::HandleRequest(Settings* settings) {
  if (request_.request == REQUEST_NONE) {
    return;
  }

  const uint8_t num_types = settings->state().multimode == MULTI_MODE_STAGES_ADVANCED ? 4 : 3;

  State* s = settings->mutable_state();
  bool dirty = false;
  for (size_t i = 0; i < kNumChannels; ++i) {
    size_t channel = local_channel_index(i);

    uint8_t type_bits = s->segment_configuration[i] & 0x3;
    uint8_t loop_bit = s->segment_configuration[i] & 0x4;

    if (request_.request == REQUEST_SET_SEGMENT_TYPE) {
      if (channel == request_.argument[0]) {
        s->segment_configuration[i] &= ~0xff00; // Reset LFO range
        s->segment_configuration[i] &= ~0b00001011; // Reset type and bipolar bits
        s->segment_configuration[i] |= ((type_bits + 1) % num_types); // Cycle through segment types
        dirty |= true;
      }
    } else if (request_.request == REQUEST_SET_LOOP) {
      uint8_t new_loop_bit = loop_bit;
      if ((channel >= request_.argument[0] && channel < request_.argument[3])) {
        new_loop_bit = 0x0;
      }
      if (channel == request_.argument[1] || channel == request_.argument[2]) {
        if (request_.argument[1] == request_.argument[2]) {
          new_loop_bit = 0x4 - loop_bit;
        } else {
          new_loop_bit = 0x4;
        }
      }
      s->segment_configuration[i] &= ~0b00000100; // Reset loop bits
      s->segment_configuration[i] |= new_loop_bit; // Set new loop bit
      if (new_loop_bit != loop_bit && request_.argument[0] == request_.argument[3]) {
        s->segment_configuration[i] &= ~0xff00; // Reset LFO range
        dirty = true;
      }
    }
  }

  if (dirty) {
    settings->SaveState();
  }
}

void ChainState::Update(
    const IOBuffer::Block& block,
    Settings* settings,
    SegmentGenerator* segment_generator,
    SegmentGenerator::Output* out) {
  switch (status_) {
    case CHAIN_DISCOVERING_NEIGHBORS:
      DiscoverNeighbors();
      return;
    case CHAIN_REINITIALIZING:
      StartReinit(*settings);
      return;
    default:
      break;
  }

  switch (counter_ & 0x3) {
    case 0:
      PollSwitches();
      UpdateLocalState(block, *settings, out[kBlockSize - 1]);
      TransmitRight();
      break;
    case 1:
      ReceiveRight();
      HandleRequest(settings);
      break;
    case 2:
      UpdateLocalPotCvSlider(block, *settings);
      TransmitLeft();
      break;
    case 3:
      ReceiveLeft();
      Configure(segment_generator, *settings);
      BindRemoteParameters(segment_generator);
      break;
  }

  BindLocalParameters(block, segment_generator, *settings);
  fill(&out[0], &out[kBlockSize], rx_last_sample_);

  ++counter_;
}

}  // namespace stages
