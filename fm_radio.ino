//  Nokia 5110 Display (PCD8544): https://github.com/adafruit/Adafruit-PCD8544-Nokia-5110-LCD-library
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

// TEA5767 Radio: https://github.com/mroger/TEA5767
#include <TEA5767N.h>

// M274 Rotary Encoder: https://github.com/PaulStoffregen/Encoder
#include <Encoder.h>


// init display
// Gnd: GND    BL: 5V    Vcc: 5V    CLK: D13    DIN: D11    DC: D10    CE: D9    RST: D8
Adafruit_PCD8544 display = Adafruit_PCD8544(13, 11, 10, 9, 8);

void init_display() {
  display.begin();
  display.clearDisplay();
  display.display();

  display.setContrast(50);
  display.setCursor(0, 0);
  display.setTextSize(2);
}


// init radio
// +5V: 5V    SDA: A4    SLC: A5    GND: GND
// 87.5 MHz - 108.0 MHz FM
TEA5767N radio = TEA5767N();

float curr_freq = 100.1f;
float lfreq     = 87.5f;
float ufreq     = 108.f;


void init_rc() {
  radio.setStereoReception();
  radio.setStereoNoiseCancellingOn();

  radio.selectFrequency(curr_freq);
}

float delta_freq(float delta) {
  float res = curr_freq + delta;

  if(res > ufreq)      res = ufreq;
  else if(res < lfreq) res = lfreq;

  curr_freq = res;
}


// init rotary encoder (ref: https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/)
// GND: GND    +: 5V    SW: N/A    DT: D3    CLK: D2
Encoder encoder(2, 3);  // both pins support interrupt for best performance

int   enc_old_posi = encoder.read();
float prev_freq    = curr_freq;
unsigned long last_tuned_ts      = 0,
              retune_interval_ms = 1000; // cooldown before retuning due to the radio chip being slow


void redraw_ui() {
  String info = "FM\n" + String(curr_freq) + "\nMHz";

  // if not tuned to the selected freq yet (undergoing retuning)
  if(prev_freq != curr_freq) {
    info += " ...";
  }
  
  display.clearDisplay();
  display.print(info);
  display.display();
}


void setup() {
  init_display();

  display.print("TUNING\n...");
  display.display();

  init_rc();

  redraw_ui();
}

void loop() {
  // poll encoder position
  int enc_new_posi          = encoder.read();
  int enc_old_posi_filtered = enc_new_posi / 4;

  // get current timestamp
  unsigned long curr_ts = millis();

  unsigned long rapid_tune_thresh_ms = 120;

  // calc next freq
  if(enc_old_posi_filtered > enc_old_posi) {
    enc_old_posi = enc_old_posi_filtered;

    // inc current freq
    if(curr_ts - last_tuned_ts > rapid_tune_thresh_ms) {
      delta_freq(+0.1f);  // inc by 0.1 MHz if rotating the encoder slowly
    } else {
      delta_freq(+1.0f);  // inc by 1 MHz if rotating the encoder rapidly
    }

    last_tuned_ts = curr_ts;  // update last tuned timestamp to now

    redraw_ui();
  } else if(enc_old_posi_filtered < enc_old_posi) {
    enc_old_posi = enc_old_posi_filtered;

    // dec current freq
    if(curr_ts - last_tuned_ts > rapid_tune_thresh_ms) {
      delta_freq(-0.1f);  // dec by 0.1 MHz if rotating the encoder slowly
    } else {
      delta_freq(-1.0f);  // dec by 1 MHz if rotating the encoder rapidly
    }

    last_tuned_ts = curr_ts;  // update last tuned timestamp to now

    redraw_ui();
  }

  // retune only if the encoder position was changed more than retune_interval_ms time ago
  if(curr_ts - last_tuned_ts > retune_interval_ms) {
    // retune only if the target freq is different than the current freq
    if(curr_freq != prev_freq) {
      prev_freq = curr_freq;
      radio.selectFrequency(curr_freq);
      redraw_ui();
    }
  }
}
