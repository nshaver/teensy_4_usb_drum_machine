#define SYNTH_DEBUG 0

// touchscreen
// images used on screen
#include "home.c"

#define USE_TOUCHSCREEN 1
#ifdef USE_TOUCHSCREEN
#include <ILI9341_t3.h>
#include <font_Arial.h> // from ILI9341_t3
#include <font_ArialBold.h> // from ILI9341_t3
#include <XPT2046_Touchscreen.h>
#include <SD.h>
#include <SPI.h>

bool sdcard_enabled=true;
File myFile;
int file_number=0;
int song_number=0;
String load_filename="";

//#define SD_CS 10
#define TIRQ_PIN  8 //2
#define CS_PIN  7 // Touch

// For optimized ILI9341_t3 library
#define TFT_DC      10
#define TFT_CS      5
#define TFT_RST    	6  // 255 = unused, connect to 3.3V
#define TFT_MOSI    11
#define TFT_SCLK    13
#define TFT_MISO    12

//XPT2046_Touchscreen ts(CS_PIN);  // Param 2 - NULL - No interrupts
//XPT2046_Touchscreen ts(CS_PIN, 255);  // Param 2 - 255 - No interrupts
//ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);
//XPT2046_Touchscreen ts(CS_PIN, TIRQ_PIN);  // Param 2 - Touch IRQ Pin - interrupt enabled polling
//ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO);
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(CS_PIN);
#endif

// encoder
long tempo=60;
#include "QuadEncoder.h"
#define ENC_PIN_1 30
#define ENC_PIN_2 31
#define ENC_PIN_SW 32
QuadEncoder enc(1, ENC_PIN_1, ENC_PIN_2);
long encoderVal=-999;
bool encoderButton=false;
unsigned long next_encoder_button_read=0;
unsigned long encoderButtonHoldStart=0;
unsigned long encoder_ms=0;
unsigned long tap_tempo_start_ms=0;

#include "USBHost_t36.h"
#include <Audio.h>

//#include "TR808V1_samples.h"
#include "drums_samples.h"

USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
USBHub hub3(myusb);
KeyboardController keyboard1(myusb);
KeyboardController keyboard2(myusb);
// accept input from two USB midi devices connected to a hub:
MIDIDevice midi1(myusb);
MIDIDevice midi2(myusb);

int midi_channel=254; // 254 = off, 0 = omni, >0 = channel

unsigned long t=0;
int thisvel;

AudioOutputI2S2 i2s2;
AudioSynthWavetable wavetable[16];
AudioMixer4 wavemixer1;
AudioMixer4 wavemixer2;
AudioMixer4 wavemixer3;
AudioMixer4 wavemixer4;
AudioMixer4 wavemixer5;
AudioMixer4 fxmixer1;
AudioMixer4 fxmixer2;
AudioMixer4 fxmixer3;
AudioMixer4 fxmixer4;
AudioMixer4 fxmixer5;
AudioMixer4 mastermixer;
AudioConnection wavePatchCord[] = {
	{wavetable[0], 0, wavemixer1, 0},
	{wavetable[1], 0, wavemixer1, 1},
	{wavetable[2], 0, wavemixer1, 2},
	{wavetable[3], 0, wavemixer1, 3},
	{wavetable[4], 0, wavemixer2, 0},
	{wavetable[5], 0, wavemixer2, 1},
	{wavetable[6], 0, wavemixer2, 2},
	{wavetable[7], 0, wavemixer2, 3},
	{wavetable[8], 0, wavemixer3, 0},
	{wavetable[9], 0, wavemixer3, 1},
	{wavetable[10], 0, wavemixer3, 2},
	{wavetable[11], 0, wavemixer3, 3},
	{wavetable[12], 0, wavemixer4, 0},
	{wavetable[13], 0, wavemixer4, 1},
	{wavetable[14], 0, wavemixer4, 2},
	{wavetable[15], 0, wavemixer4, 3}
};
AudioEffectFreeverb reverb1;
AudioConnection fxPatchCord[] = {
	{wavetable[0], 0, fxmixer1, 0},
	{wavetable[1], 0, fxmixer1, 1},
	{wavetable[2], 0, fxmixer1, 2},
	{wavetable[3], 0, fxmixer1, 3},
	{wavetable[4], 0, fxmixer2, 0},
	{wavetable[5], 0, fxmixer2, 1},
	{wavetable[6], 0, fxmixer2, 2},
	{wavetable[7], 0, fxmixer2, 3},
	{wavetable[8], 0, fxmixer3, 0},
	{wavetable[9], 0, fxmixer3, 1},
	{wavetable[10], 0, fxmixer3, 2},
	{wavetable[11], 0, fxmixer3, 3},
	{wavetable[12], 0, fxmixer4, 0},
	{wavetable[13], 0, fxmixer4, 1},
	{wavetable[14], 0, fxmixer4, 2},
	{wavetable[15], 0, fxmixer4, 3}
};
AudioAmplifier	amp1;
AudioAmplifier	amp2;
AudioConnection patchCord10(wavemixer1, 0, wavemixer5, 0);
AudioConnection patchCord11(wavemixer2, 0, wavemixer5, 1);
AudioConnection patchCord12(wavemixer3, 0, wavemixer5, 2);
AudioConnection patchCord13(wavemixer4, 0, wavemixer5, 3);
AudioConnection patchCord14(wavemixer5, 0, mastermixer, 0);

AudioConnection patchCord20(fxmixer1, 0, fxmixer5, 0);
AudioConnection patchCord21(fxmixer2, 0, fxmixer5, 1);
AudioConnection patchCord22(fxmixer3, 0, fxmixer5, 2);
AudioConnection patchCord23(fxmixer4, 0, fxmixer5, 3);
AudioConnection patchCord24(fxmixer5, 0, reverb1, 0);
AudioConnection patchCord25(reverb1, 0, mastermixer, 1);

AudioConnection patchCord91(mastermixer, amp1);
AudioConnection patchCord92(mastermixer, amp2);

AudioConnection patchCord101(amp1, 0, i2s2, 0);
AudioConnection patchCord102(amp2, 0, i2s2, 1);

/*
	 use multiple CQRobot mcp23017 boards with teensy 4.1
	 board #1 is i2c address 0x27, unsoldered, all pins used as inputs
	 board #2 is i2c address 0x26, soldered across A0, all pins used as outputs
	 board #2 is i2c address 0x25, soldered across A1, all pins used as outputs
 */

#include "Adafruit_MCP23017.h"

Adafruit_MCP23017 mcp1;
Adafruit_MCP23017 mcp2;
Adafruit_MCP23017 mcp3;
Adafruit_MCP23017 mcp4;

unsigned long loopcount=0;
unsigned long looptime_ms=0;
unsigned long thisloop_ms=0;
unsigned long nextblinkoff_ms=0;
unsigned long next_step_ms=0;
int next_step_no=0;
uint16_t redLedState=0b1111111111111111;
uint16_t grnLedState=0b1111111111111111;
uint16_t stepButtonState=0b1111111111111111;
uint16_t stepButtonStateLast=0b1111111111111111;
uint16_t mcp4InputState=0b11111111;
uint16_t quadButtonState=0b1111;
uint16_t quadLedState=0b1111;

unsigned long next_mcp_read=0;
unsigned long button_count=0;

int menuvar1=-1;
int menuvar2=-1;

bool playMode=false;

void handleButtons(){
	stepButtonState=mcp1.readGPIOAB();
	mcp4InputState=mcp4.readGPIO(0);
	for (int i=4;i<8;i++){
		bitWrite(quadButtonState, i-4, bitRead(mcp4InputState, i));
	}
}

int SOUND_MAP[16]={39,45,35,35,35,35,35,35,35,35,35,35,35,35,35,35};
int VOLUME_MAP[16]={127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127};
int FX_MAP[16]={40,40,40,40,40,40,40,40,40,40,40,40,40,40,40,40};

// SONG - up to 16 steps per song, first element=pattern number, second=times to repeat, 255=off
uint8_t SONG[16][2];
int current_song_pattern=0;
int current_song_repeats=0;

// PATTERN - 16 different patterns available
//					 each pattern is 16 tracks (one for each instrument)
//					 patterns can be up to 64 steps
//					 each step represents the velocity of the note played
//					 track #17 holds misc settings for the pattern, as follows:
//					 0 = swing pct (0-100)
//					 1 = pattern length (1-64 steps)
int pattern_page=0;
int step_record_track=-1;
int current_pattern=0;
int current_pattern_length=0;
int copy_paste_pattern=-1;
bool clear_confirm=false;
bool muting_tracks=false;
bool MUTED_TRACKS[16];
uint8_t PATTERNS[16][17][64];
/*
	 // before PATTERN
	 FLASH: code:102256, data:4325868, headers:8864   free for files:3689476
   RAM1: variables:70336, code:99064, padding:32008   free for local variables:322880
   RAM2: variables:18112  free for malloc/new:506176
*/
uint8_t UNDO_PATTERN[64];
uint8_t UNDO_PATTERN_2[64];

enum Menus {
	MENU_MAIN,
	MENU_MAP,
	MENU_MIX,
	MENU_TEMPO,
	MENU_PATTERN,
	MENU_PATTERN_SETUP,
	MENU_STEP_REC,
	MENU_STORAGE,
	MENU_MIDI,
	MENU_EG,
	MENU_FEG,
	MENU_SET_CC,
	MENU_PRG,
	MENU_SYS,
	MENU_REVERB,
	MENU_SETUP,
	MENU_SONG,
	MENU_LOAD_SONG,
	MENU_SAVE_SONG,
	MENU_BUILD_SONG
};
// do not create more than 254 menus
byte menu=255;
byte last_menu=255;

