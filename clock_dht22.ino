#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include "DHT.h"
#include <time.h>
#include <hebdate.h>
#include <IRremote.h>
#include <car_remote.h>

#define I2C_ADDR    0x27 // <<----- Add your address here.  Find it from I2C Scanner
#define BACKLIGHT_PIN     3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7
LiquidCrystal_I2C lcd(0x27, En_pin, Rw_pin, Rs_pin, D4_pin, D5_pin, D6_pin, D7_pin, BACKLIGHT_PIN, POSITIVE);
#define LCD_ROWS  2
#define LCD_COLS  16
#define IRpin 11
#define DHTPIN 2 // Data wire is plugged into port 2 on the Arduino
#define DHTTYPE DHT22   // DHT 22  (AM2302)

typedef enum State {INIT_STATE, IDLE_STATE, READ_TIME_STATE, SET_TIME_STATE, READ_TEMP_STATE, TOGGLE_LIGHT_STATE, DIM_BRT_STATE};

typedef enum SET_DATE {DATE, MONTH, YEAR};
typedef enum SET_TIME {SECONDS, MINUTES, HOURS};

#define TEMP_READ_RATE 2
#define TIME_READ_RATE 120
#define SET_TIME_TIMEOUT 4000
#define TOGGLE_HEB_DATE 3

#define DEBUG 0



void setup()
{
#ifdef DEBUG
  Serial.begin(115200);
#endif
}

