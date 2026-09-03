#include "tusb.h"

#define USBD_VID  0xCafe
#define USBD_PID  0x4004
#define USBD_DESC_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)
#define EPNUM_HID   0x81

// Generate HID Report Map for Keyboard, Mouse, and Consumer Media
uint8_t const desc_hid_report[] = {
  TUD_HID_REPORT_DESC_KEYBOARD( HID_REPORT_ID(1) ),
  TUD_HID_REPORT_DESC_MOUSE   ( HID_REPORT_ID(2) ),
  TUD_HID_REPORT_DESC_CONSUMER( HID_REPORT_ID(3) )
};

// Device Descriptor
tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = USBD_VID,
    .idProduct          = USBD_PID,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x00,
    .bNumConfigurations = 0x01
};

// Configuration Descriptor
uint8_t const desc_configuration[] = {
  TUD_CONFIG_DESCRIPTOR(1, 1, 0, USBD_DESC_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
  TUD_HID_DESCRIPTOR(0, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 10)
};

// Callbacks required by TinyUSB to read the configurations
uint8_t const * tud_descriptor_device_cb(void) { return (uint8_t const *) &desc_device; }
uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance) { return desc_hid_report; }
uint8_t const * tud_descriptor_configuration_cb(uint8_t index) { return desc_configuration; }

// Dummy strings to satisfy descriptor
char const* string_desc_arr [] = {
  (const char[]) { 0x09, 0x04 }, // 0: is supported language is English (0x0409)
  "DIY",                         // 1: Manufacturer
  "Custom Macropad",             // 2: Product
};
static uint16_t _desc_str[32];
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  uint8_t chr_count = strlen(string_desc_arr[index]);
  _desc_str[0] = (TUSB_DESC_STRING << 8 ) | (2*chr_count + 2);
  for(uint8_t i=0; i<chr_count; i++) _desc_str[1+i] = string_desc_arr[index][i];
  return _desc_str;
}