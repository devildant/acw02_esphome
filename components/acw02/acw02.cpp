#include "acw02.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#ifdef CONFIG_IDF_TARGET_ESP32C6
#include "driver/gpio.h"
#endif

namespace esphome {
  namespace acw02 {

    void ACW02::setup() {
      #if defined(DLOCALE_LANG)
        app_lang_ = DLOCALE_LANG;
      #else
        app_lang_ = "en";
      #endif
      #if defined(DBOARD)
        app_board_ = DBOARD;
      #endif

      // force use external Wi-Fi antenna (XIAO ESP32-C6)
      #if CONFIG_IDF_TARGET_ESP32C6
        // GPIO3 -> LOW
        gpio_reset_pin(GPIO_NUM_3);
        gpio_set_direction(GPIO_NUM_3, GPIO_MODE_OUTPUT);
        gpio_set_level(GPIO_NUM_3, 0);

        // GPIO14 -> HIGH
        gpio_reset_pin(GPIO_NUM_14);
        gpio_set_direction(GPIO_NUM_14, GPIO_MODE_OUTPUT);
        gpio_set_level(GPIO_NUM_14, 1);

        ESP_LOGW(TAG, "RF switch set for XIAO ESP32-C6: external Wi-Fi antenna enabled");
      #endif

      mqtt_connexion();
      app_name_ = App.get_name();
      app_friendly_name_ = App.get_friendly_name();
      app_sanitize_name_ = sanitize_name(app_name_);
      app_mac_ = get_mac_address();

      send_static_command_basic(keepalive_frame_);
      presets_list_element_config_ = PRESETS_LIST_ELEMENT_CONFIG_DEFAULT;
      presets_list_element_ = get_localized_name(app_lang_, "presetNone");
      mute_pref_ = global_preferences->make_preference<bool>(1U, "ac_mute");
      mute_pref_.load(&mute_);
      disable_mode_auto_pref_ = global_preferences->make_preference<bool>(2U, "ac_disable_mode_auto");
      disable_mode_auto_pref_.load(&disable_mode_auto_);
      disable_mode_heat_pref_ = global_preferences->make_preference<bool>(3U, "ac_disable_mode_heat");
      disable_mode_heat_pref_.load(&disable_mode_heat_);
      disable_mode_dry_pref_ = global_preferences->make_preference<bool>(4U, "ac_disable_mode_dry");
      disable_mode_dry_pref_.load(&disable_mode_dry_);
      disable_mode_fan_pref_ = global_preferences->make_preference<bool>(5U, "ac_disable_mode_fan");
      disable_mode_fan_pref_.load(&disable_mode_fan_);
      disable_swing_vertical_pref_ = global_preferences->make_preference<bool>(6U, "ac_disable_swing_vertical");
      disable_swing_vertical_pref_.load(&disable_swing_vertical_);
      disable_swing_horizontal_pref_ = global_preferences->make_preference<bool>(7U, "ac_disable_swing_horizontal");
      disable_swing_horizontal_pref_.load(&disable_swing_horizontal_);
      option_recalculate_climate_pref_ = global_preferences->make_preference<bool>(8U, "ac_option_recalculate_climate");
      option_recalculate_climate_pref_.load(&option_recalculate_climate_);
      option_G1_MQTT_pref_ = global_preferences->make_preference<bool>(9U, "ac_option_g1_mqtt");
      option_G1_MQTT_pref_.load(&option_g1_mqtt_);
      previous_temp_c_pref_ = global_preferences->make_preference<bool>(10U, "ac_previous_temp_c");
      previous_temp_c_pref_.load(&previous_target_temp_c_);
      previous_temp_f_pref_ = global_preferences->make_preference<bool>(11U, "ac_previous_temp_f");
      previous_temp_f_pref_.load(&previous_target_temp_f_);
      auto_off_options_when_ac_off_pref_ = global_preferences->make_preference<bool>(12U, "ac_auto_off_options_when_ac_off");
      auto_off_options_when_ac_off_pref_.load(&auto_off_options_when_ac_off_);
      mute_next_cmd_after_on_delay_pref_ = global_preferences->make_preference<int>(13U, "ac_mute_after_power_on_delay");
      if (!mute_next_cmd_after_on_delay_pref_.load(&mute_next_cmd_after_on_delay_)) {
        mute_next_cmd_after_on_delay_ = 0;
      }
      mute_next_cmd_delay_pref_ = global_preferences->make_preference<int>(14U, "ac_mute_next_cmd_delay");
      if (!mute_next_cmd_delay_pref_.load(&mute_next_cmd_delay_)) {
        mute_next_cmd_delay_ = 0;
      }
      publish_stats_after_power_on_delay_pref_ = global_preferences->make_preference<int>(15U, "ac_publish_stats_after_power_on_delay");
      if (!publish_stats_after_power_on_delay_pref_.load(&publish_stats_after_power_on_delay_)) {
        publish_stats_after_power_on_delay_ = 0;
      }
      preset_pref_ = global_preferences->make_preference<bool>(16U, "ac_preset");
      preset_pref_.load(&preset_);
      load_presets_from_flash();

      set_timeout("test", 10000, [this]() {
        ESP_LOGD(TAG, "Setup %s ", "INIT");
        ESP_LOGD(TAG, "Board %s ", app_board_.c_str());
        ESP_LOGD(TAG, "MAC %s ", app_mac_.c_str());
        ESP_LOGD(TAG, "name %s ", app_name_.c_str());
        ESP_LOGD(TAG, "friendly name %s ", app_friendly_name_.c_str());
        ESP_LOGD(TAG, "sanitze name %s ", app_sanitize_name_.c_str());
        ESP_LOGD(TAG, "MQTT broker %s ", mqtt_broker_address_.c_str());
        ESP_LOGD(TAG, "MQTT username %s ", mqtt_username_.c_str());
        ESP_LOGD(TAG, "MQTT port %d ", mqtt_port_);
        ESP_LOGD(TAG, "Lang %s ", app_lang_.c_str());
        ESP_LOGD(TAG, "name %s ", get_localized_name(app_lang_, "climate").c_str());
        ESP_LOGD(TAG, "mode json %s ", build_options_json(app_lang_, "mode").c_str());
        ESP_LOGD(TAG, "fan json %s ", build_options_json(app_lang_, "fan").c_str());
        ESP_LOGD(TAG, "swing json %s ", build_options_json(app_lang_, "swing").c_str());
        //  send_static_command_basic(get_status_frame_);
      });
    }

    void ACW02::loop() {
      process_tx_queue();

      while (available()) {
        uint8_t byte = read();
        rx_buffer_.push_back(byte);
        last_rx_byte_time_ = millis();
      }

      // for check size rx buffer
      if (rx_buffer_.size() > rx_max_depth_) {
        rx_max_depth_ = rx_buffer_.size();
        ESP_LOGW(TAG, "RX max depth = %u", (unsigned)rx_max_depth_);
      }

      // Wait at least 10ms of silence before processing buffer
      if (!rx_buffer_.empty() && millis() - last_rx_byte_time_ > 10) {
        static const std::array<size_t, 4> VALID_SIZES = {13, 18, 28, 34};

        size_t offset = 0;
        while (rx_buffer_.size() - offset >= 13) {  // Minimum frame size
          bool found = false;

          for (size_t size : VALID_SIZES) {
            if (offset + size > rx_buffer_.size())
              continue;

            if (rx_buffer_[offset] != 0x7A || rx_buffer_[offset + 1] != 0x7A)
              continue;

            const uint8_t *start = rx_buffer_.data() + offset;
            uint16_t expected_crc = (start[size - 2] << 8) | start[size - 1];
            uint16_t computed_crc = crc16(start, size - 2);

            if (computed_crc == expected_crc) {
              std::vector<uint8_t> frame(start, start + size);
              ESP_LOGW(TAG, "RX: [%s]", format_hex_pretty(frame).c_str());
              decode_state(frame);
              offset += size;
              found = true;
              break;
            }
          }

          if (!found) {
            // Skip invalid start byte and retry
            offset++;
          }
        }

        // Remove processed bytes from rx_buffer_
        if (offset > 0)
          rx_buffer_.erase(rx_buffer_.begin(), rx_buffer_.begin() + offset);
      }

      check_timeout_retry();

      static uint32_t last_keepalive = 0;
      if (!ack_wait_ && millis() - last_keepalive > INTERVAL_INTERNAL_AC_KEEPALIVE && tx_queue_.empty()) {
        if (millis() - last_rx_byte_time_ > 200) {
          last_keepalive = millis();
          send_static_command_basic(keepalive_frame_);
        }
      }
    }

    void ACW02::set_mode_climate(const std::string &mode) {
      if (mode != "off") {
        const Mode previous_mode = mode_;
        power_on_ = true;
        mode_ = str_to_mode_climate(mode);
        if (mode_ != Mode::COOL) {
          set_eco_internal(false);
        } else {
          // force set if eco_ is true (if eco is not disable auto when off)
          set_eco_internal(eco_, eco_);
        }
        if (mode_ == Mode::FAN) night_ = false;
        if (mode_ == Mode::AUTO) {
          publish_discovery_climate_deref();
        }
        if (previous_mode == Mode::AUTO) {
          target_temp_c_ = previous_target_temp_c_;
          target_temp_f_ = previous_target_temp_f_;
          publish_discovery_climate_deref();
        }
      } else {
        power_on_ = false;
        reset_options_when_off();
      }
    }

    void ACW02::set_mode(const std::string &mode) {
      if (mode != key_to_txt(app_lang_, "mode", "OFF")) {
        const Mode previous_mode = mode_;
        power_on_ = true;
        mode_ = str_to_mode(app_lang_, mode);
        if (mode_ != Mode::COOL) {
          set_eco_internal(false);
        } else {
          // force set if eco_ is true (if eco is not disable auto when off)
          set_eco_internal(eco_, eco_);
        }
        if (mode_ == Mode::FAN) night_ = false;
        if (mode_ == Mode::AUTO) {
          publish_discovery_climate_deref();
        }
        if (previous_mode == Mode::AUTO) {
          target_temp_c_ = previous_target_temp_c_;
          target_temp_f_ = previous_target_temp_f_;
          publish_discovery_climate_deref();
        }
      } else {
        power_on_ = false;
        reset_options_when_off();
      }
    }

    void ACW02::set_swing(const std::string &pos) {
      swing_position_ = str_to_swing(app_lang_, pos);
    }

    void ACW02::set_swing_horizontal(const std::string &pos) {
      swing_horizontal_ = str_to_swing_horizontal(app_lang_, pos);
    }

    void ACW02::set_purifier(bool on) {
      if (power_on_) {
        purifier_ = on;
      } else {
        purifier_ = false;
      }
    }

    void ACW02::set_mute(bool on) {
      if (mute_ != on) {
        mute_ = on;
        mute_pref_.save(&mute_);
        publish_state();
      }
      
    }

    void ACW02::set_unit(const std::string &unit) {
      bool oldValue = use_fahrenheit_;
      bool newValue = (unit == "°F" || unit == "°f");

      if (oldValue != newValue) {
        use_fahrenheit_ = newValue;
        publish_discovery_climate(true);
        publish_discovery_temperature_number(true);
        publish_discovery_temperature_sensor(true);
      }
    }

    void ACW02::set_clean(bool on) {
      if (on != clean_) {
        if (on && (power_on_ || mode_ != Mode::COOL && mode_ != Mode::DRY))
        {
          power_on_ = false;
          if (mode_ != Mode::COOL && mode_ != Mode::DRY) {
            mode_ = Mode::COOL;
          }
          send_command_basic(build_frame());
          force_clean_ = true;
          clean_ = on;
          set_timeout("clean_delay", 3000, [this, on]() {
            send_command_basic(build_frame(true));
            force_clean_ = false;
          });
        } else {
          clean_ = on;
          force_clean_ = false;
          send_command_basic(build_frame(true));
        }
      }
    }

    bool ACW02::set_temperature_c(float temp) {
      if (!use_fahrenheit_) {
        const uint8_t oldC = target_temp_c_;
        const uint8_t oldF = target_temp_f_;

        if (temp < 16) temp = 16;
        if (temp > 31) temp = 31;
        target_temp_f_ = static_cast<uint8_t>(celsius_to_fahrenheit(temp));
        target_temp_c_ = static_cast<uint8_t>(temp);
        if ((!eco_ && mode_ != Mode::AUTO || !power_on_)) {
          return true;
        } else {
          set_timeout("publishStateDelay", 100, [this, oldC, oldF]() {
            target_temp_f_ = oldF;
            target_temp_c_ = oldC;
            publish_state();
          });
        }
      }
      return false;
    }

    bool ACW02::set_temperature_f(float temp) {
      if (use_fahrenheit_) {
        const uint8_t oldC = target_temp_c_;
        const uint8_t oldF = target_temp_f_;

        if (temp < 61) temp = 61;
        if (temp > 88) temp = 88;
        target_temp_f_ = static_cast<uint8_t>(temp);
        target_temp_c_ = static_cast<uint8_t>(fahrenheit_to_celsius(temp));
        if ((!eco_ && mode_ != Mode::AUTO || !power_on_)) {
          return true;
        } else {
          set_timeout("publishStateDelay", 100, [this, oldC, oldF]() {
            target_temp_f_ = oldF;
            target_temp_c_ = oldC;
            publish_state();
          });
        }
      }
      return false;
    }

    bool ACW02::set_fan(const std::string &speed) {
      if (!eco_) {
        fan_ = str_to_fan(app_lang_, speed);
        return true;
      } else {
        ESP_LOGI(TAG, "Ignore command : mode ECO enabled");
        fan_ = str_to_fan(app_lang_, speed);
        set_timeout("publishStateDelay", 100, [this]() {
          fan_ = Fan::AUTO;
          publish_state();
        });
      }
      return false;
    }

    bool ACW02::set_display(bool on) {
      ESP_LOGI(TAG, "set display current : %s target : %s", display_ ? "on" : "off", on ? "on" : "off");
      if (display_ != on) {
        display_ = on;
        return true;
      }
      return false;
    }

    bool ACW02::set_eco(bool on) {
      if (eco_ != on) {
        if (power_on_) {
          if (mode_ == Mode::COOL) {
            return set_eco_internal(on);
          } else {
            eco_ = false;
          }
        } else {
          eco_ = false;
        }
      }
      return false;
    }

    bool ACW02::set_eco_internal(bool on, bool force) {
      if (eco_ != on || force) {
        eco_ = on;
        if (on) {
          night_ = false;
          previous_fan_ = fan_;
          fan_ = Fan::AUTO;
          recalculate_climate_depending_by_option();
          ESP_LOGI(TAG, "set_eco %s", "store previous fan and enable auto");
        } else {
          fan_ = previous_fan_;
          recalculate_climate_depending_by_option();
          ESP_LOGI(TAG, "set_eco %s", "restore previous");
        }
        return true;
      }
      return false;
    }

    bool ACW02::set_night(bool on) {
      if (power_on_) {
        if (mode_ == Mode::COOL || mode_ == Mode::DRY || mode_ == Mode::HEAT) {
          if (night_ != on) {
            night_ = on;
            if (night_) {
              if (eco_)
              {
                set_eco_internal(false);
              }
            }
            return true;
          }
        } else {
          night_ = false;
        }
      } else {
        night_ = false;
      }
      return false;
    }

    void ACW02::set_auto_off_options_when_ac_off(bool on) {
      if (auto_off_options_when_ac_off_ != on) {
        auto_off_options_when_ac_off_ = on;
        auto_off_options_when_ac_off_pref_.save(&auto_off_options_when_ac_off_);
        publish_state();
      }
    }

    void ACW02::set_mute_next_cmd_delay_string(const std::string &value) {
      if (value.empty() || !std::all_of(value.begin(), value.end(), ::isdigit)) {
        ESP_LOGI(TAG, "Invalid delay (non-numeric) : '%s'", value.c_str());
        mute_next_cmd_delay_ = 0;
        return;
      }
      int delay = std::stoi(value);
      if (delay > 60000) {
        delay = 60000;
      }
      int prev = mute_next_cmd_delay_;
      mute_next_cmd_delay_ = delay;
      mute_next_cmd_delay_pref_.save(&mute_next_cmd_delay_);
      publish_state();
      if (mute_next_cmd_delay_text_ != nullptr && prev != mute_next_cmd_delay_) {
        mute_next_cmd_delay_text_->publish_state(
          std::to_string(delay)
        );
      }
      ESP_LOGI(TAG, "Delay saved : %d", delay);
    }

    void ACW02::set_mute_next_cmd_after_on_delay_string(const std::string &value) {
      if (value.empty() || !std::all_of(value.begin(), value.end(), ::isdigit)) {
        ESP_LOGI(TAG, "Invalid delay (non-numeric) : '%s'", value.c_str());
        mute_next_cmd_after_on_delay_ = 0;
        return;
      }
      int delay = std::stoi(value);
      if (delay > 60000) {
        delay = 60000;
      }
      int prev = mute_next_cmd_after_on_delay_;
      mute_next_cmd_after_on_delay_ = delay;
      mute_next_cmd_after_on_delay_pref_.save(&mute_next_cmd_after_on_delay_);
      publish_state();
      if (mute_next_cmd_after_on_delay_text_ != nullptr && prev != mute_next_cmd_after_on_delay_) {
        mute_next_cmd_after_on_delay_text_->publish_state(
          std::to_string(delay)
        );
      }
      ESP_LOGI(TAG, "Delay after on saved : %d", delay);
    }

