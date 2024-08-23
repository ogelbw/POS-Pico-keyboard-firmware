#include <Arduino.h>
#include <stdint.h>
#include <stdio.h>
#include <vector>

#include "Adafruit_TinyUSB.h"

using std::vector;

// put macro definitions here: -------------------------------------------------
#define POLLING_INTERVAL_MS 2


// put variable declarations here: ---------------------------------------------
/** The pins connected to each column of the key matrix. Left to right when
 * looking at the keyboard face. */
vector<PinName> colPins{p10, p9, p8, p7, p6, p5, p16, p17, p18, p19, p20, p21,
                        p22, p27, p28};

/** The pins connected to each row of the key matrix. From top to bottom when
 * looking at the keyboard face. */
vector<PinName> rowPins{p11, p12, p13, p14, p15};

// HID report descriptor using TinyUSB's template
// Single Report (no ID) descriptor
uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

// USB HID object.
Adafruit_USBD_HID usb_hid;


// put function declarations here: ---------------------------------------------
void scanKeys();


// MAIN CODE -------------------------------------------------------------------
void setup() {
  /** Starting tinyUSB */
  if (!TinyUSBDevice.isInitialized()) {
    TinyUSBDevice.begin(0);
  }

  /** Setting up HID */
  usb_hid.setBootProtocol(HID_ITF_PROTOCOL_KEYBOARD);
  usb_hid.setPollInterval(POLLING_INTERVAL_MS);
  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
  usb_hid.setStringDescriptor("TinyUSB Keyboard");

  // Set up output report (on control endpoint) for Capslock indicator
  usb_hid.setReportCallback(NULL, hid_report_callback);
  usb_hid.begin();

  /** Putting all the column pins to output so we can drive their state */
  for (int i = 0; i < colPins.size(); i++) {
    pinMode(colPins[i], OUTPUT);
  }

  /** Putting all the row pins to input pull up so that they are naturally high 
   *  and we can detect when they are pulled low by a key press */
  for (int i = 0; i < rowPins.size(); i++) {
    pinMode(rowPins[i], INPUT_PULLUP);
  }
}

void loop() {
  /** TinyUsb needs to be called if it is not done by the core background.
   * RP2040 seems like it can do this but won't since ARDUINO_ARCH_MBED is
   * defined. */
  #ifdef TINYUSB_NEED_POLLING_TASK
  TinyUSBDevice.task();
  #endif

  /* Delaying to match interval rate */
  static uint32_t last_poll_time = 0;
  if (millis() - last_poll_time > POLLING_INTERVAL_MS) {
    last_poll_time = millis();
  }

  /* If the device is not mounted simply stop running */
  if (!TinyUSBDevice.mounted()) { return; }
  scanKeys();
}


// put function definitions here: ----------------------------------------------
void scanKeys() {
  // TODO
}

// Output report callback for LED indicator such as Caplocks
void hid_report_callback(uint8_t report_id,
                        hid_report_type_t report_type,
                        uint8_t const* buffer,
                        uint16_t bufsize) {
  (void) report_id;
  (void) bufsize;

  // LED indicator is output report with only 1 byte length
  if (report_type != HID_REPORT_TYPE_OUTPUT) return;

  // The LED bit map is as follows: (also defined by KEYBOARD_LED_* )
  // Kana (4) | Compose (3) | ScrollLock (2) | CapsLock (1) | Numlock (0)
  uint8_t ledIndicator = buffer[0];

  // TODO: Solder a caps led so this is not useless and I can tell when caps is 
  // on
}