void debug_print(int level=0, String t=""){
#if SYNTH_DEBUG > 0
	if (level <= SYNTH_DEBUG){
		Serial.println(t);
	}
#endif
}

#ifdef USE_TOUCHSCREEN
boolean wastouched = true;
unsigned long next_touched_ms=0;
unsigned long next_touched_waittime=100;
long scaled_x;
long scaled_y;
long touch_x_max=3850;
long touch_x_min=428;
long touch_y_max=3730;
long touch_y_min=231;
// slider
int slider_x=10;
int slider_y=40;
int slider_w=300;
int slider_h=70;
int indicator_w=4; // indicator width
int indicator_m=30; // indicator margin


int nr=0;

void scale_touch(long this_x, long this_y){
	if (this_x>touch_x_max) this_x=touch_x_max;
	else if (this_x<touch_x_min) this_x=touch_x_min;
	if (this_y>touch_y_max) this_y=touch_y_max;
	else if (this_y<touch_y_min) this_y=touch_y_min;
	scaled_x=map(this_x, touch_x_max, touch_x_min, 0, 319);
	scaled_y=map(this_y, touch_y_max, touch_y_min, 0, 239);
}

void info_display(String t){
	tft.fillRect(0, 220, 320, 20, ILI9341_BLACK);
	tft.setFont(Arial_12);
	tft.setTextColor(ILI9341_WHITE);
	tft.setCursor(2, 220);
	tft.print(t);
}

void showtext(int line_number, String text, boolean clearDisplay){
	// if debugging is enabled then it makes sense to send this new text to the debug window
	debug_print(2, text);

	if (clearDisplay) {
  	tft.fillScreen(ILI9341_BLACK);
		tft.drawRect(0, 0, 128, 64, ILI9341_RED);

		tft.drawRect(0, 0, 128, 22, ILI9341_RED);
		tft.drawRect(0, 21,128, 22, ILI9341_RED);
		tft.drawRect(0, 42,128, 23, ILI9341_RED);
		//tft.drawRect(0, 42,128, 11, ILI9341_RED);
		//tft.drawRect(0, 52,128, 12, ILI9341_RED);
	}

	int tx=2, ty=0;
	int ry=0;
	int rh=21;
	if (line_number==1){
    tft.setFont(Arial_12);
		ty=15;
		ry=1;
		rh=40;
	} else if (line_number==2){
    tft.setFont(Arial_12);
		ty=57;
		ry=42;
		rh=40;
	} else if (line_number==3){
    tft.setFont(Arial_12);
		ty=91;
		ry=83;
		rh=29;
	} else if (line_number==4){
    tft.setFont(Arial_12);
		ty=121;
		ry=113;
		rh=30;
	} else {
		// just do two lines, but 3 would be y=52 if needed in the future
		return;
	}
	tft.fillRect(1, ry, 126, rh, ILI9341_RED);
	tft.setTextColor(ILI9341_WHITE);
	tft.setCursor(tx, ty);
	tft.print(text);
	//tft.display();
}

void menu_banner(String t){
	// header
	tft.drawRect(0, 0, 288, 32, ILI9341_YELLOW);
	tft.fillRect(1, 1, 287, 30, ILI9341_BLACK);
	tft.setFont(Arial_24_Bold);
	tft.setTextColor(ILI9341_RED);
	tft.setCursor(3, 3);
	tft.print(t);

	// static home button
	tft.drawRect(288, 0, 32, 32, ILI9341_YELLOW);
	tft.writeRect(289, 1, home.width, home.height, (uint16_t*)(home.pixel_data));
}

void menu_draw_slider(long this_value){
	// frame
	tft.drawRect(slider_x-1, slider_y-1, slider_w+2, slider_h+2, ILI9341_YELLOW);
	tft.fillRect(slider_x, slider_y, slider_w, slider_h, ILI9341_WHITE);
	// middle line
	tft.fillRect(slider_x, slider_y+(slider_h/2)-2, slider_w, 4, ILI9341_RED);
	// position indicator
	long this_pos=map(1.*this_value/127, 0., 1., slider_x+indicator_m, slider_x+slider_w-indicator_m);
	tft.fillRect(this_pos-(indicator_w/2), slider_y+2, indicator_w, slider_h-4, ILI9341_RED);
}

void menu_drawbutton(int b_row, int b_col, int t_size, String b_text_1, String b_text_2="", String b_text_3=""){
	int bsize=3;
	int bx=0;
	int by=0;
	int bw=106;
	int bh=80;
	int tx=0;
	int ty=0;
	long int bbgcol=ILI9341_YELLOW;
	long int bfgcol=0XBDF7;
	long int btcol=ILI9341_RED;

	if (b_row==1){
		by=34;
		ty=56;
	} else if (b_row==2){
		by=114;
		ty=136;
	}

	if (b_col==1){
		bx=1;
		tx=3;
	} else if (b_col==2){
		bx=107;
		tx=109;
	} else if (b_col==3){
		bx=213;
		tx=215;
	}

	if (bsize==3){
		tft.drawRect(bx, by, bw, bh, bbgcol);
		tft.fillRect(bx+1, by+1, bw-2, bh-2, bfgcol);
		tft.setTextColor(btcol);
		if (t_size==3){
			tft.setCursor(tx, ty);
			tft.setFont(Arial_24_Bold);
			tft.print(b_text_1);
		} else if (t_size==2){
			tft.setCursor(tx, ty-6);
			tft.setFont(Arial_18_Bold);
			tft.print(b_text_1);
			tft.setCursor(tx, ty+14);
			tft.print(b_text_2);
		} else {
			tft.setCursor(tx, ty-4);
			tft.setFont(Arial_12_Bold);
			tft.print(b_text_1);
			tft.setCursor(tx, ty+10);
			tft.print(b_text_2);
			tft.setCursor(tx, ty+24);
			tft.print(b_text_3);
		}
	}
}

int menu_get_row(int my){
	if (my>=34 && my<114) return 1;
	if (my>=114 && my<194) return 2;
	return 0;
}

int menu_get_col(int mx){
	if (mx>=1 && mx<107) return 1;
	if (mx>=107 && mx<213) return 2;
	if (mx>=213 && mx<319) return 3;
	return 0;
}

void menu_mix(int mx=-1, int my=-1, int update=-1){
	if (update==-1 && (mx==-1 || my==-1)){
		// create menu
		tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
		menu_banner("MIXER");

		tft.fillRect(0, 34, 320, 205, ILI9341_BLACK);

		// create grid
    tft.setFont(Arial_12);
		tft.setTextColor(ILI9341_WHITE);

		int th=46;
		int tw=80;
		for (int i=0;i<16;i++){
			int tx, ty;
			tx=i%4 * tw;
			ty=34 + (i/4) * th;

			tft.drawRect(tx, ty, tw, th, ILI9341_YELLOW);
			tft.setCursor(tx+3, ty+2);
			tft.print((String)"B" + (i+1));

			tft.setCursor(tx+3, ty+16);
			tft.print((String)"VOL=" + VOLUME_MAP[i]);
			tft.setCursor(tx+3, ty+30);
			tft.print((String)"FX=" + FX_MAP[i]);
		}

		return;
	}

	// process clicks
	if (mx>=288 && my<=32){
		goto_menu(MENU_SETUP);
		return;
	}

	// determine which button was clicked
	int th=46;
	int tw=80;
	int unhighlight_last_button=-1;

	for (int i=0;i<16;i++){
		if (menuvar1>=0 && menuvar1!=i) continue;
		int tx, ty;
		tx=i%4 * tw;
		ty=34 + (i/4) * th;

		int tx2=tx+tw;
		int ty2=ty+th;
		tft.setFont(Arial_12);

		if (menuvar1>=0 || (mx>=tx && mx<tx2 && my>=ty && my<ty2)){
			// this button was clicked
			if (menuvar1==i && mx>=0 && my>=0){
				// it was already clicked, unclick and unhighlight it
				menu_banner("MIXER");
				tft.setFont(Arial_12);
				tft.fillRect(tx+1, ty+1, tw-2, th-2, ILI9341_BLACK);
				tft.setTextColor(ILI9341_WHITE);
				menuvar1=-1;
				menuvar2=-1;
			} else {
				// highlight it
				if (mx==-1 || my==-1){
					// this is an update from encoder
				} else {
					// track just selected
					menu_banner("MIXER - VOLUME");
				}
				tft.setFont(Arial_12);
				tft.fillRect(tx+1, ty+1, tw-2, th-2, ILI9341_YELLOW);
				tft.setTextColor(ILI9341_BLACK);
				menuvar1=i;
			}
		} else {
			// not clicked, unhighlight it
			tft.fillRect(tx+1, ty+1, tw-2, th-2, ILI9341_BLACK);
			tft.setTextColor(ILI9341_WHITE);
		}
		tft.setCursor(tx+3, ty+2);
		tft.print((String)"B" + (i+1));

		tft.setCursor(tx+3, ty+16);
		tft.print((String)"VOL=" + VOLUME_MAP[i]);
		tft.setCursor(tx+3, ty+30);
		tft.print((String)"FX=" + FX_MAP[i]);
	}
}

