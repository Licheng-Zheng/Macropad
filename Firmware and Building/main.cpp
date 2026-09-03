#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "bsp/board.h"
#include "tusb.h"
#include "pico/bootrom.h"

// ---------------------------------------------------------
// HARDWARE PIN DEFINITIONS
// ---------------------------------------------------------
const uint KEY_PINS[9] = {9, 10, 11, 6, 7, 8, 3, 4, 5}; 
const uint JOY_X_PIN = 26; 
const uint JOY_Y_PIN = 27; 

#define I2C_PORT i2c0
const uint I2C_SDA = 0;
const uint I2C_SCL = 1;
const uint8_t OLED_ADDR = 0x3C; 

// ---------------------------------------------------------
// ASCII TO HID PARSER
// ---------------------------------------------------------
void get_hid_for_ascii(char c, uint8_t* keycode, uint8_t* modifier) {
    *modifier = 0;
    *keycode = 0;
    
    if (c >= 'a' && c <= 'z') { *keycode = HID_KEY_A + (c - 'a'); }
    else if (c >= 'A' && c <= 'Z') { *keycode = HID_KEY_A + (c - 'A'); *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; }
    else if (c >= '1' && c <= '9') { *keycode = HID_KEY_1 + (c - '1'); }
    else if (c == '0') { *keycode = HID_KEY_0; }
    else if (c == '\n') { *keycode = HID_KEY_ENTER; }
    else if (c == ' ') { *keycode = HID_KEY_SPACE; }
    else if (c == '.') { *keycode = HID_KEY_PERIOD; }
    else if (c == '-') { *keycode = HID_KEY_MINUS; }
    else if (c == '_') { *keycode = HID_KEY_MINUS; *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; }
    else if (c == '/') { *keycode = HID_KEY_SLASH; }
    else if (c == '@') { *keycode = HID_KEY_2; *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; }
    else if (c == ':') { *keycode = HID_KEY_SEMICOLON; *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; }
    else if (c == '!') { *keycode = HID_KEY_1; *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; }
    else if (c == '#') { *keycode = HID_KEY_3; *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; }
    else if (c == '$') { *keycode = HID_KEY_4; *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; }
    else if (c == '%') { *keycode = HID_KEY_5; *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; }
    else if (c == '&') { *keycode = HID_KEY_7; *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; }
    else if (c == '*') { *keycode = HID_KEY_8; *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; }
    else if (c == '(') { *keycode = HID_KEY_9; *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; }
    else if (c == ')') { *keycode = HID_KEY_0; *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; }
    else if (c == '=') { *keycode = HID_KEY_EQUAL; }
    else if (c == '+') { *keycode = HID_KEY_EQUAL; *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; }
    else if (c == '[') { *keycode = HID_KEY_BRACKET_LEFT; }
    else if (c == ']') { *keycode = HID_KEY_BRACKET_RIGHT; }
    else if (c == '{') { *keycode = HID_KEY_BRACKET_LEFT; *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; }
    else if (c == '}') { *keycode = HID_KEY_BRACKET_RIGHT; *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; }
    else if (c == '\\') { *keycode = HID_KEY_BACKSLASH; }
    else if (c == '|') { *keycode = HID_KEY_BACKSLASH; *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; }
    else if (c == ';') { *keycode = HID_KEY_SEMICOLON; }
    else if (c == '\'') { *keycode = HID_KEY_APOSTROPHE; }
    else if (c == '"') { *keycode = HID_KEY_APOSTROPHE; *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; }
    else if (c == ',') { *keycode = HID_KEY_COMMA; }
    else if (c == '<') { *keycode = HID_KEY_COMMA; *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; }
    else if (c == '>') { *keycode = HID_KEY_PERIOD; *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; }
    else if (c == '?') { *keycode = HID_KEY_SLASH; *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; }
    else if (c == '`') { *keycode = HID_KEY_GRAVE; }
    else if (c == '~') { *keycode = HID_KEY_GRAVE; *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; }
}

