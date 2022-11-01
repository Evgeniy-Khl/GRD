/*****************************************************
Project : GRD-2021

Chip type           : ATmega8
PCB                 : RKlimat2021
Clock frequency     : 4,000000 MHz

*****************************************************/ 
#define GRD_001    // ГРД-1 сушильная камера DS18B20 + AM2301 + УВЛАЖНИТЕЛЬ: Program size: 4007 words (8014 bytes), 97,8% of FLASH [0x0AA2] EEPROM [0х0557] 01.11.2022
//#define GRD_002    // ГРД-1 сушильная камера DS18B20 + AM2301 + ЕЛЕКТРОСТАТИКА: Program size: 4060 words (8120 bytes), 99,1% of FLASH [0x6549] EEPROM [0x0782] 09.08.2022

#include "brend.h"
#include <mega8.h>
#include <stdio.h>
#include <delay.h>
#include <1WIREnew.h>

//**************************
#define CN1             PORTC.0 // симистор / реле (нагрев / охлаждение)
#define CN2             PORTC.1 // реле (вентилятор)
#define CN3             PORTC.2 // транзистор (увлажнитель / электростатика)
#define CN4             PORTC.3 // транзистор (ПИД сигнал)
#define BEEPER          PORTC.4 // зуммер
#define KEYPAD_PIN      PINC.5  // клавиатура
#ifdef  KTY84
  #define ADC_VREF_TYPE   0xC0    // KTY84
#endif

#define PRESET0         100     // (256 - n)*64*0.25= 2496 us (2.5 mS) длительность индикации одной цифры (209)
#define PRESET2         100     // (256 - n)*128*0.25= 4992 us (5.0 mS)
#define WAITCOUNT       20     // интервал опроса кнопок
#define DISPLprg        55     //

#define UNCHANGED       2
#define ON              1
#define OFF             0
#define ONmosfet        0
#define OFFmosfet       1

#define BL              0xFF     // пусто
#define DEF             0xBF     // -
#define D_F             0xF7     // _
#define TT              0x87     // t
#define LL              0x87     // l
#define EE              0x86     // E
#define FF              0x8E     // F
#define RS              0xAF     // r
#define DD              0xA1     // d
#define NN              0xAB     // n
#define HH              0x8B     // h
#define UU              0xE3     // u
#define OO              0xA3     // o
#define SS              0xC6     // С
#define GG              0xC2     // G
#define CC              0xA7     // c
#define PT              0xC8     // П
#define GE              0xCE     // Г
#define YY              0x91     // У
#define RT              0x8C     // P
#define Pct             0x9C     // %

// 1 Wire Bus functions 
#asm
   .equ __w1_port=0x12 ;PORTD
   .equ __w1_bit=6
#endasm

// Declare your global variables here
//flash float A1=1.8, A2=0.81, A3=0.01;  // порядок a=0.9 (A1=2a; A2=a^2; A3=(1-a)^2)
//flash float A1=1.6, A2=0.64, A3=0.04;  // порядок a=0.8 (A1=2a; A2=a^2; A3=(1-a)^2)
#ifdef KTY84
  flash float A1=1.2, A2=0.36, A3=0.16;  // порядок a=0.6 (A1=2a; A2=a^2; A3=(1-a)^2)
#endif
//flash float A1=0.8, A2=0.16, A3=0.36;  // порядок a=0.4 (A1=2a; A2=a^2; A3=(1-a)^2)
//--------------------- 0    1    2    3    4    5    6    7    8    9    A    B     C    D    E
flash char digit[15]={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,0x86,0x8E,0xA3,0xAB,0xBF};
//------- Regisr region -----------------------------------
unsigned char countled, led=1, counter, counter1, counter2, keydata, mode;
//---------------------------------------------------------
unsigned char displ, ds18b20, waitmode, waitkey=WAITCOUNT, counthum, BeepT, lock, InsideHeatON, errors, maxUser, maxOwner;
unsigned char displ_buffer[6]={FIRSTBR,SECONDBR,THERDBR,Pct,Pct,Pct}, familycode[3][9], wait[3], waitStart;
signed char countsec, countmin;
signed int pvT[3]={199,199,199}, pvT0, buf;
#ifdef ELECTROSTAT
 signed int pvTimer, wsElstat;