void menu_step_rec(int mx=-1, int my=-1, int update=-1){
	// process clicks
	if (mx>=288 && my<=32){
		goto_menu(MENU_PATTERN);
		return;
	}

	if (update==-1 && (mx==-1 || my==-1)){
		// create menu
		menu_banner("STEP RECORD");
		tft.fillRect(0, 34, 320, 205, ILI9341_BLACK);
		menu_drawbutton(1, 1, 1, "SEL", "TRACK");
		menu_drawbutton(1, 2, 1, "ENTER", "STEPS");
		menu_drawbutton(1, 3, 1, "");
		menu_drawbutton(2, 1, 1, "GROOVIFY");
		if (PATTERNS[current_pattern][16][0]>0){
			menu_drawbutton(2, 2, 1, "SWING", (String)PATTERNS[current_pattern][16][0] + "%");
		} else {
			menu_drawbutton(2, 2, 1, "SWING", "OFF");
		}
		menu_drawbutton(2, 3, 1, "UNDO");

		tft.fillRect(0, 220, 320, 20, ILI9341_BLACK);
		return;
	}

	// process menu click
	int this_row=menu_get_row(my);
	int this_col=menu_get_col(mx);

	if (this_row==1 && this_col==1){
		// select track
		menuvar1=1;
		menuvar2=-1;
		step_record_track=-1;
		info_display("Select the track to record");
		// turn off green leds
		for (int i=0;i<16;i++){
			mcp3.digitalWrite(i, LOW);
			bitWrite(grnLedState, i, LOW);
		}
	} else if (this_row==1 && this_col==2){
		// enter steps
		if (menuvar1==2){
			// already entering steps, stop entering steps
			menuvar1=-1;
			tft.fillRect(0, 220, 320, 20, ILI9341_BLACK);
		} else if (step_record_track<0){
			info_display("No Track Selected - Use SEL TRACK First");
		} else {
			// highlight steps for the selected track
			info_display((String)"Enter steps for track " + (step_record_track+1));
			menuvar1=2;
			for (int i=pattern_page*16;i<(1+pattern_page*16);i++){
				if (PATTERNS[current_pattern][step_record_track][(pattern_page*16)+i]>0){
					// grn leds
					mcp3.digitalWrite(i, HIGH);
					bitWrite(grnLedState, i, HIGH);
				} else {
					mcp3.digitalWrite(i, LOW);
					bitWrite(grnLedState, i, LOW);
				}
			}
		}
	} else if (this_row==2 && this_col==1){
		// groovify
		// emphasize one, de-emphasize ie and a two ie
		if (step_record_track>=0){
			for (int i=0;i<64;i++){
				UNDO_PATTERN[i]=PATTERNS[current_pattern][step_record_track][i];
				if (PATTERNS[current_pattern][step_record_track][i]>0){
					int this_rand_val=random(20,60);
					if (i%4==0){
						this_rand_val=random(100,127);
					} else if (i%8==0){
						this_rand_val=random(60,100);
					}
					PATTERNS[current_pattern][step_record_track][i]=this_rand_val;
				}
			}
			info_display((String)"Groove applied to track " + step_record_track);
		}
	} else if (this_row==2 && this_col==2){
		// swing
		if (PATTERNS[current_pattern][16][0]>=33){
			PATTERNS[current_pattern][16][0]=0;
			menu_drawbutton(2, 2, 1, "SWING", "OFF");
		} else if (PATTERNS[current_pattern][16][0]>=25){
			PATTERNS[current_pattern][16][0]=33;
			menu_drawbutton(2, 2, 1, "SWING", (String)PATTERNS[current_pattern][16][0] + "%");
		} else if (PATTERNS[current_pattern][16][0]>=15){
			PATTERNS[current_pattern][16][0]=25;
			menu_drawbutton(2, 2, 1, "SWING", (String)PATTERNS[current_pattern][16][0] + "%");
		} else {
			PATTERNS[current_pattern][16][0]=15;
			menu_drawbutton(2, 2, 1, "SWING", (String)PATTERNS[current_pattern][16][0] + "%");
		}
	} else if (this_row==2 && this_col==3){
		// undo
		if (step_record_track>=0){
			// use UNDO_PATTERN_2 to swap so you can undo the undo
			for (int i=0;i<64;i++){
				UNDO_PATTERN_2[i]=PATTERNS[current_pattern][step_record_track][i];
				PATTERNS[current_pattern][step_record_track][i]=UNDO_PATTERN[i];
				UNDO_PATTERN[i]=UNDO_PATTERN_2[i];
			}
			info_display((String)"Undone");
		} else {
			info_display((String)"You must select a track first");
		}
	}

	if (menuvar1==1 && menuvar2>=0){
		step_record_track=menuvar2;
		info_display((String)"Track " + (step_record_track + 1) + " Selected, enter steps");

		// highlight steps for this track
		for (int i=0;i<16;i++){
			if (PATTERNS[current_pattern][step_record_track][(pattern_page*16)+i]>0){
				// grn leds
				mcp3.digitalWrite(i, HIGH);
				bitWrite(grnLedState, i, HIGH);
			} else {
				mcp3.digitalWrite(i, LOW);
				bitWrite(grnLedState, i, LOW);
			}
		}

		// go directly to enter steps mode
		menuvar1=2;

		menuvar2=-1;
	}
}

void menu_midi_channel(int mx=-1, int my=-1, int update=-1){
	if (mx>=288 && my<=32){
		goto_menu(MENU_SETUP);
		return;
	}

	if (update==-1){
		// create menu
		tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
		menu_banner("MIDI CHANNEL");

		tft.fillRect(0, 34, 320, 205, ILI9341_BLACK);
	}

	tft.fillRect(0, 40, 320, 200, ILI9341_BLACK);
	tft.setFont(Arial_12);
	tft.setTextColor(ILI9341_WHITE);
	tft.setCursor(2, 40);
	if (midi_channel>16){
		tft.print((String)"MIDI CHANNEL = OFF");
	} else if (midi_channel==0){
		tft.print((String)"MIDI CHANNEL = OMNI");
	} else {
		tft.print((String)"MIDI CHANNEL = " + midi_channel);
	}
}


void menu_tempo(int mx=-1, int my=-1, int update=-1){
	if (mx>=288 && my<=32){
		goto_menu(MENU_SONG);
		return;
	}

	if (update==-1){
		// create menu
		tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
		menu_banner("TEMPO");

		tft.fillRect(0, 34, 320, 205, ILI9341_BLACK);
	}

	tft.fillRect(0, 40, 320, 200, ILI9341_BLACK);
	tft.setFont(Arial_12);
	tft.setTextColor(ILI9341_WHITE);
	tft.setCursor(2, 40);
	tft.print((String)"TEMPO = " + tempo);
	tft.setCursor(2, 80);
	tft.print((String)"Use D Button For Tap Tempo");

	if (menuvar1==1){
		// started tap tempo
		tft.setCursor(2, 120);
		tft.print((String)"Tap Tempo Started");
	}
}

void menu_map(int mx=-1, int my=-1, int update_map=-1){
	if (update_map==-1 && (mx==-1 || my==-1)){
		// create menu
		tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
		menu_banner("MAP SOUND");

		tft.fillRect(0, 34, 320, 205, ILI9341_BLACK);

		// create map grid
    tft.setFont(Arial_12);
		tft.setTextColor(ILI9341_WHITE);

		int th=46;
		int tw=80;
		for (int i=0;i<16;i++){
			int tx, ty;
			tx=i%4 * tw;
			ty=34 + (i/4) * th;

			tft.drawRect(tx, ty, tw, th, ILI9341_YELLOW);
			tft.setCursor(tx+3, ty+2);
			tft.print((String)"B" + (i+1));

			tft.setCursor(tx+3, ty+25);
			tft.print((String)"NOT=" + SOUND_MAP[i]);
		}

		return;
	}

	// process clicks
	if (mx>=288 && my<=32){
		goto_menu(MENU_SETUP);
		return;
	}

	// determine which button was clicked
	int th=46;
	int tw=80;
	int unhighlight_last_button=-1;

	for (int i=0;i<16;i++){
		if (menuvar1>=0 && menuvar1!=i) continue;
		int tx, ty;
		tx=i%4 * tw;
		ty=34 + (i/4) * th;

		int tx2=tx+tw;
		int ty2=ty+th;
		tft.setFont(Arial_12);

		if (menuvar1>=0 || (mx>=tx && mx<tx2 && my>=ty && my<ty2)){
			// this button was clicked
			if (menuvar1==i && mx>=0 && my>=0){
				// it was already clicked, unclick and unhighlight it
				tft.fillRect(tx+1, ty+1, tw-2, th-2, ILI9341_BLACK);
				tft.setTextColor(ILI9341_WHITE);
				menuvar1=-1;
			} else {
				// highlight it
				tft.fillRect(tx+1, ty+1, tw-2, th-2, ILI9341_YELLOW);
				tft.setTextColor(ILI9341_BLACK);
				menuvar1=i;
			}
		} else {
			// not clicked, unhighlight it
			tft.fillRect(tx+1, ty+1, tw-2, th-2, ILI9341_BLACK);
			tft.setTextColor(ILI9341_WHITE);
		}
		tft.setCursor(tx+3, ty+2);
		tft.print((String)"B" + (i+1));

		tft.setCursor(tx+3, ty+25);
		tft.print((String)"NOT=" + SOUND_MAP[i]);
	}
}

int save_settings(){
	// remove previous settings file
	SD.remove("dm_global.txt");
	// create and write to file
	myFile=SD.open("dm_global.txt", FILE_WRITE);
	if (myFile){
		myFile.print("midi_channel=");
		myFile.println(midi_channel);

		myFile.print("SOUND_MAP=");
		for (int i=0;i<16;i++){
			myFile.print((String)SOUND_MAP[i] + ",");
		}
		myFile.println();

		myFile.print("VOLUME_MAP=");
		for (int i=0;i<16;i++){
			myFile.print((String)VOLUME_MAP[i] + ",");
		}
		myFile.println();

		myFile.print("FX_MAP=");
		for (int i=0;i<16;i++){
			myFile.print((String)FX_MAP[i] + ",");
		}
		myFile.println();

		myFile.close();

		info_display((String)"settings saved to sdcard");
		debug_print(1, (String)"settings saved to sdcard");
	} else {
		info_display((String)"settings not to sdcard");
		debug_print(1, (String)"settings not saved to sdcard");
	}
}

