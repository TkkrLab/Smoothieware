/*
      This file is part of Smoothie (http://smoothieware.org/). The motion control part is heavily based on Grbl (https://github.com/simen/grbl).
      Smoothie is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
      Smoothie is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
      You should have received a copy of the GNU General Public License along with Smoothie. If not, see <http://www.gnu.org/licenses/>.
*/
#include "tkkrlab.h"

#include "Kernel.h"
#include "checksumm.h"
#include "libs/Config.h"
#include "rrdglcd/RrdGlcd.h"
#include "ConfigValue.h"
#include "PanelScreen.h"
#include "SerialMessage.h"
#include "StreamOutputPool.h"
#include "pitches.h"

// config settings
#define panel_checksum             CHECKSUM("panel")

#define up_button_pin_checksum     CHECKSUM("up_button_pin")
#define down_button_pin_checksum   CHECKSUM("down_button_pin")
#define click_button_pin_checksum  CHECKSUM("click_button_pin")
#define back_button_pin_checksum   CHECKSUM("back_button_pin")

#define test_button_pin_checksum   CHECKSUM("test_button_pin")
#define blue_button_pin_checksum   CHECKSUM("blue_button_pin")
#define yellow_button_pin_checksum CHECKSUM("yellow_button_pin")

#define led_pin_checksum           CHECKSUM("green_led_pin")
#define buzz_pin_checksum          CHECKSUM("buzz_pin")

#define spi_channel_checksum       CHECKSUM("spi_channel")
#define spi_cs_pin_checksum        CHECKSUM("spi_cs_pin")
#define spi_frequency_checksum     CHECKSUM("spi_frequency")

tkkrlabPanel::tkkrlabPanel() {
    // configure the pins to use
    this->up_pin.from_string(THEKERNEL->config->value( panel_checksum, up_button_pin_checksum)->by_default("nc")->as_string())->as_input();
    this->down_pin.from_string(THEKERNEL->config->value( panel_checksum, down_button_pin_checksum)->by_default("nc")->as_string())->as_input();
    this->click_pin.from_string(THEKERNEL->config->value( panel_checksum, click_button_pin_checksum )->by_default("nc")->as_string())->as_input();
    this->back_pin.from_string(THEKERNEL->config->value( panel_checksum, back_button_pin_checksum)->by_default("nc")->as_string())->as_input();

    this->test_pin.from_string(THEKERNEL->config->value( panel_checksum, test_button_pin_checksum)->by_default("nc")->as_string())->as_input();
    this->blue_pin.from_string(THEKERNEL->config->value( panel_checksum, blue_button_pin_checksum)->by_default("nc")->as_string())->as_input();
    this->yellow_pin.from_string(THEKERNEL->config->value( panel_checksum, yellow_button_pin_checksum)->by_default("nc")->as_string())->as_input();

    this->led_pin.from_string(THEKERNEL->config->value( panel_checksum, led_pin_checksum)->by_default("nc")->as_string())->as_output();
    this->led_pin.as_output();
    this->led_pin.set(true);
    this->buzz_pin.from_string(THEKERNEL->config->value( panel_checksum, buzz_pin_checksum)->by_default("nc")->as_string())->as_output();

    this->spi_cs_pin.from_string(THEKERNEL->config->value( panel_checksum, spi_cs_pin_checksum)->by_default("nc")->as_string())->as_output();

    // select which SPI channel to use
    int spi_channel = THEKERNEL->config->value(panel_checksum, spi_channel_checksum)->by_default(0)->as_number();
    this->glcd= new RrdGlcd(spi_channel, this->spi_cs_pin);

    int spi_frequency = THEKERNEL->config->value(panel_checksum, spi_frequency_checksum)->by_default(1000000)->as_number();
    this->glcd->setFrequency(spi_frequency);
}

tkkrlabPanel::~tkkrlabPanel() {
    delete this->glcd;
}

int tkkrlabPanel::readEncoderDelta() {
  return 0;
}

/*uint8_t tkkrlabPanel::sendCommand(char* cmd) {
  THEKERNEL->call_event(ON_CONSOLE_LINE_RECEIVED, cmd);
}*/