    void ACW02::set_publish_stats_after_power_on_delay_string(const std::string &value) {
      if (value.empty() || !std::all_of(value.begin(), value.end(), ::isdigit)) {
        ESP_LOGI(TAG, "Invalid delay (non-numeric) : '%s'", value.c_str());
        publish_stats_after_power_on_delay_ = 0;
        return;
      }
      int delay = std::stoi(value);
      if (delay > 1000) {
        delay = 1000;
      }
      int prev = publish_stats_after_power_on_delay_;
      publish_stats_after_power_on_delay_ = delay;
      publish_stats_after_power_on_delay_pref_.save(&publish_stats_after_power_on_delay_);
      publish_state();
      if (publish_stats_after_power_on_delay_text_ != nullptr && prev != publish_stats_after_power_on_delay_) {
        publish_stats_after_power_on_delay_text_->publish_state(
          std::to_string(delay)
        );
      }
      ESP_LOGI(TAG, "Delay publish after on saved : %d", delay);
    }

    
    void ACW02::set_g1_mqtt_options(bool on) {
      if (option_g1_mqtt_ != on) {
        option_g1_mqtt_ = on;
        option_G1_MQTT_pref_.save(&option_g1_mqtt_);
        publish_discovery_g1_mute_switch(true);
        publish_discovery_g1_option_recalculate_climate_switch(true);
        publish_discovery_g1_reset_eco_purifier_ac_off_switch(true);
        publish_discovery_disable_mode_auto_switch(true);
        publish_discovery_disable_mode_heat_switch(true);
        publish_discovery_disable_mode_dry_switch(true);
        publish_discovery_disable_mode_fan_switch(true);
        publish_discovery_disable_swing_vertical_switch(true);
        publish_discovery_disable_swing_horizontal_switch(true);
        publish_discovery_g1_mute_next_cmd_delay_text(true);
        publish_discovery_g1_mute_next_cmd_after_on_delay_text(true);
        publish_discovery_g1_publish_stats_after_power_on_delay_text(true);
        publish_discovery_g1_reload_button(true);
        publish_discovery_g1_rebuild_mqtt_entities_button(true);
        publish_discovery_g1_get_status_button(true);
        publish_discovery_z_config_validate_button(true);
        publish_discovery_mdns_text_sensor(true);
        publish_discovery_wifi_ip_text_sensor(true);
        publish_discovery_wifi_ssid_text_sensor(true);
        publish_discovery_wifi_bssid_text_sensor(true);
        publish_discovery_wifi_mac_text_sensor(true);
        publish_discovery_esp_version_text_sensor(true);
        publish_discovery_wifi_signal_text_sensor(true);
        publish_discovery_internal_temperature_text_sensor(true);
        publish_discovery_esp_memory_text_sensor(true);
      }
    }

    void ACW02::set_disable_mode_auto(bool on, bool published) {
      if (disable_mode_auto_ != on) {
        disable_mode_auto_ = on;
        disable_mode_auto_pref_.save(&disable_mode_auto_);
        if (published) {
          publish_state();
        }
      }
    }

    void ACW02::set_disable_mode_heat(bool on, bool published) {
      if (disable_mode_heat_ != on) {
        disable_mode_heat_ = on;
        disable_mode_heat_pref_.save(&disable_mode_heat_);
        if (published) {
          publish_state();
        }
      }
    }

    void ACW02::set_disable_mode_dry(bool on, bool published) {
      if (disable_mode_dry_ != on) {
        disable_mode_dry_ = on;
        disable_mode_dry_pref_.save(&disable_mode_dry_);
        if (published) {
          publish_state();
        }
      }
    }

    void ACW02::set_disable_mode_fan(bool on, bool published) {
      if (disable_mode_fan_ != on) {
        disable_mode_fan_ = on;
        disable_mode_fan_pref_.save(&disable_mode_fan_);
        if (published) {
          publish_state();
        }
      }
    }

    void ACW02::set_disable_swing_vertical(bool on, bool published) {
      if (disable_swing_vertical_ != on) {
        if (preset_ && !disable_swing_horizontal_ && !on) {
          set_disable_swing_horizontal(true, false);
        }
        disable_swing_vertical_ = on;
        disable_swing_vertical_pref_.save(&disable_swing_vertical_);
        if (published) {
          publish_state();
        }
      }
    }

    void ACW02::set_disable_swing_horizontal(bool on, bool published) {
      if (disable_swing_horizontal_ != on) {
        if (preset_ && !disable_swing_vertical_ && !on) {
          set_disable_swing_vertical(true, false);
        }
        disable_swing_horizontal_ = on;
        disable_swing_horizontal_pref_.save(&disable_swing_horizontal_);
        if (published) {
          publish_state();
        }
      }
    }

    void ACW02::entity_mdns_sync(const std::string &payload) {
      std::string tmpEntity = app_mdns_;
      app_mdns_ = payload;
      if (app_mdns_ != tmpEntity) {
        publish_state();
      }
    }

    void ACW02::entity_wifi_ip_sync(const std::string &payload) {
      std::string tmpEntity = app_wifi_ip_;
      app_wifi_ip_ = payload;
      if (app_wifi_ip_ != tmpEntity) {
        publish_state();
      }
    }

    void ACW02::entity_wifi_ssid_sync(const std::string &payload) {
      std::string tmpEntity = app_wifi_ssid_;
      app_wifi_ssid_ = payload;
      if (app_wifi_ssid_ != tmpEntity) {
        publish_state();
      }
    }

    void ACW02::entity_wifi_bssid_sync(const std::string &payload) {
      std::string tmpEntity = app_wifi_bssid_;
      app_wifi_bssid_ = payload;
      if (app_wifi_bssid_ != tmpEntity) {
        publish_state();
      }
    }

    void ACW02::entity_wifi_mac_sync(const std::string &payload) {
      std::string tmpEntity = app_wifi_mac_;
      app_wifi_mac_ = payload;
      if (app_wifi_mac_ != tmpEntity) {
        publish_state();
      }
    }

    void ACW02::entity_esp_version_sync(const std::string &payload) {
      std::string tmpEntity = app_esp_version_;
      app_esp_version_ = payload;
      if (app_esp_version_ != tmpEntity) {
        publish_state();
      }
    }

    void ACW02::entity_wifi_signal_sync(const std::string &payload) {
      std::string tmpEntity = app_wifi_signal_;
      app_wifi_signal_ = payload;
      if (app_wifi_signal_ != tmpEntity) {
        publish_state();
      }
    }

    void ACW02::entity_internal_temperature_sync(const std::string &payload) {
      std::string tmpEntity = app_internal_temperature_;
      std::string formatted = payload;
      std::replace(formatted.begin(), formatted.end(), '.', ',');
      app_internal_temperature_ = formatted;
      if (app_internal_temperature_ != tmpEntity) {
        publish_state();
      }
    }
    
    void ACW02::entity_esp_memory_sync(const std::string &payload) {
      std::string tmpEntity = app_esp_memory_;
      std::string formatted = payload;
      std::replace(formatted.begin(), formatted.end(), '.', ',');
      app_esp_memory_ = formatted;
      if (app_esp_memory_ != tmpEntity) {
        publish_state();
      }
    }

    void ACW02::set_preset(bool on, bool published) {
      if (preset_ != on) {
        if (on && !disable_swing_vertical_ && !disable_swing_horizontal_) {
          set_disable_swing_horizontal(true, false);
        }
        preset_ = on;
        preset_pref_.save(&preset_);
        publish_discovery_climate(true);
        if (published) {
          publish_state();
        }
      }
    }

    void ACW02::set_option_recalculate_climate(bool on) {
      if (option_recalculate_climate_ != on) {
        option_recalculate_climate_ = on;
        option_recalculate_climate_pref_.save(&option_recalculate_climate_);
        publish_discovery_climate(true);
        publish_state();
      }
    }

    void ACW02::set_mqtt_broker(const std::string &value) {
      mqtt_broker_address_ = value;
      char buf[64];
      std::strncpy(buf, value.c_str(), sizeof(buf));
      buf[sizeof(buf) - 1] = '\0';
      mqtt_broker_address_pref_.save((const char (*)[64]) &buf);
    }

    void ACW02::set_mqtt_username(const std::string &value) {
      mqtt_username_ = value;
      char buf[64];
      std::strncpy(buf, value.c_str(), sizeof(buf));
      buf[sizeof(buf) - 1] = '\0';
      mqtt_username_pref_.save((const char (*)[64]) &buf);
    }

    void ACW02::set_mqtt_password(const std::string &value) {
      mqtt_password_ = value;
      char buf[64];
      std::strncpy(buf, value.c_str(), sizeof(buf));
      buf[sizeof(buf) - 1] = '\0';
      mqtt_password_pref_.save((const char (*)[64]) &buf);
    }

    void ACW02::set_mqtt_port_from_string(const std::string &value) {
      if (value.empty() || !std::all_of(value.begin(), value.end(), ::isdigit)) {
        ESP_LOGI(TAG, "Invalid MQTT port (non-numeric) : '%s'", value.c_str());
        return;
      }

      int port = std::stoi(value);
      if (port < 1 || port > 65535) {
        ESP_LOGI(TAG, "Invalid MQTT port (out of range) : %d", port);
        port = 1883;
      }

      mqtt_port_ = port;
      mqtt_port_pref_.save(&mqtt_port_);

      ESP_LOGI(TAG, "MQTT port saved : %d", port);
    }

    void ACW02::set_mqtt_connected_sensor(esphome::binary_sensor::BinarySensor *sensor) {
      mqtt_connected_sensor_ = sensor;
    }

    void ACW02::set_filter_dirty_sensor(binary_sensor::BinarySensor *sensor) {
      filter_dirty_sensor_ = sensor;
    }

    void ACW02::set_warn_sensor(binary_sensor::BinarySensor *sensor) {
      warn_sensor_ = sensor;
    }

    void ACW02::set_error_sensor(binary_sensor::BinarySensor *sensor) {
      error_sensor_ = sensor;
    }

    void ACW02::set_warn_text_sensor(esphome::text_sensor::TextSensor *sensor) {
      warn_text_sensor_ = sensor;
    }

    void ACW02::set_error_text_sensor(esphome::text_sensor::TextSensor *sensor) {
      error_text_sensor_ = sensor;
    }

    void ACW02::set_mute_next_cmd_delay_text(esphome::text::Text *text) {
      mute_next_cmd_delay_text_ = text;
    }

    void ACW02::set_mute_next_cmd_after_on_delay_text(esphome::text::Text *text) {
      mute_next_cmd_after_on_delay_text_ = text;
    }

    void ACW02::set_publish_stats_after_power_on_delay_text(esphome::text::Text *text) {
      publish_stats_after_power_on_delay_text_ = text;
    }

    void ACW02::reload_ac_info() {
      send_static_command_basic(get_status_frame_);
    }