int load_settings(){
	if (SD.exists("dm_global.txt")){
		char b;
		String line="";
		String sValue="";
		String sKey="";
		int sPart=0;
		myFile = SD.open("dm_global.txt");
		while (myFile.available()){
			b=myFile.read();
			if (String(b)=="="){
				sPart=1;
			} else if (String(b)=="\n"){
				// end of line
				debug_print(1, (String)" found: " + sKey + " = " + sValue);

				if (sKey=="midi_channel"){
					int iVal=sValue.toInt();
					if (iVal>=0 && iVal<=256) midi_channel=iVal;
				debug_print(1, (String)"set midi_channel= " + midi_channel);
				} else if (sKey=="SOUND_MAP"){
					int vCnt=0;
					int iVal=0;
					String iStr="";
					for (int i=0;i<sValue.length();i++){
						if (String(sValue[i])==","){
							// end of value
							iVal=iStr.toInt();
							if (vCnt<16) SOUND_MAP[vCnt]=iVal;
							iStr="";
							vCnt++;
						} else {
							iStr+=sValue[i];
						}
					}
				} else if (sKey=="VOLUME_MAP"){
					int vCnt=0;
					int iVal=0;
					String iStr="";
					for (int i=0;i<sValue.length();i++){
						if (String(sValue[i])==","){
							// end of value
							iVal=iStr.toInt();
							if (vCnt<16) VOLUME_MAP[vCnt]=iVal;
							iStr="";
							vCnt++;
						} else {
							iStr+=sValue[i];
						}
					}
				} else if (sKey=="FX_MAP"){
					int vCnt=0;
					int iVal=0;
					String iStr="";
					for (int i=0;i<sValue.length();i++){
						if (String(sValue[i])==","){
							// end of value
							iVal=iStr.toInt();
							if (vCnt<16) FX_MAP[vCnt]=iVal;
							if (FX_MAP[vCnt]>127) FX_MAP[vCnt]=0;
							float thisfloat=map(1. * FX_MAP[vCnt], 0, 127, 0., 1.);
							if (vCnt<4){
								fxmixer1.gain(vCnt, thisfloat);
							} else if (vCnt<8){
								fxmixer2.gain(vCnt-4, thisfloat);
							} else if (vCnt<12){
								fxmixer3.gain(vCnt-8, thisfloat);
							} else {
								fxmixer4.gain(vCnt-12, thisfloat);
							}
							iStr="";
							vCnt++;
						} else {
							iStr+=sValue[i];
						}
					}
				}

				sValue="";
				sKey="";
				sPart=0;
			} else {
				if (sPart==0){
					sKey+=b;
				} else {
					sValue+=b;
				}
			}
		}
		myFile.close();

		info_display((String)"dm_global.txt loaded");
		debug_print(1, (String)"dm_global.txt loaded ");
	} else {
		info_display((String)"dm_global.txt not found on sd card");
		debug_print(1, (String)"dm_global.txt not found on sdcard");
	}
}

void menu_storage(int mx=-1, int my=-1){
	if (mx>=288 && my<=32){
		goto_menu(MENU_SETUP);
		return;
	}

	if (mx==-1 || my==-1){
		// create menu
		menu_banner("STORAGE MENU");
		tft.fillRect(0, 34, 320, 205, ILI9341_BLACK);
		menu_drawbutton(1, 1, 1, "LOAD", "SETTINGS");
		menu_drawbutton(1, 2, 1, "SAVE", "SETTINGS");
		menu_drawbutton(1, 3, 2, "");
		menu_drawbutton(2, 1, 2, "");
		menu_drawbutton(2, 2, 2, "");
		menu_drawbutton(2, 3, 2, "");
		return;
	}

	// process menu click
	int this_row=menu_get_row(my);
	int this_col=menu_get_col(mx);

	if (this_row==1 && this_col==1){
		if (sdcard_enabled){
			load_settings();
		}
	} else if (this_row==1 && this_col==2){
		// save
		if (sdcard_enabled){
			save_settings();
		}
	}
}

void clear_song(){
	// clear patterns
	for (int i=0;i<16;i++){
		for (int j=0;j<17;j++){
			for (int k=0;k<64;k++){
				PATTERNS[i][j][k]=0;
			}
		}
	}

	// clear song steps
	for (int i=0;i<16;i++){
		SONG[i][0]=255;
		SONG[i][1]=0;
	}
}

void menu_save_song(int mx=-1, int my=-1, bool select_file=false){
	if (mx>=288 && my<=32){
		goto_menu(MENU_SONG);
		return;
	}

	if (mx==-1 || my==-1){
		// create menu
		menu_banner("SAVE SONG");

		tft.fillRect(0, 34, 320, 205, ILI9341_BLACK);
		tft.setFont(Arial_18);
		tft.setTextColor(ILI9341_WHITE);
		tft.setCursor(2, 105);
		tft.print((String)"Scroll to select Filename:");

		if (select_file==false){
			info_display((String)"dm_song_" + song_number + ".txt");
		} else {
			char filename[50];
			String save_filename=(String)"dm_song_" + song_number + ".txt";
			save_filename.toCharArray(filename, 50);

			// remove previous settings file
			SD.remove(filename);
			// create and write to file
			myFile=SD.open(filename, FILE_WRITE);
			if (myFile){
				myFile.print("tempo=");
				myFile.println(tempo);

				myFile.print("SONG=");
				for (int i=0;i<16;i++){
					for (int j=0;j<2;j++){
						myFile.print((String) + SONG[i][j] + ",");
					}
				}
				myFile.println();

				myFile.print("PATTERN=");
				for (int i=0;i<16;i++){
					for (int j=0;j<17;j++){
						for (int k=0;k<64;k++){
							myFile.print((String) + PATTERNS[i][j][k] + ",");
						}
					}
				}
				myFile.println();

				myFile.print("SOUND_MAP=");
				for (int i=0;i<16;i++){
					myFile.print((String)SOUND_MAP[i] + ",");
				}
				myFile.println();

				myFile.print("VOLUME_MAP=");
				for (int i=0;i<16;i++){
					myFile.print((String)VOLUME_MAP[i] + ",");
				}
				myFile.println();

				myFile.print("FX_MAP=");
				for (int i=0;i<16;i++){
					myFile.print((String)FX_MAP[i] + ",");
				}
				myFile.println();

				myFile.close();

				info_display((String)save_filename + " saved to sdcard");
				debug_print(1, (String)save_filename + " saved to sdcard");
			} else {
				info_display("song not saved to sdcard");
				debug_print(1, (String)"song not saved to sdcard");
			}
		}
	}
}

void menu_build_song(int mx=-1, int my=-1, int update=-1){
	// process clicks
	if (mx>=288 && my<=32){
		goto_menu(MENU_SONG);
		return;
	}

	bool last_step_was_off;
	if (update==-1 && (mx==-1 || my==-1)){
		// create menu
		tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
		menu_banner("BUILD SONG");

		tft.fillRect(0, 34, 320, 205, ILI9341_BLACK);

		// create grid
    tft.setFont(Arial_12);
		tft.setTextColor(ILI9341_WHITE);

		int th=46;
		int tw=80;
		last_step_was_off=false;
		for (int i=0;i<16;i++){
			int tx, ty;
			tx=i%4 * tw;
			ty=34 + (i/4) * th;

			tft.drawRect(tx, ty, tw, th, ILI9341_YELLOW);

			if (last_step_was_off==false){
				tft.setCursor(tx+3, ty+2);
				tft.print((String)"STEP " + (i+1));

				if (SONG[i][0]>=255){
					tft.setCursor(tx+3, ty+16);
					tft.print((String)" PTN=off");
					tft.setCursor(tx+3, ty+30);
					tft.print((String)" RPT=");
					last_step_was_off=true;
				} else {
					// make sure the repeats is at least one for any step that is used
					if (SONG[i][1]==0) SONG[i][1]=1;

					tft.setCursor(tx+3, ty+16);
					tft.print((String)" PTN=" + (SONG[i][0] + 1));
					tft.setCursor(tx+3, ty+30);
					tft.print((String)" RPT=" + SONG[i][1]);
				}
			}
		}

		return;
	}

	// determine which button was clicked
	int th=46;
	int tw=80;
	int unhighlight_last_button=-1;

	last_step_was_off=false;
	for (int i=0;i<16;i++){
		if (menuvar1>=0 && menuvar1!=i) continue;
		int tx, ty;
		tx=i%4 * tw;
		ty=34 + (i/4) * th;

		int tx2=tx+tw;
		int ty2=ty+th;
		tft.setFont(Arial_12);

		if (last_step_was_off==false){
			if (menuvar1>=0 || (mx>=tx && mx<tx2 && my>=ty && my<ty2)){
				// this button was clicked
				if (menuvar1==i && mx>=0 && my>=0){
					// it was already clicked, unclick and unhighlight it
					menu_banner("BUILD SONG");
					tft.setFont(Arial_12);
					tft.fillRect(tx+1, ty+1, tw-2, th-2, ILI9341_BLACK);
					tft.setTextColor(ILI9341_WHITE);
					menuvar1=-1;
					menuvar2=-1;
				} else {
					// highlight it
					if (mx==-1 || my==-1){
						// this is an update from encoder
					} else {
						// track just selected
						if (last_step_was_off==false){
							menu_banner("BUILD - PATTERN");
						}
					}
					tft.setFont(Arial_12);
					tft.fillRect(tx+1, ty+1, tw-2, th-2, ILI9341_YELLOW);
					tft.setTextColor(ILI9341_BLACK);
					menuvar1=i;
				}
			} else {
				// not clicked, unhighlight it
				tft.fillRect(tx+1, ty+1, tw-2, th-2, ILI9341_BLACK);
				tft.setTextColor(ILI9341_WHITE);
			}

			tft.setCursor(tx+3, ty+2);
			tft.print((String)"STEP " + (i+1));

			if (SONG[i][0]>=255){
				tft.setCursor(tx+3, ty+16);
				tft.print((String)" PTN=off");
				tft.setCursor(tx+3, ty+30);
				tft.print((String)" RPT=");
				last_step_was_off=true;
			} else {
				// make sure the repeats is at least one for any step that is used
				if (SONG[i][1]==0) SONG[i][1]=1;

				tft.setCursor(tx+3, ty+16);
				tft.print((String)" PTN=" + (SONG[i][0] + 1));
				tft.setCursor(tx+3, ty+30);
				tft.print((String)" RPT=" + SONG[i][1]);
			}
		}
	}
}