#endif
 unsigned char pwTriac0;

//--- union declaration
  union 
    {
      unsigned char ds_buffer[8];
      signed int  intval;
    } data;
#ifdef KTY84
  float PVold1, PVold2;
#else
  signed char lost, pvRH=125;
#endif

eeprom signed int spT1[4]={40,81,82,83};              // Уставка температуры воздуха для DS18B20 
eeprom signed int spT2[4]={50,71,72,73};              // Уставка температуры среды для DS18B20
eeprom signed int spT3[4]={40,61,62,63};              // Уставка температуры влажного датчика KTY84
eeprom unsigned int SpTmr[4]={0,05,06,30};            // Продолжительность операции
eeprom unsigned char timeCool[4]={0,0,2,3};           // Длительность продувки
#ifdef ELECTROSTAT
  eeprom signed int maxSP[4]={35,130,130,130};        // Ограничитель максимальной температуры
#else
  eeprom signed int maxSP=130;       // Ограничитель максимальной температуры
#endif
eeprom unsigned char OvHeat=8,Hyst=5,Step=0;          // Тревога; Гистерезис; Шаг программы;
eeprom unsigned char Program=0, LastProg=1;           // Выполняемая программа, прошлая программа;
eeprom unsigned int Timer=0;                          // Таймер - текущее время операции
eeprom unsigned char DimTmr=1;                        // Размерность таймера
eeprom unsigned char Dim=0;                           // 
eeprom unsigned char koff[2]={10,500};                // [0]-cof->K; [1]-cof->Ti
eeprom unsigned char reverse=1;                       // инверсия реле нагрева
eeprom unsigned char Priority=0;                      // приоритет датчиков
eeprom unsigned char coolMax=40;                      // температура с которой ЗАПРЕЩЕНО включение охлаждения
#ifdef KTY84
 eeprom signed char rider[2]={20,-10};                // коррекция датчиков температуры Bias=rider[0], koff=rider[1];
#else
 eeprom signed char rider[2]={0,0};                   // коррекция датчиков температуры Bias=rider[0], koff=rider[1];
#endif
#ifdef ELECTROSTAT
 eeprom unsigned int ocra1=250;                       // Clock value: 62500 kHz / 250 Hz = 250 = ocra1
 eeprom unsigned int ocrb1=50;                        // Заполнение 50% ocra1 / 2 = 125 = ocrb1
 eeprom unsigned int timerMain[3]={10,20,30};         // таймеры электростатики ([0]-задержка; [1]-работа; [2]-пауза;)
#else
 eeprom unsigned char humidMin=60;                    // температура с которой начинается увлажнени 
 eeprom unsigned char timerHum[2]={40,0};             // Длительность [0]-паузы / [1]-впрыска увлажнителя
#endif

eeprom unsigned char numPrg = 3;                      // количество доступных программ

bit SetUp;
bit Check;
bit Heat;
bit ToInsideHeat; // отсчет по температуре продукта.
bit Cooling;
bit KeyDown;
bit Sensor;
bit ElStat;

