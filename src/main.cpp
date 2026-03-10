#include <Arduino.h>
#include <pwm.h>

namespace {

constexpr pin_size_t kMosfet0PwmPin = 11;
constexpr pin_size_t kMosfet1PwmPin = 10;
constexpr pin_size_t kMosfet0DirPin = 12;
constexpr pin_size_t kMosfet1DirPin = 3;
constexpr pin_size_t kStatusLedPin = LED_BUILTIN;

constexpr float kPwmFrequencyHz = 30000.0f;
constexpr unsigned long kSirenCycleMs = 1200;

PwmOut g_mosfet0(kMosfet0PwmPin);
PwmOut g_mosfet1(kMosfet1PwmPin);

void set_led_output(PwmOut& pwm, pin_size_t dir_pin, float duty_percent) {
  const float clamped = constrain(duty_percent, 0.0f, 100.0f);
  digitalWrite(dir_pin, LOW);
  pwm.pulse_perc(clamped);
}

float triangle_wave(unsigned long now_ms, unsigned long phase_offset_ms) {
  const unsigned long phase = (now_ms + phase_offset_ms) % kSirenCycleMs;
  const unsigned long half_cycle = kSirenCycleMs / 2;
  if (phase < half_cycle) {
    return 100.0f * static_cast<float>(phase) / static_cast<float>(half_cycle);
  }
  return 100.0f * static_cast<float>(kSirenCycleMs - phase) /
         static_cast<float>(half_cycle);
}

void set_fault_pattern(unsigned long now_ms) {
  const float led0_duty = triangle_wave(now_ms, 0);
  const float led1_duty = triangle_wave(now_ms, kSirenCycleMs / 2);
  set_led_output(g_mosfet0, kMosfet0DirPin, led0_duty);
  set_led_output(g_mosfet1, kMosfet1DirPin, led1_duty);
  digitalWrite(kStatusLedPin, (now_ms / 150) % 2 == 0 ? HIGH : LOW);
}

}  // namespace

void setup() {
  Serial.begin(115200);

  pinMode(kMosfet0PwmPin, OUTPUT);
  pinMode(kMosfet1PwmPin, OUTPUT);
  pinMode(kMosfet0DirPin, OUTPUT);
  pinMode(kMosfet1DirPin, OUTPUT);
  pinMode(kStatusLedPin, OUTPUT);

  digitalWrite(kMosfet0PwmPin, LOW);
  digitalWrite(kMosfet1PwmPin, LOW);
  digitalWrite(kMosfet0DirPin, LOW);
  digitalWrite(kMosfet1DirPin, LOW);
  digitalWrite(kStatusLedPin, LOW);

  g_mosfet0.begin(kPwmFrequencyHz, 0.0f);
  g_mosfet1.begin(kPwmFrequencyHz, 0.0f);
  Serial.println("crub igniter siren led ready");
}

void loop() {
  const unsigned long now_ms = millis();
  set_fault_pattern(now_ms);
}