void menu_load_song(int mx=-1, int my=-1, bool select_file=false){
	if (mx>=288 && my<=32){
		goto_menu(MENU_SONG);
		return;
	}

	// create menu
	menu_banner("LOAD SONG");
	tft.fillRect(0, 34, 320, 205, ILI9341_BLACK);
	// list songs on sd card
	Serial.println("Directory Listing:\n");
	File root = SD.open("/");
	int file_count=0;
	if (file_number<0) file_number=0;
	while(true) {
		File entry = root.openNextFile();
		if (! entry) {
			//Serial.println("** no more files **");
			// probably reached end of listing, do not allow to advance
			file_number=file_number-1;
			entry.close();
			break;
		}
	
		if (file_number==file_count && (String)entry.name()!=""){
			load_filename=(String)entry.name();
			entry.close();
			break;
		}
		entry.close();
		file_count++;
	}

	tft.setFont(Arial_18);
	tft.setTextColor(ILI9341_WHITE);
	tft.setCursor(2, 55);
	tft.print((String)"Select File To Load:");

	// button
	menu_drawbutton(2, 1, 1, "Load", "Selected", "File");

	int this_row=menu_get_row(my);
	int this_col=menu_get_col(mx);

	if (this_row==2 && this_col==1){
		select_file=true;
	}

	info_display((String)load_filename);
	Serial.println(load_filename);

	if (select_file){
		if (load_filename==""){
			info_display((String)"No file selected");
		} else {
			char filename[50];
			load_filename.toCharArray(filename, 50);
			if (SD.exists(filename)){
				clear_song();

				char b;
				String line="";
				String sValue="";
				String sKey="";
				int sPart=0;
				myFile = SD.open(filename);
				while (myFile.available()){
					b=myFile.read();
					if (String(b)=="="){
						sPart=1;
					} else if (String(b)=="\n"){
						// end of line
						debug_print(2, (String)" found: " + sKey + " = " + sValue);

						if (sKey=="tempo"){
							int iVal=sValue.toInt();
							if (iVal>=10 && iVal<=300) tempo=iVal;
							debug_print(2, (String)"set tempo= " + tempo);
						} else if (sKey=="SONG"){
							int vCnt=0;
							int iVal=0;
							int iTrk=0;
							int iStep=0;
							String iStr="";
							for (int i=0;i<sValue.length();i++){
								if (String(sValue[i])==","){
									// end of value
									iVal=iStr.toInt();
									if (iTrk<16 && iStep<2) SONG[iTrk][iStep]=iVal;
									iStr="";
									vCnt++;
									iStep++;
									if (iStep>1){
										iStep=0;
										iTrk++;
									}
								} else {
									iStr+=sValue[i];
								}
							}
						} else if (sKey=="PATTERN"){
							int vCnt=0;
							int iVal=0;
							int iTrk=0;
							int iPtn=0;
							int iStep=0;
							String iStr="";
							for (int i=0;i<sValue.length();i++){
								if (String(sValue[i])==","){
									// end of value
									iVal=iStr.toInt();
									if (iPtn<16 && iTrk<17 && iStep<64) PATTERNS[iPtn][iTrk][iStep]=iVal;
									iStr="";
									vCnt++;
									iStep++;
									if (iStep>=64){
										iStep=0;
										iTrk++;
										if (iTrk>=17){
											// use 17 because that additional track is used for pattern settings
											iPtn++;
											iTrk=0;
										}
									}
								} else {
									iStr+=sValue[i];
								}
							}
						} else if (sKey=="SOUND_MAP"){
							int vCnt=0;
							int iVal=0;
							String iStr="";
							for (int i=0;i<sValue.length();i++){
								if (String(sValue[i])==","){
									// end of value
									iVal=iStr.toInt();
									if (vCnt<16) SOUND_MAP[vCnt]=iVal;
									iStr="";
									vCnt++;
								} else {
									iStr+=sValue[i];
								}
							}
						} else if (sKey=="VOLUME_MAP"){
							int vCnt=0;
							int iVal=0;
							String iStr="";
							for (int i=0;i<sValue.length();i++){
								if (String(sValue[i])==","){
									// end of value
									iVal=iStr.toInt();
									if (vCnt<16) VOLUME_MAP[vCnt]=iVal;
									iStr="";
									vCnt++;
								} else {
									iStr+=sValue[i];
								}
							}
						} else if (sKey=="FX_MAP"){
							int vCnt=0;
							int iVal=0;
							String iStr="";
							for (int i=0;i<sValue.length();i++){
								if (String(sValue[i])==","){
									// end of value
									iVal=iStr.toInt();
									if (vCnt<16) FX_MAP[vCnt]=iVal;
									if (FX_MAP[vCnt]>127) FX_MAP[vCnt]=0;
									float thisfloat=map(1. * FX_MAP[vCnt], 0, 127, 0., 1.);
									if (vCnt<4){
										fxmixer1.gain(vCnt, thisfloat);
									} else if (vCnt<8){
										fxmixer2.gain(vCnt-4, thisfloat);
									} else if (vCnt<12){
										fxmixer3.gain(vCnt-8, thisfloat);
									} else {
										fxmixer4.gain(vCnt-12, thisfloat);
									}
									iStr="";
									vCnt++;
								} else {
									iStr+=sValue[i];
								}
							}
						}

						sValue="";
						sKey="";
						sPart=0;
					} else {
						if (sPart==0){
							sKey+=b;
						} else {
							sValue+=b;
						}
					}
				}
				myFile.close();

				info_display((String)load_filename + " loaded");
				debug_print(1, (String)load_filename + " loaded ");
			} else {
				info_display((String)"song not found on sd card");
				debug_print(1, (String)"song not found on sd card");
			}
		}
	}
}

void menu_pattern_setup(int mx=-1, int my=-1, int update=-1){
	// process clicks
	if (mx>=288 && my<=32){
		goto_menu(MENU_PATTERN);
		return;
	}

	if (update==-1 && (mx==-1 || my==-1)){
		// create menu
		menu_banner((String)"PATTERN " + (current_pattern + 1));
		tft.fillRect(0, 34, 320, 205, ILI9341_BLACK);
	}

	menu_drawbutton(1, 1, 1, "LENGTH", (String)current_pattern_length + " STEPS");

	// process menu click
	int this_row=menu_get_row(my);
	int this_col=menu_get_col(mx);

	if (this_row==1 && this_col==1){
		if (menuvar1==1){
			// already selecting length, stop selecting length
			menuvar1=-1;
		} else {
			// start selecting length
			menuvar1=1;
		}
	}

	if (menuvar1==1){
		// scrolling through length selection
		info_display((String)"Scroll to change pattern length");
	}
}

void menu_pattern(int mx=-1, int my=-1, int update=-1){
	// process clicks
	if (mx>=288 && my<=32){
		goto_menu(MENU_MAIN);
		return;
	}

	if (update==-1 && (mx==-1 || my==-1)){
		// create menu
		menu_banner((String)"PATTERN " + (current_pattern + 1));
		tft.fillRect(0, 34, 320, 205, ILI9341_BLACK);
		menu_drawbutton(1, 1, 1, "SELECT", "PATTERN");
		menu_drawbutton(1, 2, 1, "STEP", "RECORD");
		menu_drawbutton(1, 3, 1, "COPY", "PATTERN");
		menu_drawbutton(2, 1, 1, "CLEAR", "PATTERN");
		menu_drawbutton(2, 2, 1, "SETUP", (String)"LEN=" + current_pattern_length);
		menu_drawbutton(2, 3, 1, "PASTE", "PATTERN");

		tft.fillRect(0, 220, 320, 20, ILI9341_BLACK);

		clear_confirm=false;
		return;
	}

	// process menu click
	int this_row=menu_get_row(my);
	int this_col=menu_get_col(mx);

	if (this_row==1 && this_col==1){
		// select pattern
		menuvar1=1;
		menuvar2=-1;
		info_display((String)"Select the pattern to use");
		// turn off green leds
		for (int i=0;i<16;i++){
			mcp3.digitalWrite(i, LOW);
			bitWrite(grnLedState, i, LOW);
		}
	} else if (this_row==1 && this_col==2){
		// step record
		goto_menu(MENU_STEP_REC);
		return;
	} else if (this_row==1 && this_col==3){
		// copy current pattern
		copy_paste_pattern=current_pattern;
		info_display((String)"Pattern " + (copy_paste_pattern + 1) + " copied to buffer");
	} else if (this_row==2 && this_col==1){
		if (clear_confirm){
			// clear pattern
			for (int i=0;i<16;i++){
				for (int j=0;j<64;j++){
					PATTERNS[current_pattern][i][j]=0;
				}
			}
			info_display((String)"Pattern " + (copy_paste_pattern + 1) + " cleared");
		} else {
			clear_confirm=true;
			info_display((String)"Select CLEAR PTN again to clear pattern " + (copy_paste_pattern + 1));
		}
	} else if (this_row==2 && this_col==2){
		menuvar1=-1;
		goto_menu(MENU_PATTERN_SETUP);
	} else if (this_row==2 && this_col==3){
		// paste pattern
		if (copy_paste_pattern>=0){
			for (int i=0;i<16;i++){
				for (int j=0;j<64;j++){
					PATTERNS[current_pattern][i][j]=PATTERNS[copy_paste_pattern][i][j];
				}
			}
			info_display((String)"Pasted from copy/paste buffer into pattern " + (current_pattern + 1));
		} else {
			info_display((String)"Cannot paste pattern. You have to copy a pattern first.");
		}
	}

	if (menuvar1==1 && menuvar2>=0){
		set_current_pattern(menuvar2);
		menu_banner((String)"PATTERN " + (current_pattern + 1));
		menu_drawbutton(2, 2, 1, "SETUP", (String)"LEN=" + current_pattern_length);
		info_display((String)"Pattern " + (current_pattern + 1) + " selected");

		menuvar1=-1;
		menuvar2=-1;
	}
}