// Timer 0 overflow interrupt service routine
interrupt [TIM0_OVF] void timer0_ovf_isr(void)
{
 unsigned char pD;
  TCNT0 = PRESET0;                      // (2,5 mS) длительность индикации одной цифры
  PORTB = 0xFF;                         // гасим цифру
  pD = PORTD|0x3F;                      // ТОЛЬКО ТАК !!!
  pD &= ~led;                           // ИНАЧЕ НЕЧИТАЕТ DS18B20 !!!
  PORTD = pD;                           // зажигаем цифру
  if(BeepT) {--BeepT; BEEPER=ON;}       // включить серену  1
  else BEEPER=OFF;                      // отключить серену 0
  led <<= 1;                            // поготовим следующию цифру дисплея
  PORTB = displ_buffer[countled];
  if (!KEYPAD_PIN) {keydata = countled; KeyDown=1;} // если нажата кнопка то ...
  if (++countled == 6) {if(!KeyDown){keydata = 6;} KeyDown=0; countled=0; led=1;}// новый цикл
}
#ifdef ELECTROSTAT
// Timer 1 output compare A interrupt service routine
interrupt [TIM1_COMPA] void timer1_compa_isr(void){
  CN3 = ONmosfet;
}
// Timer 1 output compare B interrupt service routine
interrupt [TIM1_COMPB] void timer1_compb_isr(void){
  CN3 = OFFmosfet;
}
#endif
// Timer 2 overflow interrupt service routine
interrupt [TIM2_OVF] void timer2_ovf_isr(void)  // (5,0 mS)
{
  TCNT2 = PRESET2; ++counter; ++counter1; ++counter2;
  if (pwTriac0) --pwTriac0;
  else CN4 = OFF;                    // отключить канал 4 (SSR-25DA)
}
#ifdef KTY84
 #include "adc_proc.c"
#else
 #include "dht11.c"
#endif
#include "proc.c"
#include "displ.c"
#ifdef ELECTROSTAT
 #include "keypad2.c"
#else
 #include "keypad1.c"
#endif
#include "pi.c"