// ---------------------------------------------------------
// USER CONFIGURATION ZONE (9 PROFILES)
// ---------------------------------------------------------
const char* MACRO_STRINGS[9][9] = {
    // Profile 1: Numpad (Mode 0)
    {
        "7", "8", "9",
        "4", "5", "6",
        "1", "2", "3"
    },
    // Profile 2: Emacs (Mode 1)
    {
    "", "", "",
    "", "", "",
    "", "", ""
    },
    // Profile 3: Custom (Mode 2)
    {
        "", "", "",
        "", "", "",
        "", "", ""
    },
    // Profile 4: Custom (Mode 3)
    {
        "", "", "",
        "", "", "",
        "", "", ""
    },
    // Profile 5: Custom (Mode 4)
    {
        "", "", "",
        "", "", "",
        "", "", ""
    },
    // Profile 6: Custom (Mode 5)
    {
        "", "", "",
        "", "", "",
        "", "", ""
    },
    // Profile 7: Custom (Mode 6)
    {
        "", "", "",
        "", "", "",
        "", "", ""
    },
    // Profile 8: Custom (Mode 7)
    {
        "", "", "",
        "", "", "",
        "", "", ""
    },
    // Profile 9: Custom (Mode 8)
    {
        "", "", "",
        "", "", "",
        "", "", ""
    }
};

const char* MODE_NAMES[9] = {
    "MODE: NUMPAD",   "MODE: EMACS", "MODE: CUSTOM 2", 
    "MODE: CUSTOM 3", "MODE: CUSTOM 4", "MODE: CUSTOM 5", 
    "MODE: CUSTOM 6", "MODE: CUSTOM 7", "MODE: CUSTOM 8"
};

// ---------------------------------------------------------
// PROFILE ROUTING MAP
// ---------------------------------------------------------
// Maps the 9 physical keys to a specific Profile index (0-8)
// Layout: 
// [Top L (Num 7), Top M (Num 8), Top R (Num 9)]
// [Mid L (Num 4), Mid M (Num 5), Mid R (Num 6)]
// [Bot L (Num 1), Bot M (Num 2), Bot R (Num 3)]
const uint8_t MODE_SELECTION_MAP[9] = {
    7, 8, 0,  // Top Row: Maps to Mode 7, Mode 8, Mode 0 (Numpad)
    4, 5, 6,  // Mid Row: Maps to Mode 4, Mode 5, Mode 6
    1, 2, 3   // Bot Row: Maps to Mode 1, Mode 2, Mode 3
};

