#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include <vector>
#include <cstdint>
#include "esphome/core/preferences.h"
#include <deque>
#include "esphome/components/mqtt/mqtt_client.h"
#include <esp_wifi.h>
#include <esp_mac.h>
#include "esphome/core/application.h"
#include <algorithm>
#include "esphome/components/wifi/wifi_component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "localization.h"

namespace esphome {
namespace acw02 {

using namespace acw02_localization;

  // Enum for mapping action to byte
  enum class Mode : uint8_t { COOL = 0x01, DRY = 0x02, FAN = 0x03, HEAT = 0x04, AUTO = 0x00 };
  enum class Fan  : uint8_t { AUTO = 0x0, P20 = 0x1, P40 = 0x2, P60 = 0x3,
                              P80 = 0x4, P100 = 0x5, SILENT = 0x6, TURBO = 0xD };
  enum class Swing  : uint8_t { P1 = 0x02, P2 = 0x03, P3 = 0x04, P4 = 0x05,
                          P5 = 0x06, SWING = 0x01, STOP = 0x00 };
  enum class SwingHorizontal : uint8_t { STOP = 0x0, AUTO_LEFT = 0x1, P1 = 0x2, P2 = 0x3,
                                         P3 = 0x4, P4 = 0x5, P5 = 0x6, RANGE_P1_P5 = 0xC, AUTO_MID_OUT = 0xD};

  // globals Statics
  static const char *const TAG = "acw02";

  // globals Statics base trame (keep alive?)
  static const std::vector<uint8_t> keepalive_frame_ = {
    0x7A, 0x7A, 0x21, 0xD5, 0x0C, 0x00, 0x00, 0xAB,
    0x0A, 0x0A, 0xFC, 0xF9
  };

  // globals Statics base trame (ask status AC)
  static const std::vector<uint8_t> get_status_frame_ = {
    0x7A, 0x7A, 0x21, 0xD5, 0x0C, 0x00, 0x00, 0xA2,
    0x0A, 0x0A, 0xFE, 0x29
  };

  // globals Statics byte for °F from 61 to 88
  static const uint8_t fahrenheit_encoding_table_[28] = { 
    0x20, 0x21, 0x31, 0x22, 0x32, 0x23, 0x33, 0x24,0x25,
    0x35, 0x26, 0x36, 0x27, 0x37, 0x28, 0x38, 0x29,0x2A, 
    0x3A, 0x2B, 0x3B, 0x2C, 0x3C, 0x2D, 0x3D, 0x2E, 0x2F, 
    0x3F };

class ACW02 : public Component, public uart::UARTDevice {
 public:
  

  //override
  void setup() override;
  void loop() override;
  
  // Setters publics AC
  void set_mode_climate(const std::string &mode);      
  void set_mode(const std::string &mode);
  void set_temperature_c(float temp); 
  void set_temperature_f(float temp);              
  void set_fan(const std::string &speed);         
  void set_swing(const std::string &pos);   
  void set_swing_horizontal(const std::string &pos);       
  void set_display(bool on);
  void set_eco(bool on);
  void set_eco_internal(bool on, bool sendCmd);
  void set_night(bool on);
  void set_purifier(bool on);
  void set_mute(bool on);
  void set_unit(const std::string &unit); 
  void set_clean(bool on);

  // Setters publics disable option mode
  void set_disable_mode_auto(bool on);
  void set_disable_mode_heat(bool on);
  void set_disable_mode_dry(bool on);
  void set_disable_mode_fan(bool on);
  void set_disable_swing_vertical(bool on);
  void set_disable_swing_horizontal(bool on);

  // Setters publics option for rebuild climate if mode auto or option eco enable
  void set_option_recalculate_climate(bool on);

  // Setters publics MQTT
  void set_mqtt_broker(const std::string &value);
  void set_mqtt_username(const std::string &value);
  void set_mqtt_password(const std::string &value);
  void set_mqtt_port_from_string(const std::string &value);
  void set_mqtt_connected_sensor(esphome::binary_sensor::BinarySensor *sensor);