void loop()
{
  float temperature;
  float humidity;
  char temp_buffer[5];
  static int temp_humid = 0;
  char buffer[80];     // the code uses 70 characters.
  int remote_result = 0;
  long current_time = 0;
  static int backlight_state = HIGH;
  static int backlight_val = 255;
  static IRrecv irrecv(IRpin);
  static decode_results results;
  static State system_state = INIT_STATE;
  static ds1302_struct rtc(5, 6, 7);
  static int_time prev_time, curr_time;
  static SET_TIME set_time_var = HOURS;
  static SET_DATE set_date_var = YEAR;
  static long last_temp_read = 0;
  static long last_time_read = 0;
  static long set_time_press = 0;
  static long set_date_press = 0;
  static int toggle_heb_date = TOGGLE_HEB_DATE;

  static DHT dht(DHTPIN, DHTTYPE);

  static GregorianDate g(1, 1, 2000);
  static long a = g;
  static HebrewDate h(a);

  current_time = millis();

  switch (system_state)
  {
    case INIT_STATE:
      lcd.begin (LCD_COLS, LCD_ROWS);
      lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
      lcd.setBacklight(backlight_state);
      lcd.home (); // go home
      lcd.print("SainSmartI2C16x2");
      delay(100);
      lcd.clear();


      irrecv.enableIRIn(); // Start the receiver
      system_state = IDLE_STATE;
      //system_state = SET_TIME_STATE;
      break;

    case IDLE_STATE:
      remote_result = irrecv.decode(&results);
      if (remote_result)
      {

        switch (results.value)
        {
          case NUM_2_BTN:
            system_state = TOGGLE_LIGHT_STATE;
            break;
          case CH_BTN:
            set_date_press = current_time;
            switch (set_date_var)
            {
              case DATE:
                set_date_var = MONTH;
                break;
              case MONTH:
                set_date_var = YEAR;
                break;
              case YEAR:
                set_date_var = DATE;
                break;
            }
            system_state = SET_TIME_STATE;
            break;
          case CH_PLUS_BTN:
            set_date_press = current_time;
            rtc._DS1302_get_time_and_date(curr_time);
            switch (set_date_var)
            {
              case DATE:
                curr_time.Date = curr_time.Date == numDayInMonth(curr_time.Year + 2000, curr_time.Month) ? 1 : curr_time.Date + 1;
                break;
              case MONTH:
                curr_time.Month = curr_time.Month == 12 ? 1 : curr_time.Month + 1;
                curr_time.Date = curr_time.Date <= numDayInMonth(curr_time.Year + 2000, curr_time.Month) ? curr_time.Date : numDayInMonth(curr_time.Year + 2000, curr_time.Month);
                break;
              case YEAR:
                curr_time.Year = curr_time.Year == 99 ? 0 : curr_time.Year + 1;;
                curr_time.Date = curr_time.Date <= numDayInMonth(curr_time.Year + 2000, curr_time.Month) ? curr_time.Date : numDayInMonth(curr_time.Year + 2000, curr_time.Month);
                break;
            }
            system_state = SET_TIME_STATE;
            break;
          case CH_MINUS_BTN:
            set_date_press = current_time;
            rtc._DS1302_get_time_and_date(curr_time);
            switch (set_date_var)
            {
              case DATE:
                curr_time.Date = curr_time.Date == 1 ? numDayInMonth(curr_time.Year + 2000, curr_time.Month) : curr_time.Date - 1;
                break;
              case MONTH:
                curr_time.Month = curr_time.Month == 1 ? 12  : curr_time.Month - 1;
                curr_time.Date = curr_time.Date <= numDayInMonth(curr_time.Year + 2000, curr_time.Month) ? curr_time.Date : numDayInMonth(curr_time.Year + 2000, curr_time.Month);
                break;
              case YEAR:
                curr_time.Year = curr_time.Year == 0 ? 99 : curr_time.Year - 1;
                curr_time.Date = curr_time.Date <= numDayInMonth(curr_time.Year + 2000, curr_time.Month) ? curr_time.Date : numDayInMonth(curr_time.Year + 2000, curr_time.Month);
                break;
            }
            system_state = SET_TIME_STATE;
            break;
          case EQ_BTN:
            set_time_press = current_time;
            switch (set_time_var)
            {
              case SECONDS:
                set_time_var = HOURS;
                break;
              case MINUTES:
                set_time_var = SECONDS;
                break;
              case HOURS:
                set_time_var = MINUTES;
                break;
            }
            system_state = SET_TIME_STATE;
            break;
          case VOL_PLUS_BTN:
            set_time_press = current_time;
            rtc._DS1302_get_time_and_date(curr_time);
            switch (set_time_var)
            {
              case SECONDS:
                curr_time.Seconds = 0;
                break;
              case MINUTES:
                curr_time.Minutes = curr_time.Minutes == 59 ? 0 : curr_time.Minutes + 1;
                break;
              case HOURS:
                curr_time.Hours = curr_time.Hours == 23 ? 0 : curr_time.Hours + 1;
                break;
            }
            system_state = SET_TIME_STATE;
            break;
          case VOL_MINUS_BTN:
            set_time_press = current_time;
            rtc._DS1302_get_time_and_date(curr_time);
            switch (set_time_var)
            {
              case SECONDS:
                curr_time.Seconds = 0;
                break;
              case MINUTES:
                curr_time.Minutes = curr_time.Minutes == 0 ? 59 : curr_time.Minutes - 1;
                break;
              case HOURS:
                curr_time.Hours = curr_time.Hours == 0 ? 23 : curr_time.Hours - 1;
                break;
            }
            system_state = SET_TIME_STATE;
            break;
        }
        irrecv.resume();   // Receive the next value
      }
      if ((current_time - last_time_read > TIME_READ_RATE) && (IDLE_STATE == system_state))
      {
        last_time_read = current_time;
        system_state = READ_TIME_STATE;
      }
      break;

    case READ_TIME_STATE:
      rtc._DS1302_update();
      rtc._DS1302_get_time_and_date(curr_time);

      rtc._DS1302_get_time(buffer);
      lcd.setCursor (0, 0);
      lcd.print(buffer);
      if (current_time - set_time_press < SET_TIME_TIMEOUT)
      {
        switch (set_time_var)
        {
          case SECONDS:
            lcd.print("S");
            break;
          case MINUTES:
            lcd.print("M");
            break;
          case HOURS:
            lcd.print("H");
            break;
        }
      }
      else
      {
        lcd.print(" ");
      }

      lcd.setCursor (13, 0);
      lcd.print(MonthNames[curr_time.Day]);

      lcd.setCursor (5, 1);
      if (current_time - set_date_press < SET_TIME_TIMEOUT)
      {
        switch (set_date_var)
        {
          case DATE:
            lcd.print("D");
            break;
          case MONTH:
            lcd.print("M");
            break;
          case YEAR:
            lcd.print("Y");
            break;
        }
      }
      else
      {
        lcd.print(" ");
      }
#ifdef DEBUG
      Serial.println(curr_time.Seconds);
#endif
      if (curr_time.Seconds != prev_time.Seconds
          && (current_time - set_date_press > SET_TIME_TIMEOUT) && (current_time - set_time_press > SET_TIME_TIMEOUT) )
      {
        if (toggle_heb_date == -TOGGLE_HEB_DATE)
        {
          toggle_heb_date = TOGGLE_HEB_DATE;
        }
        toggle_heb_date--;
        if (toggle_heb_date >= 0)
        {
          g.SetDate(curr_time.Month, curr_time.Date, 2000 + curr_time.Year);
          a = g;
          h.SetDate(a);
          h.GetDate(buffer);
        }
        else
        {
          rtc._DS1302_get_date(buffer);
        }
        lcd.print(buffer);
      }
      else if ((current_time - set_date_press < SET_TIME_TIMEOUT) || (current_time - set_time_press < SET_TIME_TIMEOUT))
      {
        rtc._DS1302_get_date(buffer);
        lcd.print(buffer);
      }
      if (curr_time.Seconds != prev_time.Seconds && last_temp_read % TEMP_READ_RATE == 0
          && (current_time - set_date_press > SET_TIME_TIMEOUT) && (current_time - set_time_press > SET_TIME_TIMEOUT) )
      {

        system_state = READ_TEMP_STATE;
      }
      else
      {
        system_state = IDLE_STATE;
      }

      if (curr_time.Seconds != prev_time.Seconds)
      {
        last_temp_read++;
      }
      prev_time = curr_time;

      break;

    case SET_TIME_STATE:
      rtc._DS1302_set_date_time(curr_time);
      system_state = READ_TIME_STATE;
      break;

    case READ_TEMP_STATE:
      //sensors.requestTemperatures(); // Send the command to get temperatures
      //temperature = sensors.getTempCByIndex(0);
      lcd.setCursor (0, 1);       // go to start of 2nd line
      if (temp_humid == 0)
      {
        temp_humid = 1;
        temperature = dht.readTemperature();
        ftoa(temp_buffer, temperature,1);
        temp_buffer[4] = (char)223;
        temp_buffer[5] = 0;
      }
      else
      {
        temp_humid = 0;
        humidity = dht.readHumidity();
        ftoa(temp_buffer, humidity,1);
        temp_buffer[4] = '%';
        temp_buffer[5] = 0;
      }
      lcd.print(temp_buffer);
      system_state = IDLE_STATE;
      break;

    case TOGGLE_LIGHT_STATE:
      backlight_state = !backlight_state;
      lcd.setBacklight(backlight_state);
      system_state = IDLE_STATE;
      break;

    case DIM_BRT_STATE:
      lcd.setBacklight(backlight_val);
      system_state = IDLE_STATE;
      break;
  }
}


char *ftoa(char *a, double f, int precision)
{
 long p[] = {0,10,100,1000,10000,100000,1000000,10000000,100000000};
 
 char *ret = a;
 long heiltal = (long)f;
 itoa(heiltal, a, 10);
 while (*a != '\0') a++;
 *a++ = '.';
 long desimal = abs((long)((f - heiltal) * p[precision]));
 itoa(desimal, a, 10);
 return ret;
}