// ---------------------------------------------------------
// FONT DICTIONARY (Standard 5x7 ASCII)
// ---------------------------------------------------------
static const uint8_t font_5x7[96][5] = {
    {0x00,0x00,0x00,0x00,0x00}, {0x00,0x00,0x5f,0x00,0x00}, {0x00,0x07,0x00,0x07,0x00}, {0x14,0x7f,0x14,0x7f,0x14},
    {0x24,0x2a,0x7f,0x2a,0x12}, {0x23,0x13,0x08,0x64,0x62}, {0x36,0x49,0x55,0x22,0x50}, {0x00,0x05,0x03,0x00,0x00},
    {0x00,0x1c,0x22,0x41,0x00}, {0x00,0x41,0x22,0x1c,0x00}, {0x14,0x08,0x3e,0x08,0x14}, {0x08,0x08,0x3e,0x08,0x08},
    {0x00,0x50,0x30,0x00,0x00}, {0x08,0x08,0x08,0x08,0x08}, {0x00,0x60,0x60,0x00,0x00}, {0x20,0x10,0x08,0x04,0x02},
    {0x3e,0x51,0x49,0x45,0x3e}, {0x00,0x42,0x7f,0x40,0x00}, {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4b,0x31},
    {0x18,0x14,0x12,0x7f,0x10}, {0x27,0x45,0x45,0x45,0x39}, {0x3c,0x4a,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1e}, {0x00,0x36,0x36,0x00,0x00}, {0x00,0x56,0x36,0x00,0x00},
    {0x08,0x14,0x22,0x41,0x00}, {0x14,0x14,0x14,0x14,0x14}, {0x00,0x41,0x22,0x14,0x08}, {0x02,0x01,0x51,0x09,0x06},
    {0x32,0x49,0x79,0x41,0x3e}, {0x7e,0x11,0x11,0x11,0x7e}, {0x7f,0x49,0x49,0x49,0x36}, {0x3e,0x41,0x41,0x41,0x22},
    {0x7f,0x41,0x41,0x22,0x1c}, {0x7f,0x49,0x49,0x49,0x41}, {0x7f,0x09,0x09,0x09,0x01}, {0x3e,0x41,0x49,0x49,0x7a},
    {0x7f,0x08,0x08,0x08,0x7f}, {0x00,0x41,0x7f,0x41,0x00}, {0x20,0x40,0x41,0x3f,0x01}, {0x7f,0x08,0x14,0x22,0x41},
    {0x7f,0x40,0x40,0x40,0x40}, {0x7f,0x02,0x0c,0x02,0x7f}, {0x7f,0x04,0x08,0x10,0x7f}, {0x3e,0x41,0x41,0x41,0x3e},
    {0x7f,0x09,0x09,0x09,0x06}, {0x3e,0x41,0x51,0x21,0x5e}, {0x7f,0x09,0x19,0x29,0x46}, {0x46,0x49,0x49,0x49,0x31},
    {0x01,0x01,0x7f,0x01,0x01}, {0x3f,0x40,0x40,0x40,0x3f}, {0x1f,0x20,0x40,0x20,0x1f}, {0x3f,0x40,0x38,0x40,0x3f},
    {0x63,0x14,0x08,0x14,0x63}, {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43}, {0x00,0x7f,0x41,0x41,0x00},
    {0x02,0x04,0x08,0x10,0x20}, {0x00,0x41,0x41,0x7f,0x00}, {0x04,0x02,0x01,0x02,0x04}, {0x40,0x40,0x40,0x40,0x40},
    {0x00,0x01,0x02,0x04,0x00}, {0x20,0x54,0x54,0x54,0x78}, {0x7f,0x48,0x44,0x44,0x38}, {0x38,0x44,0x44,0x44,0x20},
    {0x38,0x44,0x44,0x48,0x7f}, {0x38,0x54,0x54,0x54,0x18}, {0x08,0x7e,0x09,0x01,0x02}, {0x0c,0x52,0x52,0x52,0x3e},
    {0x7f,0x08,0x04,0x04,0x78}, {0x00,0x44,0x7d,0x40,0x00}, {0x20,0x40,0x44,0x3d,0x00}, {0x7f,0x10,0x28,0x44,0x00},
    {0x00,0x41,0x7f,0x40,0x00}, {0x7c,0x04,0x18,0x04,0x78}, {0x7c,0x08,0x04,0x04,0x78}, {0x38,0x44,0x44,0x44,0x38},
    {0x7c,0x14,0x14,0x14,0x08}, {0x08,0x14,0x14,0x18,0x7c}, {0x7c,0x08,0x04,0x04,0x08}, {0x48,0x54,0x54,0x54,0x20},
    {0x04,0x3f,0x44,0x40,0x20}, {0x3c,0x40,0x40,0x20,0x7c}, {0x1c,0x20,0x40,0x20,0x1c}, {0x3c,0x40,0x30,0x40,0x3c},
    {0x44,0x28,0x10,0x28,0x44}, {0x0c,0x50,0x50,0x50,0x3c}, {0x44,0x64,0x54,0x4c,0x44}, {0x00,0x08,0x36,0x41,0x00},
    {0x00,0x00,0x7f,0x00,0x00}, {0x00,0x41,0x36,0x08,0x00}, {0x10,0x08,0x08,0x10,0x08}, {0x00,0x00,0x00,0x00,0x00}
};

// ---------------------------------------------------------
// OLED DRIVER & GRAPHICS
// ---------------------------------------------------------
uint8_t frame_buffer[1024];

void oled_send_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd}; 
    i2c_write_blocking(I2C_PORT, OLED_ADDR, buf, 2, false);
}

