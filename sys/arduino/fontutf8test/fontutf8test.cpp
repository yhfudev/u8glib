/**
 * @file    fontutf8.cpp
 * @brief   font api test for u8g lib
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @date    2015-04-19
 * @copyright GPL
 */
#define USE_SDL 0
#if defined(ARDUINO)

#if ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#elif defined(U8G_RASPBERRY_PI)
#include <unistd.h>
//#define delay(a) usleep((a) * 1000)

#else
#undef USE_SDL
#define USE_SDL 1
#define delay(a) SDL_Delay((a)*1000)
#endif

//#include <U8glib.h>
#include "U8glib.h"

#include "fontutf8-data.h"

//#include "bleeding-cowboys.h"

#if defined(ARDUINO) || defined(U8G_RASPBERRY_PI)

#define OLED_SPI1_CS    7   //   ---   x Not Connected
#define OLED_SPI1_DC   10   //   D/C   pin# 6 (data or command)
#define OLED_SPI1_RST   2   //   RST   pin# 5 U8G_PIN_NONE
#define OLED_SPI1_MOSI  4   //   SDA   pin# 4
#define OLED_SPI1_CLK   6   //   SCL   pin# 3

// SW SPI Com: SCK = 10, MOSI = 9, CS = 12, A0 = 11, reset=13
//U8GLIB_SH1106_128X64 u8g(OLED_SPI1_CLK, OLED_SPI1_MOSI, OLED_SPI1_CS, OLED_SPI1_DC, OLED_SPI1_RST);

//U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NO_ACK); // I2C
//U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE|U8G_I2C_OPT_DEV_0); // I2C
// 或者
//U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_DEV_0|U8G_I2C_OPT_NO_ACK|U8G_I2C_OPT_FAST); // I2C
U8GLIB_SSD1306_128X64 u8g(OLED_SPI1_CLK, OLED_SPI1_MOSI, OLED_SPI1_CS, OLED_SPI1_DC, OLED_SPI1_RST); // SPI

// the pins
//https://www.youtube.com/watch?v=4CD8ERaylmY
//U8GLIB_ST7920_128X64_1X u8g(13/*SCK=en, E1.5*/, 11/*MOSI, E1.3*/, 12/*CS, E1.4*/);	// SPI Com: SCK = en = 23, MOSI = rw = 17, CS = di = 16
// E1.9 GND
// E1.10 +5V

/*
Arduino digital pin

clockPin --> SCK(EN) Arduino D13
latchPin --> CS(RS)          D12
dataPin --> SID(RW)          D11
*/

#else
// SDL
U8GLIB u8g(&u8g_dev_sdl_2bit);
#endif

void u8g_prepare(void) {
    u8g.setFont(u8g_font_6x10);
    //u8g.setFont(bleeding_cowboys);
    u8g.setFontRefHeightExtendedText();
    u8g.setDefaultForegroundColor();
    u8g.setFontPosTop();
}

void setup(void) {
#if 0
    Serial.begin(9600);
    pinMode(13, OUTPUT);
    digitalWrite(13, HIGH);
#endif
    u8g_prepare();
    u8g_SetUtf8Fonts (g_fontinfo, NUM_ARRAY(g_fontinfo));
}

void u8g_ascii() {
    //char * s1 = "The quick brown";
    char * s1 = "next";
    char * s2 = "fox jumps over the";
    char * s3 = "lazy dog.";
    char buf[20] = _U8GT("ASCII Glyph");
    //sprintf (buf, "u32=%d,w=%d,s=%d",sizeof(uint32_t),sizeof(wchar_t),sizeof(size_t));
    //sprintf (buf, "i=%d,l=%d,u=%d",sizeof(int),sizeof(long),sizeof(unsigned));

    //s1 = buf;
    //s2 = teststrings[cnt];
    //s3 = teststrings[(cnt + 1) % NUM_TYPE(teststrings)];

    u8g.drawStr (1, 18, s1);
    u8g.drawStr (5, 36, s2);
    u8g.drawStr (5, 54, s3);
}

#define NUM_TYPE(a) (sizeof(a)/sizeof(a[0]))
char * teststrings[] = {
    _U8GT("黄沙百戰穿金甲"),
    _U8GT("不破樓蘭終不還"),
    _U8GT("abfgjlpyx"),
    _U8GT("ナイン"),
    _U8GT("セード ンウニユウアレマシタ"),
    _U8GT("セードゼアリマセン"),
    _U8GT("ヅドウセイシ"),
    _U8GT("モーターデンゲン オフ"),
    _U8GT("ゲンテンニイドウ"),
    _U8GT("キヅユンオフセツトセツテイ"),
    _U8GT("キヅユンセツト"),
};

int cnt = 0;
void u8g_chinese() {
    char * s1;
    char * s2;
    char * s3;
    char buf[20] = _U8GT("UTF-8 Glyph");
    //sprintf (buf, "u32=%d,w=%d,s=%d",sizeof(uint32_t),sizeof(wchar_t),sizeof(size_t));
    //sprintf (buf, "i=%d,l=%d,u=%d",sizeof(int),sizeof(long),sizeof(unsigned));

    s1 = buf;
    s2 = teststrings[cnt];
    s3 = teststrings[(cnt + 1) % NUM_TYPE(teststrings)];

    u8g.drawUtf8Str (5, 36, s2);
    u8g.drawUtf8Str (5, 54, s3);
    //sprintf (buf, "ls=%d, wid=%d", u8g.getFontLineSpacing(), u8g.getUtf8StrPixelWidth(s3));
    u8g.drawUtf8Str (1, 18, s1);
}

void draw(void) {
    u8g_chinese();
    //u8g_ascii();
}

// calculate new output values
void uiStep(void) {
#if USE_SDL
    int key = u8g_sdl_get_key();
    switch (key) {
    case 'q':
    case ' ':
        exit(0);
    }
#endif
}

void loop(void) {
    // picture loop
    cnt = (cnt + 1) % NUM_TYPE(teststrings);

    u8g.firstPage();
    do {
        draw();
    } while( u8g.nextPage() );
    uiStep();
    // rebuild the picture after some delay
    delay(500);
}

#if 0 // #if ! defined(ARDUINO)
int
main(void)
{
    setup();
    while (1) {
        loop();
    }
    return 0;
}

#endif