void main(void)
{
// Declare your local variables here
signed char byte;
#include "Init.c"

while (1)
  {
    if(counter > 200) {counter=0; Check = 1;}  // 1 sec.
    if(counter2 > waitkey) {counter2=0; byte=checkkey();};
#ifndef ELECTROSTAT
// ----------------- УВЛАЖНИТЕЛЬ -------------------------------------
    if(Heat){
      if(counter1>49){
        counter1=0; 
        byte=humidifier();
        switch (byte){
           case ON:  CN3 = ON;  break;
           case OFF: CN3 = OFF; break;
        }; 
      }
     }
    else CN3 = OFF;
#endif
    if(mode) display_setup();  // режим установок
//------- Начало проверки каждую 1 сек. --------------------------------------------------
    if(Check){
      #asm("wdr")
      Check = 0;
      if(waitmode) {if(--waitmode==0) {if (SetUp) saveset(); else displ=0;};};
      //----------- DS18B20 --------------------------------------------------
      if(ds18b20) errors=temperature_check();
#ifdef KTY84 //----------- KTY84 --------------------------------------------------
      if(lock){--lock; BeepT=50;}
      if(Sensor)
       {
         pvT[2] = calc_t(LowPassF2(read_adc(6)));   // значение температуры KTY84
         //pvT[2] = LowPassF2(read_adc(6));           // только для поиска неисправности
         switch (ds18b20)// ПОДМЕНА датчиков температуры 
          {
            case 0: pvT[0]=pvT[2]; pvT[1]=pvT[2]; break;
            case 1: if((pvT[0]==199)) pvT[0]=pvT[2]; break;
            case 2: if((pvT[0]==199)) pvT[0]=pvT[1];
                    if((pvT[0]==199)) pvT[0]=pvT[2]; break;
          };
       }
#else       //----------- DHT-21 --------------------------------------------------
      if(Sensor){
         byte=reaDHT();
         if(byte==0){if(++lost>3){pvRH=125; pvT[2]=199; errors |=0x04; lost=4;}}// потерян датчик DHT-21
         else {
           lost=0;
           switch (ds18b20){// ПОДМЕНА датчиков температуры 
              case 0: pvT[0]=pvT[2]; pvT[1]=pvT[2]; pvT[2]=pvRH; break;
              case 1: pvT[1]=pvT[2]; pvT[2]=pvRH; if((pvT[0]==199)) pvT[0]=pvT[2]; break;
              case 2: if((pvT[0]==199)) pvT[0]=pvT[1];
                      if((pvT[0]==199)) pvT[0]=pvT[2];
                      else pvT[2]=pvRH;
                      break;
           };
         }
      };
#endif       
//--------------------------------------------------- НАГРЕВ ВКЛЮЧЕН ----------------------------------------------------------------------------                       
      if(Heat){
         if(errors) BeepT=120;  // Ошибка датчиков
         if(ToInsideHeat){      // если завершение программы по температуре продукта.          
            if(++countsec>59){countsec=0; Timer++; if(Timer==1) Dim=1;}
            if(InsideHeatON==OFF){Heat=OFF; if(timeCool[Step])Cooling=ON; Timer=timeCool[Step]; BeepT=255; countsec=0;}// окончание приготовления среды
         }
         else if(--countsec<0){  // если завершение программы по таймеру
           switch (Dim){
              case 2:      // часы
               {
                 countsec=59; displ=0;
                 if(Timer>2) {if(--countmin<0) {countmin=59; Timer--;}}
                 else {if(--countmin<0) {countmin=0; Timer=59; Dim=1;}};
               }; break;
              case 1:     // минуты
               {
                 countsec=59; displ=0;
                 if(Timer>2) Timer--; else {Timer=0; Dim=0;}
               }; break;
              case 0: Heat=OFF; if(timeCool[Step]){Cooling=ON; Timer=timeCool[Step];} else if(Program==0) lock=30;  BeepT=255; countsec=0; break;
           }; 
         }
         byte = (int)spT1[Step] - pvT[0] + OvHeat;     // величина ошибки регулирования
         if(byte<0) BeepT=255;                         // ПЕРЕГРЕВ
#ifdef ELECTROSTAT
         if(Step==0){if(wsElstat) wsElstat--; else electrostat();}
         else {TIMSK&=0xEF;}
#endif
         // ----------------------------------------НАГРЕВАТЕЛЬ / ОХЛАДИТЕЛЬ -------------------------------------
         //------ работает как нагреватель
         if(pvT[0]<199 && pvT[0]>1) byte = Relay((int)spT1[Step] - pvT[0], Hyst);  // температура воздуха
         else if(ds18b20>1 && pvT[1]<199){// если датчик потерян то опираемся на температура среды
           byte = Relay((int)spT2[Step] - pvT[1], Hyst);
         }
         else byte = OFF;
         if(ds18b20>1 && pvT[1]<199) InsideHeatON = Relay((int)spT2[Step] - pvT[1], 0); // температура среды
         if(InsideHeatON == OFF) byte = OFF;   // если работает как нагреватель то отключаем !!!
         else {
           pwTriac0 = UpdatePID();             // ПИД нагреватель 
           if(pwTriac0) CN4 = ON;
         }
         //------ работает как охладитель
         if(reverse){  
             byte = Relay((int)pvT[0] - spT1[Step], Hyst);
             if(pvT[0] > coolMax) byte = OFF;
         }
         switch (byte){
           case ON:  CN1 = ON;  break;
           case OFF: CN1 = OFF; break;
         }
         CN2 = ON;    // таймер включен 
      }
//---- ПРОДУВКА -----------------------------------------------------------------------------------------------
      else if(Cooling){// продувка
         CN1 = OFF; CN2 = ON; ElStat = OFF;
#ifdef ELECTROSTAT
         TIMSK&=0xEF; // Compare A: Off; Compare B: On
#endif
         if(--countsec<0){
            countsec=59; displ=0; 
            if(Timer) Timer--;
            else if(Program) nextPrg();
            else {Cooling=OFF; Step=0; countsec=0; lock=30;}
         }
      }
      else if(Program && waitStart==0) nextPrg();
//---- ВСЕ ОТКЛЮЧЕНО ------------------------------------------------------------------------------------------
      else {
        CN1 = OFF; CN2 = OFF; ElStat = OFF;
#ifdef ELECTROSTAT 
        TIMSK&=0xEF; // Compare A: Off; Compare B: On
#endif
      }// все отключено;  
      if(waitStart) {LeftPrg(); if(--waitStart==0){LastProg=Program; if(Program>3) Program<<=4; nextPrg();}} 
      else if(mode==0) display();
     };
//------ Конец проверки каждую 1 сек. -----------------------------------------------------
  };
}
