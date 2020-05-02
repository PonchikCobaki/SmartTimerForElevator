#include <EEPROM.h>
#include <GyverOLED.h>  // вывод на экран


#define TIMER_SEC
//#define TIMER_MIN
//#define TIMER_HOUR

#define OUTPUT_PIN 11   //пин вывода 
#define RESET_PIN 12    //цифровой пин сброса

#define TIME_FOR_FIRST_CLICK 720          // в ед указаных выше
#define TIME_FOR_SECOND_CLICK 2160   
#define TIME_FOR_THIRD_CLICK 4320   
#define TIME_FOR_FOURTH_CLICK 8640   
#define TIME_FOR_FIFTH_CLICK 10800   
#define SAVE_TIME 10                // время сохранения в eeprom память время таймера 
#define OUR_PERIOD_DEBOUNCE 28      // начало преиода с ошибками с конца таймера
#define PERIOD_DEBOUNCE_MIN 1       // мин время ошибки
#define PERIOD_DEBOUNCE_MAX 12      // макс время ошибки

#define EE_CELL_KEY 1023
#define EE_KEY 1

#define OLED_HARD_BUFFER_64     // вывод на экран
//#define OLED_ON                 // вывод на экран
#define TEST_CUR_TIME 690    // для теста
#define SERIAL_TEST

GyverOLED oled;                 // вывод на экран

word wdt;
word hours;
word hours_flag;
word old_hours;
word time_add;
word deb_hour;
bool flag_deb;
int8_t rnd_time_err;

extern volatile unsigned long timer0_millis;

void setup() {
    Serial.begin(9600);
    Serial.println("__________SYSTEM_ON_____________");

#ifdef OLED_ON
    oled.init(OLED128x64, 1200);
    oled.setContrast(126);
#endif

#ifdef TEST_CUR_TIME
    eeprom_write_word(0, 0);
    delay(150);
    EEPROM.put(0, TEST_CUR_TIME);
    delay(150);
#endif

    if (EEPROM.read(EE_CELL_KEY) != EE_KEY) {
        Serial.println("!!!FIRST START!!!");
        EEPROM.put(0, 0);
        EEPROM.put(2, 0);
        EEPROM.write(4, 0);
        EEPROM.put(EE_CELL_KEY, EE_KEY);
    }


    pinMode(RESET_PIN, INPUT_PULLUP);
    pinMode(OUTPUT_PIN, OUTPUT);
    pinMode(13, OUTPUT);
    randomSeed(analogRead(6));

    Serial.print("EEPROM mem time - ");
    EEPROM.get(0, old_hours);
    Serial.println(old_hours);
    Serial.print("EEPROM WDT - ");
    EEPROM.get(2, wdt);
    Serial.println(wdt);
    EEPROM.get(4, flag_deb);

    deb_hour = hours;
    hours_flag = hours - SAVE_TIME;
    Serial.print("delay time ");
    rnd_time_err = random_err_time();

    if (hours < wdt) {
        digitalWrite(OUTPUT_PIN, HIGH);
    }

    timer0_millis = 4294000000;
}

void loop() {
#ifdef OLED_ON
    oled_pr();
#endif

    hour();

    save();

    end();

    debounce();

    res();
}

// Ф. сохранения
void save() {
    if ((hours - hours_flag) >= SAVE_TIME) {
        hours_flag = hours;
        EEPROM.put(0, hours);
        EEPROM.get(0, old_hours);
        Serial.println("SAVED");
    }
}

// Ф. выкл. таймера
void end() {
    if (hours >= wdt) {
        save();
#ifdef OLED_ON
        flag_deb = 0;
        EEPROM.update(4, 0);
        oled_pr();
#endif

        Serial.println("TIMER OFF");
        digitalWrite(OUTPUT_PIN, LOW);

        while (hours >= wdt) {
            res();
        }
        digitalWrite(OUTPUT_PIN, HIGH);
        Serial.println("TIMER ON");
    }
}

