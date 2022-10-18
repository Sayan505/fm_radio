#include <Wire.h>
#include <SPI.h>

// Display
#include <Adafruit_GFX.h>        // https://github.com/adafruit/Adafruit-GFX-Library
#include <Adafruit_PCD8544.h>    // https://github.com/adafruit/Adafruit-PCD8544-Nokia-5110-LCD-library

// RC
#include <TEA5767N.h>           // https://github.com/mroger/TEA5767

// Rotary Encoder
#include <Encoder.h>            // https://github.com/PaulStoffregen/Encoder


// display: Nokia 5110 (PCD8544)
// Gnd: GND    BL: 5V    Vcc: 3v3    CLK: D12    DIN: D11    DC: D10    CE/CS: D9    RST: D8
Adafruit_PCD8544 display = Adafruit_PCD8544(12, 11, 10, 9, 8);

void init_display() {
    display.begin();

    display.clearDisplay();
    display.display();

    display.setContrast(127);
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.display();
}


// RC: TEA5767
// +5V: 5V    SDA: A4    SLC: A5    GND: GND
TEA5767N radio = TEA5767N();
float freq = 100.1f;  // initial freq.

void init_rc() {
    radio.setStereoReception();
    radio.setStereoNoiseCancellingOn();

    // set initial freq.
    radio.selectFrequency(freq);
}



// RE: M274 Rotary Encoder
// GND: GND    +: 5V    SW: N/A    DT: D3    CLK: D2
Encoder encoder(2, 3);
long old_posi = encoder.read();

unsigned long curr_ts, prev_ts = 0, retune_interval = 1000;
float prev_freq = freq;


// UI
void draw_ui(bool retuned = true) {
    // craft info string
    String str_retuned = "FM\n" + String(freq)+ "\nMHz";
    String str_retuning = "FM\n" + String(freq)+ "\nMHz ...";

    // render
    display.clearDisplay();
    display.write(retuned ? str_retuned.c_str() : str_retuning.c_str());
    display.display();
}




void setup() {
    init_display();
    init_rc();

    draw_ui();
}


// BAND II: 87.5 MHz - 108.0 MHz FM
void loop() {
    // query rotary encoder position
    long long int new_posi = encoder.read();

    // store current ts
    curr_ts = millis();

    // calc. next freq.
    if (new_posi / 4 > old_posi) {
        
        old_posi = new_posi / 4;

        freq += 0.1f;   // next

        draw_ui(false);

        prev_ts = curr_ts;// store last changed ts
    }
    else if (new_posi / 4 < old_posi) {
        old_posi = new_posi / 4;

        freq -= 0.1f;   // prev.

        draw_ui(false);
        prev_ts = curr_ts; // store last changed ts
    }

    // retune handler
    //curr_ts = millis();
    if(curr_ts - prev_ts > retune_interval) {
        if(freq != prev_freq) {
            prev_freq = freq;
            radio.selectFrequency(freq);
            draw_ui();
        }
        //prev_ts = curr_ts;
    }
}
