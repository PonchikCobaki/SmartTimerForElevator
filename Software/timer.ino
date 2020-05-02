#include <EEPROM.h>
//#include <GyverOLED.h>  // вывод на экран


//#define TIMER_SEC
//#define TIMER_MIN
#define TIMER_HOUR

#define OUTPUT_PIN 11   //пин вывода 
#define RESET_PIN 12    //цифровой пин сброса

#define WDT 720                     // время таймера в ед укахзанных ранее
#define SAVE_TIME 1                 // время сохранения в eeprom память время таймера 
#define OUR_PERIOD_DEBOUNCE 28      // начало преиода с ошибками с конца таймера
#define PERIOD_DEBOUNCE_MIN 1       // мин время ошибки
#define PERIOD_DEBOUNCE_MAX 12      // макс время ошибки

#define EE_CELL_KEY 1023
#define EE_KEY 1

//#define OLED_HARD_BUFFER_64     // вывод на экран
//#define OLED_ON                 // вывод на экран
//#define TEST_CUR_TIME 690    // для теста

//GyverOLED oled;                 // вывод на экран

word hours;
word hours_flag;
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

    if (EEPROM[EE_CELL_KEY] != EE_KEY) {
        Serial.println("!!!FIRST START!!!");
        EEPROM.write(4, 0);
        eeprom_write_word(0, 0);
        EEPROM.put(EE_CELL_KEY, EE_KEY);
    }



    Serial.print("EEPROM mem time - ");
    EEPROM.get(0, hours);
    
#ifdef TIMER_SEC
    timer0_millis = ((long)hours * 1000ul);
#endif

#ifdef TIMER_MIN
    timer0_millis = (hours * 1000ul * 60ul);
#endif

#ifdef TIMER_HOUR
    timer0_millis = (hours * 1000ul * 60ul * 60ul);
#endif


    pinMode(RESET_PIN, INPUT_PULLUP);
    pinMode(OUTPUT_PIN, OUTPUT);
    randomSeed(analogRead(6));
    
    EEPROM.get(4, flag_deb);

    Serial.println(hours);
    deb_hour = hours;
    hours_flag = hours - SAVE_TIME;
    Serial.print("delay time ");
    rnd_time_err = random_err_time();

    if (hours < WDT) {
        digitalWrite(OUTPUT_PIN, HIGH);
    }
    
}
void loop() {
#ifdef OLED_ON
    oled_pr();
#endif

    hours = hour();

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
        Serial.println("SAVED");
    }
}

// Ф. выкл. таймера
void end() {
    if (hours >= WDT) {
        EEPROM.put(0, WDT);
#ifdef OLED_ON
        flag_deb = 0;
        EEPROM.update(4, 0);
        oled_pr();
#endif

        Serial.println("TIMER OFF");

        while (hours >= WDT) {
            res();
            digitalWrite(OUTPUT_PIN, LOW);
        }
        digitalWrite(OUTPUT_PIN, HIGH);
        Serial.println("TIMER ON");
    }
}

// Ф. значения времени
word hour() {
    //hours = millis() / (1000ul * 60ul * 60ul);
#ifdef TIMER_SEC
    hours = (millis() / 1000ul);
#endif

#ifdef TIMER_MIN
    hours = (millis() / (1000ul * 60ul));
#endif

#ifdef TIMER_HOUR
    hours = (millis() / (1000ul * 60ul * 60ul));
#endif
    Serial.print("time - ");
    Serial.println(hours);
    save();
    return hours;

    /*
    В часах
    hours = millis() / (1000ul * 60ul * 60ul);
    Serial.print(millis()/1000ul);
    Serial.print("-sec | ");
    Serial.print(millis() / (1000ul * 60ul));
    Serial.print("-min | ");
    Serial.print(hours);
    Serial.println("-hour ");

    В мнутах
    hours = millis() / (1000ul * 60ul * 60ul);
    Serial.print(millis()/1000ul);
    Serial.print("-sec | ");
    Serial.print(hours);
    Serial.print("-min | ");
    Serial.print(hours / 60ul);
    Serial.println("-hour ");

    В секундах
    Serial.print(hours);
    Serial.print("-sec | ");
    Serial.print(hours / 60ul);
    Serial.print("-min | ");
    Serial.print(hours / (60ul * 60ul));
    Serial.println("-hour ");

    */

}

// Функция сброса таймера
void res() {
    if (!digitalRead(RESET_PIN) == HIGH) {
        eeprom_write_word(0, 0);
        EEPROM.write(4, 0);
        timer0_millis = 0;
        hours = 0;
        hours_flag = 0;
        flag_deb = 0;
        save();
        Serial.println("RESET");
        digitalWrite(OUTPUT_PIN, HIGH);
    }
}




// Функция барохления под конец срока таймера
void debounce() {
    if (hours + OUR_PERIOD_DEBOUNCE >= WDT) {


        if ((flag_deb == 0) && (hours - deb_hour >= rnd_time_err)){
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

    oled.clear();
    oled.home();
    oled.scale2X();
    if (hour() >= WDT) {
        oled.println("Таймер");
        oled.println("выключен");
    }
    else {
        oled.print("Время ");
        oled.println(hour());
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
    delay(1000);
}
#endif