// Ф. значения времени
word hour() {
#ifdef TIMER_SEC
    if ( (millis() / 1000ul - time_add) >= 1) {
        hours = old_hours ++;
        save();
#ifdef SERIAL_TEST
    Serial.print("time - ");
    Serial.println(hours);
#endif
    time_add = millis() / 1000ul;
    return hours;
    }
#endif

#ifdef TIMER_MIN
    if (((millis() / 1000ul * 60ul) - time_add) >= 1) {
        hours = old_hours++;
        save();
#ifdef SERIAL_TEST
        Serial.print("time - ");
        Serial.println(hours);
#endif
        time_add = (millis() / 1000ul * 60ul);
        return hours;
    }
#endif

#ifdef TIMER_HOUR
    if (((millis() / 1000ul * 60ul * 60ul) - time_add) >= 1) {
        hours = old_hours++;
        save();
#ifdef SERIAL_TEST
        Serial.print("time - ");
        Serial.println(hours);
#endif
        (millis() / 1000ul * 60ul * 60ul);
        return hours;
    }
#endif
}

// Функция сброса таймера
void res() {
    if (!digitalRead(RESET_PIN) == 1) {
        word request = millis() / 1000ul;
        int8_t flag_click = 0;
        digitalWrite(13, HIGH);
        delay(500);
        while (((millis() / 1000ul) - request) <= 10 && flag_click <= 3) {
            if (!digitalRead(RESET_PIN) == 1) {
                digitalWrite(13, LOW);
                delay(500);
                flag_click++;
                Serial.println(flag_click);
                digitalWrite(13, HIGH);
            }
        }
        digitalWrite(13, LOW);
        delay(500);
        Serial.print("mode ");
        switch (flag_click) {
        case 0:
            wdt = TIME_FOR_FIRST_CLICK;
            Serial.println("1");
            break;
        case 1:
            wdt = TIME_FOR_SECOND_CLICK;
            Serial.println("2");
            break;
        case 2:
            wdt = TIME_FOR_THIRD_CLICK;
            Serial.println("3");
            break;
        case 3:
            wdt = TIME_FOR_FOURTH_CLICK;
            Serial.println("4");
            break;
        case 4:
            wdt = TIME_FOR_FIFTH_CLICK;
            Serial.println("5");
            break;

        }

        hours = 0;
        time_add = 0;
        hours_flag = 0;
        flag_deb = 0;
        old_hours = 0;
        timer0_millis = 0;

        eeprom_write_word(0, 0);
        EEPROM.put(2, wdt);
        EEPROM.write(4, 0);

        Serial.println("RESET");
        digitalWrite(OUTPUT_PIN, HIGH);
    }
}




// Функция барохления под конец срока таймера
void debounce() {
    if (hours + OUR_PERIOD_DEBOUNCE >= wdt) {


        if ((flag_deb == 0) && (hours - deb_hour >= rnd_time_err)) {
            deb_hour = hours;
            flag_deb = 1;
            EEPROM.update(4, 1);

            Serial.print("error time ");
            rnd_time_err = random_err_time();
            Serial.println("error - ON");

            digitalWrite(OUTPUT_PIN, LOW);

            save();
            res();
            end();

#ifdef OLED_ON
            oled_pr();
#endif
        }

        else if ((flag_deb == 1) && (hours - deb_hour >= rnd_time_err)) {
            flag_deb = 0;
            EEPROM.update(4, 0);

            digitalWrite(OUTPUT_PIN, HIGH);

            Serial.println("error - OFF");
            Serial.print("pause time ");
            rnd_time_err = random_err_time();
#ifdef OLED_ON
            oled_pr();
#endif
        }
    }

}

// Ф. случайного времение для ошибки
byte random_err_time() {
    Serial.println(random(PERIOD_DEBOUNCE_MIN, PERIOD_DEBOUNCE_MAX));

    return random(PERIOD_DEBOUNCE_MIN, PERIOD_DEBOUNCE_MAX);
}



// Ф. вкл. экрана для тестов. С ним может быть некорректная работа !
#ifdef OLED_ON
void oled_pr() {
    if ((millis() / 1000ul - time_add) >= 1) {
        oled.clear();
        oled.home();
        oled.scale2X();
        if (hour() >= wdt) {
            oled.println("Таймер");
            oled.println("выключен");
        }
        else {
            oled.print("Время ");
            oled.println((int)hours);
        }
        oled.scale1X();
        oled.print("Задержка ");
        oled.println((int)rnd_time_err - 1);
        oled.print("ошибка ");
        if (flag_deb == 1) {
            oled.println("ВКЛ");
        }
        else {
            oled.println("ВЫКЛ");
        }
        oled.update();
    }
}
#endif