    std::string ACW02::get_mac_address() {
      uint8_t mac[6];
      esp_read_mac(mac, ESP_MAC_WIFI_STA);
      char buf[13];
      snprintf(buf, sizeof(buf), "%02x%02x%02x%02x%02x%02x",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      return std::string(buf);
    }

    std::string ACW02::get_address() {
      return wifi::global_wifi_component->get_use_address();
    }

    std::string ACW02::get_mode_string_climate() const {
      if (is_power_on()) {
        return mode_to_string_climate(mode_);
      } else {
        return "off";
      }
    }

    std::string ACW02::get_mode_string() const {
      if (is_power_on()) {
        return mode_to_string(app_lang_, mode_);
      } else {
        return key_to_txt(app_lang_, "mode", "OFF");
      }
    }

    std::string ACW02::get_fan_string() const {
      return fan_to_string(app_lang_, fan_);
    }

    std::string ACW02::get_swing_string() const {
      return swing_to_string(app_lang_, swing_position_);
    }

    std::string ACW02::get_unit() const {
      return use_fahrenheit_ ? "°F" : "°C";
    }

    float ACW02::get_target_temperature_c() const {
      return target_temp_c_;
    }

    float ACW02::get_target_temperature_f() const {
      return target_temp_f_;
    }

    bool ACW02::is_power_on() const {
      return power_on_;
    }

    bool ACW02::is_eco_on() const {
      return eco_;
    }

    bool ACW02::is_night_on() const {
      return night_;
    }

    bool ACW02::is_purifier_on() const {
      return purifier_;
    }

    bool ACW02::is_display_on() const {
      return display_;
    }

    bool ACW02::is_mute_on() const {
      return mute_;
    }

    bool ACW02::is_clean_on() const {
      return clean_;
    }

    bool ACW02::is_using_fahrenheit() const {
      return use_fahrenheit_;
    }

    bool ACW02::is_auto_off_options_when_ac_off() const {
      return auto_off_options_when_ac_off_;
    }

    int ACW02::get_mute_next_cmd_delay() const {
      return mute_next_cmd_delay_;
    }

    int ACW02::get_mute_next_cmd_after_on_delay() const {
      return mute_next_cmd_after_on_delay_;
    }

    int ACW02::get_publish_stats_after_power_on_delay() const {
      return publish_stats_after_power_on_delay_;
    }

    bool ACW02::is_g1_mqtt_options() const {
      return option_g1_mqtt_;
    }
    
    bool ACW02::is_disable_mode_auto() const {
      return disable_mode_auto_;
    }

    bool ACW02::is_disable_mode_heat() const {
      return disable_mode_heat_;
    }

    bool ACW02::is_disable_mode_dry() const {
      return disable_mode_dry_;
    }

    bool ACW02::is_disable_mode_fan() const {
      return disable_mode_fan_;
    }

    bool ACW02::is_disable_swing_vertical() const {
      return disable_swing_vertical_;
    }

    bool ACW02::is_disable_swing_horizontal() const {
      return disable_swing_horizontal_;
    }

    bool ACW02::is_option_recalculate_climate() const {
      return option_recalculate_climate_;
    }

    std::string ACW02::get_mqtt_broker() const {
      return mqtt_broker_address_;
    }

    std::string ACW02::get_mqtt_username() const {
      return mqtt_username_;
    }

    std::string ACW02::get_mqtt_password() const {
      return mqtt_password_;
    }

    int ACW02::get_mqtt_port() const {
      return mqtt_port_;
    }

    void ACW02::mqtt_connexion() {
      mqtt_ = mqtt::global_mqtt_client;
      mqtt_broker_address_pref_ = global_preferences->make_preference<char[64]>(9991U);
      char bufBroker[64] = {0};
      if (mqtt_broker_address_pref_.load((char (*)[64]) &bufBroker)) {
        mqtt_broker_address_ = bufBroker;
      } else {
        mqtt_broker_address_ = "";
      }

      mqtt_username_pref_ = global_preferences->make_preference<char[64]>(9992U);
      char bufUsername[64] = {0};
      if (mqtt_username_pref_.load((char (*)[64]) &bufUsername)) {
        mqtt_username_ = bufUsername;
      } else {
        mqtt_username_ = "";
      }

      mqtt_password_pref_ = global_preferences->make_preference<char[64]>(9993U);
      char bufPassword[64] = {0};
      if (mqtt_password_pref_.load((char (*)[64]) &bufPassword)) {
        mqtt_password_ = bufPassword;
      } else {
        mqtt_password_ = "";
      }

      mqtt_port_pref_ = global_preferences->make_preference<int>(9994U);
      if (!mqtt_port_pref_.load(&mqtt_port_)) {
        mqtt_port_ = 1883;
      }

      mqtt_->disable();
      mqtt_->set_enable_on_boot(false);
      mqtt_->set_broker_address(mqtt_broker_address_);
      mqtt_->set_broker_port(mqtt_port_);
      if (!mqtt_username_.empty())
      mqtt_->set_username(mqtt_username_);
      if (!mqtt_password_.empty())
      mqtt_->set_password(mqtt_password_);

      set_timeout("mqtt_connect", 2000, [this]() {
        if (wifi::global_wifi_component->is_connected()) {
          ESP_LOGW(TAG, "Wi-Fi OK; attempting MQTT connection…");
          mqtt_->enable();
          mqtt_initializer();
          // The code below is commented out because it can cause a bug with MQTT if it is stopped for a long time.
          // set_timeout("mqtt_retry", 5000, [this]() {
          //   if (!mqtt_->is_connected()) {
          //     mqtt_->disable();
          //   }
          // });
        } else {
          ESP_LOGW(TAG, "Wi-Fi not connected; retrying in 5 s");
          set_timeout("mqtt_retry", 2000, [this]() { mqtt_connexion(); });
        }
      });
    }

    void ACW02::mqtt_initializer() {
      if (mqtt_) {
        mqtt_->set_on_connect([this](bool first) {
          mqtt_publish_queue_.clear();
          ESP_LOGW(TAG, "MQTT connected → publishing discovery and state");
          if (mqtt_connected_sensor_) mqtt_connected_sensor_->publish_state(true);
          set_timeout("mqtt_discovery_delay", 100, [this]() {
            set_interval("mqtt_publish_flush", INTERVAL_MQTT_BETWEEN_CMD, [this]() {
              // The code below is commented out because it can cause a bug with MQTT if it is stopped for a long time.
              // if (!mqtt_ || !mqtt_->is_connected()) {
              //   ESP_LOGE(TAG, "mqtt_publish_flush return");
              //   return;
              // }
              if (!mqtt_ ) {
                return;
              }
              if (!mqtt_publish_queue_.empty()) {
                const auto &entry = mqtt_publish_queue_.front();
                ESP_LOGW(TAG, "mqtt_publish_queue_ publish to mqtt %s", entry.topic.c_str());
                mqtt_->publish(entry.topic, entry.payload, entry.qos, entry.retain);
                mqtt_publish_queue_.pop_front();
              }
              });
              this->set_interval("hb_last_seen", INTERVAL_INTERNAL_MQTT_KEEPALIVE, [this]() {
                // Get current UNIX time (epoch seconds)
                time_t now = ::time(nullptr);

                // Convert to string
                std::string payload = std::to_string(static_cast<long>(now));

                // Publish retained false, QoS 1
                publish_async(app_name_ + "/last_seen", payload, 1, false);

                ESP_LOGW(TAG, "Heartbeat last_seen published (epoch): %s", payload.c_str());
              });
              publish_discovery_climate();
              publish_discovery_mode_select();
              publish_discovery_fan_select();
              publish_discovery_swing_select();
              publish_discovery_swing_horizontal_select();
              publish_discovery_preset_select();
              publish_discovery_preset_config_select();
              publish_discovery_unit_select();
              publish_discovery_clean_switch();
              publish_discovery_eco_switch();
              publish_discovery_display_switch();
              publish_discovery_night_switch();
              publish_discovery_purifier_switch();
              publish_discovery_preset_switch();
              publish_discovery_preset_name_config_text();
              publish_discovery_temperature_number();
              publish_discovery_preset_save_button();
              publish_discovery_preset_delete_button();
              publish_discovery_temperature_sensor();
              publish_discovery_last_cmd_origin_sensor();
              publish_discovery_filter_dirty_sensor();
              publish_discovery_warn_sensor();
              publish_discovery_error_sensor();
              publish_discovery_cmd_failure_counter_sensor();
              publish_discovery_warn_text_sensor();
              publish_discovery_error_text_sensor();
              if (option_g1_mqtt_) {
                set_timeout("publish_discovery_option_g1_mqtt_recreate", DELAY_MQTT_G1_GENERATE, [this]() {
                  publish_discovery_g1_mute_switch();
                  publish_discovery_g1_option_recalculate_climate_switch();
                  publish_discovery_g1_reset_eco_purifier_ac_off_switch();
                  publish_discovery_disable_mode_auto_switch();
                  publish_discovery_disable_mode_heat_switch();
                  publish_discovery_disable_mode_dry_switch();
                  publish_discovery_disable_mode_fan_switch();
                  publish_discovery_disable_swing_vertical_switch();
                  publish_discovery_disable_swing_horizontal_switch();
                  publish_discovery_g1_mute_next_cmd_delay_text();
                  publish_discovery_g1_mute_next_cmd_after_on_delay_text();
                  publish_discovery_g1_publish_stats_after_power_on_delay_text();
                  publish_discovery_g1_reload_button();
                  publish_discovery_g1_rebuild_mqtt_entities_button();
                  publish_discovery_g1_get_status_button();
                  publish_discovery_z_config_validate_button();
                  publish_discovery_mdns_text_sensor();
                  publish_discovery_wifi_ip_text_sensor();
                  publish_discovery_wifi_ssid_text_sensor();
                  publish_discovery_wifi_bssid_text_sensor();
                  publish_discovery_wifi_mac_text_sensor();
                  publish_discovery_esp_version_text_sensor();
                  publish_discovery_wifi_signal_text_sensor();
                  publish_discovery_internal_temperature_text_sensor();
                  publish_discovery_esp_memory_text_sensor();
                });
              }
              send_static_command_basic(get_status_frame_);
            });
            mqtt_->subscribe(app_name_ + "/cmd/#", [this](const std::string &topic, const std::string &payload) {
              mqtt_callback(topic, payload);
            }, 1);
            mqtt_->subscribe(app_name_ + "/cmd_ac/#", [this](const std::string &topic, const std::string &payload) {
              mqtt_callback(topic, payload);
            }, 1);
            mqtt_->subscribe(app_name_ + "/ping/request", [this](const std::string &topic, const std::string &payload) {
              ESP_LOGW(TAG, "Ping request received: %s", payload.c_str());
              this->set_timeout("delayed_ping_response", 200, [this, payload]() {
                publish_async(app_name_ + "/ping/response", payload, 1, false);
              });
            }, 1);
            // for generate failed compare fingerprint
            // set_interval("test", 500, [this]() {
            //   write_array(get_status_frame_);
            // });
        });
        mqtt_->set_on_disconnect([this](mqtt::MQTTClientDisconnectReason reason) {
          ESP_LOGW(TAG, "MQTT disconnected (reason=%d)", static_cast<int>(reason));
          if (mqtt_connected_sensor_) mqtt_connected_sensor_->publish_state(false);
          this->cancel_interval("mqtt_publish_flush");
          this->cancel_interval("hb_last_seen");
          this->cancel_timeout("mqtt_discovery_delay");
          ack_wait_ = false;
          ack_block_until_ = 0;
          timeout_retry_pending_ = false;
          cmd_send_fingerprint_ = {0, "", {}, 0, 0};
        });
      }
    }

    void ACW02::mqtt_callback(const std::string &topic, const std::string &payload) {
      ESP_LOGW(TAG, "mqtt_callback =====>>>> start !");
      std::string cmd = topic.substr(topic.find_last_of('/') + 1);
      if (topic.find("/cmd_ac/") != std::string::npos) {
        ESP_LOGW(TAG, "mqtt_callback cmd type cmd_ac !");
      }

      //calc start fingerprint
      bool tmp_send_cmd = false;
      uint32_t start_f = ac_to_fingerprint();
      bool prev_power_status = is_power_on();
      bool isDisplayCmd = false;
      bool was_power = power_on_;
      Mode was_mode = mode_;

      ESP_LOGW(TAG, "mqtt_callback_ payload %s %s %s", cmd.c_str(), topic.c_str(), payload.c_str());
      ESP_LOGW(TAG, "mqtt_callback_ status %s %s", power_on_ ? "on" : "off",  mode_to_string_climate(mode_).c_str());
      if (cmd == "power_climate") {
        set_mode_climate(payload == "OFF" ? "off" : mode_to_string_climate(mode_));
        tmp_send_cmd = true;
      } else if (cmd == "power") {
        set_mode(payload == key_to_txt(app_lang_, "mode", "OFF") ? key_to_txt(app_lang_, "mode", "OFF") : mode_to_string(app_lang_, mode_));
        tmp_send_cmd = true;
      } else if (cmd == "mode_climate") {
        ESP_LOGW(TAG, "mqtt_callback_ set_mode_climate");
        set_mode_climate(payload);
        tmp_send_cmd = true;
      } else if (cmd == "mode") {
        set_mode(payload);
        tmp_send_cmd = true;
      } else if (cmd == "fan") {
        tmp_send_cmd = set_fan(payload);
      } else if (cmd == "temp_c") {
        tmp_send_cmd = set_temperature_c(std::stof(payload));
      } else if (cmd == "temp_f") {
        tmp_send_cmd = set_temperature_f(std::stof(payload));
      } else if (cmd == "eco") {
        tmp_send_cmd = set_eco(payload == "on");
      } else if (cmd == "night") {
        tmp_send_cmd = set_night(payload == "on");
      } else if (cmd == "purifier") {
        set_purifier(payload == "on");
        tmp_send_cmd = true;
      } else if (cmd == "display") {
        tmp_send_cmd = set_display(payload == "on");
        isDisplayCmd = true;
      } else if (cmd == "swing") {
        set_swing(payload);
        tmp_send_cmd = true;
      }  else if (cmd == "swing_horizontal") {
        set_swing_horizontal(payload);
        tmp_send_cmd = true;
      } else if (cmd == "clean") {
        set_clean(payload == "on");
      } else if (cmd == "unit") {
        set_unit(payload);
        tmp_send_cmd = true;
      } else if (cmd == "g1_mute") {
        set_mute(payload == "on" ? true : false);
      } else if (cmd == "g1_reset_eco_purifier") {
        set_auto_off_options_when_ac_off(payload == "on" ? true : false);
      } else if (cmd == "g1_option_recalculate_climate") {
        set_option_recalculate_climate(payload == "on" ? true : false);
      } else if (cmd == "g1_restart_module_ac") {
        esphome::App.safe_reboot();
      } else if (cmd == "g1_rebuild_mqtt_entities") {
        rebuild_mqtt_entity();
      } else if (cmd == "g1_get_status") {
        reload_ac_info();
      } else if (cmd == "g1_mute_next_cmd_delay") {
        set_mute_next_cmd_delay_string(payload);
      } else if (cmd == "g1_mute_next_cmd_after_on_delay") {
        set_mute_next_cmd_after_on_delay_string(payload);
      } else if (cmd == "g1_publish_stats_after_power_on_delay") {
        set_publish_stats_after_power_on_delay_string(payload);
      } else if (cmd == "disable_mode_auto") {
        set_disable_mode_auto(payload == "on" ? true : false, false);
      } else if (cmd == "disable_mode_heat") {
        set_disable_mode_heat(payload == "on" ? true : false, false);
      } else if (cmd == "disable_mode_dry") {
        set_disable_mode_dry(payload == "on" ? true : false, false);
      } else if (cmd == "disable_mode_fan") {
        set_disable_mode_fan(payload == "on" ? true : false, false);
      } else if (cmd == "disable_swing_vertical") {
        set_disable_swing_vertical(payload == "on" ? true : false, false);
      } else if (cmd == "disable_swing_horizontal") {
        set_disable_swing_horizontal(payload == "on" ? true : false, false);
      } else if (cmd == "z_config_validate") {
        apply_disable_settings();
      } else if (cmd == "preset") {
        set_preset(payload == "on" ? true : false, false);
      } else if (cmd == "presets_list_element_config") {
        presets_list_element_config_ = payload;
        PresetSlot preset = get_preset_by_name(presets_list_element_config_);

        if (!preset.name.empty()) {
          ESP_LOGI(TAG, "Preset %d frame found with %d octets", preset.index, preset.frame_with_fp.frame.size());
          ESP_LOGW(TAG, "Preset \"%s\" frame: %s fingerprint: 0x%08X", preset.name.c_str(), format_hex_pretty(preset.frame_with_fp.frame).c_str(), preset.frame_with_fp.fingerprint);
          size_t pos = preset.name.find(" (empty)");
          if (pos != std::string::npos) {
            preset.name = "Preset";
          }
          if (preset.name.length() >= 2 && std::isdigit(preset.name[0]) && preset.name[1] == ' ') {
            preset.name = preset.name.substr(2);
          }
          preset_name_config_ = preset.name;
        } else {
          preset_name_config_ = "";
        }
      } else if (cmd == "presets_list_element") {
        presets_list_element_ = payload;
        if (payload == "none") {
          presets_list_element_ = get_localized_name(app_lang_, "presetNone");
        } else {
          presets_list_element_ = payload;
        }
        PresetSlot preset = get_preset_by_name(presets_list_element_);
        if (preset.name != get_localized_name(app_lang_, "presetNone")) {
          preset.frame_with_fp.timestamp_ms = millis();
          send_command_basic(preset.frame_with_fp);
        }
      } else if (cmd == "preset_save") {
        update_selected_preset(preset_name_config_, build_frame());
        preset_name_config_ = "";
        presets_list_element_ = get_localized_name(app_lang_, "presetNone");
      } else if (cmd == "preset_delete") {
        delete_preset_by_name();
        preset_name_config_ = "";
        presets_list_element_ = get_localized_name(app_lang_, "presetNone");
      } else if (cmd == "preset_name_config") {
        if (payload.size() > 22) {
            preset_name_config_ = payload.substr(0, 22);
        } else {
          preset_name_config_ = payload;
        }
      }

      bool after_power_status = is_power_on();
      uint32_t end_f = ac_to_fingerprint();
      bool changed = !compare_fingerprints(start_f, end_f);

      ESP_LOGW(TAG,
        "DECIDE cmd=%s fp_start=0x%08X fp_end=0x%08X changed=%d | before{pwr=%d,mode=%d,temp_enc=%u} after{pwr=%d,mode=%d,temp_enc=%u}",
        cmd.c_str(), start_f, end_f, (int)changed,
        (int)was_power, (int)was_mode, (unsigned)encode_temperature_byte(),
        (int)power_on_, (int)mode_, (unsigned)encode_temperature_byte());

      ESP_LOGW(TAG, "mqtt_callback_ compare_fingerprints 0x%08X 0x%08X", start_f, end_f);
      if (tmp_send_cmd && changed) {
        ESP_LOGW(TAG, "mqtt_callback_ send_command");
        send_command(isDisplayCmd);
        presets_list_element_ = get_localized_name(app_lang_, "presetNone");
        if (mute_next_cmd_after_on_delay_ > 0 && !prev_power_status && after_power_status) {
          mute_tmp_mqtt_ = true;
          this->set_timeout("mute_tmp_after_on", mute_next_cmd_after_on_delay_, [this]() {
            mute_tmp_mqtt_ = false;
          });
        } else if (mute_next_cmd_delay_ > 0) {
          mute_tmp_mqtt_ = true;
          this->set_timeout("mute_tmp", mute_next_cmd_delay_, [this]() {
            mute_tmp_mqtt_ = false;
          });
        }
      }
      if (publish_stats_after_power_on_delay_ > 0 && !prev_power_status && after_power_status) {
        time_publish_stats_after_power_on_ = millis() + publish_stats_after_power_on_delay_;
      }
      publish_state();
      ESP_LOGW(TAG, "mqtt_callback =====>>>> end !");
    }

    void ACW02::publish_state() {
      if (!mqtt_) {
        ESP_LOGE(TAG, "publish_state =====>>>> mqtt_ is false");
        return;
      }

      if (publish_stats_after_power_on_delay_ > 0 && time_publish_stats_after_power_on_ > 0 && time_publish_stats_after_power_on_ > millis()) {
         set_timeout("publish_state_after_on", 100, [this]() {
          publish_state();
        });
        return;
      }
      time_publish_stats_after_power_on_ = 0;

      std::string payload = "{";
      payload += "\"power_climate\":\"" + std::string(power_on_ ? "on" : "off") + "\",";
      payload += "\"power\":\"" + std::string(power_on_ ? "on" : "off") + "\",";
      payload += "\"mode_climate\":\"" + get_mode_string_climate() + "\",";
      payload += "\"mode\":\"" + get_mode_string() + "\",";
      payload += "\"fan\":\"" + fan_to_string(app_lang_, fan_) + "\",";
      payload += "\"temp_c\":" + std::to_string(target_temp_c_) + ",";
      payload += "\"temp_f\":" + std::to_string(target_temp_f_) + ",";
      payload += "\"ambient_c\":" + std::to_string(ambient_temp_c_) + ",";
      payload += "\"ambient_f\":" + std::to_string(ambient_temp_f_) + ",";
      payload += "\"eco\":\"" + std::string(eco_ ? "on" : "off") + "\",";
      payload += "\"night\":\"" + std::string(night_ ? "on" : "off") + "\",";
      payload += "\"clean\":\"" + std::string(clean_ ? "on" : "off") + "\",";
      payload += "\"purifier\":\"" + std::string(purifier_ ? "on" : "off") + "\",";
      payload += "\"display\":\"" + std::string(display_ ? "on" : "off") + "\",";
      payload += "\"swing\":\"" + swing_to_string(app_lang_, swing_position_) + "\",";
      std::string swingH = swing_horizontal_to_string(app_lang_, swing_horizontal_);
      payload += "\"swing_horizontal\":\"" + swingH + "\",";
      if (swingH == key_to_txt(app_lang_, "swingHorizontal", "STOP")) {
        swingH = "none";
      }
      payload += "\"swing_horizontal_climate\":\"" + swingH + "\",";
      payload += "\"unit\":\"" + std::string(use_fahrenheit_ ? "°F" : "°C") + "\",";
      payload += "\"last_cmd_origin\":\"" + std::string(from_remote_ ? "Remote" : "ESP") + "\",";
      payload += "\"filter_dirty\":\"" + std::string(filter_dirty_ ? "true" : "false") + "\",";
      payload += "\"warn\":\"" + std::string(warn_ ? "true" : "false") + "\",";
      payload += "\"error\":\"" + std::string(error_ ? "true" : "false") + "\",";
      payload += "\"warn_text\":\"" + warn_text_ + "\",";
      payload += "\"error_text\":\"" + error_text_ + "\",";
      payload += "\"g1_mute\":\"" + std::string(mute_ ? "on" : "off") + "\",";
      payload += "\"g1_mute_next_cmd_delay\":" + int_to_string(mute_next_cmd_delay_) + ",";
      payload += "\"g1_mute_next_cmd_after_on_delay\":" + int_to_string(mute_next_cmd_after_on_delay_) + ",";
      payload += "\"g1_publish_stats_after_power_on_delay\":" + int_to_string(publish_stats_after_power_on_delay_) + ",";
      payload += "\"g1_reset_eco_purifier\":\"" + std::string(auto_off_options_when_ac_off_ ? "on" : "off") + "\",";
      payload += "\"g1_option_recalculate_climate\":\"" + std::string(option_recalculate_climate_ ? "on" : "off") + "\",";
      payload += "\"disable_mode_auto\":\"" + std::string(disable_mode_auto_ ? "on" : "off") + "\",";
      payload += "\"disable_mode_heat\":\"" + std::string(disable_mode_heat_ ? "on" : "off") + "\",";
      payload += "\"disable_mode_dry\":\"" + std::string(disable_mode_dry_ ? "on" : "off") + "\",";
      payload += "\"disable_mode_fan\":\"" + std::string(disable_mode_fan_ ? "on" : "off") + "\",";
      payload += "\"disable_swing_vertical\":\"" + std::string(disable_swing_vertical_ ? "on" : "off") + "\",";
      payload += "\"disable_swing_horizontal\":\"" + std::string(disable_swing_horizontal_ ? "on" : "off") + "\",";
      payload += "\"mdns\":\""  + app_mdns_ + "\",";
      payload += "\"wifi_ip\":\""  + app_wifi_ip_ + "\",";
      payload += "\"wifi_ssid\":\""  + app_wifi_ssid_ + "\",";
      payload += "\"wifi_bssid\":\""  + app_wifi_bssid_ + "\",";
      payload += "\"wifi_mac\":\""  + app_wifi_mac_ + "\",";
      payload += "\"esp_version\":\""  + app_esp_version_ + "\",";
      payload += "\"wifi_signal\":\""  + app_wifi_signal_ + " dBm\",";
      payload += "\"internal_temperature\":\""  + app_internal_temperature_ + " °C\",";
      payload += "\"esp_memory\":\""  + app_esp_memory_ + " kB\",";
      payload += "\"preset\":\"" + std::string(preset_ ? "on" : "off") + "\",";
      payload += "\"presets_list_element_config\":\"" + presets_list_element_config_ + "\",";
      payload += "\"presets_list_element\":\"" + presets_list_element_ + "\",";
      std::string presets_list_element_climate = presets_list_element_;
      if (presets_list_element_climate == get_localized_name(app_lang_, "presetNone")) {
        presets_list_element_climate = "none";
      }
      payload += "\"presets_list_element_climate\":\"" + presets_list_element_climate + "\",";
      payload += "\"preset_name_config\":\"" + preset_name_config_ + "\",";
      payload += "\"cmd_failure_counter\":\"" + std::to_string(cmd_failure_counter_) + "\"";
      payload += "}";
      ESP_LOGW(TAG, "publish_state send");
      publish_async(app_name_ + "/state", payload, 1, true);
    }

    void ACW02::publish_availability() {
      if (!mqtt_) return;

      const std::string topic_eco = app_name_ + "/eco_availability";
      std::string payload_eco = (power_on_ && mode_ == Mode::COOL) ? "available" : "unavailable";
      publish_async(topic_eco, payload_eco, 1, true);

      const std::string topic_night = app_name_ + "/night_availability";
      std::string payload_night = (power_on_ && (mode_ == Mode::COOL || mode_ == Mode::DRY || mode_ == Mode::HEAT)) ? "available" : "unavailable";
      publish_async(topic_night, payload_night, 1, true);

      const std::string topic_fan_speed = app_name_ + "/fan_speed_availability";
      std::string payload_fan_speed = (eco_ == false || !power_on_) ? "available" : "unavailable";
      publish_async(topic_fan_speed, payload_fan_speed, 1, true);

      const std::string topic_target_temp = app_name_ + "/target_temp_availability";
      std::string payload_target_temp = ((mode_ != Mode::AUTO && eco_ == false) || !power_on_) ? "available" : "unavailable";
      publish_async(topic_target_temp, payload_target_temp, 1, true);

      const std::string topic_purifier = app_name_ + "/purifier_availability";
      std::string payload_purifier = (power_on_) ? "available" : "unavailable";
      publish_async(topic_purifier, payload_purifier, 1, true);

      const std::string topic_clean = app_name_ + "/clean_availability";
      std::string payload_clean = (!power_on_) ? "available" : "unavailable";
      publish_async(topic_clean, payload_clean, 1, true);
    }

    std::string ACW02::build_common_config_suffix() const {
      return R"(
      ,"qos": 1
      ,"dev": {
        "ids": [")" + app_sanitize_name_ + R"("],
        "name": ")" + app_friendly_name_ + R"(",
        "mdl": ")" + app_board_ + R"(",
        "mf": "Espressif",
        "cns": [["mac",")" + app_mac_ + R"("]]
      })";
    }