void menu_song(int mx=-1, int my=-1, int update=-1){
	if (mx>=288 && my<=32){
		goto_menu(MENU_MAIN);
		return;
	}

	if (mx==-1 || my==-1){
		// create menu
		menu_banner("SONG");
		tft.fillRect(0, 34, 320, 205, ILI9341_BLACK);
		menu_drawbutton(1, 1, 2, "BUILD", "SONG");
		menu_drawbutton(1, 2, 2, "TEMPO");
		menu_drawbutton(1, 3, 2, "LOAD", "SONG");
		menu_drawbutton(2, 1, 2, "", "");
		menu_drawbutton(2, 2, 2, "CLEAR", "SONG");
		menu_drawbutton(2, 3, 2, "SAVE", "SONG");
		clear_confirm=false;
		return;
	}

	// process menu click
	int this_row=menu_get_row(my);
	int this_col=menu_get_col(mx);

	if (this_row==1 && this_col==3){
		goto_menu(MENU_LOAD_SONG);
	} else if (this_row==1 && this_col==2){
		goto_menu(MENU_TEMPO);
	} else if (this_row==2 && this_col==3){
		goto_menu(MENU_SAVE_SONG);
	} else if (this_row==2 && this_col==2){
		if (clear_confirm){
			clear_song();
			info_display((String)"Song and all patterns cleared");
		} else {
			clear_confirm=true;
			info_display((String)"Select again to clear song and all patterns");
		}
	} else if (this_row==1 && this_col==1){
		goto_menu(MENU_BUILD_SONG);
	}
}

void menu_setup(int mx=-1, int my=-1){
	if (mx>=288 && my<=32){
		goto_menu(MENU_MAIN);
		return;
	}

	if (mx==-1 || my==-1){
		// create menu
		menu_banner("SETUP");
		tft.fillRect(0, 34, 320, 205, ILI9341_BLACK);
		menu_drawbutton(1, 1, 1, "MIDI", "CHANNEL");
		menu_drawbutton(1, 2, 1, "LOAD/SAVE", "SETTINGS");
		menu_drawbutton(1, 3, 1, "MIXER");
		menu_drawbutton(2, 1, 1, "SOUND", "MAP");
		menu_drawbutton(2, 2, 1, "");
		menu_drawbutton(2, 3, 1, "");
		return;
	}

	// process menu click
	int this_row=menu_get_row(my);
	int this_col=menu_get_col(mx);

	if (this_row==2 && this_col==1){
		goto_menu(MENU_MAP);
	} else if (this_row==1 && this_col==1){
		goto_menu(MENU_MIDI);
	} else if (this_row==1 && this_col==2){
		goto_menu(MENU_STORAGE);
	} else if (this_row==1 && this_col==3){
		goto_menu(MENU_MIX);
	}
}

void menu_main(int mx=-1, int my=-1){
	if (mx>=288 && my<=32){
		goto_menu(MENU_MAIN);
		return;
	}

	if (mx==-1 || my==-1){
		// create menu
		menu_banner("MAIN MENU");
		tft.fillRect(0, 34, 320, 205, ILI9341_BLACK);
		menu_drawbutton(1, 1, 2, "SONG");
		menu_drawbutton(1, 2, 2, "PTN");
		menu_drawbutton(1, 3, 2, "");
		menu_drawbutton(2, 1, 2, "");
		menu_drawbutton(2, 2, 2, "");
		menu_drawbutton(2, 3, 2, "SETUP");
		return;
	}

	// process menu click
	int this_row=menu_get_row(my);
	int this_col=menu_get_col(mx);

	if (this_row==2 && this_col==3){
		goto_menu(MENU_SETUP);
	} else if (this_row==1 && this_col==1){
		goto_menu(MENU_SONG);
	} else if (this_row==1 && this_col==2){
		goto_menu(MENU_PATTERN);
	}
}

void goto_menu(byte this_menu){
	if (menu!=this_menu){
		last_menu=menu;
		menu=this_menu;

		menuvar1=-1;
		menuvar2=-1;
		if (this_menu==MENU_MAIN){
			menu_main();
		} else if (this_menu==MENU_MAP){
			menu_map();
		} else if (this_menu==MENU_MIX){
			menu_mix();
		} else if (this_menu==MENU_TEMPO){
			menu_tempo();
		} else if (this_menu==MENU_MIDI){
			menu_midi_channel();
		} else if (this_menu==MENU_STORAGE){
			menu_storage();
		} else if (this_menu==MENU_SETUP){
			menu_setup();
		} else if (this_menu==MENU_SONG){
			menu_song();
		} else if (this_menu==MENU_LOAD_SONG){
			file_number=0;
			menu_load_song();
		} else if (this_menu==MENU_BUILD_SONG){
			menu_build_song();
		} else if (this_menu==MENU_SAVE_SONG){
			menu_save_song();
		} else if (this_menu==MENU_PATTERN){
			menu_pattern();
		} else if (this_menu==MENU_PATTERN_SETUP){
			menu_pattern_setup();
		} else if (this_menu==MENU_STEP_REC){
			// turn off all leds
			for (int i=0;i<16;i++){
				// red leds
				mcp2.digitalWrite(i, LOW);
				bitWrite(redLedState, i, HIGH);
				// grn leds
				mcp3.digitalWrite(i, LOW);
				bitWrite(grnLedState, i, LOW);
			}
			menu_step_rec();
		}
	}
}

void process_touch(int mx, int my){
	debug_print(2, (String)"in process_touch, mx=" + mx + " my=" + my);
	if (menu==MENU_MAIN){
		menu_main(mx, my);
	} else if (menu==MENU_MAP){
		menu_map(mx, my);
	} else if (menu==MENU_MIX){
		menu_mix(mx, my);
	} else if (menu==MENU_TEMPO){
		menu_tempo(mx, my);
	} else if (menu==MENU_MIDI){
		menu_midi_channel(mx, my);
	} else if (menu==MENU_PATTERN){
		menu_pattern(mx, my);
	} else if (menu==MENU_PATTERN_SETUP){
		menu_pattern_setup(mx, my);
	} else if (menu==MENU_STEP_REC){
		menu_step_rec(mx, my);
	} else if (menu==MENU_STORAGE){
		menu_storage(mx, my);
	} else if (menu==MENU_SETUP){
		menu_setup(mx, my);
	} else if (menu==MENU_SONG){
		menu_song(mx, my);
	} else if (menu==MENU_BUILD_SONG){
		menu_build_song(mx, my);
	} else if (menu==MENU_LOAD_SONG){
		menu_load_song(mx, my);
	} else if (menu==MENU_SAVE_SONG){
		menu_save_song(mx, my);
	}
}

void set_current_pattern(int this_pattern_number){
	current_pattern=this_pattern_number;
	current_pattern_length=PATTERNS[current_pattern][16][1];
	if (current_pattern_length<1) current_pattern_length=1;
	else if (current_pattern_length>64) current_pattern_length=64;
}
#endif

