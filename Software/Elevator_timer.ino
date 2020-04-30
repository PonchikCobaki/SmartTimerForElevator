#include <EEPROM.h>
#include <GyverOLED.h>  // ����� �� �����


#define TIMER_SEC
//#define TIMER_MIN
//#define TIMER_HOUR

#define OUTPUT_PIN 11   //��� ������ 
#define RESET_PIN 12    //�������� ��� ������

#define WDT 720                     // ����� ������� � �� ���������� �����
#define SAVE_TIME 1                 // ����� ���������� � eeprom ������ ����� ������� 
#define OUR_PERIOD_DEBOUNCE 28      // ������ ������� � �������� � ����� �������
#define PERIOD_DEBOUNCE_MIN 1       // ��� ����� ������
#define PERIOD_DEBOUNCE_MAX 12      // ���� ����� ������

#define EE_CELL_KEY 1023
#define EE_KEY 1

#define OLED_HARD_BUFFER_64     // ����� �� �����
#define OLED_ON                 // ����� �� �����
//#define TEST_CUR_TIME 692     // ��� �����

GyverOLED oled;                 // ����� �� �����

word hours;
word hours_flag;
word deb_hour;
bool flag_deb;
byte rnd_time_err;

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

    Serial.print("EEPROM mem time - ");

    pinMode(RESET_PIN, INPUT_PULLUP);

    randomSeed(analogRead(6));

    if (EEPROM[EE_CELL_KEY] != EE_KEY) {
        Serial.println("!!!FIRST START!!!");
        EEPROM.write(4, 0);
        eeprom_write_word(0, 0);
        EEPROM.put(EE_CELL_KEY, EE_KEY);
    }
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
    hours_flag = hours - SAVE_TIME;

    EEPROM.get(4, flag_deb);

    Serial.println(hours);
    deb_hour = hours;
    Serial.print("delay time ");
    rnd_time_err = random_err_time();
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

    digitalWrite(OUTPUT_PIN, HIGH);
}

// �. ����������
void save() {
    if ((hours - hours_flag) >= SAVE_TIME) {
        hours_flag = hours;
        EEPROM.put(0, hours);
        Serial.println("SAVED");
    }
}
// �. ����. �������
bool end() {
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

// �. �������� �������
word hour() {
    hours = millis() / (1000ul * 60ul * 60ul);
#ifdef TIMER_SEC
    hours = (millis() / 1000ul);
#endif

#ifdef TIMER_MIN
    hours = (millis() / (1000ul * 60ul));
#endif

#ifdef TIMER_HOUR
    hours = (millis() / (1000ul * 60ul * 60ul));
#endif
    // Serial.print("time - ");
    // Serial.println(hours);
    save();
    return hours;

    /*
    � �����
    hours = millis() / (1000ul * 60ul * 60ul);
    Serial.print(millis()/1000ul);
    Serial.print("-sec | ");
    Serial.print(millis() / (1000ul * 60ul));
    Serial.print("-min | ");
    Serial.print(hours);
    Serial.println("-hour ");

    � ������
    hours = millis() / (1000ul * 60ul * 60ul);
    Serial.print(millis()/1000ul);
    Serial.print("-sec | ");
    Serial.print(hours);
    Serial.print("-min | ");
    Serial.print(hours / 60ul);
    Serial.println("-hour ");

    � ��������
    Serial.print(hours);
    Serial.print("-sec | ");
    Serial.print(hours / 60ul);
    Serial.print("-min | ");
    Serial.print(hours / (60ul * 60ul));
    Serial.println("-hour ");

    */

}

// ������� ������ �������
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
    }
}

// ������� ���������� ��� ����� ����� �������
void debounce() {
    if (hours + OUR_PERIOD_DEBOUNCE >= WDT) {


        if (flag_deb == 0) {
            deb_hour = hours;
            flag_deb = 1;

#ifdef OLED_ON
            oled_pr();
#endif

            Serial.print("error time ");
            rnd_time_err = random_err_time();

            EEPROM.update(4, 1);

            Serial.println("error - ON");

            digitalWrite(OUTPUT_PIN, LOW);

            while (((hour() - (long)rnd_time_err) <= deb_hour) && (hour() <= WDT)) {
                save();
                res();
                end();

#ifdef OLED_ON
                oled_pr();
#endif

            }
            deb_hour = hours;
            digitalWrite(OUTPUT_PIN, HIGH);

            Serial.println("error - OFF");
            Serial.print("delay time ");
            rnd_time_err = random_err_time();
        }

        else if ((flag_deb == 1) && (hours - deb_hour >= rnd_time_err)) {
            flag_deb = 0;
            EEPROM.update(4, 0);
            Serial.print("delay time ");
            rnd_time_err = random_err_time();
        }
    }

}

// �. ���������� �������� ��� ������
byte random_err_time() {
    Serial.print("random number - ");
    Serial.println(random(PERIOD_DEBOUNCE_MIN, PERIOD_DEBOUNCE_MAX));

    return random(PERIOD_DEBOUNCE_MIN, PERIOD_DEBOUNCE_MAX);
}


// �. ���. ������ ��� ������. � ��� ����� ���� ������������ ������ !
#ifdef OLED_ON
void oled_pr() {
    oled.clear();
    oled.home();
    oled.scale2X();
    if (hour() >= WDT) {
        oled.println("������ ���");
    }
    else {
        oled.print("����� ");
        oled.println(hour());
    }
    oled.scale1X();
    oled.print("�������� ");
    oled.println(rnd_time_err);
    oled.print("������ ");
    if (flag_deb == 1) {
        oled.println("���");
    }
    else {
        oled.println("����");
    }
    oled.update();
    delay(1000);

}
#endif