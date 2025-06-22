// Hardware proxy for Funbox DIY DSP Platform
// Copyright (C) 2024  Cleveland Music Co.  <code@clevelandmusicco.com>
// Copyright (C) 2025  Boyd Timothy <btimothy@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// This is a modified version of the Hothouse hardware proxy libary built for
// the Funbox DIY DSP Platform, which is also based on the Daisy Seed platform.

#include "funbox.h"

#include "optional"

using clevelandmusicco::Hothouse;
using daisy::System;

#ifndef SAMPLE_RATE
// #define SAMPLE_RATE DSY_AUDIO_SAMPLE_RATE
#define SAMPLE_RATE 48014.f
#endif

// Hardware related defines.
// Switches
constexpr Pin PIN_SW_1_LEFT = daisy::seed::D14;
constexpr Pin PIN_SW_1_RIGHT = daisy::seed::D13;
constexpr Pin PIN_SW_2_LEFT = daisy::seed::D7;
constexpr Pin PIN_SW_2_RIGHT = daisy::seed::D10;
constexpr Pin PIN_SW_3_LEFT = daisy::seed::D2;
constexpr Pin PIN_SW_3_RIGHT = daisy::seed::D4;

constexpr Pin PIN_DIP_SW_1 = daisy::seed::D1;
constexpr Pin PIN_DIP_SW_2 = daisy::seed::D3;
constexpr Pin PIN_DIP_SW_3 = daisy::seed::D5;
constexpr Pin PIN_DIP_SW_4 = daisy::seed::D6;

constexpr Pin PIN_FSW_1 = daisy::seed::D25;
constexpr Pin PIN_FSW_2 = daisy::seed::D26;

// Knobs
constexpr Pin PIN_KNOB_1 = daisy::seed::D16;
constexpr Pin PIN_KNOB_2 = daisy::seed::D17;
constexpr Pin PIN_KNOB_3 = daisy::seed::D18;
constexpr Pin PIN_KNOB_4 = daisy::seed::D19;
constexpr Pin PIN_KNOB_5 = daisy::seed::D20;
constexpr Pin PIN_KNOB_6 = daisy::seed::D21;

const uint32_t Hothouse::HOLD_THRESHOLD_MS;

void Hothouse::Init(bool boost) {
  // Initialize the hardware.
  seed.Configure();
  seed.Init(boost);
  InitSwitches();
  InitAnalogControls();
  SetAudioBlockSize(48);
}

void Hothouse::DelayMs(size_t del) { seed.DelayMs(del); }

void Hothouse::SetHidUpdateRates() {
  for (size_t i = 0; i < KNOB_LAST; i++) {
    knobs[i].SetSampleRate(AudioCallbackRate());
  }
}

void Hothouse::StartAudio(AudioHandle::InterleavingAudioCallback cb) {
  seed.StartAudio(cb);
}

void Hothouse::StartAudio(AudioHandle::AudioCallback cb) {
  seed.StartAudio(cb);
}

void Hothouse::ChangeAudioCallback(AudioHandle::InterleavingAudioCallback cb) {
  seed.ChangeAudioCallback(cb);
}

void Hothouse::ChangeAudioCallback(AudioHandle::AudioCallback cb) {
  seed.ChangeAudioCallback(cb);
}

void Hothouse::StopAudio() { seed.StopAudio(); }

void Hothouse::SetAudioBlockSize(size_t size) {
  seed.SetAudioBlockSize(size);
  SetHidUpdateRates();
}

size_t Hothouse::AudioBlockSize() { return seed.AudioBlockSize(); }

void Hothouse::SetAudioSampleRate(SaiHandle::Config::SampleRate samplerate) {
  seed.SetAudioSampleRate(samplerate);
  SetHidUpdateRates();
}

float Hothouse::AudioSampleRate() { return seed.AudioSampleRate(); }

float Hothouse::AudioCallbackRate() { return seed.AudioCallbackRate(); }

void Hothouse::StartAdc() { seed.adc.Start(); }

void Hothouse::StopAdc() { seed.adc.Stop(); }

void Hothouse::ProcessAnalogControls() {
  for (size_t i = 0; i < KNOB_LAST; i++) {
    knobs[i].Process();
  }
}

float Hothouse::GetKnobValue(Knob k) {
  size_t idx;
  idx = k < KNOB_LAST ? k : KNOB_1;
  return knobs[idx].Value();
}

void Hothouse::ProcessDigitalControls() {
  for (size_t i = 0; i < SWITCH_LAST; i++) {
    switches[i].Debounce();
  }
  ProcessFootswitchPresses(FOOTSWITCH_1);
  ProcessFootswitchPresses(FOOTSWITCH_2);
}

void Hothouse::InitSwitches() {
  constexpr Pin pin_numbers[SWITCH_LAST] = {
      PIN_SW_1_RIGHT, PIN_SW_1_LEFT, PIN_SW_2_RIGHT, PIN_SW_2_LEFT,
      PIN_SW_3_RIGHT, PIN_SW_3_LEFT, PIN_FSW_1,   PIN_FSW_2,
      PIN_DIP_SW_1, PIN_DIP_SW_2, PIN_DIP_SW_3, PIN_DIP_SW_4
  };

  for (size_t i = 0; i < SWITCH_LAST; i++) {
    switches[i].Init(pin_numbers[i]);
  }
}