void setup() {
	delay(500);
	debug_print(1, "setup started");

	// setup sd card
	if (!SD.begin(BUILTIN_SDCARD)){
		sdcard_enabled=false;
		debug_print(1, "could not initialize sd card");
	} else {
		debug_print(1, "sd card initialized");
	}

	// setup touchscreen
#ifdef USE_TOUCHSCREEN
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  tft.begin();
	delay(500);
  tft.setClock(45000000);
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  ts.begin();
  ts.setRotation(1);
#endif

	// setup encoder
	enc.setInitConfig();
	enc.EncConfig.filterCount = 5;
	enc.EncConfig.filterSamplePeriod = 255;
	enc.init();
	pinMode(ENC_PIN_SW, INPUT_PULLUP);
	encoderVal=enc.read();

  mcp1.begin(7);
  mcp2.begin(6);
  mcp3.begin(5);
  mcp4.begin(3);

	for (int i=0;i<16;i++){
  	mcp1.pinMode(i, INPUT); // set all pins on board #1 as inputs
  	mcp1.pullUp(i, HIGH);  // turn on a 100K pullup internally

  	mcp2.pinMode(i, OUTPUT); // pb86 red leds. set all pins on board #2 as outputs, for LEDs
  	mcp3.pinMode(i, OUTPUT); // pb86 green leds. set all pins on board #3 as outputs, for LEDs
		// turn off all LEDs at startup
		mcp2.digitalWrite(i, LOW);
		mcp3.digitalWrite(i, LOW);

		if (i<8){
			// set port A on board #4 as input
			mcp4.pinMode(i, INPUT);
			mcp4.pullUp(i, HIGH);
		} else {
			// set port B on board #4 as output
  		mcp4.pinMode(i, OUTPUT);
			// turn off all LEDs at startup
			mcp4.digitalWrite(i, LOW);
		}
	}

	mcp1.readGPIOAB();
	mcp4.readGPIO(0);

	myusb.begin();
	keyboard1.attachPress(OnPress);
	keyboard2.attachPress(OnPress);
	midi1.setHandleNoteOff(OnNoteOff);
	midi1.setHandleNoteOn(OnNoteOn);
	midi1.setHandleControlChange(OnControlChange);
	midi2.setHandleNoteOff(OnNoteOff);
	midi2.setHandleNoteOn(OnNoteOn);
	midi2.setHandleControlChange(OnControlChange);

	AudioMemory(20);

	for (int i=0;i<16;i++){
		//wavetable[i].setInstrument(TR808V1);
		wavetable[i].setInstrument(drums);
		wavetable[i].amplitude(1.0);
	}

	for (int i = 0; i < 4; ++i){
		wavemixer1.gain(i, 0.6);
		wavemixer2.gain(i, 0.6);
		wavemixer3.gain(i, 0.6);
		wavemixer4.gain(i, 0.6);
		wavemixer5.gain(i, 0.6);
		fxmixer1.gain(i, 0.6);
		fxmixer2.gain(i, 0.6);
		fxmixer3.gain(i, 0.6);
		fxmixer4.gain(i, 0.6);
		fxmixer5.gain(i, 0.6);
	}
	mastermixer.gain(0, 1.0);
	mastermixer.gain(1, 1.0);
	reverb1.roomsize(0.9);
	reverb1.damping(0.5);

	amp1.gain(2.0);
	amp2.gain(2.0);

	debug_print(1, "setup finished");

	load_settings();
	clear_song();

	goto_menu(MENU_MAIN);
}

int nbf = 0; // count the number of frames drawn.

