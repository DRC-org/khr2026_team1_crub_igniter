#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <Arduino_CAN.h>
#include <pwm.h>

namespace {

constexpr pin_size_t kMosfet0PwmPin = 11;
constexpr pin_size_t kMosfet1PwmPin = 10;
constexpr pin_size_t kMosfet0DirPin = 12;
constexpr pin_size_t kMosfet1DirPin = 3;
constexpr pin_size_t kCanRxPin = 5;
constexpr pin_size_t kCanTxPin = 4;
constexpr pin_size_t kStatusLedPin = LED_BUILTIN;
constexpr pin_size_t kRgbPin = 6;

constexpr uint32_t kCanId = 0x302;
constexpr uint32_t kHealthResponseCanId = 0x000;
constexpr uint8_t kHealthCheckCmd = 0xFF;
constexpr uint8_t kHealthResponseId = 0x32;
constexpr float kPwmFrequencyHz = 30000.0f;
constexpr uint32_t kHeartbeatIntervalMs = 1000;

PwmOut g_mosfet0(kMosfet0PwmPin);
PwmOut g_mosfet1(kMosfet1PwmPin);

Adafruit_NeoPixel strip(1, kRgbPin, NEO_GRB + NEO_KHZ800);

uint32_t g_last_heartbeat_ms = 0;
bool g_heartbeat_on = false;
bool g_mosfet0_on = false;
bool g_mosfet1_on = false;

}  // namespace

void setup() {
  Serial.begin(115200);

  pinMode(kMosfet0PwmPin, OUTPUT);
  pinMode(kMosfet1PwmPin, OUTPUT);
  pinMode(kMosfet0DirPin, OUTPUT);
  pinMode(kMosfet1DirPin, OUTPUT);
  pinMode(kStatusLedPin, OUTPUT);
  pinMode(kRgbPin, OUTPUT);

  digitalWrite(kMosfet0PwmPin, LOW);
  digitalWrite(kMosfet1PwmPin, LOW);
  digitalWrite(kMosfet0DirPin, LOW);
  digitalWrite(kMosfet1DirPin, LOW);
  digitalWrite(kStatusLedPin, LOW);

  g_mosfet0.begin(kPwmFrequencyHz, 0.0f);
  g_mosfet1.begin(kPwmFrequencyHz, 0.0f);

  strip.begin();
  strip.setBrightness(100);

  if (!CAN.begin(CanBitRate::BR_1000k)) {
    Serial.println("CAN.begin(...) failed.");
    while (1) {
      strip.setPixelColor(0, strip.Color(255, 0, 0));
      strip.show();
      delay(100);
      strip.setPixelColor(0, strip.Color(0, 0, 0));
      strip.show();
      delay(100);
    }
  }

  strip.setPixelColor(0, strip.Color(0, 255, 0));
  strip.show();

  Serial.println("crub igniter ready");
}

void loop() {
  if (CAN.available()) {
    CanMsg const msg = CAN.read();

    if (msg.id == kCanId && msg.data_length > 0) {
      const uint8_t cmd = msg.data[0];

      switch (cmd) {
        case 0x00:
          g_mosfet0.pulse_perc(0.0f);
          g_mosfet0_on = false;
          Serial.println("MOSFET 0 OFF");
          break;
        case 0x01:
          g_mosfet0.pulse_perc(100.0f);
          g_mosfet0_on = true;
          Serial.println("MOSFET 0 ON");
          break;
        case 0x10:
          g_mosfet1.pulse_perc(0.0f);
          g_mosfet1_on = false;
          Serial.println("MOSFET 1 OFF");
          break;
        case 0x11:
          g_mosfet1.pulse_perc(100.0f);
          g_mosfet1_on = true;
          Serial.println("MOSFET 1 ON");
          break;
        case kHealthCheckCmd: {
          uint8_t response[3] = {
              kHealthResponseId,
              g_mosfet0_on ? uint8_t(1) : uint8_t(0),
              g_mosfet1_on ? uint8_t(1) : uint8_t(0),
          };
          CanMsg health_msg(kHealthResponseCanId, sizeof(response), response);
          CAN.write(health_msg);
          Serial.println("Health check response sent");
          break;
        }
        default:
          break;
      }
    }
  }

  uint32_t now = millis();
  if (now - g_last_heartbeat_ms >= kHeartbeatIntervalMs) {
    g_last_heartbeat_ms = now;
    g_heartbeat_on = !g_heartbeat_on;
    strip.setPixelColor(
        0, g_heartbeat_on ? strip.Color(0, 255, 0) : strip.Color(0, 0, 0));
    strip.show();
  }
}