    void ACW02::publish_discovery_climate_deref() {
      set_timeout("publish_discovery_climate_deref", 100, [this]() {
        publish_discovery_climate();
      });
    }

    void ACW02::publish_discovery_climate(bool recreate) {
      if (!mqtt_) return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_climate";

      const std::string temp_suffix = use_fahrenheit_ ? "_f" : "_c";

      std::string mintemp = use_fahrenheit_ ? "61" : "16";
      std::string maxtemp = use_fahrenheit_ ? "88" : "31";

      if (power_on_ && mode_ == Mode::AUTO) {
        auto_temp_defined_heat_cool_calculator();
      }

      if (power_on_ && (eco_ || mode_ == Mode::AUTO)) {
        mintemp = use_fahrenheit_ ? std::to_string(target_temp_f_) : std::to_string(target_temp_c_);
        maxtemp = mintemp;
      }
      
      
      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "climate") + R"(",
        "default_entity_id": ")" + "climate." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "stat_t": ")" + topic_base + R"(/state",
        "cmd_t": ")" + topic_base + R"(/cmd_ac/mode_climate",)";
        if (!disable_swing_vertical_ && !disable_swing_horizontal_) {
          payload += R"(
          "preset_mode_command_topic": ")" + topic_base + R"(/cmd_ac/swing_horizontal",
          "preset_mode_state_topic": ")" + topic_base + R"(/state",
          "preset_mode_value_template": "{{ value_json.swing_horizontal_climate }}",
          "preset_modes": )" + build_options_json(app_lang_, "swingHorizontal", "STOP") + R"(,)";
        } else if (preset_) {
          payload += R"(
          "preset_mode_command_topic": ")" + topic_base + R"(/cmd_ac/presets_list_element",
          "preset_mode_state_topic": ")" + topic_base + R"(/state",
          "preset_mode_value_template": "{{ value_json.presets_list_element_climate }}",
          "preset_modes": )" + get_preset_list(true, true) + R"(,)";
        }
        payload += R"(
        "mode_stat_t": ")" + topic_base + R"(/state",
        "mode_stat_tpl": "{{ value_json.mode_climate }}",
        "mode_cmd_t": ")" + topic_base + R"(/cmd_ac/mode_climate",
        "modes": )" + build_modes_json_climate() + R"(,
        "temp_cmd_t": ")" + topic_base + "/cmd_ac/temp" + temp_suffix + R"(",
        "temp_stat_t": ")" + topic_base + R"(/state",
        "temp_stat_tpl": "{{ value_json.temp)" + temp_suffix + R"( }}",
        "curr_temp_t": ")" + topic_base + R"(/state",
        "curr_temp_tpl": "{{ value_json.ambient)" + temp_suffix + R"( }}",
        "fan_mode_cmd_t": ")" + topic_base + R"(/cmd_ac/fan",
        "fan_mode_stat_t": ")" + topic_base + R"(/state",
        "fan_mode_stat_tpl": "{{ value_json.fan }}",
        "fan_modes": )" + build_fan_speed_json() + R"(,)";
        
        if (!disable_swing_vertical_ && !disable_swing_horizontal_) {
          payload += R"(
          "swing_mode_cmd_t": ")" + topic_base + R"(/cmd_ac/swing",
          "swing_mode_stat_t": ")" + topic_base + R"(/state",
          "swing_mode_stat_tpl": "{{ value_json.swing }}",
          "swing_modes": )" + build_options_json(app_lang_, "swing") + R"(,)";
        } else {
          if (disable_swing_vertical_ && !disable_swing_horizontal_) {
             payload += R"(
            "swing_mode_cmd_t": ")" + topic_base + R"(/cmd_ac/swing_horizontal",
            "swing_mode_stat_t": ")" + topic_base + R"(/state",
            "swing_mode_stat_tpl": "{{ value_json.swing_horizontal }}",
            "swing_modes": )" + build_options_json(app_lang_, "swingHorizontal") + R"(,)";
          } else if (disable_swing_horizontal_ && !disable_swing_vertical_) {
             payload += R"(
            "swing_mode_cmd_t": ")" + topic_base + R"(/cmd_ac/swing",
            "swing_mode_stat_t": ")" + topic_base + R"(/state",
            "swing_mode_stat_tpl": "{{ value_json.swing }}",
            "swing_modes": )" + build_options_json(app_lang_, "swing") + R"(,)";
          }
        }
        payload += R"(
        "pow_cmd_t": ")" + topic_base + R"(/cmd_ac/power_climate",
        "pow_stat_t": ")" + topic_base + R"(/state",
        "pow_stat_tpl": "{{ value_json.power_climate }}",
        "min_temp": )" + (mintemp) + R"(,
        "max_temp": )" + (maxtemp) + R"(,
        "temp_step": 1,
        "avty_t":")" + topic_base + R"(/status")" +
        build_common_config_suffix() + R"(
      })";

      std::string config_topic = "homeassistant/climate/" + topic_base + "/config";
      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_climate_publish", mqtt_delay_rebuild_short_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_mode_select(bool recreate) {
      if (!mqtt_)
      return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_mode";

      std::string config_topic = "homeassistant/select/" + topic_base + "-mode/config";

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "mode") + R"(",
        "default_entity_id": ")" + "select." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "icon": "mdi:thermostat",
        "cmd_t": ")" + topic_base + R"(/cmd_ac/mode",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.mode }}",
        "options": )" + build_modes_json() + R"(,
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_mode_select_publish", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_fan_select(bool recreate) {
      if (!mqtt_)
      return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_fan";

      std::string config_topic = "homeassistant/select/" + topic_base + "-fan/config";

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "fan") + R"(",
        "default_entity_id": ")" + "select." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "icon": "mdi:fan",
        "cmd_t": ")" + topic_base + R"(/cmd_ac/fan",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.fan }}",
        "options": )" + build_options_json(app_lang_, "fan") + R"(,
        "availability_mode": "all",
        "availability": [
        {
          "topic": ")" + topic_base + R"(/status",
          "payload_available": "online",
          "payload_not_available": "offline"
        },
        {
          "topic": ")" + topic_base + R"(/fan_speed_availability",
          "payload_available": "available",
          "payload_not_available": "unavailable"
        }
        ])" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_fan_select_publish", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_unit_select(bool recreate) {
      if (!mqtt_)
      return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_unit";

      std::string config_topic = "homeassistant/select/" + topic_base + "-unit/config";

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "unit") + R"(",
        "default_entity_id": ")" + "select." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "cmd_t": ")" + topic_base + R"(/cmd_ac/unit",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.unit }}",
        "options": ["°C", "°F"],
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_unit_select_publish", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_swing_select(bool recreate) {
      if (!mqtt_)
      return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_swing";

      std::string config_topic = "homeassistant/select/" + topic_base + "-swing/config";

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "swing") + R"(",
        "default_entity_id": ")" + "select." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "icon": "mdi:swap-vertical",
        "cmd_t": ")" + topic_base + R"(/cmd_ac/swing",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.swing }}",
        "options": )" + build_options_json(app_lang_, "swing") + R"(,
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_swing_select_publish", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_swing_horizontal_select(bool recreate) {
      if (!mqtt_)
      return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_swing_horizontal";

      std::string config_topic = "homeassistant/select/" + topic_base + "-swing-horizontal/config";

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "swingHorizontal") + R"(",
        "default_entity_id": ")" + "select." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "icon": "mdi:swap-horizontal",
        "cmd_t": ")" + topic_base + R"(/cmd_ac/swing_horizontal",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.swing_horizontal }}",
        "options": )" + build_options_json(app_lang_, "swingHorizontal") + R"(,
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_swing_horizontal_select_publish", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_preset_select(bool recreate) {
      if (!mqtt_)
      return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_preset_list";

      std::string config_topic = "homeassistant/select/" + topic_base + "-preset-list/config";

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "presetList") + R"(",
        "default_entity_id": ")" + "select." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "icon": "mdi:content-save",
        "cmd_t": ")" + topic_base + R"(/cmd_ac/presets_list_element",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.presets_list_element }}",
        "options": )" + get_preset_list(true) + R"(,
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("publish_discovery_preset_select", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_preset_config_select(bool recreate) {
      if (!mqtt_)
      return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_preset_list_config";

      std::string config_topic = "homeassistant/select/" + topic_base + "-preset-list-config/config";

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "presetListConfig") + R"(",
        "default_entity_id": ")" + "select." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "entity_category": "config",
        "icon": "mdi:content-save",
        "cmd_t": ")" + topic_base + R"(/cmd/presets_list_element_config",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.presets_list_element_config }}",
        "options": )" + get_preset_list(false) + R"(,
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("publish_discovery_preset_config_select", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_clean_switch(bool recreate) {
      if (!mqtt_) return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_clean";

      std::string config_topic = "homeassistant/switch/" + topic_base + "-clean/config";

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "clean") + R"(",
        "default_entity_id": ")" + "switch." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "icon": "mdi:spray-bottle",
        "cmd_t": ")" + topic_base + R"(/cmd_ac/clean",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.clean }}",
        "pl_on": "on",
        "pl_off": "off",
        "availability_mode": "all",
        "availability": [
        {
          "topic": ")" + topic_base + R"(/status",
          "payload_available": "online",
          "payload_not_available": "offline"
        },
        {
          "topic": ")" + topic_base + R"(/clean_availability",
          "payload_available": "available",
          "payload_not_available": "unavailable"
        }
        ])" +
        build_common_config_suffix() + R"(
      })";


      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_clean_switch_publish", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_eco_switch(bool recreate) {
      if (!mqtt_) return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_eco";

      std::string config_topic = "homeassistant/switch/" + topic_base + "-eco/config";

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "eco") + R"(",
        "default_entity_id": ")" + "switch." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "icon": "mdi:currency-usd-off",
        "cmd_t": ")" + topic_base + R"(/cmd_ac/eco",
        "stat_t": ")" + topic_base + R"(/state",
        "availability_mode": "all",
        "availability_template": "{{ value_json.mode == 'cool' }}",
        "val_tpl": "{{ value_json.eco }}",
        "pl_on": "on",
        "pl_off": "off",
        "availability_mode": "all",
        "availability": [
        {
          "topic": ")" + topic_base + R"(/status",
          "payload_available": "online",
          "payload_not_available": "offline"
        },
        {
          "topic": ")" + topic_base + R"(/eco_availability",
          "payload_available": "available",
          "payload_not_available": "unavailable"
        }
        ])" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_eco_switch_publish", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_display_switch(bool recreate) {
      if (!mqtt_)
      return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_display";

      std::string config_topic = "homeassistant/switch/" + topic_base + "-display/config";

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "display") + R"(",
        "default_entity_id": ")" + "switch." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "icon": "mdi:lightbulb",
        "cmd_t": ")" + topic_base + R"(/cmd_ac/display",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.display }}",
        "pl_on": "on",
        "pl_off": "off",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_display_switch_publish", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_night_switch(bool recreate) {
      if (!mqtt_)
      return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_night";

      std::string config_topic = "homeassistant/switch/" + topic_base + "-night/config";

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "night") + R"(",
        "default_entity_id": ")" + "switch." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "cmd_t": ")" + topic_base + R"(/cmd_ac/night",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.night }}",
        "pl_on": "on",
        "pl_off": "off",
        "icon": "mdi:power-sleep",
        "availability_mode": "all",
        "availability": [
        {
          "topic": ")" + topic_base + R"(/status",
          "payload_available": "online",
          "payload_not_available": "offline"
        },
        {
          "topic": ")" + topic_base + R"(/night_availability",
          "payload_available": "available",
          "payload_not_available": "unavailable"
        }
        ])" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_night_switch_publish", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_purifier_switch(bool recreate) {
      if (!mqtt_)
      return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_purifier";

      std::string config_topic = "homeassistant/switch/" + topic_base + "-purifier/config";

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "purifier") + R"(",
        "default_entity_id": ")" + "switch." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "cmd_t": ")" + topic_base + R"(/cmd_ac/purifier",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.purifier }}",
        "pl_on": "on",
        "pl_off": "off",
        "icon": "mdi:leaf-circle",
        "availability_mode": "all",
        "availability": [
        {
          "topic": ")" + topic_base + R"(/status",
          "payload_available": "online",
          "payload_not_available": "offline"
        },
        {
          "topic": ")" + topic_base + R"(/purifier_availability",
          "payload_available": "available",
          "payload_not_available": "unavailable"
        }
        ])" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_purifier_switch_publish", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_g1_mute_switch(bool recreate) {
      if (!mqtt_)
      return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_g1_mute";

      std::string config_topic = "homeassistant/switch/" + topic_base + "-g1-mute/config";

      if (!option_g1_mqtt_) {
        publish_async(config_topic, std::string(""), 1, true);
        return;
      }

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "g1Mute") + R"(",
        "default_entity_id": ")" + "switch." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "cmd_t": ")" + topic_base + R"(/cmd/g1_mute",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.g1_mute }}",
        "pl_on": "on",
        "pl_off": "off",
        "icon": "mdi:volume-off",
        "entity_category": "config",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_g1_mute_switch_publish", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_g1_reset_eco_purifier_ac_off_switch(bool recreate) {
      if (!mqtt_)
      return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_reset_eco_purifier";

      std::string config_topic = "homeassistant/switch/" + topic_base + "-g1-reset-eco-purifier/config";

      if (!option_g1_mqtt_) {
        publish_async(config_topic, std::string(""), 1, true);
        return;
      }

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "resetEcoPurifier") + R"(",
        "default_entity_id": ")" + "switch." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "cmd_t": ")" + topic_base + R"(/cmd/g1_reset_eco_purifier",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.g1_reset_eco_purifier }}",
        "pl_on": "on",
        "pl_off": "off",
        "icon": "mdi:lock-reset",
        "entity_category": "config",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_g1_reset_eco_purifier_ac_off_switch_publish", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_g1_option_recalculate_climate_switch(bool recreate) {
      if (!mqtt_)
      return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_g1_option_recalculate_climate";

      std::string config_topic = "homeassistant/switch/" + topic_base + "-g1-option-recalculate-climate/config";

      if (!option_g1_mqtt_) {
        publish_async(config_topic, std::string(""), 1, true);
        return;
      }

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "g1OptionRecalculateClimate") + R"(",
        "default_entity_id": ")" + "switch." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "cmd_t": ")" + topic_base + R"(/cmd/g1_option_recalculate_climate",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.g1_option_recalculate_climate }}",
        "pl_on": "on",
        "pl_off": "off",
        "entity_category": "config",
        "icon": "mdi:autorenew",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_g1_option_recalculate_climate_switch_publish", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_disable_mode_auto_switch(bool recreate) {
      if (!mqtt_)
      return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_disable_mode_auto";

      std::string config_topic = "homeassistant/switch/" + topic_base + "-disable-mode-auto/config";

      if (!option_g1_mqtt_) {
        publish_async(config_topic, std::string(""), 1, true);
        return;
      }

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "disableModeAuto") + R"(",
        "default_entity_id": ")" + "switch." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "cmd_t": ")" + topic_base + R"(/cmd/disable_mode_auto",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.disable_mode_auto }}",
        "pl_on": "on",
        "pl_off": "off",
        "entity_category": "config",
        "icon": "mdi:thermostat-cog",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_disable_mode_auto_switch_publish", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_disable_mode_heat_switch(bool recreate) {
      if (!mqtt_)
      return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_disable_mode_heat";

      std::string config_topic = "homeassistant/switch/" + topic_base + "-disable-mode-heat/config";

      if (!option_g1_mqtt_) {
        publish_async(config_topic, std::string(""), 1, true);
        return;
      }

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "disableModeHeat") + R"(",
        "default_entity_id": ")" + "switch." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "cmd_t": ")" + topic_base + R"(/cmd/disable_mode_heat",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.disable_mode_heat }}",
        "pl_on": "on",
        "pl_off": "off",
        "entity_category": "config",
        "icon": "mdi:thermostat-cog",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_disable_mode_heat_switch_publish", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_disable_mode_dry_switch(bool recreate) {
      if (!mqtt_)
      return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_disable_mode_dry";

      std::string config_topic = "homeassistant/switch/" + topic_base + "-disable-mode-dry/config";

      if (!option_g1_mqtt_) {
        publish_async(config_topic, std::string(""), 1, true);
        return;
      }

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "disableModeDry") + R"(",
        "default_entity_id": ")" + "switch." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "cmd_t": ")" + topic_base + R"(/cmd/disable_mode_dry",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.disable_mode_dry }}",
        "pl_on": "on",
        "pl_off": "off",
        "entity_category": "config",
        "icon": "mdi:thermostat-cog",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_disable_mode_dry_switch_publish", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_disable_mode_fan_switch(bool recreate) {
      if (!mqtt_)
      return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_disable_mode_fan";

      std::string config_topic = "homeassistant/switch/" + topic_base + "-disable-mode-fan/config";

      if (!option_g1_mqtt_) {
        publish_async(config_topic, std::string(""), 1, true);
        return;
      }

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "disableModeFan") + R"(",
        "default_entity_id": ")" + "switch." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "cmd_t": ")" + topic_base + R"(/cmd/disable_mode_fan",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.disable_mode_fan }}",
        "pl_on": "on",
        "pl_off": "off",
        "entity_category": "config",
        "icon": "mdi:thermostat-cog",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_disable_mode_fan_switch_publish", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_disable_swing_vertical_switch(bool recreate) {
      if (!mqtt_)
      return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_disable_swing_vertical";

      std::string config_topic = "homeassistant/switch/" + topic_base + "-disable-swing_vertical/config";

      if (!option_g1_mqtt_) {
        publish_async(config_topic, std::string(""), 1, true);
        return;
      }

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "disableSwingVertical") + R"(",
        "default_entity_id": ")" + "switch." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "cmd_t": ")" + topic_base + R"(/cmd/disable_swing_vertical",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.disable_swing_vertical }}",
        "pl_on": "on",
        "pl_off": "off",
        "entity_category": "config",
        "icon": "mdi:thermostat-cog",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_disable_swing_vertical_switch_publish", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_disable_swing_horizontal_switch(bool recreate) {
      if (!mqtt_)
      return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_disable_swing_horizontal";

      std::string config_topic = "homeassistant/switch/" + topic_base + "-disable-swing_horizontal/config";

      if (!option_g1_mqtt_) {
        publish_async(config_topic, std::string(""), 1, true);
        return;
      }

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "disableSwingHorizontal") + R"(",
        "default_entity_id": ")" + "switch." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "cmd_t": ")" + topic_base + R"(/cmd/disable_swing_horizontal",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.disable_swing_horizontal }}",
        "pl_on": "on",
        "pl_off": "off",
        "entity_category": "config",
        "icon": "mdi:thermostat-cog",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_disable_swing_horizontal_switch_publish", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_preset_switch(bool recreate) {
      if (!mqtt_)
      return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_preset";

      std::string config_topic = "homeassistant/switch/" + topic_base + "-preset/config";

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "preset") + R"(",
        "default_entity_id": ")" + "switch." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "cmd_t": ")" + topic_base + R"(/cmd/preset",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.preset }}",
        "pl_on": "on",
        "pl_off": "off",
        "entity_category": "config",
        "icon": "mdi:thermostat-cog",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_preset_switch_publish", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_g1_mute_next_cmd_delay_text(bool recreate) {
      if (!mqtt_)
        return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_g1_mute_next_cmd_delay";

      std::string config_topic = "homeassistant/text/" + topic_base + "-g1-mute-next-cmd-delay/config";

      if (!option_g1_mqtt_) {
        publish_async(config_topic, std::string(""), 1, true);
        return;
      }

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "ZCMDMuteDelay") + R"(",
        "default_entity_id": ")" + "text." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "cmd_t": ")" + topic_base + R"(/cmd/g1_mute_next_cmd_delay",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.g1_mute_next_cmd_delay }}",
        "entity_category": "config",
        "mode": "text",
        "icon": "mdi:volume-off",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_g1_mute_next_cmd_delay_text_publish", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_g1_mute_next_cmd_after_on_delay_text(bool recreate) {
      if (!mqtt_)
        return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_g1_mute_next_cmd_after_on_delay";

      std::string config_topic = "homeassistant/text/" + topic_base + "-g1-mute-next-cmd-after-on-delay/config";

      if (!option_g1_mqtt_) {
        publish_async(config_topic, std::string(""), 1, true);
        return;
      }

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "ZCMDMuteAfterOnDelay") + R"(",
        "default_entity_id": ")" + "text." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "cmd_t": ")" + topic_base + R"(/cmd/g1_mute_next_cmd_after_on_delay",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.g1_mute_next_cmd_after_on_delay }}",
        "entity_category": "config",
        "mode": "text",
        "icon": "mdi:volume-off",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_g1_mute_next_cmd_after_on_delay_text_publish", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_g1_publish_stats_after_power_on_delay_text(bool recreate) {
      if (!mqtt_)
        return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_g1_publish_stats_after_power_on_delay";

      std::string config_topic = "homeassistant/text/" + topic_base + "-g1-publish-stats-after-power-on-delay/config";

      if (!option_g1_mqtt_) {
        publish_async(config_topic, std::string(""), 1, true);
        return;
      }

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "ZCMDPublishAfterOnDelay") + R"(",
        "default_entity_id": ")" + "text." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "cmd_t": ")" + topic_base + R"(/cmd/g1_publish_stats_after_power_on_delay",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.g1_publish_stats_after_power_on_delay }}",
        "entity_category": "config",
        "mode": "text",
        "icon": "mdi:publish",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_g1_publish_stats_after_power_on_delay_text_publish", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_preset_name_config_text(bool recreate) {
      if (!mqtt_)
        return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_preset_name_config";

      std::string config_topic = "homeassistant/text/" + topic_base + "-preset-name-config/config";

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "presetName") + R"(",
        "default_entity_id": ")" + "text." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "cmd_t": ")" + topic_base + R"(/cmd/preset_name_config",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.preset_name_config }}",
        "entity_category": "config",
        "mode": "text",
        "icon": "mdi:content-save",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("publish_discovery_preset_name_config_text", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_temperature_number(bool recreate) {
      if (!mqtt_)
      return;

      const std::string topic_base = app_name_;
      const std::string unit = use_fahrenheit_ ? "°F" : "°C";
      const std::string temp_suffix = use_fahrenheit_ ? "_f" : "_c";
      const std::string unique_id = app_sanitize_name_ + "_mqtt_temp";
      const std::string icon = use_fahrenheit_ ? "mdi:temperature-fahrenheit" : "mdi:temperature-celsius";

      float min = use_fahrenheit_ ? 61.0f : 16.0f;
      float max = use_fahrenheit_ ? 88.0f : 31.0f;

      std::string config_topic = "homeassistant/number/" + topic_base + "-temp/config";

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "temp") + R"(",
        "default_entity_id": ")" + "number." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "cmd_t": ")" + topic_base + "/cmd_ac/temp" + temp_suffix + R"(",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.temp)" + temp_suffix + R"( }}",
        "min": )" + to_string(min) + R"(,
        "max": )" + to_string(max) + R"(,
        "step": 1,
        "mode": "box",
        "icon": ")" + icon + R"(",
        "unit_of_meas": ")" + unit + R"(",
        "availability_mode": "all",
        "availability": [
        {
          "topic": ")" + topic_base + R"(/status",
          "payload_available": "online",
          "payload_not_available": "offline"
        },
        {
          "topic": ")" + topic_base + R"(/target_temp_availability",
          "payload_available": "available",
          "payload_not_available": "unavailable"
        }
        ])" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_temperature_number_publish", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_g1_reload_button(bool recreate) {
      if (!mqtt_)
        return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_g1_restart_module_ac";
      const std::string config_topic = "homeassistant/button/" + topic_base + "-g1-restart-module-ac/config";

      if (!option_g1_mqtt_) {
        publish_async(config_topic, std::string(""), 1, true);
        return;
      }

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "restartModule") + R"(",
        "default_entity_id": ")" + "button." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "cmd_t": ")" + topic_base + R"(/cmd/g1_restart_module_ac",
        "icon": "mdi:restart",
        "entity_category": "config",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_g1_reload_button_publish", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_g1_rebuild_mqtt_entities_button(bool recreate) {
      if (!mqtt_)
        return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_g1_rebuild_mqtt_entities";
      const std::string config_topic = "homeassistant/button/" + topic_base + "-g1-rebuild-mqtt-entities/config";

      if (!option_g1_mqtt_) {
        publish_async(config_topic, std::string(""), 1, true);
        return;
      }

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "rebuildMQTT") + R"(",
        "default_entity_id": ")" + "button." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "cmd_t": ")" + topic_base + R"(/cmd/g1_rebuild_mqtt_entities",
        "icon": "mdi:cloud-refresh-variant",
        "entity_category": "config",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("publish_discovery_g1_rebuild_mqtt_entities_button", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_g1_get_status_button(bool recreate) {
      if (!mqtt_)
        return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_g1_get_status";
      const std::string config_topic = "homeassistant/button/" + topic_base + "-g1-get-status/config";

      if (!option_g1_mqtt_) {
        publish_async(config_topic, std::string(""), 1, true);
        return;
      }

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "getStatus") + R"(",
        "default_entity_id": ")" + "button." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "cmd_t": ")" + topic_base + R"(/cmd/g1_get_status",
        "icon": "mdi:cloud-refresh-variant",
        "entity_category": "config",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("publish_discovery_g1_get_status_button", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_z_config_validate_button(bool recreate) {
      if (!mqtt_)
        return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_z_config_validate";
      const std::string config_topic = "homeassistant/button/" + topic_base + "-z-config-validate/config";

      if (!option_g1_mqtt_) {
        publish_async(config_topic, std::string(""), 1, true);
        return;
      }

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "disableValidate") + R"(",
        "default_entity_id": ")" + "button." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "cmd_t": ")" + topic_base + R"(/cmd/z_config_validate",
        "icon": "mdi:send",
        "entity_category": "config",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("publish_discovery_z_config_validate_button", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_preset_save_button(bool recreate) {
      if (!mqtt_)
        return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_preset_save";
      const std::string config_topic = "homeassistant/button/" + topic_base + "-preset-save/config";

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "presetSave") + R"(",
        "default_entity_id": ")" + "button." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "cmd_t": ")" + topic_base + R"(/cmd/preset_save",
        "icon": "mdi:content-save",
        "entity_category": "config",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("publish_discovery_preset_save_button", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_preset_delete_button(bool recreate) {
      if (!mqtt_)
        return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_preset_delete";
      const std::string config_topic = "homeassistant/button/" + topic_base + "-preset-delete/config";

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "presetDelete") + R"(",
        "default_entity_id": ")" + "button." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "cmd_t": ")" + topic_base + R"(/cmd/preset_delete",
        "icon": "mdi:content-save",
        "entity_category": "config",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("publish_discovery_preset_delete_button", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_temperature_sensor(bool recreate) {
      if (!mqtt_) return;

      const std::string topic_base = app_name_;
      const std::string unit = use_fahrenheit_ ? "°F" : "°C";
      const std::string temp_suffix = use_fahrenheit_ ? "_f" : "_c";
      const std::string unique_id = app_sanitize_name_ + "_mqtt_sensor_temp";
      const std::string icon = use_fahrenheit_ ? "mdi:temperature-fahrenheit" : "mdi:temperature-celsius";

      std::string config_topic = "homeassistant/sensor/" + topic_base + "-temp/config";

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "ambient") + R"(",
        "default_entity_id": ")" + "sensor." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "stat_t": ")" + topic_base + R"(/state",
        "unit_of_meas": ")" + unit + R"(",
        "val_tpl": "{{ value_json.ambient)" + temp_suffix + R"( }}",
        "state_class": "measurement",
        "icon": ")" + icon + R"(",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_temp_sensor", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_last_cmd_origin_sensor(bool recreate) {
      if (!mqtt_) return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_sensor_last_cmd_origin";

      std::string config_topic = "homeassistant/sensor/" + topic_base + "-last-cmd-origin/config";

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "lastCmdOrigin") + R"(",
        "default_entity_id": ")" + "sensor." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "stat_t": ")" + topic_base + R"(/state",
        "icon" : "mdi:origin",
        "val_tpl": "{{ value_json.last_cmd_origin }}",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_last_cmd_origin_sensor", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_filter_dirty_sensor(bool recreate) {
      if (!mqtt_)
        return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_sensor_filter_dirty";
      std::string config_topic = "homeassistant/binary_sensor/" + topic_base + "-filter-dirty/config";

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "filterToClean") + R"(",
        "default_entity_id": ")" + "binary_sensor." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "stat_t": ")" + topic_base + R"(/state",
        "icon": "mdi:air-filter",
        "val_tpl": "{{ value_json.filter_dirty }}",
        "device_class": "problem",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_on": "true",
        "pl_off": "false",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("mqtt_publish_discovery_filter_dirty_sensor", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_warn_sensor(bool recreate) {
      if (!mqtt_)
        return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_sensor_warn";
      std::string config_topic = "homeassistant/binary_sensor/" + topic_base + "-warn/config";

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "warn") + R"(",
        "default_entity_id": ")" + "binary_sensor." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "stat_t": ")" + topic_base + R"(/state",
        "icon": "mdi:alert-circle-outline",
        "val_tpl": "{{ value_json.warn }}",
        "device_class": "problem",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_on": "true",
        "pl_off": "false",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("publish_discovery_warn_sensor", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_error_sensor(bool recreate) {
      if (!mqtt_)
        return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_sensor_error";
      std::string config_topic = "homeassistant/binary_sensor/" + topic_base + "-error/config";

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "error") + R"(",
        "default_entity_id": ")" + "binary_sensor." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "stat_t": ")" + topic_base + R"(/state",
        "icon": "mdi:alert-octagon-outline",
        "val_tpl": "{{ value_json.error }}",
        "device_class": "problem",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_on": "true",
        "pl_off": "false",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("publish_discovery_error_sensor", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_cmd_failure_counter_sensor(bool recreate) {
      if (!mqtt_)
        return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_cmd_failure_counter_sensor";
      std::string config_topic = "homeassistant/sensor/" + topic_base + "-cmd-failure-counter/config";

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "cmdFailureCounter") + R"(",
        "default_entity_id": ")" + "sensor." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "stat_t": ")" + topic_base + R"(/state",
        "icon": "mdi:alert-box",
        "val_tpl": "{{ value_json.cmd_failure_counter }}",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_on": "true",
        "pl_off": "false",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("publish_discovery_cmd_failure_counter_sensor", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_warn_text_sensor(bool recreate) {
      if (!mqtt_)
        return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_warn_text";
      std::string config_topic = "homeassistant/sensor/" + topic_base + "-warn-text/config";

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "warnText") + R"(",
        "default_entity_id": ")" + "sensor." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.warn_text }}",
        "icon": "mdi:alert-circle-outline",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("publish_discovery_warn_text_sensor", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_error_text_sensor(bool recreate) {
      if (!mqtt_)
        return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_error_text";
      std::string config_topic = "homeassistant/sensor/" + topic_base + "-error-text/config";

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "errorText") + R"(",
        "default_entity_id": ")" + "sensor." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.error_text }}",
        "icon": "mdi:alert-octagon-outline",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("publish_discovery_error_text_sensor", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_mdns_text_sensor(bool recreate) {
      if (!mqtt_)
        return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_mdns";
      std::string config_topic = "homeassistant/sensor/" + topic_base + "-mdns/config";

      if (!option_g1_mqtt_) {
        publish_async(config_topic, std::string(""), 1, true);
        return;
      }

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "MDNS") + R"(",
        "default_entity_id": ")" + "sensor." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "entity_category": "diagnostic",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.mdns }}",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("publish_discovery_mdns_text_sensor", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_wifi_ip_text_sensor(bool recreate) {
      if (!mqtt_)
        return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_wifi_ip";
      std::string config_topic = "homeassistant/sensor/" + topic_base + "-wifi-ip/config";

      if (!option_g1_mqtt_) {
        publish_async(config_topic, std::string(""), 1, true);
        return;
      }

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "WiFiIP") + R"(",
        "default_entity_id": ")" + "sensor." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "entity_category": "diagnostic",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.wifi_ip }}",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("publish_discovery_wifi_ip_text_sensor", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_wifi_ssid_text_sensor(bool recreate) {
      if (!mqtt_)
        return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_wifi_ssid";
      std::string config_topic = "homeassistant/sensor/" + topic_base + "-wifi-ssid/config";

      if (!option_g1_mqtt_) {
        publish_async(config_topic, std::string(""), 1, true);
        return;
      }

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "WiFiSSID") + R"(",
        "default_entity_id": ")" + "sensor." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "entity_category": "diagnostic",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.wifi_ssid }}",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("publish_discovery_wifi_ssid_text_sensor", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_wifi_bssid_text_sensor(bool recreate) {
      if (!mqtt_)
        return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_wifi_bssid";
      std::string config_topic = "homeassistant/sensor/" + topic_base + "-wifi-bssid/config";

      if (!option_g1_mqtt_) {
        publish_async(config_topic, std::string(""), 1, true);
        return;
      }

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "WiFiBSSID") + R"(",
        "default_entity_id": ")" + "sensor." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "entity_category": "diagnostic",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.wifi_bssid }}",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("publish_discovery_wifi_bssid_text_sensor", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_wifi_mac_text_sensor(bool recreate) {
      if (!mqtt_)
        return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_wifi_mac";
      std::string config_topic = "homeassistant/sensor/" + topic_base + "-wifi-mac/config";

      if (!option_g1_mqtt_) {
        publish_async(config_topic, std::string(""), 1, true);
        return;
      }

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "WiFiMAC") + R"(",
        "default_entity_id": ")" + "sensor." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "entity_category": "diagnostic",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.wifi_mac }}",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("publish_discovery_wifi_mac_text_sensor", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_esp_version_text_sensor(bool recreate) {
      if (!mqtt_)
        return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_esp_version";
      std::string config_topic = "homeassistant/sensor/" + topic_base + "-esp-version/config";

      if (!option_g1_mqtt_) {
        publish_async(config_topic, std::string(""), 1, true);
        return;
      }

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "ESPVERSION") + R"(",
        "default_entity_id": ")" + "sensor." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "entity_category": "diagnostic",
        "icon": "mdi:new-box",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.esp_version }}",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("publish_discovery_esp_version_text_sensor", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_wifi_signal_text_sensor(bool recreate) {
      if (!mqtt_)
        return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_wifi_signal";
      std::string config_topic = "homeassistant/sensor/" + topic_base + "-wifi-signal/config";

      if (!option_g1_mqtt_) {
        publish_async(config_topic, std::string(""), 1, true);
        return;
      }

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "WiFiSIGNAL") + R"(",
        "default_entity_id": ")" + "sensor." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "entity_category": "diagnostic",
        "icon": "mdi:wifi",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.wifi_signal }}",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("publish_discovery_wifi_signal_text_sensor", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_internal_temperature_text_sensor(bool recreate) {
      if (!mqtt_)
        return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_internal_temperature";
      std::string config_topic = "homeassistant/sensor/" + topic_base + "-internal-temperature/config";

      if (!option_g1_mqtt_) {
        publish_async(config_topic, std::string(""), 1, true);
        return;
      }

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "INTERNALTEMP") + R"(",
        "default_entity_id": ")" + "sensor." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "entity_category": "diagnostic",
        "icon": "mdi:thermometer",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.internal_temperature }}",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("publish_discovery_internal_temperature_text_sensor", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::publish_discovery_esp_memory_text_sensor(bool recreate) {
      if (!mqtt_)
        return;

      const std::string topic_base = app_name_;
      const std::string unique_id = app_sanitize_name_ + "_mqtt_esp_memory";
      std::string config_topic = "homeassistant/sensor/" + topic_base + "-esp-memory/config";

      if (!option_g1_mqtt_) {
        publish_async(config_topic, std::string(""), 1, true);
        return;
      }

      std::string payload = R"({
        "name": ")" + get_localized_name(app_lang_, "ESPMEMORY") + R"(",
        "default_entity_id": ")" + "sensor." + unique_id + R"(",
        "unique_id": ")" + unique_id + R"(",
        "entity_category": "diagnostic",
        "stat_t": ")" + topic_base + R"(/state",
        "val_tpl": "{{ value_json.esp_memory }}",
        "avty_t": ")" + topic_base + R"(/status",
        "pl_avail": "online",
        "pl_not_avail": "offline")" +
        build_common_config_suffix() + R"(
      })";

      if (recreate) {
        publish_async(config_topic, std::string(""), 1, true);
        set_timeout("publish_discovery_esp_memory_text_sensor", mqtt_delay_rebuild_, [this, config_topic, payload]() {
          publish_async(config_topic, payload, 1, true);
        });
      } else {
        publish_async(config_topic, payload, 1, true);
      }
    }

    void ACW02::rebuild_mqtt_entity() {
      publish_discovery_climate(true);
      publish_discovery_mode_select(true);
      publish_discovery_fan_select(true);
      publish_discovery_swing_select(true);
      publish_discovery_swing_horizontal_select(true);
      publish_discovery_preset_select(true);
      publish_discovery_preset_config_select(true);
      publish_discovery_unit_select(true);
      publish_discovery_clean_switch(true);
      publish_discovery_eco_switch(true);
      publish_discovery_display_switch(true);
      publish_discovery_night_switch(true);
      publish_discovery_purifier_switch(true);
      publish_discovery_preset_switch(true);
      publish_discovery_preset_name_config_text(true);
      publish_discovery_temperature_number(true);
      publish_discovery_preset_save_button(true);
      publish_discovery_preset_delete_button(true);
      publish_discovery_temperature_sensor(true);
      publish_discovery_last_cmd_origin_sensor(true);
      publish_discovery_filter_dirty_sensor(true);
      publish_discovery_warn_sensor(true);
      publish_discovery_error_sensor(true);
      publish_discovery_cmd_failure_counter_sensor(true);
      publish_discovery_warn_text_sensor(true);
      publish_discovery_error_text_sensor(true);
      if (option_g1_mqtt_) {
         set_timeout("publish_discovery_option_g1_mqtt_recreate", DELAY_MQTT_G1_GENERATE, [this]() {
          publish_discovery_g1_mute_switch(true);
          publish_discovery_g1_option_recalculate_climate_switch(true);
          publish_discovery_g1_reset_eco_purifier_ac_off_switch(true);
          publish_discovery_disable_mode_auto_switch(true);
          publish_discovery_disable_mode_heat_switch(true);
          publish_discovery_disable_mode_dry_switch(true);
          publish_discovery_disable_mode_fan_switch(true);
          publish_discovery_disable_swing_vertical_switch(true);
          publish_discovery_disable_swing_horizontal_switch(true);
          publish_discovery_g1_mute_next_cmd_delay_text(true);
          publish_discovery_g1_mute_next_cmd_after_on_delay_text(true);
          publish_discovery_g1_publish_stats_after_power_on_delay_text(true);
          publish_discovery_g1_reload_button(true);
          publish_discovery_g1_rebuild_mqtt_entities_button(true);
          publish_discovery_g1_get_status_button(true);
          publish_discovery_z_config_validate_button(true);
          publish_discovery_mdns_text_sensor(true);
          publish_discovery_wifi_ip_text_sensor(true);
          publish_discovery_wifi_ssid_text_sensor(true);
          publish_discovery_wifi_bssid_text_sensor(true);
          publish_discovery_wifi_mac_text_sensor(true);
          publish_discovery_esp_version_text_sensor(true);
          publish_discovery_wifi_signal_text_sensor(true);
          publish_discovery_internal_temperature_text_sensor(true);
          publish_discovery_esp_memory_text_sensor(true);
        });
      }
    }

    void ACW02::apply_disable_settings() {
      if (mqtt_) {
        publish_discovery_climate(true);
        publish_discovery_mode_select(true);
        publish_discovery_fan_select(true);
        publish_discovery_swing_select(true);
        publish_discovery_swing_horizontal_select(true);
      }
    }

    std::string ACW02::sanitize_name(const std::string &input) const  {
      std::string output = input;
      std::replace(output.begin(), output.end(), '-', '_');
      std::replace(output.begin(), output.end(), ' ', '_');
      std::transform(output.begin(), output.end(), output.begin(), ::tolower);
      return output;
    }

    std::string ACW02::int_to_string(int value) {
      char buf[12];
      snprintf(buf, sizeof(buf), "%d", value);
      return std::string(buf);
    }

    void ACW02::process_tx_queue() {
      if (tx_queue_.empty())
      return;

      // security for concurr RX/TX
      if (!rx_buffer_.empty() || (millis() - last_rx_byte_time_ < SILENCE_RX_MS)) {
        return;
      }

      if (millis() - last_tx_ < TX_INTERVAL_MS)
      return;

      const auto &pkt = tx_queue_.front();

      if (millis() < ack_block_until_) {
        return;
      }

      if (ack_wait_ && pkt.fingerprint != 0 && pkt.tryCnt == 0) {
        return;
      }
      ESP_LOGW(TAG, "TX: [%s]", format_hex_pretty(pkt.frame).c_str());
      write_array(pkt.frame);
      last_tx_ = millis();

      if (pkt.fingerprint != 0) {
        // Track only "real" AC commands (frames that carry a state change)
        cmd_send_fingerprint_ = pkt;
        log_fingerprint("write_frame", cmd_send_fingerprint_);
        cmd_send_fingerprint_.timestamp_ms = last_tx_;

        // New (re)TX of a real command → allow future timeout-based retries
        timeout_retry_pending_ = false;

        // ACK window only for a fresh real command
        if (pkt.tryCnt == 0) {
          ack_wait_ = true;
        }
        uint32_t extra = 0;
        if (pkt.tryCnt > 0) {
          extra = compute_retry_jitter(pkt.tryCnt);
        }
        ack_block_until_ = last_tx_ + ACK_WINDOW_MS + extra;
      } else {
        // Keepalive / status poll: do not touch in-flight tracking
        // (no cmd_send_fingerprint_, no ack_wait_, no ack_block_until_, no timeout flag)
      }
      tx_queue_.pop_front();
    }

    uint32_t ACW02::compute_retry_jitter(int tryCnt) const {
      const uint32_t jitter_min = 5;
      const uint32_t jitter_max = 15;

      // We mix millis() with tryCnt to vary
      uint32_t seed = (millis() / 7u) + (tryCnt * 131u);

      // Bounded pseudo-random value
      uint32_t jitter = jitter_min + (seed % (jitter_max - jitter_min + 1));

      ESP_LOGW(TAG, "Retry jitter (tryCnt=%d) = %u ms", tryCnt, jitter);
      return jitter;
    }

    void ACW02::check_timeout_retry() {
      // Trigger a silent timeout-based retry if no 34B ACK-like RX was seen
      // within the expected window after the last TX. This complements the
      // fingerprint-based retry path inside decode_state().
      const uint32_t now = millis();
      const auto &p = cmd_send_fingerprint_;

      if (p.fingerprint == 0) return;  // nothing in-flight

      const uint32_t dt = (uint32_t)(now - p.timestamp_ms);

      // Conditions:
      // - we have waited long enough to allow the AC to respond (ACK_EVAL_MIN_MS)
      // - we crossed the timeout (ACK_RETRY_TIMEOUT_MS)
      // - we still have retries left
      // - we have not already enqueued a timeout-based retry for this TX
      if (dt >= ACK_EVAL_MIN_MS &&
          dt >= ACK_RETRY_TIMEOUT_MS &&
          p.tryCnt < maxRetry &&
          !timeout_retry_pending_) {

        Frame_with_Fingerprint retry = p;  // snapshot current in-flight
        retry.tryCnt++;

        // Make the frame "mute" only if it was not already mute, then fix CRC.
        retry.frame = make_muted_with_fixed_crc(retry.frame);

        // Your send_command_basic() already prioritizes retries (push_front when tryCnt>0).
        send_command_basic(retry);

        // Prevent multiple enqueues while we wait for the retry to actually TX.
        timeout_retry_pending_ = true;

        ESP_LOGW(TAG, "Retry on timeout: dt=%lu ms, try=%d/%d",
         (unsigned long)dt, (int)retry.tryCnt, (int)maxRetry);
      }

      // --- Safety net / fail-safe ---------------------------------------------
      // If no RX 34B ever arrives, decode_state() will never clear ack_wait_.
      // After we have exhausted maxRetry, release the gate after a global timeout
      // so that the system doesn't remain blocked forever.

      if (p.tryCnt >= maxRetry) {
        if (dt >= ACK_ABORT_MS) {
          ESP_LOGE(TAG, "Ack timeout: aborting in-flight cmd after %u ms (tries=%d/%d)",
             (unsigned)dt, (int)p.tryCnt, (int)maxRetry);
          cmd_send_fingerprint_ = {0, "", {}, 0, 0};  // clear in-flight
          cmd_failure_counter_++;       // count a failure for observability
          ack_wait_ = false;            // release the "fresh command" gate
          ack_block_until_ = 0;
          timeout_retry_pending_ = false;
          // Fallback: force a quick state resync now (34B status frame)
          send_static_command_basic(get_status_frame_);
          ESP_LOGW(TAG, "Fingerprint Reset (abort)");
        }
      }
    }


    Frame_with_Fingerprint ACW02::build_frame(bool bypassMute) const {
      Frame_with_Fingerprint cmd_send = fingerprint();
      std::vector<uint8_t> frame(24, 0x00);
      frame[0] = frame[1] = 0x7A;
      frame[2] = 0x21;
      frame[3] = 0xD5;
      frame[4] = 0x18;
      frame[5] = frame[6] = 0x00;
      frame[7] = 0xA1;
      
      uint8_t fan_n = (static_cast<uint8_t>(fan_) & 0x0F) << 4;
      uint8_t pwr   = (power_on_ ? 1 : 0) << 3;
      uint8_t mode  = static_cast<uint8_t>(mode_) & 0x07;
      frame[12] = fan_n | pwr | mode;
      if (clean_ == false) {
        uint8_t base = encode_temperature_byte();
        if (fan_ == Fan::SILENT) {
          frame[13] = base  + 0x40;
        }
        else {
          frame[13] = base;
        }
      } else {
        frame[13] = 0x0B;
      }
      frame[14] = (static_cast<uint8_t>(swing_horizontal_) << 4) | (static_cast<uint8_t>(swing_position_) & 0x0F);

      uint8_t b15 = 0;
      if (eco_)      b15 |= 0x01;
      if (night_)    b15 |= 0x02;
      if (clean_)    b15 |= 0x10;
      if (purifier_) b15 |= 0x40;
      if (display_)  b15 |= 0x80;

      frame[15] = b15;
      if (is_mute_on() || mute_tmp_mqtt_ || mute_tmp_other_) {
        if (bypassMute == false) {
          frame[16] = 0x01;
        }
      }
      uint16_t crc = crc16(frame.data(), 22);
      frame[22] = (crc >> 8) & 0xFF;
      frame[23] = crc & 0xFF;
      cmd_send.frame = frame;
      return cmd_send;
    }

    std::vector<uint8_t> ACW02::make_muted_with_fixed_crc(const std::vector<uint8_t>& frame) const {
      // Copy by value (we do not modify 'frame')
      std::vector<uint8_t> out = frame;

      // Command frames: 24 bytes, CRC on the first 22, MSB@22 / LSB@23.
      if (out.size() < 24) {
        ESP_LOGW(TAG, "make_muted_with_fixed_crc: frame too short (%u)", (unsigned)out.size());
        return out;  // returns the unchanged copy
      }

      // "mute" byte = index 16, bit 0.
      if (out[16] & 0x01) {
        // already muted -> nothing to do
        return out;
      }

      // Force mute
      out[16] |= 0x01;

      // Recalculate CRC16 on the first 22 bytes (same rules as build_frame)
      const uint16_t crc = crc16(out.data(), 22);
      out[22] = (crc >> 8) & 0xFF;  // MSB
      out[23] = crc & 0xFF;         // LSB

      return out;
    }

    void ACW02::send_static_command_basic(const std::vector<uint8_t> &data) {
      Frame_with_Fingerprint data_frame = {0, "", {}, 0, 0};
      data_frame.frame = data;
      tx_queue_.push_back(data_frame);
    }

    void ACW02::send_command_basic(const Frame_with_Fingerprint &data) {
      if (data.tryCnt > 0){
        tx_queue_.push_front(data);
      }
      else { 
        tx_queue_.push_back(data);
      }
    }

    void ACW02::send_command(bool skipResetClean) {
      if (!skipResetClean) {
        clean_ = false;
        force_clean_ = false;
      }
      ESP_LOGW(TAG, "send command");
      send_command_basic(build_frame());
    }

    uint16_t ACW02::crc16(const uint8_t *data, size_t len) {
      uint16_t crc = 0xFFFF;
      for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; ++j)
        crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
      }
      return crc;
    }

    void ACW02::decode_state(const std::vector<uint8_t> &f) {
      Frame_with_Fingerprint cmd_send_fingerprint_tmp = cmd_send_fingerprint_;
      if (f.size() == 28 && f[0] == 0x7A && f[1] == 0x7A && f[2] == 0xD5 && f[3] == 0x21) {
        bool old_filter_dirty_ = filter_dirty_;
        bool old_warn_ = warn_;
        bool old_error_ = error_;
        uint8_t warn = f[10];
        uint8_t fault = f[12];
        if (fault != 0x00) {
          std::string fault_msg;
          switch (fault) {
            case 0x04: fault_msg = "PC: Conflict mode"; break;
            default: fault_msg = "??: Unknown fault"; break;
          }
          
          ESP_LOGE(TAG, "AC error : fault_code=0x%02X (%s)", fault, fault_msg.c_str());
          error_ = true;
          if (error_sensor_) {
            error_sensor_->publish_state(true);
          }
          if (error_text_sensor_) {
            char buf[32];
            snprintf(buf, sizeof(buf), "0x%02X (%s)", fault, fault_msg.c_str());
            error_text_sensor_->publish_state(buf);
            error_text_ = std::string(buf);
          }

        } else if (warn != 0x00) {
          std::string warn_msg;
          switch (warn) {
            case 0x80: 
              warn_msg = "CL: Filter to clean";
              filter_dirty_ = true;
              if (filter_dirty_sensor_) {
                filter_dirty_sensor_->publish_state(true);
              }
              break;
            default: warn_msg = "??: Unknown warn"; break;
          }
          ESP_LOGW(TAG, "AC warn : warn_code=0x%02X (%s)", warn, warn_msg.c_str());
          warn_ = true;
          if (warn_sensor_) {
            warn_sensor_->publish_state(true);
          }
          if (warn_text_sensor_) {
            char buf[32];
            snprintf(buf, sizeof(buf), "0x%02X (%s)", warn, warn_msg.c_str());
            warn_text_sensor_->publish_state(buf);
            warn_text_ = std::string(buf);
          }
        } else {
          filter_dirty_ = false;
          if (filter_dirty_sensor_) {
            filter_dirty_sensor_->publish_state(false);
          }
          warn_ = false;
          error_ = false;
          if (warn_sensor_) {
            warn_sensor_->publish_state(false);
          }
          if (error_sensor_) {
            error_sensor_->publish_state(false);
          }
          if (warn_text_sensor_) {
            warn_text_sensor_->publish_state("No Warn");
            warn_text_ = "No Warn";
          }
          if (error_text_sensor_) {
            error_text_sensor_->publish_state("No Error");
            error_text_ = "No Error";
          }
        }

        if (mqtt_ && (old_filter_dirty_ != filter_dirty_ || old_warn_ != warn_ || old_error_ != error_)) {
          publish_state();
        }
        return;
      }


      if (f.size() != 34 || f[0] != 0x7A || f[1] != 0x7A) {
        return;
      }
      ESP_LOGW(TAG, "decode_state =====>>>> AC frame start !");

      bool previous_power_on = power_on_;
      bool previous_fahrenheit = use_fahrenheit_;
      bool previous_eco = eco_;
      Mode previous_mode = mode_;

      const uint8_t b13       = f[13];
      power_on_               = b13 & 0x08;

      mode_ = static_cast<Mode>(b13 & 0x07);
      fan_  = static_cast<Fan>((b13 >> 4) & 0x0F);


      uint8_t temp_byte_rx = f[14];
      const bool silent_bit = (temp_byte_rx & 0x40);

      temp_byte_rx &= 0x3F;

      float temp_c{}, temp_f{};
      bool is_fahrenheit = false;

      for (int i = 0; i < 28; i++) {
        if (temp_byte_rx == fahrenheit_encoding_table_[i]) {
          is_fahrenheit   = true;
          use_fahrenheit_ = true;
          target_temp_f_  = static_cast<uint8_t>(i + 61);
          target_temp_c_  = static_cast<uint8_t>(fahrenheit_to_celsius(target_temp_f_));
          break;
        }
      }
      if (!is_fahrenheit) {
        use_fahrenheit_ = false;
        target_temp_c_  = 16 + temp_byte_rx;
        target_temp_f_  = static_cast<uint8_t>(celsius_to_fahrenheit(target_temp_c_));
      }

      if (silent_bit) {
        fan_ = Fan::SILENT;
      }

      // swing_position_ = static_cast<Swing>(f[15]);
      const uint8_t swing_raw = f[15];
      swing_position_ = static_cast<Swing>(swing_raw & 0x0F);
      swing_horizontal_ = static_cast<SwingHorizontal>((swing_raw >> 4) & 0x0F);

      const uint8_t flags = f[16];
      eco_      = flags & 0x01;
      night_    = flags & 0x02;
      if (!force_clean_) {
        // Only consider the clean bit if mode cool of dry
        if (mode_ == Mode::COOL || mode_ == Mode::DRY) {
          clean_ = flags & 0x10;
        } else {
          clean_ = false;
        }
      }
      purifier_ = flags & 0x40;
      display_  = flags & 0x80;
      from_remote_ = flags & 0x04 || (display_ && flags & 0x08);

      ESP_LOGW(TAG,
      "RX decode: PWR=%s | Mode=%s | Mode climate=%s | Fan=%s | Temp=%.1f°C / %.1f°F [%s] | "
      "Eco=%d | Night=%d | Clean=%d | Purifier=%d | Display=%d | Swing=%s | SwingH=%s | Silent=%s | Origin=%s",
      power_on_ ? "ON" : "OFF",
      mode_to_string(app_lang_, mode_).c_str(),
      mode_to_string_climate(mode_).c_str(),
      fan_to_string(app_lang_, fan_).c_str(),
      static_cast<float>(target_temp_c_), static_cast<float>(target_temp_f_), is_fahrenheit ? "°F" : "°C",
      eco_, night_, clean_, purifier_, display_,
      swing_to_string(app_lang_, swing_position_).c_str(),
      swing_horizontal_to_string(app_lang_, swing_horizontal_).c_str(),
      silent_bit ? "YES" : "NO",
      from_remote_ ? "Remote" : "ESP");

      if (f.size() >= 12) {
        const uint8_t temp_int = f[10];
        const uint8_t temp_dec = f[11];
        ambient_temp_c_  = temp_int + temp_dec / 10.0f;
        ambient_temp_f_  = celsius_to_fahrenheit(ambient_temp_c_, false);

        ESP_LOGW(TAG, "RX ambient temp: %.1f°C / %.1f°F (raw=0x%02X 0x%02X)",
        ambient_temp_c_, ambient_temp_f_, temp_int, temp_dec);
      }

      uint32_t now = millis();
      if (now - cmd_send_fingerprint_tmp.timestamp_ms >= ACK_EVAL_MIN_MS) {
        if (!from_remote_ && cmd_send_fingerprint_tmp.fingerprint != 0) {
          Frame_with_Fingerprint cmd_recieve_fingerprint = fingerprint();
          log_fingerprint("decode_frame", cmd_recieve_fingerprint);
          if (!compare_fingerprints(cmd_send_fingerprint_tmp.fingerprint, cmd_recieve_fingerprint.fingerprint)) {
            if (cmd_send_fingerprint_tmp.tryCnt < maxRetry) {
              Frame_with_Fingerprint cmd_send_fingerprint_before_send = cmd_send_fingerprint_tmp;
              if (cmd_send_fingerprint_before_send.tryCnt == 0) {
                log_fingerprint("Mismatch cmd", cmd_send_fingerprint_tmp, cmd_recieve_fingerprint);
                cmd_send_fingerprint_before_send.frame = make_muted_with_fixed_crc(cmd_send_fingerprint_before_send.frame);
                cmd_failure_counter_ = cmd_failure_counter_ + 1;
              } else {
                log_fingerprint("Mismatch retry cmd", cmd_send_fingerprint_tmp, cmd_recieve_fingerprint);
              }
              cmd_send_fingerprint_before_send.tryCnt = cmd_send_fingerprint_before_send.tryCnt + 1;
              ESP_LOGE(TAG, "Last tx retry %d/%d", cmd_send_fingerprint_before_send.tryCnt, maxRetry);
              send_command_basic(cmd_send_fingerprint_before_send);
            } else {
              ESP_LOGE(TAG, "Last tx cannot retry because max exceeded");
              cmd_failure_counter_++;
              ack_wait_ = false;
              ack_block_until_ = 0;
              timeout_retry_pending_ = false;
              // Fallback: resync immediately (fp==0 does not touch in-flight tracking)
              send_static_command_basic(get_status_frame_);
              ESP_LOGW(TAG, "Fingerprint Reset (max-retry fallback)");
            }
            
          } else {
            const uint32_t rtt = (uint32_t)(now - cmd_send_fingerprint_tmp.timestamp_ms);
            ESP_LOGW(TAG, "ACK matched after %lu ms", (unsigned long)rtt);
            ack_wait_ = false;
            timeout_retry_pending_ = false;
          }
          if (cmd_send_fingerprint_tmp.fingerprint == cmd_send_fingerprint_.fingerprint &&
            cmd_send_fingerprint_tmp.timestamp_ms == cmd_send_fingerprint_.timestamp_ms &&
            cmd_send_fingerprint_tmp.tryCnt == cmd_send_fingerprint_.tryCnt) {
            cmd_send_fingerprint_ = {0, "", {}, 0, 0};
            ESP_LOGW(TAG, "Fingerprint Reset");
          } else {
            ESP_LOGE(TAG, "Fingerprint Not reset because cmd fp diff");
          }
        }
      } else {
        ESP_LOGE(TAG, "Fingerprint ignored because time < %lu ms", (unsigned long)ACK_EVAL_MIN_MS);
      }

      if (mqtt_) {
        publish_availability();
        publish_state();
      }

      if (previous_power_on != power_on_) {
        reset_options_when_off();
      }

      if ((!eco_ && mode_ != Mode::AUTO) || !power_on_) {
          previous_target_temp_c_ = target_temp_c_;
          previous_temp_c_pref_.save(&previous_target_temp_c_);
          previous_target_temp_f_ = target_temp_f_;
          previous_temp_f_pref_.save(&previous_target_temp_f_);
      }
      
      
      if (previous_eco != eco_) {
        recalculate_climate_depending_by_option();
      } else if (previous_mode != mode_ && (previous_mode == Mode::AUTO || mode_ == Mode::AUTO)) {
        publish_discovery_climate_deref();
      } else if (previous_fahrenheit != use_fahrenheit_) {
        publish_discovery_climate(true);
        publish_discovery_temperature_number(true);
        publish_discovery_temperature_sensor(true);
      }


      if (force_cool_mode_if_disabled()) {
        if (mode_ == Mode::AUTO && from_remote_) {
            const uint8_t temptarget_temp_c_ = target_temp_c_;
            const uint8_t tempCalc = auto_temp_defined_heat_cool_calculator();
            if (tempCalc != temptarget_temp_c_) {
              mute_tmp_other_ = true;
              send_command();
              mute_tmp_other_ = false;
            } else {
              send_command();
            }
        } else {
          send_command();
        }
      } else {
        if (mode_ == Mode::AUTO && from_remote_) {
            const uint8_t temptarget_temp_c_ = target_temp_c_;
            const uint8_t tempCalc = auto_temp_defined_heat_cool_calculator();
            if (tempCalc != temptarget_temp_c_) {
              mute_tmp_other_ = true;
              send_command();
              mute_tmp_other_ = false;
            }
        }
      }
      ESP_LOGW(TAG, "decode_state =====>>>> AC frame end !");
    }

    bool ACW02::force_cool_mode_if_disabled() {
      Mode tmp = mode_;
      if (mode_ == Mode::AUTO && is_disable_mode_auto()) tmp = Mode::COOL;
      if (mode_ == Mode::HEAT && is_disable_mode_heat()) tmp = Mode::COOL;
      if (mode_ == Mode::FAN && is_disable_mode_fan())   tmp = Mode::COOL;
      if (mode_ == Mode::DRY && is_disable_mode_dry())   tmp = Mode::COOL;
      if (tmp != mode_) {
        if (mode_ == Mode::AUTO) {
          mode_ = tmp;
          target_temp_c_ = previous_target_temp_c_;
          target_temp_f_ = previous_target_temp_f_;
          publish_discovery_climate_deref();
        } else {
          mode_ = tmp;
        }
        return true;
      }
      return false;
    }

    void ACW02::publish_async(const std::string &topic, const std::string &payload, int qos, bool retain) {
      if (mqtt_publish_queue_.size() >= 128) {
        mqtt_publish_queue_.pop_front();
      }
      mqtt_publish_queue_.push_back(MqttPublishEntry{topic, payload, qos, retain});
    }

    Fan ACW02::str_to_fan(const std::string& lang, const std::string &s) {
      if (s == key_to_txt(lang, "fan", "AUTO"))    return Fan::AUTO;
      if (s == key_to_txt(lang, "fan", "P20"))     return Fan::P20;
      if (s == key_to_txt(lang, "fan", "P40"))     return Fan::P40;
      if (s == key_to_txt(lang, "fan", "P60"))     return Fan::P60;
      if (s == key_to_txt(lang, "fan", "P80"))     return Fan::P80;
      if (s == key_to_txt(lang, "fan", "P100"))    return Fan::P100;
      if (s == key_to_txt(lang, "fan", "SILENT"))  return Fan::SILENT;
      if (s == key_to_txt(lang, "fan", "TURBO"))   return Fan::TURBO;
      return Fan::AUTO;
    }

    Mode ACW02::str_to_mode_climate(const std::string &m) {
      if (m == "dry") return Mode::DRY;
      if (m == "fan_only") return Mode::FAN;
      if (m == "auto") return Mode::AUTO;
      if (m == "heat") return Mode::HEAT;
      return Mode::COOL;
    }

    Mode ACW02::str_to_mode(const std::string& lang, const std::string &m) {
      if (m == key_to_txt(lang, "mode", "DRY")) return Mode::DRY;
      if (m == key_to_txt(lang, "mode", "FAN")) return Mode::FAN;
      if (m == key_to_txt(lang, "mode", "AUTO")) return Mode::AUTO;
      if (m == key_to_txt(lang, "mode", "HEAT")) return Mode::HEAT;
      return Mode::COOL;
    }

    std::string ACW02::mode_to_string_climate(Mode mode) {
      switch (mode) {
        case Mode::COOL: return "cool";
        case Mode::DRY:  return "dry";
        case Mode::FAN:  return "fan_only";
        case Mode::HEAT: return "heat";
        case Mode::AUTO: return "auto";
        default:         return "?";
      }
    }

    std::string ACW02::mode_to_string(const std::string& lang, Mode mode) {
      switch (mode) {
        case Mode::COOL: return key_to_txt(lang, "mode", "COOL");
        case Mode::DRY:  return key_to_txt(lang, "mode", "DRY");
        case Mode::FAN:  return key_to_txt(lang, "mode", "FAN");
        case Mode::HEAT: return key_to_txt(lang, "mode", "HEAT");
        case Mode::AUTO: return key_to_txt(lang, "mode", "AUTO");
        default:         return "?";
      }
    }

    std::string ACW02::fan_to_string(const std::string& lang, Fan fan) {
      switch (fan) {
        case Fan::AUTO:   return key_to_txt(lang, "fan", "AUTO");
        case Fan::P20:    return key_to_txt(lang, "fan", "P20");
        case Fan::P40:    return key_to_txt(lang, "fan", "P40");
        case Fan::P60:    return key_to_txt(lang, "fan", "P60");
        case Fan::P80:    return key_to_txt(lang, "fan", "P80");
        case Fan::P100:   return key_to_txt(lang, "fan", "P100");
        case Fan::SILENT: return key_to_txt(lang, "fan", "SILENT");
        case Fan::TURBO:  return key_to_txt(lang, "fan", "TURBO");
        default:          return "?";
      }
    }

    Swing ACW02::str_to_swing(const std::string& lang, const std::string &s) {
      if (s == key_to_txt(lang, "swing", "STOP")) return Swing::STOP;
      if (s == key_to_txt(lang, "swing", "SWING")) return Swing::SWING;
      if (s == key_to_txt(lang, "swing", "P1"))   return Swing::P1;
      if (s == key_to_txt(lang, "swing", "P2"))   return Swing::P2;
      if (s == key_to_txt(lang, "swing", "P3"))   return Swing::P3;
      if (s == key_to_txt(lang, "swing", "P4"))   return Swing::P4;
      if (s == key_to_txt(lang, "swing", "P5"))   return Swing::P5;
      return Swing::P1;
    }

    std::string ACW02::swing_to_string(const std::string& lang, Swing swing) {
      switch (swing) {
        case Swing::STOP:  return key_to_txt(lang, "swing", "STOP");
        case Swing::SWING: return key_to_txt(lang, "swing", "SWING");
        case Swing::P1:    return key_to_txt(lang, "swing", "P1");
        case Swing::P2:    return key_to_txt(lang, "swing", "P2");
        case Swing::P3:    return key_to_txt(lang, "swing", "P3");
        case Swing::P4:    return key_to_txt(lang, "swing", "P4");
        case Swing::P5:    return key_to_txt(lang, "swing", "P5");
        default:           return "?";
      }
    }

    SwingHorizontal ACW02::str_to_swing_horizontal(const std::string& lang, const std::string &s) {
      if (s == key_to_txt(lang, "swingHorizontal", "STOP"))           return SwingHorizontal::STOP;
      if (s == key_to_txt(lang, "swingHorizontal", "AUTO_LEFT"))      return SwingHorizontal::AUTO_LEFT;
      if (s == key_to_txt(lang, "swingHorizontal", "P1"))             return SwingHorizontal::P1;
      if (s == key_to_txt(lang, "swingHorizontal", "P2"))             return SwingHorizontal::P2;
      if (s == key_to_txt(lang, "swingHorizontal", "P3"))             return SwingHorizontal::P3;
      if (s == key_to_txt(lang, "swingHorizontal", "P4"))             return SwingHorizontal::P4;
      if (s == key_to_txt(lang, "swingHorizontal", "P5"))             return SwingHorizontal::P5;
      if (s == key_to_txt(lang, "swingHorizontal", "RANGE_P1_P5"))    return SwingHorizontal::RANGE_P1_P5;
      if (s == key_to_txt(lang, "swingHorizontal", "AUTO_MID_OUT"))   return SwingHorizontal::AUTO_MID_OUT;
      return SwingHorizontal::STOP;
    }

    std::string ACW02::swing_horizontal_to_string(const std::string& lang, SwingHorizontal swing) {
       switch (swing) {
        case SwingHorizontal::STOP:           return key_to_txt(lang, "swingHorizontal", "STOP");
        case SwingHorizontal::AUTO_LEFT:     return key_to_txt(lang, "swingHorizontal", "AUTO_LEFT");
        case SwingHorizontal::P1:            return key_to_txt(lang, "swingHorizontal", "P1");
        case SwingHorizontal::P2:            return key_to_txt(lang, "swingHorizontal", "P2");
        case SwingHorizontal::P3:            return key_to_txt(lang, "swingHorizontal", "P3");
        case SwingHorizontal::P4:            return key_to_txt(lang, "swingHorizontal", "P4");
        case SwingHorizontal::P5:            return key_to_txt(lang, "swingHorizontal", "P5");
        case SwingHorizontal::RANGE_P1_P5:   return key_to_txt(lang, "swingHorizontal", "RANGE_P1_P5");
        case SwingHorizontal::AUTO_MID_OUT:  return key_to_txt(lang, "swingHorizontal", "AUTO_MID_OUT");
        default:                             return "?";
      }
    }

    float ACW02::celsius_to_fahrenheit(float c, bool floor) const {
      if (floor) {
        return floorf(c * 9.0f / 5.0f + 32.0f);
      } else {
        return (c * 9.0f / 5.0f + 32.0f);
      }
    }

    float ACW02::fahrenheit_to_celsius(float f) const {
      return roundf((f - 32.0f) * 5.0f / 9.0f);
    }

    uint8_t ACW02::encode_temperature_byte() const {
      if (use_fahrenheit_) {
        uint8_t t = target_temp_f_;
        if (t < 61) t = 61;
        if (t > 88) t = 88;
        uint8_t idx = t - 61;
        if (idx >= sizeof(fahrenheit_encoding_table_)) return 0x20;
        return fahrenheit_encoding_table_[idx];
      } else {
        uint8_t t = target_temp_c_;
        if (t < 16) t = 16;
        if (t > 31) t = 31;
        return t - 16;
      }
    }

    std::string ACW02::build_modes_json_climate() const {
      std::string out = "[\"off\"";
      auto push = [&out](const char *m) {
        out += ",\""; out += m; out += "\"";
      };

      if (!is_disable_mode_auto()) push("auto");
      push("cool");
      if (!is_disable_mode_dry() ) push("dry");
      if (!is_disable_mode_heat()) push("heat");
      if (!is_disable_mode_fan() ) push("fan_only");

      out += "]";
      return out;
    }

    std::string ACW02::build_modes_json() const {
      const auto &mode_dict = LOCALIZED.at(app_lang_).at("mode");
      std::vector<std::string> out;
      for (auto it = mode_dict.begin(); it != mode_dict.end(); ++it) {
        const std::string &key = it->first;
        const std::string &label = it->second;
        if (key == "OFF") {
          out.push_back(label);
        } else if (key == "AUTO" && !is_disable_mode_auto()) {
          out.push_back(label);
        } else if (key == "COOL") {
          out.push_back(label);
        } else if (key == "DRY" && !is_disable_mode_dry()) {
          out.push_back(label);
        } else if (key == "HEAT" && !is_disable_mode_heat()) {
          out.push_back(label);
        } else if (key == "FAN" && !is_disable_mode_fan()) {
          out.push_back(label);
        }
      }
      return "[\"" + join(out, R"(",")") + "\"]";
    }

    std::string ACW02::build_fan_speed_json() const {
      const auto &fan_map = LOCALIZED.at(app_lang_).at("fan");
      std::vector<std::string> options;
      for (const auto &kv : fan_map) {
        const std::string &key = kv.first;
        if (eco_ && option_recalculate_climate_ && key != "AUTO")
        continue;
        options.push_back(kv.second);
      }
      return "[\"" + join(options, R"(",")") + "\"]";
    }


    void ACW02::reset_options_when_off() {
        if (auto_off_options_when_ac_off_) {
          //disable mode eco
          set_eco_internal(false);
          //disable mode purifier
          purifier_ = false;
        }
        //disable mode night
        night_ = false;
        publish_discovery_climate_deref();
    }

    uint8_t ACW02::auto_temp_defined_heat_cool_calculator() {
      if (ambient_temp_c_ < 20) {
        target_temp_c_ = 20.0f;
        target_temp_f_ = 68.0f;
        return target_temp_c_;
      } else {
        target_temp_c_ = 25.0f;
        target_temp_f_ = 77.0f;
        return target_temp_c_;
      }
    }

    void ACW02::recalculate_climate_depending_by_option() {
      if (option_recalculate_climate_) {
        publish_discovery_climate(true);
      } else {
        publish_discovery_climate_deref();
      }
      
    }

    Frame_with_Fingerprint ACW02::fingerprint() const {
      return {
        ac_to_fingerprint(),
        fingerprint_to_string(),
        {},
        millis(),
        0
      };
    }

    uint32_t ACW02::ac_to_fingerprint() const {
      uint32_t hash = 2166136261u;
      auto hash_combine = [&hash](uint8_t value) {
        hash ^= value;
        hash *= 16777619u;
      };

      hash_combine(static_cast<uint8_t>(mode_));
      hash_combine(static_cast<uint8_t>(fan_));
      hash_combine(power_on_ ? 1 : 0);
      hash_combine(encode_temperature_byte());
      hash_combine(static_cast<uint8_t>(swing_horizontal_));
      hash_combine(static_cast<uint8_t>(swing_position_));
      hash_combine(eco_ ? 1 : 0);
      hash_combine(night_ ? 1 : 0);
      hash_combine(clean_ ? 1 : 0);
      hash_combine(purifier_ ? 1 : 0);
      hash_combine(display_ ? 1 : 0);

      return hash;
    }

     std::string ACW02::fingerprint_to_string() const {
      char buf[256];
      snprintf(buf, sizeof(buf),
        "Mode=%s Fan=%s Power=%s Temp=%d SwingH=%s SwingPos=%s Eco=%s Night=%s Clean=%s Purifier=%s Display=%s",
        mode_to_string(app_lang_, mode_).c_str(),
        fan_to_string(app_lang_, fan_).c_str(),
        power_on_ ? "On" : "Off",
        target_temp_c_,
        swing_horizontal_to_string(app_lang_, swing_horizontal_).c_str(),
        swing_to_string(app_lang_, swing_position_).c_str(),
        eco_ ? "On" : "Off",
        night_ ? "On" : "Off",
        clean_ ? "On" : "Off",
        purifier_ ? "On" : "Off",
        display_ ? "On" : "Off"
      );
      return std::string(buf);
    }

    void ACW02::log_fingerprint(std::string from, Frame_with_Fingerprint fp, Frame_with_Fingerprint tfp) const {
      if (tfp.fingerprint != 0)
      {
        ESP_LOGE(TAG, "%s", from.c_str());
        char buftx[255];
        snprintf(buftx, sizeof(buftx), "Fingerprint TX = 0x%08X -> %s", fp.fingerprint, fp.description.c_str());
        ESP_LOGE(TAG, "%s", buftx);
        char bufrx[255];
        snprintf(bufrx, sizeof(bufrx), "Fingerprint RX = 0x%08X -> %s", tfp.fingerprint, tfp.description.c_str());
        ESP_LOGE(TAG, "%s", bufrx);    
      } else {
        char buf[255];
        snprintf(buf, sizeof(buf), "%s : Fingerprint = 0x%08X -> %s", from.c_str(), fp.fingerprint, fp.description.c_str());
        ESP_LOGW(TAG, "%s", buf);
      }
      
    }

    bool ACW02::compare_fingerprints(uint32_t a, uint32_t b) {
      return a == b;
    }

    std::string ACW02::get_preset_list(bool only_non_empty, bool forClimate) {
      std::string json = "[\"" + PRESETS_LIST_ELEMENT_CONFIG_DEFAULT + "\",";
      if (only_non_empty) {
        if (forClimate) {
          json = json = "[";
        } else {
          json = json = "[\"" + get_localized_name(app_lang_, "presetNone") + "\"";
        }
      }
      bool first = true;
      for (const auto &slot : presets_list) {
        if (only_non_empty && slot.frame_with_fp.frame.empty())
          continue;
        if (first && only_non_empty && !forClimate)
          json += ",";
        if (!first)
          json += ",";
        first = false;
        json += "\"" + slot.name + "\"";
      }
      json += "]";
      return json;
    }

    std::vector<uint8_t> ACW02::decode_frame_base64(const std::string &input) {
      auto decode_char = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
      };

      std::vector<uint8_t> output;
      int val = 0, valb = -8;

      for (char c : input) {
        if (c == '=')
          break;
        int d = decode_char(c);
        if (d == -1)
          continue;
        val = (val << 6) + d;
        valb += 6;
        if (valb >= 0) {
          output.push_back((val >> valb) & 0xFF);
          valb -= 8;
        }
      }

      return output;
    }

    std::string ACW02::encode_frame_base64(const std::vector<uint8_t> &input) {
      static const char *base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
      std::string output;
      int val = 0, valb = -6;

      for (uint8_t c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
          output.push_back(base64_chars[(val >> valb) & 0x3F]);
          valb -= 6;
        }
      }

      if (valb > -6)
        output.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);

      while (output.size() % 4)
        output.push_back('=');

      return output;
    }

    void ACW02::update_selected_preset(const std::string &new_name, const Frame_with_Fingerprint &new_frame) {
      for (auto &preset : presets_list) {
        if (preset.name == presets_list_element_config_) {
          preset.frame_with_fp = new_frame;
          preset.name = std::to_string(preset.index) + " " + new_name;
          preset.frame_with_fp.description = "presets \"" + preset.name + "\"";
          save_single_preset_to_flash(preset);
          ESP_LOGW(TAG, "Save Preset: \"%s\" frame: %s fingerprint: 0x%08X", preset.name.c_str(), format_hex_pretty(preset.frame_with_fp.frame).c_str(), preset.frame_with_fp.fingerprint);
          presets_list_element_config_ = PRESETS_LIST_ELEMENT_CONFIG_DEFAULT;
          publish_discovery_preset_select(true);
          publish_discovery_preset_config_select(true);
          publish_discovery_climate(true);
          return;
        }
      }
      ESP_LOGW(TAG, "No preset found with name: %s", presets_list_element_config_.c_str());
    }

    void ACW02::load_presets_from_flash() {
      for (auto &preset : presets_list) {
        int i = preset.index;

        // read name
        char name_key[32];
        snprintf(name_key, sizeof(name_key), "preset_%d_name", i);
        auto name_pref = global_preferences->make_preference<char[25]>(fnv1_hash(name_key));
        char name_buf[25];
        if (name_pref.load(&name_buf)) {
          preset.name = name_buf;
        }

        // read encoded frame
        char frame_key[32];
        snprintf(frame_key, sizeof(frame_key), "preset_%d_trame", i);
        auto frame_pref = global_preferences->make_preference<char[64]>(fnv1_hash(frame_key));
        char frame_buf[64];
        if (frame_pref.load(&frame_buf)) {
          preset.frame_with_fp.frame = decode_frame_base64(frame_buf);
        }

        // read encoded fingerprint
        char fp_key[32];
        snprintf(fp_key, sizeof(fp_key), "preset_%d_fp", i);
        auto fp_pref = global_preferences->make_preference<uint32_t>(fnv1_hash(fp_key));
        uint32_t fp = 0;
        if (fp_pref.load(&fp)) {
          preset.frame_with_fp.fingerprint = fp;
        } else {
          preset.frame_with_fp.fingerprint = 0;
        }
        preset.frame_with_fp.description = "presets \"" + preset.name + "\"";;
        preset.frame_with_fp.timestamp_ms = 0;
        preset.frame_with_fp.tryCnt = 0;
      }
    }

    void ACW02::save_presets_to_flash() {
      for (const auto &preset : presets_list) {
        save_single_preset_to_flash(preset);
      }
    }

    void ACW02::save_single_preset_to_flash(const PresetSlot &preset) {
      // save name
      char name_key[32];
      snprintf(name_key, sizeof(name_key), "preset_%d_name", preset.index);
      auto name_pref = global_preferences->make_preference<char[25]>(fnv1_hash(name_key));
      char name_buf[25];
      strncpy(name_buf, preset.name.c_str(), sizeof(name_buf));
      name_buf[sizeof(name_buf) - 1] = '\0';  // sécurité
      name_pref.save(&name_buf);

      // save frame
      char frame_key[32];
      snprintf(frame_key, sizeof(frame_key), "preset_%d_trame", preset.index);
      auto frame_pref = global_preferences->make_preference<char[64]>(fnv1_hash(frame_key));
      std::string base64 = encode_frame_base64(preset.frame_with_fp.frame);
      char base64_buf[64];
      strncpy(base64_buf, base64.c_str(), sizeof(base64_buf));
      base64_buf[sizeof(base64_buf) - 1] = '\0';
      frame_pref.save(&base64_buf);

      // save fingerprint
      char fp_key[32];
      snprintf(fp_key, sizeof(fp_key), "preset_%d_fp", preset.index);
      auto fp_pref = global_preferences->make_preference<uint32_t>(fnv1_hash(fp_key));
      uint32_t fp = preset.frame_with_fp.fingerprint;
      fp_pref.save(&fp);
    }

    void ACW02::delete_preset_by_name() {
      for (auto &preset : presets_list) {
        if (preset.name == presets_list_element_config_) {
          ESP_LOGI(TAG, "Deleting preset: %s (index %d)", preset.name.c_str(), preset.index);

          // Delete the custom name by overwriting with the default name
          char name_key[32];
          snprintf(name_key, sizeof(name_key), "preset_%d_name", preset.index);
          auto name_pref = global_preferences->make_preference<char[25]>(fnv1_hash(name_key));
          char default_name[25];
          snprintf(default_name, sizeof(default_name), "Preset %d (empty)", preset.index);
          name_pref.save(&default_name);

          // Delete the frame by overwriting with an empty string
          char frame_key[32];
          snprintf(frame_key, sizeof(frame_key), "preset_%d_trame", preset.index);
          auto frame_pref = global_preferences->make_preference<char[64]>(fnv1_hash(frame_key));
          char empty[64] = {0};
          frame_pref.save(&empty);

          // Remove the fp by overwriting with an empty string
          char fp_key[32];
          snprintf(fp_key, sizeof(fp_key), "preset_%d_fp", preset.index);
          auto fp_pref = global_preferences->make_preference<uint32_t>(fnv1_hash(fp_key));
          uint32_t zero = 0;
          fp_pref.save(&zero);

          // Reset to memory
          preset.name = default_name;
          preset.frame_with_fp = {0, "", {}, 0, 0};
          presets_list_element_config_ = PRESETS_LIST_ELEMENT_CONFIG_DEFAULT;
          publish_discovery_preset_select(true);
          publish_discovery_preset_config_select(true);
          publish_discovery_climate(true);
          return;
        }
      }

      ESP_LOGW(TAG, "Preset name not found: %s", presets_list_element_config_.c_str());
    }

    PresetSlot ACW02::get_preset_by_name(const std::string &name) {
      for (const auto &preset : presets_list) {
        if (preset.name == name) {
          return preset;
        }
      }
      return PresetSlot{};
    }
  }
}