  // Setters sensor fault/warn
  void set_filter_dirty_sensor(binary_sensor::BinarySensor *sensor);


  // Send command UART
  void reload_ac_info();

  // Getters publics globals
  std::string get_mac_address();
  std::string get_address();

  // Getters publics AC
  std::string get_mode_string_climate() const;
  std::string get_mode_string() const;
  std::string get_fan_string() const;
  std::string get_swing_string() const;
  std::string get_unit() const;

  float get_target_temperature_c() const;
  float get_target_temperature_f() const;
 
  bool is_power_on() const;
  bool is_eco_on() const;
  bool is_night_on() const;
  bool is_purifier_on() const;
  bool is_display_on() const;
  bool is_mute_on() const;
  bool is_clean_on() const;
  bool is_using_fahrenheit() const;

  // Getters publics disable option mode
  bool is_disable_mode_auto() const;
  bool is_disable_mode_heat() const;
  bool is_disable_mode_dry() const;
  bool is_disable_mode_fan() const;
  bool is_disable_swing_vertical() const;
  bool is_disable_swing_horizontal() const;

  bool is_option_recalculate_climate() const;

  //  Getters publics MQTT
  std::string get_mqtt_broker() const;
  std::string get_mqtt_username() const;
  std::string get_mqtt_password() const;
  int get_mqtt_port() const;

  // MQTT publics function
  void mqtt_connexion();
  void mqtt_initializer();
  void mqtt_callback(const std::string &topic, const std::string &payload);
  void publish_state();
  void publish_availability();
  std::string build_common_config_suffix() const;
  void publish_discovery_climate(bool recreate = false);
  void publish_discovery_mode_select(bool recreate = false);
  void publish_discovery_fan_select(bool recreate = false);
  void publish_discovery_unit_select(bool recreate = false);
  void publish_discovery_swing_select(bool recreate = false);
  void publish_discovery_swing_horizontal_select(bool recreate = false);
  void publish_discovery_clean_switch(bool recreate = false);
  void publish_discovery_eco_switch(bool recreate = false);
  void publish_discovery_display_switch(bool recreate = false);
  void publish_discovery_night_switch(bool recreate = false);
  void publish_discovery_purifier_switch(bool recreate = false);
  void publish_discovery_temperature_number(bool recreate = false);
  void publish_discovery_temperature_sensor(bool recreate = false);
  void publish_discovery_last_cmd_origin_sensor(bool recreate = false);
  void publish_discovery_filter_dirty_sensor(bool recreate = false);

  // MQTT publics function for rebuild all mqtt entities
  void rebuild_mqtt_entity();
  // MQTT publics function for apply disable option mode
  void apply_disable_settings();
 
  // Functions publics
  std::string sanitize_name(const std::string &input) const;
  std::string int_to_string(int value);

 protected:
  // variables
  // variables globales
  std::string app_name_ {""};
  std::string app_friendly_name_ {""};
  std::string app_sanitize_name_ {""};
  std::string app_mac_ {""};
  std::string app_lang_ {"en"};
  std::string app_board_ {""};

  // variables command queue
  std::vector<uint8_t> rx_buffer_;
  uint32_t last_rx_byte_time_{0};
  std::deque<std::vector<uint8_t>> tx_queue_;
  uint32_t last_tx_{0};
  static constexpr uint32_t TX_INTERVAL_MS = 300;

  // variables AC
  Mode mode_ {Mode::COOL};
  bool power_on_ {true};
  Fan  fan_ {Fan::AUTO};
  Fan  previous_fan_ {Fan::AUTO};
  uint8_t target_temp_c_ {26};
  uint8_t target_temp_f_ {78};
  float ambient_temp_c_ {0.0f};
  float ambient_temp_f_ {0.0f};
  