bool badapple = false;
uint32_t bapos = 0;
bool yellowbuttonstate = false;
bool yellowbuttontriggered = false;

void tkkrlabPanel::testlaser() {
    struct SerialMessage message;
    //message.message = "G0 X100 Y100";
    message.message = "L 1000";
    message.stream = &(StreamOutput::NullStream);
    THEKERNEL->call_event(ON_CONSOLE_LINE_RECEIVED, &message );
}

uint8_t tkkrlabPanel::readButtons() {
    uint8_t state= 0;
    //setCursor(0, 6);
    state |= (this->click_pin.get() ? BUTTON_SELECT : 0);
    if(this->down_pin.connected() && this->down_pin.get()) {this->led_pin.set(0); state |= BUTTON_DOWN; }
    if(this->up_pin.connected() && this->up_pin.get()) { this->led_pin.set(1); state |= BUTTON_UP; }
    if(this->back_pin.connected() && this->back_pin.get()) state |= BUTTON_LEFT;
    if(this->blue_pin.connected() && this->blue_pin.get()) {THEKERNEL->call_event(ON_HALT, nullptr); bapos = 0; badapple = true;}//state |= BUTTON_PAUSE;
    if(this->test_pin.connected() && this->test_pin.get()) {testlaser();} //this->sendCommand((char *)"S0.1"); this->sendCommand((char *)"T0"); }
    //if(this->yellow_pin.connected() && this->yellow_pin.get()) { if (!yellowbuttonstate) {yellowbuttontriggered = true; } yellowbuttonstate = true; } else { yellowbuttonstate = false; }
    return state;
}

// cycle the buzzer pin at a certain frequency (hz) for a certain duration (ms)
void tkkrlabPanel::buzz(long duration, uint16_t freq) {
    if(!this->buzz_pin.connected()) return;

    duration *=1000; //convert from ms to us
    long period = 1000000 / freq; // period in us
    long elapsed_time = 0;
    while (elapsed_time < duration) {
        this->buzz_pin.set(1);
        wait_us(period / 2);
        this->buzz_pin.set(0);
        wait_us(period / 2);
        elapsed_time += (period);
    }
}

bool tkkrlabPanel::getunkillbutton() {
  return yellowbuttonstate;
}

bool tkkrlabPanel::getunkillbuttontriggered() {
  return yellowbuttontriggered;
}

void tkkrlabPanel::write(const char* line, int len) {
    this->glcd->displayString(this->row, this->col, line, len);
    this->col+=len;
}

void tkkrlabPanel::home(){
    this->col= 0;
    this->row= 0;
}

void tkkrlabPanel::clear(){
    this->glcd->clearScreen();
    this->col= 0;
    this->row= 0;
}

void tkkrlabPanel::display() {
    // it is always on
}

void tkkrlabPanel::setCursor(uint8_t col, uint8_t row){
    this->col= col;
    this->row= row;
}

void tkkrlabPanel::init(){
    this->glcd->initDisplay();
}

// displays a selectable rectangle from the glyph
void tkkrlabPanel::bltGlyph(int x, int y, int w, int h, const uint8_t *glyph, int span, int x_offset, int y_offset) {
    if(x_offset == 0 && y_offset == 0 && span == 0) {
        // blt the whole thing
        this->glcd->renderGlyph(x, y, glyph, w, h);

    }else{
        // copy portion of glyph into g where x_offset is left byte aligned
        // Note currently the x_offset must be byte aligned
        int n= w/8; // bytes per line to copy
        if(w%8 != 0) n++; // round up to next byte
        uint8_t g[n*h];
        uint8_t *dst= g;
        const uint8_t *src= &glyph[y_offset*span + x_offset/8];
        for (int i = 0; i < h; ++i) {
            memcpy(dst, src, n);
            dst+=n;
            src+= span;
        }
        this->glcd->renderGlyph(x, y, g, w, h);
    }
}