void loop() {
	thisloop_ms=millis();

	myusb.Task();
	midi1.read();
	midi2.read();

	// read the encoder
	long newVal;
	newVal = enc.read();
	newVal=newVal/4;
	if (newVal != encoderVal){
		if (menu==MENU_MAP){
			if (menuvar1>=0){
				// change sound mapping
				SOUND_MAP[menuvar1]=SOUND_MAP[menuvar1]+newVal-encoderVal;
				if (SOUND_MAP[menuvar1]>127) SOUND_MAP[menuvar1]=127;
				else if (SOUND_MAP[menuvar1]<0) SOUND_MAP[menuvar1]=0;
				menu_map(-1, -1, menuvar1);
			}
		} else if (menu==MENU_MIX){
			if (menuvar1>=0){
				// change mixer
				if (menuvar2==-1){
					// volume
					VOLUME_MAP[menuvar1]=VOLUME_MAP[menuvar1]+newVal-encoderVal;
					if (VOLUME_MAP[menuvar1]>127) VOLUME_MAP[menuvar1]=127;
					else if (VOLUME_MAP[menuvar1]<0) VOLUME_MAP[menuvar1]=0;
					menu_mix(-1, -1, menuvar1);
				} else {
					// fx
					FX_MAP[menuvar1]=FX_MAP[menuvar1]+newVal-encoderVal;
					if (FX_MAP[menuvar1]>127) FX_MAP[menuvar1]=127;
					else if (FX_MAP[menuvar1]<0) FX_MAP[menuvar1]=0;
				
					float thisfx=map(1. * FX_MAP[menuvar1], 0, 127, 0., 1.);
					if (menuvar1<4){
						fxmixer1.gain(menuvar1, thisfx);
					} else if (menuvar1<8){
						fxmixer2.gain(menuvar1-4, thisfx);
					} else if (menuvar1<12){
						fxmixer3.gain(menuvar1-8, thisfx);
					} else {
						fxmixer4.gain(menuvar1-12, thisfx);
					}

					menu_mix(-1, -1, menuvar1);
				}
			}
		} else if (menu==MENU_BUILD_SONG){
			if (menuvar1>=0){
				if (menuvar2==-1){
					// pattern
					int this_val=SONG[menuvar1][0]+newVal-encoderVal;
					SONG[menuvar1][0]=this_val;
					// 255 = off
					if (this_val<0) SONG[menuvar1][0]=255;
					if (SONG[menuvar1][0]>=254) SONG[menuvar1][0]=255;
					else if (SONG[menuvar1][0]>15) SONG[menuvar1][0]=15;
				} else {
					// repeat times
					int this_val=SONG[menuvar1][1]+newVal-encoderVal;
					SONG[menuvar1][1]=this_val;
					if (this_val<1) SONG[menuvar1][1]=1;
					else if (SONG[menuvar1][1]>255) SONG[menuvar1][1]=255;
				}
				menu_build_song(-1, -1, menuvar1);
			}
		} else if (menu==MENU_TEMPO){
			tempo=tempo+newVal-encoderVal;
			if (tempo<10) tempo=10;
			if (tempo>300) tempo=300;
			menu_tempo(-1, -1, 1);
		} else if (menu==MENU_MIDI){
			midi_channel=midi_channel+newVal-encoderVal;
			if (midi_channel<-1) midi_channel=-1;
			if (midi_channel>16) midi_channel=16;
			menu_midi_channel(-1, -1, 1);
		} else if (menu==MENU_LOAD_SONG){
			file_number=file_number+newVal-encoderVal;
			menu_load_song(-1, -1, false);
		} else if (menu==MENU_SAVE_SONG){
			song_number=song_number+newVal-encoderVal;
			if (song_number<0) song_number=0;
			menu_save_song(-1, -1, false);
		} else if (menu==MENU_PATTERN_SETUP){
			if (menuvar1==1){
				// selecting pattern length
				current_pattern_length=current_pattern_length+newVal-encoderVal;
				if (current_pattern_length<1) current_pattern_length=1;
				else if (current_pattern_length>64) current_pattern_length=64;
				// save to current pattern
				PATTERNS[current_pattern][16][1]=current_pattern_length;
				menu_pattern_setup(-1, -1, 1);
			}
		} else {
			tempo=tempo+newVal-encoderVal;
			if (tempo<10) tempo=10;
			if (tempo>300) tempo=300;
		}
		encoderVal = newVal;
	}

	// read the encoder switch
	if (thisloop_ms>next_encoder_button_read){
		bool this_sw_val=digitalRead(ENC_PIN_SW);
		if (this_sw_val!=encoderButton){
			encoderButton=this_sw_val;
			if (encoderButton==0){
				// button pushed
				encoderButtonHoldStart=thisloop_ms;
				if (menu==MENU_MAP && menuvar1>=0){
					// end selection of sound
					menuvar1=-1;
					menu_map();
				} else if (menu==MENU_LOAD_SONG){
					menu_load_song(-1, -1, true);
				} else if (menu==MENU_SAVE_SONG){
					menu_save_song(-1, -1, true);
				} else if (menu==MENU_MIX && menuvar1>=0){
					if (menuvar2==-1){
						// switch to fx
						menuvar2=1;
						menu_banner("MIXER - FX");
					} else {
						// switch to volume
						menuvar2=-1;
						menu_banner("MIXER - VOL");
					}
				} else if (menu==MENU_BUILD_SONG && menuvar1>=0){
					if (menuvar2==-1){
						// switch to repeats
						menuvar2=1;
						menu_banner("BUILD - REPEATS");
					} else {
						// switch to pattern
						menuvar2=-1;
						menu_banner("BUILD - PATTERN");
					}
				}
			} else {
				// button released
				encoder_ms=thisloop_ms-encoderButtonHoldStart;
			}
		}
		next_encoder_button_read=thisloop_ms+1;
	}

	// read MCP23017 every x ms
	if (thisloop_ms>=next_mcp_read){
		handleButtons();
		next_mcp_read=thisloop_ms+5;
	}

	loopcount++;
	if (thisloop_ms>=looptime_ms){
		debug_print(2, (String)"Loops/Sec: " + loopcount + "   Buttons: " + button_count);
		loopcount=0;
		looptime_ms=thisloop_ms+1000;
	}

	// turn off debugging blinks after a few ms
	if (thisloop_ms>nextblinkoff_ms){
		// turn off LED
	}

	if (quadLedState!=quadButtonState){
		for (int i=0;i<4;i++){
			bool thisBit=bitRead(quadButtonState, i);
			bool thisLed=bitRead(quadLedState, i);
			if (thisBit==thisLed) {
				// this is not the button that changed
				continue;
			}
			// A=3, B=1, C=2, D=0
			if (thisBit){
				// mechanical switch released
				mcp4.digitalWrite(i+12, LOW);
				if (i==3){
				} else if (i==2){
					// C
					muting_tracks=false;
					// unlight all green leds
					for (int j=0;j<16;j++){
						mcp3.digitalWrite(j, LOW);
						bitWrite(grnLedState, j, LOW);
					}
				} else if (i==1){
				} else if (i==0){
				}
			} else {
				// mechanical switch pressed
				mcp4.digitalWrite(i+12, HIGH);
				if (i==3){
					// A
					// start/stop
					if (menu==MENU_STEP_REC){
						// do not mess with lights in step record, they represent steps
					} else {
						// turn off all leds
						for (int j=0;j<16;j++){
							mcp2.digitalWrite(j, LOW);
							bitWrite(redLedState, j, LOW);
							mcp3.digitalWrite(j, LOW);
							bitWrite(grnLedState, j, LOW);
						}
					}
					playMode=!playMode;
					if (playMode){
						// just pressed play
						if (menu==MENU_PATTERN || menu==MENU_STEP_REC || menu==MENU_PATTERN_SETUP){
							// just restart current pattern on pattern screens
							next_step_ms=0;
							next_step_no=0;
							info_display((String)"Started pattern " + (current_pattern+1));
						} else {
							// start song from beginning
							current_song_pattern=0;
							current_song_repeats=1;
							next_step_ms=0;
							next_step_no=0;
							set_current_pattern(SONG[current_song_pattern][0]);
							if (SONG[current_song_pattern][0]>=255){
								// song is empty
								playMode=false;
								info_display((String)"Song is empty");
							} else {
								info_display((String)"Song step=" + (current_song_pattern+1) + ", pattern=" + (current_pattern + 1) + ", repeat=" + current_song_repeats + "/" + SONG[current_song_pattern][1]);
							}
						}
					} else {
						// just pressed stop
						info_display("Stopped");
					}
				} else if (i==2){
					// C
					if (menu==MENU_STEP_REC && menuvar1==2 && menuvar2==-1){
						// do not enter muting mode if already in step enter mode
					} else {
						// mute
						muting_tracks=true;
						info_display("Muting tracks");
						// use green leds
						for (int j=0;j<16;j++){
							if (MUTED_TRACKS[j]){
								mcp3.digitalWrite(j, LOW);
								bitWrite(grnLedState, j, LOW);
							} else {
								mcp3.digitalWrite(j, HIGH);
								bitWrite(grnLedState, j, HIGH);
							}
						}
					}
				} else if (i==1){
					// B
					// cycle through pages for green led cycing
					pattern_page++;
					if ((1+pattern_page)*16>current_pattern_length) pattern_page=0;
					info_display((String)"Page " + (pattern_page+1) + ", steps " + (1+(pattern_page*16)) + "-" + ((1+pattern_page)*16));
					if (menu==MENU_STEP_REC && step_record_track>=0){
						// in step enter mode, refresh green leds for this new page
						for (int j=0;j<16;j++){
							if (PATTERNS[current_pattern][step_record_track][(pattern_page*16)+j]>0){
								// grn leds
								mcp3.digitalWrite(j, HIGH);
								bitWrite(grnLedState, j, HIGH);
							} else {
								mcp3.digitalWrite(j, LOW);
								bitWrite(grnLedState, j, LOW);
							}
						}
					}
				} else if (i==0){
					// D
					if (menu==MENU_TEMPO){
						if (menuvar1==-1){
							// start tap
							tap_tempo_start_ms=millis();
							menuvar1=1;
						} else {
							// end tap
							tempo=60000/(millis() - tap_tempo_start_ms);
							if (tempo<10) tempo=10;
							if (tempo>300) tempo=300;
							menuvar1=-1;
						}
						menu_tempo(-1, -1, 1);
					}
				}
			}
		}
		// keep up with which LEDs are lit
		quadLedState=quadButtonState;
	}

	if (stepButtonState!=stepButtonStateLast){
		for (int i=0;i<16;i++){
			bool thisBit=bitRead(stepButtonState, i);
			bool lastBit=bitRead(stepButtonStateLast, i);
			if (thisBit!=lastBit){
				if (thisBit==false){
					// pb86 button just pressed
					if (muting_tracks){
						// turn muted tracks on/off
						MUTED_TRACKS[i]=!MUTED_TRACKS[i];

						// light unmuted tracks with green on
						if (MUTED_TRACKS[i]){
							// green off
							mcp3.digitalWrite(i, LOW);
							bitWrite(grnLedState, i, LOW);
						} else {
							// green on
							mcp3.digitalWrite(i, HIGH);
							bitWrite(grnLedState, i, HIGH);
						}
					} else if (menu==MENU_STEP_REC && menuvar1==1){
						// selecting step record track
						menuvar2=i;
						menu_step_rec(-1, -1, 1);
					} else if (menu==MENU_STEP_REC && menuvar1==2){
						// entering steps
						if (PATTERNS[current_pattern][step_record_track][(pattern_page*16)+i]>0){
							PATTERNS[current_pattern][step_record_track][(pattern_page*16)+i]=0;
							// grn leds
							mcp3.digitalWrite(i, LOW);
							bitWrite(grnLedState, i, LOW);
						} else {
							PATTERNS[current_pattern][step_record_track][(pattern_page*16)+i]=100;
							mcp3.digitalWrite(i, HIGH);
							bitWrite(grnLedState, i, HIGH);
						}
					} else if (menu==MENU_PATTERN && menuvar1==1){
						// selecting pattern to use
						menuvar2=i;
						menu_pattern(-1, -1, 1);
					} else {
						// here is where pb86 pressed can play a sound
						wavetable[i].playNote(SOUND_MAP[i], VOLUME_MAP[i]);
					}
				}
			}
		}
		stepButtonStateLast=stepButtonState;
	}

	if (menu==MENU_STEP_REC){
		// do not cycle through pattern when in step record menu
	}

	if (true){
		// when no buttons are pressed, cycle through LEDs
		if (thisloop_ms>=next_step_ms){
			if (playMode){
				// perform next step
				for (int i=0;i<16;i++){
					if (PATTERNS[current_pattern][i][next_step_no]>0){
						// here is all pattern/song notes are played
						if (MUTED_TRACKS[i]){
							// do not play, this track is muted
						} else {
							wavetable[i].playNote(SOUND_MAP[i], (PATTERNS[current_pattern][i][next_step_no] * VOLUME_MAP[i])/127);
						}

						// light red led
						mcp2.digitalWrite(i, HIGH);
						bitWrite(redLedState, i, HIGH);
					} else {
						mcp2.digitalWrite(i, LOW);
						bitWrite(redLedState, i, LOW);
					}
				}

				bool show_green_cycling=true;
				if (muting_tracks){
					show_green_cycling=false;
				} else if (menu==MENU_STEP_REC && menuvar1==2){
					// entering steps
					show_green_cycling=false;
				}

				if (show_green_cycling){
					for (int i=0;i<16;i++){
						// grn leds
						if (next_step_no==i+(16*pattern_page)){
							mcp3.digitalWrite(i, HIGH);
							bitWrite(grnLedState, i, HIGH);
						} else {
							mcp3.digitalWrite(i, LOW);
							bitWrite(grnLedState, i, LOW);
						}
					}
				}
			}

			if (playMode){
				// cycle to next LED
				next_step_no++;
				if (next_step_no>(current_pattern_length-1)) {
					next_step_no=0;
					if (menu==MENU_PATTERN || menu==MENU_STEP_REC || menu==MENU_PATTERN_SETUP){
						// stay on current pattern
					} else {
						// next pattern repeat, or next song step
						current_song_repeats++;
						if (current_song_repeats<=SONG[current_song_pattern][1]){
							// still more repeats of current pattern, restart current pattern
							info_display((String)"Song step=" + (current_song_pattern+1) + ", pattern=" + (current_pattern + 1) + ", repeat=" + current_song_repeats + "/" + SONG[current_song_pattern][1]);
						} else {
							// move to next song step
							current_song_pattern++;
							current_song_repeats=1;
							if (SONG[current_song_pattern][0]<current_pattern_length){
								set_current_pattern(SONG[current_song_pattern][0]);
								info_display((String)"Song step=" + (current_song_pattern+1) + ", pattern=" + (current_pattern + 1) + ", repeat=" + current_song_repeats + "/" + SONG[current_song_pattern][1]);
							} else {
								// reached end of song
								playMode=false;
								info_display("Song ended");
							}
						}
					}
				}
			}

			unsigned long tempo_ms=((60000/tempo)/4);
			signed long swing_offset=0;
			if (PATTERNS[current_pattern][16][0]>0){
				if (next_step_no%2==0){
					// for swing - even steps last longer, odd steps last shorter.
					// except... the "next step" is the next step, not this step.
					// so, if the next step is odd, this step is even.
					swing_offset=tempo_ms * PATTERNS[current_pattern][16][0] * -0.01;
				} else {
					swing_offset=tempo_ms * PATTERNS[current_pattern][16][0] * 0.01;
				}
			}
			next_step_ms=thisloop_ms+tempo_ms+swing_offset;
			debug_print(2, (String)next_step_ms + " = " + thisloop_ms + " + " + tempo_ms + " + " + swing_offset);
		}
	}

	// touchscreen
  boolean istouched = ts.touched();

	if (istouched) {
		TS_Point p = ts.getPoint();
		if (thisloop_ms>=next_touched_ms) {
			// scale touched x/y to match the 320x240 tft display grid
			scale_touch(p.x, p.y);

			// this is kind of a hack to keep the screen updates from affecting the beat
			if (thisloop_ms<(next_step_ms-(((60000/tempo)/4)/2))){
				process_touch(scaled_x, scaled_y);
				next_touched_ms=millis()+next_touched_waittime;
			}
		}
	}
}

void OnPress(int key) {
	debug_print(2, (String)"key " + key);
}

void OnNoteOn(byte channel, byte note, byte velocity) {
	if (midi_channel!=channel && midi_channel!=0) return;
	debug_print(1, (String)"NoteOn: channel=" + channel + ",note=" + note + ",velocity=" + velocity);
	wavetable[0].playNote(note, velocity);
}

void OnNoteOff(byte channel, byte note, byte velocity) {
	if (midi_channel!=channel && midi_channel!=0) return;
	debug_print(1, (String)"NoteOff: channel=" + channel + ",note=" + note + ",velocity=" + velocity);
	wavetable[0].stop();
}

void OnControlChange(byte channel, byte control, byte value) {
	if (midi_channel!=channel && midi_channel!=0) return;
	debug_print(1, (String)"CC, channel=" + channel + ",value=" + value);
}

inline float noteToFreq(float note) {
  return 440*pow(2,(note - 69)/12.);
}

