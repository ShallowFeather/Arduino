#include <Arduino.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ILI9341.h> // Hardware-specific library
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <Wire.h>
#define TOUCH_CS_PIN D3
#define TOUCH_IRQ_PIN D2

#define TS_MINX 330
#define TS_MINY 213
#define TS_MAXX 3963
#define TS_MAXY 3890

// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate
XPT2046_Touchscreen ts(TOUCH_CS_PIN);

// The display also uses hardware SPI(D5,D6,D7), SD3, D4, D8
static uint8_t SD3 = 10;
#define TFT_CS SD3
#define TFT_DC D4
#define BL_LED D8

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

double answer = 0;
char lastchar = ' ';
String key1;
String key2;
double key1i = 0;
double key2i = 0;
char operation = ' ';
/*long Time = 0;
long millicount = 0;
int interval = 1000;
int screenTimout = 15;*/
bool beentouched = false;
bool toLong = false;
bool equal = false;
bool beep = false;

#define row1x 0
#define boxsize 48

#define r1x 144
#define extray 48

int x, y = 0;

double calc(double num1, double num2, char op);
char idbutton();
bool Tf_check = false;
char pastOp = ' ';
bool key1D = 0, key2D = 0;

char button[6][6] = {
        { '7', '8', '9', '/', '^', 'S' },
        { '4', '5', '6', 'X', 'T', 'L'},
        { '1', '2', '3', '-', '%', '.' },
        { 'C', '0', '=', '+', '<'}
};

void draw()
{
    tft.fillScreen(ILI9341_BLACK);

    tft.drawRoundRect(row1x, extray, boxsize, boxsize, 8, ILI9341_WHITE);
    tft.drawRoundRect(row1x, extray + boxsize, boxsize, boxsize, 8, ILI9341_WHITE);
    tft.drawRoundRect(row1x, extray + boxsize * 2, boxsize, boxsize, 8, ILI9341_WHITE);
    tft.drawRoundRect(row1x, extray + boxsize * 3, boxsize, boxsize, 8, ILI9341_RED);

    for (int b = extray; b <= 320; b += boxsize)
    {
        tft.drawRoundRect  (row1x + boxsize, b, boxsize, boxsize, 8, ILI9341_WHITE);
        tft.drawRoundRect  (row1x + boxsize * 3, b, boxsize, boxsize, 8, ILI9341_BLUE);
        tft.drawRoundRect  (row1x + boxsize * 4, b, boxsize, boxsize, 8, ILI9341_BLUE);
        tft.drawRoundRect  (row1x + boxsize * 5, b, boxsize, boxsize, 8, ILI9341_BLUE);
        tft.drawRoundRect  (row1x + boxsize * 6, b, boxsize, boxsize, 8, ILI9341_BLUE);
    }
    tft.drawRoundRect(row1x + boxsize * 2, extray, boxsize, boxsize, 8, ILI9341_WHITE);
    tft.drawRoundRect(row1x + boxsize * 2, extray + boxsize, boxsize, boxsize, 8, ILI9341_WHITE);
    tft.drawRoundRect(row1x + boxsize * 2, extray + boxsize * 2, boxsize, boxsize, 8, ILI9341_WHITE);
    tft.drawRoundRect(row1x + boxsize * 2, extray + boxsize * 3, boxsize, boxsize, 8, ILI9341_GREEN);

    for (int j = 0; j < 6; j++) {
        for (int i = 0; i < 6; i++) {
            tft.setCursor(16 + (boxsize * i), extray + 12 + (boxsize * j));
            tft.setTextSize(2.5);
            tft.setTextColor(ILI9341_WHITE);
            tft.println(button[j][i]);
        }
    }
    tft.drawRoundRect(0, 0, 320, 48, 8, ILI9341_ORANGE);
    tft.setCursor(4,12);
}