void oled_update() {
    uint8_t payload[1025];
    payload[0] = 0x40; 
    memcpy(&payload[1], frame_buffer, 1024);
    oled_send_cmd(0x21); oled_send_cmd(0); oled_send_cmd(127);
    oled_send_cmd(0x22); oled_send_cmd(0); oled_send_cmd(7);
    i2c_write_blocking(I2C_PORT, OLED_ADDR, payload, 1025, false);
}

void oled_draw_pixel(int x, int y, bool on) {
    if (x < 0 || x >= 128 || y < 0 || y >= 64) return;
    int index = ((y / 8) * 128) + x;
    if (on) frame_buffer[index] |= (1 << (y % 8));
    else    frame_buffer[index] &= ~(1 << (y % 8));
}

void oled_fill_rect(int x, int y, int w, int h, bool on) {
    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) oled_draw_pixel(x + i, y + j, on);
    }
}

void oled_draw_char(int x, int y, char c, bool color) {
    if (c < 32 || c > 127) return; 
    for (int i = 0; i < 5; i++) {
        uint8_t line = font_5x7[c - 32][i];
        for (int j = 0; j < 7; j++) {
            if (line & 0x01) oled_draw_pixel(x + i, y + j, color);
            line >>= 1;
        }
    }
}

void oled_print(int x, int y, const char* str, bool color = true) {
    while (*str) {
        if (*str == '\n') break; 
        oled_draw_char(x, y, *str, color);
        x += 6; 
        str++;
    }
}

// ---------------------------------------------------------
// TINYUSB REQUIRED CALLBACKS 
// ---------------------------------------------------------
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len) {}
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) { return 0; }
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {}

// ---------------------------------------------------------
// HARDWARE INITIALIZATION
// ---------------------------------------------------------
void setup_hardware() {
    board_init(); 
    tusb_init();  
    stdio_init_all();

    for (int i = 0; i < 9; i++) {
        gpio_init(KEY_PINS[i]);
        gpio_set_dir(KEY_PINS[i], GPIO_IN);
        gpio_pull_up(KEY_PINS[i]);
    }

    adc_init();
    adc_gpio_init(JOY_X_PIN);
    adc_gpio_init(JOY_Y_PIN);

    i2c_init(I2C_PORT, 400 * 1000); 
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    oled_send_cmd(0xAE); 
    oled_send_cmd(0x20); oled_send_cmd(0x00); 
    oled_send_cmd(0x81); oled_send_cmd(0xFF); 
    oled_send_cmd(0xA1); 
    oled_send_cmd(0xC8); 
    oled_send_cmd(0x8D); oled_send_cmd(0x14); 
    oled_send_cmd(0xAF); 
}