void Hothouse::InitAnalogControls() {
  constexpr Pin knob_pins[KNOB_LAST] = {PIN_KNOB_1, PIN_KNOB_2, PIN_KNOB_3,
                                        PIN_KNOB_4, PIN_KNOB_5, PIN_KNOB_6};

  // Set order of ADCs based on CHANNEL NUMBER
  AdcChannelConfig cfg[KNOB_LAST];

  // Initialize ADC configuration with Single Pins
  for (size_t i = 0; i < KNOB_LAST; ++i) {
    cfg[i].InitSingle(knob_pins[i]);
  }

  // Initialize ADC with configuration
  seed.adc.Init(cfg, KNOB_LAST);

  // Get the audio callback rate once
  float callback_rate = AudioCallbackRate();

  // Initialize knobs with ADC pointers and callback rate
  for (size_t i = 0; i < KNOB_LAST; ++i) {
    knobs[i].Init(seed.adc.GetPtr(i), callback_rate);
  }
}

// Public convenience function to get position of toggleswitches 1-3.
Hothouse::ToggleswitchPosition Hothouse::GetToggleswitchPosition(
    Toggleswitch tsw) {
  switch (tsw) {
    case (TOGGLESWITCH_1):
      return GetLogicalSwitchPosition(switches[SWITCH_1_RIGHT],
                                      switches[SWITCH_1_LEFT]);
    case (TOGGLESWITCH_2):
      return GetLogicalSwitchPosition(switches[SWITCH_2_RIGHT],
                                      switches[SWITCH_2_LEFT]);
    case (TOGGLESWITCH_3):
      return GetLogicalSwitchPosition(switches[SWITCH_3_RIGHT],
                                      switches[SWITCH_3_LEFT]);
    default:
      seed.PrintLine(
          "ERROR: Unexpected value provided for Toggleswitch 'tsw'. "
          "Returning TOGGLESWITCH_UNKNOWN by default.");
      return TOGGLESWITCH_UNKNOWN;
  }
}

void Hothouse::CheckResetToBootloader() {
  if (switches[Hothouse::FOOTSWITCH_1].Pressed()) {
    if (footswitch_start_time[0] == 0) {
      footswitch_start_time [0]= System::GetNow();
    } else if (System::GetNow() - footswitch_start_time[0] >= HOLD_THRESHOLD_MS) {
      // Shut 'er down so the LEDs always flash
      StopAdc();
      StopAudio();
      
      daisy::Led _led_1, _led_2;
      _led_1.Init(seed.GetPin(22), false);
      _led_2.Init(seed.GetPin(23), false);

      // Alternately flash the LEDs 3 times
      for (int i = 0; i < 3; i++) {
        _led_1.Set(1);
        _led_2.Set(0);
        _led_1.Update();
        _led_2.Update();
        System::Delay(100);

        _led_1.Set(0);
        _led_2.Set(1);
        _led_1.Update();
        _led_2.Update();
        System::Delay(100);
      }

      // Reset system to bootloader after LED flashing
      System::ResetToBootloader();
    }
  } else {
    // Reset the hold timer if the footswitch is released
    footswitch_start_time[0] = 0;
  }
}

Hothouse::ToggleswitchPosition Hothouse::GetLogicalSwitchPosition(Switch up,
                                                                  Switch down) {
  return up.Pressed()
             ? TOGGLESWITCH_RIGHT
             : (down.Pressed() ? TOGGLESWITCH_LEFT : TOGGLESWITCH_MIDDLE);
}

void Hothouse::RegisterFootswitchCallbacks(FootswitchCallbacks *callbacks) {
  footswitchCallbacks = callbacks;
}

// Watches for normal, double, and long presses of the footswitches.
void Hothouse::ProcessFootswitchPresses(Switches footswitch) {
  if (footswitchCallbacks == NULL) return;

  int idx = (footswitch == Hothouse::FOOTSWITCH_1) ? 0 : 1;
  bool is_pressed = switches[footswitch].RisingEdge();
  uint32_t now = System::GetNow();

  // Handle pending single press if enough time has passed without second press
  if (footswitch_pending_single_press[idx] && 
      (now - footswitch_pending_press_time[idx] >= DOUBLE_PRESS_THRESHOLD_MS)) {
    if (footswitchCallbacks->HandleNormalPress != NULL) {
      footswitchCallbacks->HandleNormalPress(footswitch);
    }
    footswitch_pending_single_press[idx] = false;
    footswitch_press_count[idx] = 0;
  }

  if (is_pressed && !footswitch_last_state[idx]) {
    footswitch_start_time[idx] = now;

    if ((now - footswitch_last_press_time[idx]) <= DOUBLE_PRESS_THRESHOLD_MS) {
      footswitch_press_count[idx]++;
    } else {
      footswitch_press_count[idx] = 1;
    }

    footswitch_last_press_time[idx] = now;
    footswitch_long_press_triggered[idx] = false;
  }

  uint32_t press_duration = now - footswitch_start_time[idx];

  if (is_pressed && press_duration >= HOLD_THRESHOLD_MS && !footswitch_long_press_triggered[idx]) {
    if (footswitchCallbacks->HandleLongPress != NULL) {
      footswitchCallbacks->HandleLongPress(footswitch);
    }
    footswitch_long_press_triggered[idx] = true;
    footswitch_press_count[idx] = 0;
    footswitch_pending_single_press[idx] = false;
  }

  if (!is_pressed && footswitch_last_state[idx]) {
    if (!footswitch_long_press_triggered[idx]) {
      if (footswitch_press_count[idx] >= 2) {
        if (footswitchCallbacks->HandleDoublePress != NULL) {
          footswitchCallbacks->HandleDoublePress(footswitch);
        }
        footswitch_press_count[idx] = 0;
        footswitch_pending_single_press[idx] = false;
      } else if (press_duration < HOLD_THRESHOLD_MS) {
        // Defer normal press in case this is a double
        footswitch_pending_single_press[idx] = true;
        footswitch_pending_press_time[idx] = now;
      }
    }
  }

  footswitch_last_state[idx] = is_pressed;
}