void setup()
{
    key1 = "";
    key2 = "";
    pinMode(BL_LED, OUTPUT);
    digitalWrite(BL_LED, HIGH);
    Serial.begin(9600);
    Serial.println("Calculator");
    ts.begin();
    tft.begin();
    tft.setRotation(3);
    draw();
    tft.setCursor(4, 12);
}

void loop()
{
    if (ts.touched() && !beentouched)
    {
        if(equal)
        {
            equal = false;
            draw();
        }

        if(key1.length()+key2.length()+int(operation != ' ') > 16)
        {
            toLong = true;
        }

        tone(D1, 392);
        TS_Point p = ts.getPoint();     // Read touchscreen
        beentouched = true;

        x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
        y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());


        lastchar = ' ';
        lastchar = idbutton();
        if(lastchar == '<') {
            tft.setCursor(4, 12);
            tft.fillRoundRect(0, 0, 320, 48, 8, ILI9341_BLACK);
            tft.drawRoundRect(0, 0, 320, 48, 8, ILI9341_ORANGE);
            //tft.print(pastOp);
            if (pastOp == ' ') {
                String s = "";
                for(int i = 0; i < key1.length() - 1; i++) {
                    s += key1[i];
                }
                key1 = s;
                tft.print(key1);
            } else {
                if (key2 == "") {
                    operation = ' ';
                    pastOp = ' ';
                    tft.print(key1);
                } else {

                    operation = pastOp;
                    tft.print(key1);
                    tft.print(operation);
                    String s = "";
                    for(int i = 0; i < key2.length() - 1; i++) {
                        s += key2[i];
                    }
                    key2 = s;
                    tft.print(key2);
                }
            }
        }
        else if (lastchar >= '0' && lastchar <= '9' && !toLong || lastchar == '.') {
            if (Tf_check == true) {
                if (lastchar != '1' && lastchar != '2' && lastchar != '3') {
                    tft.setCursor(4, 12);
                    tft.fillRoundRect(0, 0, 320, 48, 8, ILI9341_BLACK);
                    tft.drawRoundRect(0, 0, 320, 48, 8, ILI9341_ORANGE);
                    tft.print("Error");
                    delay(30);
                    key1 = "";
                    key2 = "";
                    answer = 0;

                    key1i = 0;
                    key2i = 0;

                    operation = ' ';
                    draw();
                    tft.fillRoundRect(0, 0, 320, 48, 8, ILI9341_BLACK);
                    tft.drawRoundRect(0, 0, 320, 48, 8, ILI9341_ORANGE);
                    tft.setCursor(4, 12);
                    toLong = false;

                } else {
                    tft.setCursor(4, 12);
                    tft.fillRoundRect(0, 0, 320, 48, 8, ILI9341_BLACK);
                    tft.drawRoundRect(0, 0, 320, 48, 8, ILI9341_ORANGE);
                    tft.print("=");
                    if (lastchar == '1') tft.print(std::sin(key1.toDouble()));
                    else if (lastchar == '2') tft.print(std::cos(key1.toDouble()));
                    else if (lastchar == '3') tft.print(std::tan(key1.toDouble()));
                    key1 = "";
                    key2 = "";
                    answer = 0;

                    key1i = 0;
                    key2i = 0;
                    equal = true;
                    operation = ' ';
                }
                Tf_check = 0;
            } else {
                if (operation == ' ') {
                    if(lastchar == '.') {
                        if(!std::count(key1.begin(), key1.end(), '.')) {
                            key1 += lastchar;
                            tft.print(lastchar);
                        }
                    }
                    else {
                        key1 += lastchar;
                        tft.print(lastchar);
                        Serial.println(key1);
                    }
                } else {
                    if(lastchar == '.') {
                        if(!std::count(key2.begin(), key2.end(), '.')) {
                            key2 += lastchar;
                            tft.print(lastchar);
                        }
                    }
                    else {
                        key2 += lastchar;
                        tft.print(lastchar);
                        Serial.println(key2);
                    }
                }
            }
        }

        else if ((lastchar == '+' || lastchar == '-' || lastchar == '/'
                  || lastchar == 'X' || lastchar == '^' || lastchar == 'T' || lastchar == '%') && key2 == "" && key1 != "" && !toLong) {
            if(lastchar != 'T') pastOp = lastchar;
            if (operation != ' ') {
                operation = lastchar;
                tft.fillRoundRect(0, 0, 320, 48, 8, ILI9341_BLACK);
                tft.drawRoundRect(0, 0, 320, 48, 8, ILI9341_ORANGE);
                tft.setCursor(4, 12);
                tft.print(key1);
                tft.print(operation);
            } else {
                operation = lastchar;
                if (operation == 'T') {
                    tft.setCursor(4, 12);
                    tft.fillRoundRect(0, 0, 320, 48, 8, ILI9341_BLACK);
                    tft.drawRoundRect(0, 0, 320, 48, 8, ILI9341_ORANGE);
                    tft.print("1.sin,2.cos,3.tan");
                    Tf_check = true;
                } else tft.print(operation);
            }
        }

        if (lastchar == 'C') {
            key1 = "";
            key2 = "";
            answer = 0;

            key1i = 0;
            key2i = 0;

            pastOp = ' ';
            operation = ' ';
            draw();
            tft.fillRoundRect(0, 0, 320, 48, 8, ILI9341_BLACK);
            tft.drawRoundRect(0, 0, 320, 48, 8, ILI9341_ORANGE);
            tft.setCursor(4, 12);
            toLong = false;
        }

        if (lastchar == '=') {
            if (key1 != "" && key2 != "") {
                pastOp = lastchar;
                equal = true;
                Serial.println("Calculate");
                if (key2.toDouble() == 0 && operation == '/') {
                    tft.setCursor(4, 12);
                    tft.fillRoundRect(0, 0, 320, 48, 8, ILI9341_BLACK);
                    tft.drawRoundRect(0, 0, 320, 48, 8, ILI9341_ORANGE);
                    tft.print("=");
                    tft.setTextColor(ILI9341_RED);
                    tft.print("ERROR");
                    tft.setTextColor(ILI9341_WHITE);
                    key1 = "";
                    key2 = "";
                    operation = ' ';
                } else {
                    key1i = 0;
                    key2i = 0;
                    key1i = key1.toDouble();
                    key2i = key2.toDouble();
                    answer = calc(key1i, key2i, operation);

                    tft.fillRoundRect(0, 0, 320, 48, 8, ILI9341_BLACK);
                    tft.drawRoundRect(0, 0, 320, 48, 8, ILI9341_ORANGE);
                    tft.setCursor(4, 12);
                    tft.print('=');
                    tft.print(answer);
                    key1i = 0;
                    key2i = 0;
                    operation = ' ';
                    key1 = "";
                    key2 = "";
                }
            }
        }
        if(lastchar == 'S') {
            tft.fillRoundRect(0, 0, 320, 48, 8, ILI9341_BLACK);
            tft.drawRoundRect(0, 0, 320, 48, 8, ILI9341_ORANGE);
            tft.setCursor(4, 12);
            tft.print('=');
            tft.print(std::sqrt(key1.toDouble()));
            key1i = 0;
            key2i = 0;
            operation = ' ';
            key1 = "";
            key2 = "";
            equal = true;
        }
        if(lastchar == 'L') {
            tft.fillRoundRect(0, 0, 320, 48, 8, ILI9341_BLACK);
            tft.drawRoundRect(0, 0, 320, 48, 8, ILI9341_ORANGE);
            tft.setCursor(4, 12);
            tft.print('=');
            tft.print(std::log10(key1.toDouble()));
            key1i = 0;
            key2i = 0;
            operation = ' ';
            key1 = "";
            key2 = "";
            equal = true;
        }
        //wait for release
        while (ts.touched()) {delay(10);};
        beentouched = false;
    }
}