  bool eco_ {false};
  bool night_ {false};
  bool purifier_ {false};
  bool display_ {false};
  bool mute_ {false};
  bool clean_ {false};
  bool force_clean_ {false};
  bool use_fahrenheit_ {false};
  bool option_recalculate_climate_ {false};
  bool from_remote_ {false};
  bool filter_dirty_ {false};
  Swing  swing_position_{Swing::P1};
  SwingHorizontal  swing_horizontal_{SwingHorizontal::STOP};

  // variables AC fault/warn
  binary_sensor::BinarySensor *filter_dirty_sensor_{nullptr};

  // variables persisted AC settings
  ESPPreferenceObject mute_pref_;
  ESPPreferenceObject disable_mode_auto_pref_; 
  ESPPreferenceObject disable_mode_heat_pref_; 
  ESPPreferenceObject disable_mode_dry_pref_; 
  ESPPreferenceObject disable_mode_fan_pref_; 
  ESPPreferenceObject disable_swing_vertical_pref_; 
  ESPPreferenceObject disable_swing_horizontal_pref_; 
  ESPPreferenceObject option_recalculate_climate_pref_;

  // variables persisted MQTT
  ESPPreferenceObject mqtt_broker_address_pref_;
  ESPPreferenceObject mqtt_username_pref_;
  ESPPreferenceObject mqtt_password_pref_;
  ESPPreferenceObject mqtt_port_pref_;

  // variables MQTT
  mqtt::MQTTClientComponent *mqtt_ = nullptr;
  std::string mqtt_broker_address_;
  std::string mqtt_username_;
  std::string mqtt_password_;
  int mqtt_port_ = 1883;
  int mqtt_delay_rebuild_ = 300;
  binary_sensor::BinarySensor *mqtt_connected_sensor_{nullptr};


  // Protected functions for disable mode
  bool disable_mode_auto_ {false};
  bool disable_mode_heat_ {false};
  bool disable_mode_dry_ {false};
  bool disable_mode_fan_ {false};
  bool disable_swing_vertical_ {false};
  bool disable_swing_horizontal_ {false};

  // Protected variables optimization
  struct MqttPublishEntry {
    std::string topic;
    std::string payload;
    int qos;
    bool retain;
  };
  std::deque<MqttPublishEntry> mqtt_publish_queue_;

  // Protected functions for command queue
  void process_tx_queue();

  // Protected functions for command UART
  std::vector<uint8_t> build_frame(bool bypassMute = false) const;
  void send_command_basic(const std::vector<uint8_t> &data);
  void send_command();
  static uint16_t crc16(const uint8_t *data, size_t len);
  void decode_state(const std::vector<uint8_t> &frame);

  // force mode if select disable mode
  void force_cool_mode_if_disabled();

  // Protected functions optimization
  void publish_async(const std::string &topic, const std::string &payload, int qos, bool retain);

  // Protected functions for AC convert
  static Fan str_to_fan(const std::string& lang, const std::string &speed);
  static Mode str_to_mode_climate(const std::string &mode);
  static Mode str_to_mode(const std::string& lang, const std::string &mode);
  static std::string mode_to_string_climate(Mode mode);
  static std::string mode_to_string(const std::string& lang, Mode mode);
  static std::string fan_to_string(const std::string& lang, Fan fan);
  static Swing str_to_swing(const std::string& lang, const std::string &s);
  static SwingHorizontal str_to_swing_horizontal(const std::string& lang, const std::string &s);
  static std::string swing_to_string(const std::string& lang, Swing swing);
  static std::string swing_horizontal_to_string(const std::string& lang, SwingHorizontal swing);
  float celsius_to_fahrenheit(float c, bool floor = true) const;
  float fahrenheit_to_celsius(float f) const;
  uint8_t encode_temperature_byte() const;
  std::string build_modes_json_climate() const;
  std::string build_modes_json() const;
  std::string build_fan_speed_json() const;

  // Protected functions for rebuild climate if mode auto or option eco enable
  void recalculate_climate_depending_by_option();
};

}  // namespace acw02
}  // namespace esphome