// ---------------------------------------------------------
// MAIN EXECUTION
// ---------------------------------------------------------
int main() {
    setup_hardware();

    const int JOYSTICK_DEADZONE = 150;

    uint32_t cal_x_sum = 0;
    uint32_t cal_y_sum = 0;
    for (int i = 0; i < 10; i++) {
        adc_select_input(0); cal_x_sum += adc_read();
        adc_select_input(1); cal_y_sum += adc_read();
        sleep_ms(10);
    }
    
    uint16_t center_x = cal_x_sum / 10;
    uint16_t center_y = cal_y_sum / 10;

    uint32_t last_oled_update = 0;
    uint32_t last_hid_update = 0;
    
    uint8_t current_mode = 0; 
    
    bool raw_state[9] = {false};
    bool stable_state[9] = {false};
    uint32_t last_edge_time[9] = {0};
    const uint32_t DEBOUNCE_MS = 100;
    
    bool top_chord_was_pressed = false;
    bool mid_chord_was_pressed = false;
    bool bot_chord_was_pressed = false;
    bool right_chord_was_pressed = false;
    
    bool pending_mode_select = false;
    bool waiting_for_chord = false;
    bool can_process_keys = true; 
    uint32_t chord_timer = 0;
    const uint32_t CHORD_WINDOW_MS = 20;

    bool joystick_enabled = false; 
    uint16_t current_joy_x = center_x;
    uint16_t current_joy_y = center_y;
    uint32_t last_joy_action_time = 0;
    bool joy_action_active = false;
    
    int active_key_index = -1;

    const char* active_string = nullptr;
    int string_index = 0;
    bool waiting_for_release_report = false;
    uint32_t last_type_time = 0;

    while (true) {
        tud_task(); 
        uint32_t current_time = to_ms_since_boot(get_absolute_time());

        if (current_time - last_hid_update >= 1) {
            last_hid_update = current_time;

            for (int i = 0; i < 9; i++) {
                bool current_raw = !gpio_get(KEY_PINS[i]);
                if (current_raw != raw_state[i]) {
                    last_edge_time[i] = current_time;
                    raw_state[i] = current_raw;
                }
                if ((current_time - last_edge_time[i]) > DEBOUNCE_MS) {
                    stable_state[i] = current_raw;
                }
            }

            bool all_top_chord_keys = (stable_state[0] && stable_state[1] && stable_state[2]);
            bool all_mid_chord_keys = (stable_state[3] && stable_state[4] && stable_state[5]);
            bool all_bot_chord_keys = (stable_state[6] && stable_state[7] && stable_state[8]);
            bool all_right_chord_keys = (stable_state[2] && stable_state[5] && stable_state[8]);
            
            bool any_top_chord_key = (stable_state[0] || stable_state[1] || stable_state[2]);
            bool any_mid_chord_key = (stable_state[3] || stable_state[4] || stable_state[5]);
            bool any_bot_chord_key = (stable_state[6] || stable_state[7] || stable_state[8]);
            bool any_right_chord_key = (stable_state[2] || stable_state[5] || stable_state[8]);
            bool any_chord_key = (any_top_chord_key || any_mid_chord_key || any_bot_chord_key || any_right_chord_key);

            // Execute Top Chord (Recalibrate Joystick)
            if (all_top_chord_keys && !top_chord_was_pressed && !all_right_chord_keys) {
                top_chord_was_pressed = true;
                waiting_for_chord = false; 
                active_key_index = -1;
                uint32_t recal_x_sum = 0, recal_y_sum = 0;
                for (int i = 0; i < 10; i++) {
                    adc_select_input(0); recal_x_sum += adc_read();
                    adc_select_input(1); recal_y_sum += adc_read();
                    sleep_ms(2); 
                }
                center_x = recal_x_sum / 10;
                center_y = recal_y_sum / 10;
                current_joy_x = center_x;
                current_joy_y = center_y;
                joy_action_active = false;
            } else if (!all_top_chord_keys) top_chord_was_pressed = false;

            // Execute Middle Chord (Joystick Toggle)
            if (all_mid_chord_keys && !mid_chord_was_pressed && !all_right_chord_keys) {
                joystick_enabled = !joystick_enabled; 
                mid_chord_was_pressed = true;
                waiting_for_chord = false; 
                active_key_index = -1;
            } else if (!all_mid_chord_keys) mid_chord_was_pressed = false;

            // Execute Bottom Chord (Enter Mode Selection)
            if (all_bot_chord_keys && !bot_chord_was_pressed && !all_right_chord_keys) {
                bot_chord_was_pressed = true;
                waiting_for_chord = false; 
                active_key_index = -1;
                pending_mode_select = true; 
            } else if (!all_bot_chord_keys) bot_chord_was_pressed = false;

            // Execute Right Chord (BOOTSEL)
            if (all_right_chord_keys && !right_chord_was_pressed) {
                right_chord_was_pressed = true;
                waiting_for_chord = false; 
                active_key_index = -1;
                oled_fill_rect(0, 0, 128, 64, false);
                oled_print(4, 28, "ENTERING BOOTLOADER");
                oled_update();
                sleep_ms(100); 
                reset_usb_boot(0, 0); 
            } else if (!all_right_chord_keys) right_chord_was_pressed = false;

            if (any_chord_key && !top_chord_was_pressed && !mid_chord_was_pressed && !bot_chord_was_pressed && !right_chord_was_pressed && !waiting_for_chord) {
                waiting_for_chord = true;
                chord_timer = current_time;
            }

            can_process_keys = true;
            if (waiting_for_chord) {
                if (current_time - chord_timer < CHORD_WINDOW_MS) can_process_keys = false; 
                else waiting_for_chord = false; 
            }

            adc_select_input(0);
            current_joy_x = adc_read();
            adc_select_input(1);
            current_joy_y = adc_read();

            int joy_x_offset = (int)current_joy_x - (int)center_x;
            int joy_y_offset = (int)current_joy_y - (int)center_y;
            if (joy_x_offset >= -JOYSTICK_DEADZONE && joy_x_offset <= JOYSTICK_DEADZONE) current_joy_x = center_x;
            if (joy_y_offset >= -JOYSTICK_DEADZONE && joy_y_offset <= JOYSTICK_DEADZONE) current_joy_y = center_y;

            if (tud_hid_ready()) {
                if (can_process_keys && !top_chord_was_pressed && !mid_chord_was_pressed && !bot_chord_was_pressed && !right_chord_was_pressed) {
                    bool any_key_pressed = false;
                    for (int i = 0; i < 9; i++) {
                        if (stable_state[i]) {
                            
                            if (pending_mode_select) {
                                if (active_key_index != i) {
                                    current_mode = MODE_SELECTION_MAP[i]; 
                                    pending_mode_select = false;
                                    active_key_index = i; 
                                    active_string = nullptr; 
                                }
                            } else {
                                if (active_key_index != i && active_string == nullptr) {
                                    active_string = MACRO_STRINGS[current_mode][i];
                                    string_index = 0;
                                    waiting_for_release_report = false;
                                    active_key_index = i;
                                }
                            }
                            
                            any_key_pressed = true;
                            break; 
                        }
                    }
                    if (!any_key_pressed) active_key_index = -1;
                }

                if (active_string != nullptr && !pending_mode_select) {
                    if (current_time - last_type_time > 1) {
                        last_type_time = current_time;
                        char c = active_string[string_index];
                        
                        if (c == '\0') {
                            active_string = nullptr;
                            tud_hid_keyboard_report(1, 0, NULL);
                        } else if (waiting_for_release_report) {
                            tud_hid_keyboard_report(1, 0, NULL);
                            waiting_for_release_report = false;
                            string_index++;
                        } else {
                            uint8_t mod = 0, key = 0;
                            get_hid_for_ascii(c, &key, &mod);
                            if (key != 0) {
                                uint8_t keycode_arr[6] = { key };
                                tud_hid_keyboard_report(1, mod, keycode_arr);
                            }
                            waiting_for_release_report = true;
                        }
                    }
                }

                if (joystick_enabled) {
                    int8_t mouse_x = -((int)current_joy_y - (int)center_y) / 128;
                    int8_t mouse_y = ((int)current_joy_x - (int)center_x) / 128;

                    switch (current_mode) {
                        case 0: // Numpad: Volume Control
                            if (mouse_y != 0) {
                                if (current_time - last_joy_action_time > 150) { 
                                    last_joy_action_time = current_time;
                                    uint16_t vol = (mouse_y < 0) ? HID_USAGE_CONSUMER_VOLUME_INCREMENT : HID_USAGE_CONSUMER_VOLUME_DECREMENT;
                                    tud_hid_report(3, &vol, sizeof(vol)); 
                                    joy_action_active = true;
                                } else if (joy_action_active && current_time - last_joy_action_time > 20) {
                                    uint16_t empty = 0;
                                    tud_hid_report(3, &empty, sizeof(empty));
                                    joy_action_active = false;
                                }
                            } else if (joy_action_active) {
                                uint16_t empty = 0;
                                tud_hid_report(3, &empty, sizeof(empty));
                                joy_action_active = false;
                            }
                            break;
                            
                        case 1: // Custom 1: Middle-Click Panning
                            if (mouse_x != 0 || mouse_y != 0) {
                                tud_hid_mouse_report(2, MOUSE_BUTTON_MIDDLE, mouse_x, mouse_y, 0, 0);
                                joy_action_active = true;
                            } else if (joy_action_active) {
                                tud_hid_mouse_report(2, 0, 0, 0, 0, 0);
                                joy_action_active = false;
                            }
                            break;
                            
                        case 2: // Custom 2: Left/Right Arrow Scrubbing
                            if (mouse_x != 0) {
                                if (current_time - last_joy_action_time > 80) { 
                                    last_joy_action_time = current_time;
                                    uint8_t key[6] = { (uint8_t)((mouse_x < 0) ? HID_KEY_ARROW_LEFT : HID_KEY_ARROW_RIGHT) };
                                    tud_hid_keyboard_report(1, 0, key);
                                    joy_action_active = true;
                                } else if (joy_action_active && current_time - last_joy_action_time > 20) {
                                    tud_hid_keyboard_report(1, 0, NULL);
                                    joy_action_active = false;
                                }
                            } else if (joy_action_active) {
                                tud_hid_keyboard_report(1, 0, NULL);
                                joy_action_active = false;
                            }
                            break;
                            
                        case 3: // Custom 3: Page Up/Down Scrolling
                            if (mouse_y != 0) {
                                if (current_time - last_joy_action_time > 200) { 
                                    last_joy_action_time = current_time;
                                    uint8_t key[6] = { (uint8_t)((mouse_y < 0) ? HID_KEY_PAGE_UP : HID_KEY_PAGE_DOWN) };
                                    tud_hid_keyboard_report(1, 0, key);
                                    joy_action_active = true;
                                } else if (joy_action_active && current_time - last_joy_action_time > 20) {
                                    tud_hid_keyboard_report(1, 0, NULL);
                                    joy_action_active = false;
                                }
                            } else if (joy_action_active) {
                                tud_hid_keyboard_report(1, 0, NULL);
                                joy_action_active = false;
                            }
                            break;
                            
                        default: // Modes 4 through 8: Standard Mouse Cursor
                            if (mouse_x != 0 || mouse_y != 0) {
                                tud_hid_mouse_report(2, 0, mouse_x, mouse_y, 0, 0);
                                joy_action_active = true;
                            } else if (joy_action_active) {
                                tud_hid_mouse_report(2, 0, 0, 0, 0, 0);
                                joy_action_active = false;
                            }
                            break;
                    }
                } else if (joy_action_active) {
                    tud_hid_mouse_report(2, 0, 0, 0, 0, 0);
                    tud_hid_keyboard_report(1, 0, NULL);
                    uint16_t empty = 0;
                    tud_hid_report(3, &empty, sizeof(empty));
                    joy_action_active = false;
                }
            }
        }

        if (current_time - last_oled_update >= 50) {
            last_oled_update = current_time;
            memset(frame_buffer, 0, sizeof(frame_buffer));
            
            // Draw a solid rectangle for the top bar
            oled_fill_rect(0, 0, 128, 11, true);
            
            // Render the joystick status inside the bar (false = black text on white background)
            char top_buffer[32];
            sprintf(top_buffer, "J:%s X:%04d Y:%04d", joystick_enabled ? "ON " : "OFF", current_joy_x, current_joy_y);
            oled_print(2, 2, top_buffer, false);

            // Display Active Profile Name underneath
            oled_print(4, 18, MODE_NAMES[current_mode]);

            // Display Active Macro or Chord Status
            if (pending_mode_select) {
                oled_print(4, 32, "SELECT PROFILE 1-9");
            } else if (active_key_index != -1 && can_process_keys) {
                if (MACRO_STRINGS[current_mode][active_key_index][0] != '\0') {
                    oled_print(4, 32, MACRO_STRINGS[current_mode][active_key_index]);
                } else {
                    oled_print(4, 32, "UNASSIGNED KEY");
                }
            } else if (top_chord_was_pressed) {
                oled_print(4, 32, "CALIBRATING...");
            } else if (mid_chord_was_pressed) {
                oled_print(4, 32, "TOGGLING JOYSTICK");
            } else if (waiting_for_chord) {
                oled_print(4, 32, "DETECTING CHORD...");
            }

            oled_update();
        }
    }
    return 0;
}