char idbutton()
{
    //Row 1 identification
    if ((x>=0) && (x <= boxsize))
    {
        //    Serial.println("Row 1  ");
        //7
        if (((extray + boxsize) >= y) && (y >= extray))
        {
            tft.drawRoundRect(row1x, extray, boxsize, boxsize, 8, ILI9341_RED);
            delay(100);
            tft.drawRoundRect(row1x, extray, boxsize, boxsize, 8, ILI9341_WHITE);
            return '7';
        }
        //4
        if (((extray + (boxsize * 2)) >= y) && (y >= (extray + boxsize)))
        {
            tft.drawRoundRect(row1x, extray + boxsize, boxsize, boxsize, 8, ILI9341_RED);
            delay(100);
            tft.drawRoundRect(row1x, extray + boxsize, boxsize, boxsize, 8, ILI9341_WHITE);
            return '4';
        }
        //1
        if (((extray + (boxsize * 3)) >= y) && (y >= (extray + (boxsize * 2))))
        {
            tft.drawRoundRect(row1x, extray + boxsize * 2, boxsize, boxsize, 8, ILI9341_RED);
            delay(100);
            tft.drawRoundRect(row1x, extray + boxsize * 2, boxsize, boxsize, 8, ILI9341_WHITE);
            return '1';
        }
        //C
        if (((extray + (boxsize * 4)) >= y) && (y >= (extray + (boxsize * 3))))
        {
            tft.drawRoundRect(row1x, extray + boxsize * 3, boxsize, boxsize, 8, ILI9341_WHITE);
            delay(100);
            tft.drawRoundRect(row1x, extray + boxsize * 3, boxsize, boxsize, 8, ILI9341_RED);
            return 'C';
        }

    }

    //Row 2 identification
    if ((x>=boxsize) && (x <= (boxsize * 2)))
    {
        //    Serial.println("Row 2  ");
        //8
        if (((extray + boxsize) >= y) && (y >= extray))
        {
            tft.drawRoundRect(row1x + boxsize, extray, boxsize, boxsize, 8, ILI9341_RED);
            delay(100);
            tft.drawRoundRect(row1x + boxsize, extray, boxsize, boxsize, 8, ILI9341_WHITE);
            return '8';
        }
        //5
        if (((extray + (boxsize * 2)) >= y) && (y >= (extray + boxsize)))
        {
            tft.drawRoundRect(row1x + boxsize, extray + boxsize, boxsize, boxsize, 8, ILI9341_RED);
            delay(100);
            tft.drawRoundRect(row1x + boxsize, extray + boxsize, boxsize, boxsize, 8, ILI9341_WHITE);
            return '5';
        }
        //2
        if (((extray + (boxsize * 3)) >= y) && (y >= (extray + (boxsize * 2))))
        {
            tft.drawRoundRect(row1x + boxsize, extray + boxsize * 2, boxsize, boxsize, 8, ILI9341_RED);
            delay(100);
            tft.drawRoundRect(row1x + boxsize, extray + boxsize * 2, boxsize, boxsize, 8, ILI9341_WHITE);
            return '2';
        }
        //0
        if (((extray + (boxsize * 4)) >= y) && (y >= (extray + (boxsize * 3))))
        {
            tft.drawRoundRect(row1x + boxsize, extray + boxsize * 3, boxsize, boxsize, 8, ILI9341_RED);
            delay(100);
            tft.drawRoundRect(row1x + boxsize, extray + boxsize * 3, boxsize, boxsize, 8, ILI9341_WHITE);
            return '0';
        }
    }

    //Row 3 identification
    if ((x>=(boxsize * 2)) && (x <= (boxsize * 3)))
    {
        //    Serial.println("Row 3  ");
        //9
        if (((extray + boxsize) >= y) && (y >= extray))
        {
            tft.drawRoundRect(row1x + boxsize * 2, extray, boxsize, boxsize, 8, ILI9341_RED);
            delay(100);
            tft.drawRoundRect(row1x + boxsize * 2, extray, boxsize, boxsize, 8, ILI9341_WHITE);
            return '9';
        }
        //6
        if (((extray + (boxsize * 2)) >= y) && (y >= (extray + boxsize)))
        {
            tft.drawRoundRect(row1x + boxsize * 2, extray + boxsize, boxsize, boxsize, 8, ILI9341_RED);
            delay(100);
            tft.drawRoundRect(row1x + boxsize * 2, extray + boxsize, boxsize, boxsize, 8, ILI9341_WHITE);
            return '6';
        }
        //3
        if (((extray + (boxsize * 3)) >= y) && (y >= (extray + (boxsize * 2))))
        {
            tft.drawRoundRect(row1x + boxsize * 2, extray + boxsize * 2, boxsize, boxsize, 8, ILI9341_RED);
            delay(100);
            tft.drawRoundRect(row1x + boxsize * 2, extray + boxsize * 2, boxsize, boxsize, 8, ILI9341_WHITE);
            return '3';
        }
        //=
        if (((extray + (boxsize * 4)) >= y) && (y >= (extray + (boxsize * 3))))
        {
            tft.drawRoundRect(row1x + boxsize * 2, extray + boxsize * 3, boxsize, boxsize, 8, ILI9341_BLUE);
            delay(100);
            tft.drawRoundRect(row1x + boxsize * 2, extray + boxsize * 3, boxsize, boxsize, 8, ILI9341_GREEN);
            return '=';
        }

    }

    //Row 4 identification
    if ((x>=(boxsize * 3)) && (x <= (boxsize * 4)))
    {
        //    Serial.println("Row 4  ");
        //+
        if (((extray + boxsize) >= y) && (y >= extray))
        {
            tft.drawRoundRect(row1x + boxsize * 3, extray, boxsize, boxsize, 8, ILI9341_GREEN);
            delay(100);
            tft.drawRoundRect(row1x + boxsize * 3, extray, boxsize, boxsize, 8, ILI9341_BLUE);
            return '/';
        }
        //-
        if (((extray + (boxsize * 2)) >= y) && (y >= (extray + boxsize)))
        {
            tft.drawRoundRect(row1x + boxsize * 3, extray + boxsize, boxsize, boxsize, 8, ILI9341_GREEN);
            delay(100);
            tft.drawRoundRect(row1x + boxsize * 3, extray + boxsize, boxsize, boxsize, 8, ILI9341_BLUE);
            return 'X';
        }
        //*
        if (((extray + (boxsize * 3)) >= y) && (y >= (extray + (boxsize * 2))))
        {
            tft.drawRoundRect(row1x + boxsize * 3, extray + boxsize * 2, boxsize, boxsize, 8, ILI9341_GREEN);
            delay(100);
            tft.drawRoundRect(row1x + boxsize * 3, extray + boxsize * 2, boxsize, boxsize, 8, ILI9341_BLUE);
            return '-';
        }
        // /
        if (((extray + (boxsize * 4)) >= y) && (y >= (extray + (boxsize * 3))))
        {
            tft.drawRoundRect(row1x + boxsize * 3, extray + boxsize * 3, boxsize, boxsize, 8, ILI9341_GREEN);
            delay(100);
            tft.drawRoundRect(row1x + boxsize * 3, extray + boxsize * 3, boxsize, boxsize, 8, ILI9341_BLUE);
            return '+';
        }
    }

    if ((x>=(boxsize * 4)) && (x <= (boxsize * 5)))
    {
        //    Serial.println("Row 4  ");
        //+
        if (((extray + boxsize) >= y) && (y >= extray))
        {
            tft.drawRoundRect(row1x + boxsize * 4, extray, boxsize, boxsize, 8, ILI9341_GREEN);
            delay(100);
            tft.drawRoundRect(row1x + boxsize * 4, extray, boxsize, boxsize, 8, ILI9341_BLUE);
            return '^';
        }
        //-
        if (((extray + (boxsize * 2)) >= y) && (y >= (extray + boxsize)))
        {
            tft.drawRoundRect(row1x + boxsize * 4, extray + boxsize, boxsize, boxsize, 8, ILI9341_GREEN);
            delay(100);
            tft.drawRoundRect(row1x + boxsize * 4, extray + boxsize, boxsize, boxsize, 8, ILI9341_BLUE);
            return 'T';
        }
        //*
        if (((extray + (boxsize * 3)) >= y) && (y >= (extray + (boxsize * 2))))
        {
            tft.drawRoundRect(row1x + boxsize * 4, extray + boxsize * 2, boxsize, boxsize, 8, ILI9341_GREEN);
            delay(100);
            tft.drawRoundRect(row1x + boxsize * 4, extray + boxsize * 2, boxsize, boxsize, 8, ILI9341_BLUE);
            return '%';
        }
        // /
        if (((extray + (boxsize * 4)) >= y) && (y >= (extray + (boxsize * 3))))
        {
            tft.drawRoundRect(row1x + boxsize * 4, extray + boxsize * 3, boxsize, boxsize, 8, ILI9341_GREEN);
            delay(100);
            tft.drawRoundRect(row1x + boxsize * 4, extray + boxsize * 3, boxsize, boxsize, 8, ILI9341_BLUE);
            return '<';
        }
    }
    if ((x>=(boxsize * 5)) && (x <= (boxsize * 6)))
    {
        //    Serial.println("Row 4  ");
        //+
        if (((extray + boxsize) >= y) && (y >= extray))
        {
            tft.drawRoundRect(row1x + boxsize * 5, extray, boxsize, boxsize, 8, ILI9341_GREEN);
            delay(100);
            tft.drawRoundRect(row1x + boxsize * 5, extray, boxsize, boxsize, 8, ILI9341_BLUE);
            return 'S';
        }
        //-
        if (((extray + (boxsize * 2)) >= y) && (y >= (extray + boxsize)))
        {
            tft.drawRoundRect(row1x + boxsize * 5, extray + boxsize, boxsize, boxsize, 8, ILI9341_GREEN);
            delay(100);
            tft.drawRoundRect(row1x + boxsize * 5, extray + boxsize, boxsize, boxsize, 8, ILI9341_BLUE);
            return 'L';
        }
        //*
        if (((extray + (boxsize * 3)) >= y) && (y >= (extray + (boxsize * 2))))
        {
            tft.drawRoundRect(row1x + boxsize * 5, extray + boxsize * 2, boxsize, boxsize, 8, ILI9341_GREEN);
            delay(100);
            tft.drawRoundRect(row1x + boxsize * 5, extray + boxsize * 2, boxsize, boxsize, 8, ILI9341_BLUE);
            return '.';
        }
        // /
        if (((extray + (boxsize * 4)) >= y) && (y >= (extray + (boxsize * 3))))
        {
            tft.drawRoundRect(row1x + boxsize * 5, extray + boxsize * 3, boxsize, boxsize, 8, ILI9341_GREEN);
            delay(100);
            tft.drawRoundRect(row1x + boxsize * 5, extray + boxsize * 3, boxsize, boxsize, 8, ILI9341_BLUE);
            return '<';
        }
    }
    return 'a';
}

double calc(double num1, double num2, char op)
{
    switch (op) {
        case '+':
            return num1 + num2;
        case '-':
            return num1 - num2;
        case 'X':
            return num1 * num2;
        case '/':
            return num1 / num2;
        case '^':
            return std::pow(num1, num2);
        case '%':
            return int(num1) % int(num2);
    }
    return 1;
}