#define banotes 330
static const uint16_t badapplenotes[] {
NOTE_DS4, 214, 
NOTE_F4, 213, 
NOTE_FS4, 213, 
NOTE_GS4, 214, 
NOTE_AS4, 448, 
NOTE_DS5, 213, 
NOTE_CS5, 214, 
NOTE_AS4, 428, 
NOTE_DS4, 446, 
NOTE_AS4, 213, 
NOTE_GS4, 214, 
NOTE_FS4, 213, 
NOTE_F4, 235, 
NOTE_DS4, 213, 
NOTE_F4, 213, 
NOTE_FS4, 214, 
NOTE_GS4, 213, 
NOTE_AS4, 448, 
NOTE_GS4, 214, 
NOTE_FS4, 213, 
NOTE_F4, 213, 
NOTE_DS4, 234, 
NOTE_F4, 214, 
NOTE_FS4, 213, 
NOTE_F4, 214, 
NOTE_DS4, 213, 
NOTE_D4, 214, 
NOTE_F4, 233, 
NOTE_DS4, 214, 
NOTE_F4, 214, 
NOTE_FS4, 213, 
NOTE_GS4, 213, 
NOTE_AS4, 342, 
0, 106, 
NOTE_DS5, 214, 
NOTE_CS5, 213, 
NOTE_AS4, 447, 
NOTE_DS4, 428, 
NOTE_AS4, 214, 
NOTE_GS4, 213, 
NOTE_FS4, 233, 
NOTE_F4, 214, 
NOTE_DS4, 214, 
NOTE_F4, 213, 
NOTE_FS4, 213, 
NOTE_GS4, 214, 
NOTE_AS4, 448, 
NOTE_GS4, 213, 
NOTE_FS4, 213, 
NOTE_F4, 448, 
NOTE_FS4, 427, 
NOTE_GS4, 428, 
NOTE_AS4, 446, 
NOTE_DS4, 213, 
NOTE_F4, 214, 
NOTE_FS4, 214, 
NOTE_GS4, 234, 
NOTE_AS4, 427, 
NOTE_DS5, 213, 
NOTE_CS5, 214, 
NOTE_AS4, 447, 
NOTE_DS4, 428, 
NOTE_AS4, 213, 
NOTE_GS4, 213, 
NOTE_FS4, 234, 
NOTE_F4, 214, 
NOTE_DS4, 213, 
NOTE_F4, 214, 
NOTE_FS4, 213, 
NOTE_GS4, 234, 
NOTE_AS4, 428, 
NOTE_GS4, 213, 
NOTE_FS4, 214, 
NOTE_F4, 213, 
NOTE_DS4, 234, 
NOTE_F4, 213, 
NOTE_FS4, 214, 
NOTE_F4, 214, 
NOTE_DS4, 213, 
NOTE_D4, 233, 
NOTE_F4, 214, 
NOTE_DS4, 214, 
NOTE_F4, 213, 
NOTE_FS4, 214, 
NOTE_GS4, 233, 
NOTE_AS4, 428, 
NOTE_DS5, 213, 
NOTE_CS5, 214, 
NOTE_AS4, 447, 
NOTE_DS4, 428, 
NOTE_AS4, 213, 
NOTE_GS4, 213, 
NOTE_FS4, 234, 
NOTE_F4, 214, 
NOTE_DS4, 213, 
NOTE_F4, 213, 
NOTE_FS4, 214, 
NOTE_GS4, 234, 
NOTE_AS4, 427, 
NOTE_GS4, 213, 
NOTE_FS4, 214, 
NOTE_F4, 448, 
NOTE_FS4, 426, 
NOTE_GS4, 448, 
NOTE_AS4, 427, 
NOTE_CS5, 213, 
NOTE_DS5, 214, 
NOTE_AS4, 213, 
NOTE_GS4, 234, 
NOTE_AS4, 427, 
NOTE_GS4, 214, 
NOTE_AS4, 214, 
NOTE_CS5, 233, 
NOTE_DS5, 214, 
NOTE_AS4, 213, 
NOTE_GS4, 214, 
NOTE_AS4, 448, 
NOTE_GS4, 213, 
NOTE_AS4, 214, 
NOTE_GS4, 213, 
NOTE_FS4, 214, 
NOTE_F4, 233, 
NOTE_CS4, 214, 
NOTE_DS4, 427, 
NOTE_CS4, 213, 
NOTE_DS4, 214, 
NOTE_F4, 233, 
NOTE_FS4, 214, 
NOTE_GS4, 214, 
NOTE_AS4, 213, 
NOTE_DS4, 214, 
0, 234, 
NOTE_AS4, 214, 
NOTE_CS5, 106, 
0, 107, 
NOTE_CS5, 213, 
NOTE_DS5, 213, 
NOTE_AS4, 235, 
NOTE_GS4, 213, 
NOTE_AS4, 427, 
NOTE_GS4, 214, 
NOTE_AS4, 233, 
NOTE_CS5, 214, 
NOTE_DS5, 213, 
NOTE_AS4, 214, 
NOTE_GS4, 213, 
NOTE_AS4, 448, 
NOTE_GS4, 214, 
NOTE_AS4, 213, 
NOTE_GS4, 214, 
NOTE_FS4, 213, 
NOTE_F4, 234, 
NOTE_CS4, 214, 
NOTE_DS4, 426, 
NOTE_CS4, 214, 
NOTE_DS4, 234, 
NOTE_F4, 214, 
NOTE_FS4, 213, 
NOTE_GS4, 214, 
NOTE_AS4, 212, 
NOTE_DS4, 235, 
0, 213, 
NOTE_AS4, 214, 
NOTE_CS5, 107, 
0, 106, 
NOTE_CS5, 214, 
NOTE_DS5, 213, 
NOTE_AS4, 234, 
NOTE_GS4, 214, 
NOTE_AS4, 427, 
NOTE_GS4, 213, 
NOTE_AS4, 234, 
NOTE_CS5, 214, 
NOTE_DS5, 214, 
NOTE_AS4, 213, 
NOTE_GS4, 214, 
NOTE_AS4, 447, 
NOTE_GS4, 213, 
NOTE_AS4, 214, 
NOTE_GS4, 213, 
NOTE_FS4, 234, 
NOTE_F4, 214, 
NOTE_CS4, 213, 
NOTE_DS4, 428, 
NOTE_CS4, 213, 
NOTE_DS4, 233, 
NOTE_F4, 214, 
NOTE_FS4, 214, 
NOTE_GS4, 213, 
NOTE_AS4, 214, 
NOTE_DS4, 447, 
NOTE_AS4, 214, 
NOTE_CS5, 106, 
0, 107, 
NOTE_CS5, 214, 
NOTE_DS5, 233, 
NOTE_AS4, 214, 
NOTE_GS4, 213, 
NOTE_AS4, 428, 
NOTE_GS4, 234, 
NOTE_AS4, 213, 
NOTE_CS5, 214, 
NOTE_DS5, 213, 
NOTE_AS4, 214, 
NOTE_GS4, 213, 
NOTE_AS4, 448, 
NOTE_CS5, 213, 
NOTE_DS5, 213, 
NOTE_FS5, 214, 
NOTE_F5, 234, 
NOTE_DS5, 214, 
NOTE_CS5, 213, 
NOTE_AS4, 427, 
NOTE_GS4, 234, 
NOTE_AS4, 214, 
NOTE_GS4, 213, 
NOTE_FS4, 214, 
NOTE_F4, 213, 
NOTE_CS4, 234, 
NOTE_DS4, 427, 
0, 427, 
NOTE_CS5, 214, 
NOTE_DS5, 233, 
NOTE_AS4, 214, 
NOTE_GS4, 213, 
NOTE_AS4, 427, 
NOTE_GS4, 235, 
NOTE_AS4, 213, 
NOTE_CS5, 214, 
NOTE_DS5, 213, 
NOTE_AS4, 214, 
NOTE_GS4, 233, 
NOTE_AS4, 427, 
NOTE_GS4, 214, 
NOTE_AS4, 213, 
NOTE_GS4, 214, 
NOTE_FS4, 234, 
NOTE_F4, 213, 
NOTE_CS4, 214, 
NOTE_DS4, 427, 
NOTE_CS4, 234, 
NOTE_DS4, 213, 
NOTE_F4, 214, 
NOTE_FS4, 213, 
NOTE_GS4, 214, 
NOTE_AS4, 234, 
NOTE_DS4, 427, 
NOTE_AS4, 213, 
NOTE_CS5, 213, 
NOTE_CS5, 235, 
NOTE_DS5, 213, 
NOTE_AS4, 214, 
NOTE_GS4, 213, 
NOTE_AS4, 427, 
NOTE_GS4, 234, 
NOTE_AS4, 214, 
NOTE_CS5, 214, 
NOTE_DS5, 213, 
NOTE_AS4, 213, 
NOTE_GS4, 234, 
NOTE_AS4, 427, 
NOTE_GS4, 213, 
NOTE_AS4, 214, 
NOTE_GS4, 234, 
NOTE_FS4, 213, 
NOTE_F4, 214, 
NOTE_CS4, 214, 
NOTE_DS4, 447, 
NOTE_CS4, 214, 
NOTE_DS4, 213, 
NOTE_F4, 213, 
NOTE_FS4, 214, 
NOTE_GS4, 213, 
NOTE_AS4, 235, 
NOTE_DS4, 426, 
NOTE_AS4, 213, 
NOTE_CS5, 214, 
NOTE_CS5, 234, 
NOTE_DS5, 214, 
NOTE_AS4, 214, 
NOTE_GS4, 213, 
NOTE_AS4, 447, 
NOTE_GS4, 214, 
NOTE_AS4, 214, 
NOTE_CS5, 213, 
NOTE_DS5, 213, 
NOTE_AS4, 234, 
NOTE_GS4, 214, 
NOTE_AS4, 427, 
NOTE_GS4, 214, 
NOTE_AS4, 213, 
NOTE_GS4, 234, 
NOTE_FS4, 213, 
NOTE_F4, 214, 
NOTE_CS4, 213, 
NOTE_DS4, 447, 
NOTE_CS4, 214, 
NOTE_DS4, 214, 
NOTE_F4, 213, 
NOTE_FS4, 214, 
NOTE_GS4, 234, 
NOTE_AS4, 213, 
NOTE_DS4, 428, 
NOTE_AS4, 213, 
NOTE_CS5, 234, 
NOTE_CS5, 213, 
NOTE_DS5, 213, 
NOTE_AS4, 214, 
NOTE_GS4, 214, 
NOTE_AS4, 447, 
NOTE_GS4, 214, 
NOTE_AS4, 213, 
NOTE_CS5, 214, 
NOTE_DS5, 213, 
NOTE_AS4, 234, 
NOTE_GS4, 213, 
NOTE_AS4, 428, 
NOTE_CS5, 213, 
NOTE_DS5, 234, 
NOTE_FS5, 214, 
NOTE_F5, 213, 
NOTE_DS5, 214, 
NOTE_CS5, 213, 
NOTE_AS4, 448, 
NOTE_GS4, 213, 
NOTE_AS4, 214, 
NOTE_GS4, 213, 
NOTE_FS4, 213, 
NOTE_F4, 235, 
NOTE_CS4, 213, 
NOTE_DS4, 874
};

void tkkrlabPanel::on_refresh(bool now){
    static int refresh_counts = 0;
    refresh_counts++;
    // 10Hz refresh rate
    if (!badapple) {
      if(now || refresh_counts % 2 == 0 ) this->glcd->refresh();
    } else {
      this->glcd->clearScreen();
      char buffer[128] = {0};
      sprintf(buffer, "~(^_^')~");
      this->glcd->displayString(0,0,buffer,strlen(buffer));
      THEKERNEL->call_event(ON_HALT, nullptr);
      //sprintf(buffer, "%lu", bapos);
      //this->glcd->displayString(0,20,buffer,strlen(buffer));
      this->glcd->refresh();
      if (bapos>=banotes) { bapos = 0; badapple = false; return;}
      if( badapplenotes[bapos*2]> 0) {
        this->buzz(badapplenotes[bapos*2+1], badapplenotes[bapos*2]); //Note
      } else {
        wait_us(badapplenotes[bapos*2+1]); //Silence
      }
      bapos++;
    }
    
}
