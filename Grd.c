/*****************************************************
Project : GRD-2021

Chip type           : ATmega8
PCB                 : RKlimat2021
Clock frequency     : 4,000000 MHz

*****************************************************/ 
#define GRD_001    // ГРД-1 сушильная камера с увл. DS18B20 + AM2301                  Program size: 4022 words (8044 bytes), 98,2% of FLASH [0х27BD] EEPROM [0х0530] 23.11.2021
//#define GRD_002    // ГРД-1 сушильная камера с увл. DS18B20 + AM2301 + ЕЛЕКТРОСТАТИКА Program size: 3499 words (6998 bytes), 85,4% of FLASH [43FA] EEPROM [0549] 25.03.2021

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
eeprom signed int maxSP=130;                          // Ограничение по максимальной температуре
eeprom unsigned char OvHeat=8,Hyst=5,Step=0;          // Тревога; Гистерезис; Шаг программы;
eeprom unsigned char Program=0, LastProg=1;           // Выполняемая программа, прошлая программа;
eeprom unsigned int Timer=0;                          // Таймер - текущее время операции
eeprom unsigned char DimTmr=1;                        // Размерность таймера
eeprom unsigned char Dim=0;                           // 
eeprom unsigned char Humid[2]={40,0};                 // Длительность паузы / впрыска
eeprom unsigned char HumMin=60;                       // температура с которой начинается увлажнени
eeprom unsigned char Priority=0;                      // приоритет датчиков
#ifdef KTY84
 eeprom signed char rider[2]={20,-10};                // коррекция датчиков температуры Bias=rider[0], koff=rider[1];
#else
 eeprom signed char rider[2]={0,0};                   // коррекция датчиков температуры Bias=rider[0], koff=rider[1];
#endif
#ifdef ELECTROSTAT
 eeprom unsigned int ocra1=3125;                      // Clock value: 62500 kHz / 200 Hz = 312,5 = ocra1
 eeprom unsigned int ocrb1=156;                       // Заполнение 50% ocra1 / 2 = 156 = ocrb1
 eeprom unsigned int timerElst[3]={10,20,30};         // таймеры электростатики ([0]-задержка; [1]-работа; [2]-пауза;)
#else
 eeprom unsigned int ocra1=10;                        // cof->K
 eeprom unsigned int ocrb1=500;                       // cof->Ti
 eeprom unsigned int timerElst[3]={0,1,0};            // таймеры электростатики ([0]-неиспользуется; [1]-инверсия реле нагрева; [2]-неиспользуется;)
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
  CN4 = ONmosfet;
}
// Timer 1 output compare B interrupt service routine
interrupt [TIM1_COMPB] void timer1_compb_isr(void){
  CN4 = OFFmosfet;
}
#endif
// Timer 2 overflow interrupt service routine
interrupt [TIM2_OVF] void timer2_ovf_isr(void)  // (5,0 mS)
{
  TCNT2 = PRESET2; ++counter; ++counter1; ++counter2;
#ifndef ELECTROSTAT
  if (pwTriac0) --pwTriac0;
  else CN4 = OFF;                    // отключить канал 4 (SSR-25DA)
#endif
}
#ifdef KTY84
 #include "adc_proc.c"
#else
 #include "dht11.c"
#endif
#include "proc.c"
#include "displ.c"
#include "keypad.c"
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
    if(Heat) 
     {
      if(counter1>49)
       {
        counter1=0; 
        byte=humidifier();
        switch (byte)// --------- УВЛАЖНИТЕЛЬ -------------------------------------
          {
           case ON:  CN3 = ON;  break;
           case OFF: CN3 = OFF; break;
          }; 
       }
     }
    else CN3 = OFF;
    if(mode) display_setup();  // режим установок
//------- Начало проверки каждую 1 сек. --------------------------------------------------
    if(Check)  
     {
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
//----- НАГРЕВ ВКЛЮЧЕН -----------------------------------------------------------------------------------------                       
      if(Heat)        // нагрев
       {
         if(errors) BeepT=120; // Ошибка датчиков
         if(ToInsideHeat)// если отсчет по температуре продукта.
          {
            if(++countsec>59){countsec=0; Timer++; if(Timer==1) Dim=1;}
            if(InsideHeatON==OFF){Heat=OFF; if(timeCool[Step])Cooling=ON; Timer=timeCool[Step]; BeepT=255; countsec=0;}// окончание приготовления среды
          }
         else if(--countsec<0)// если отсчет по таймеру
          {
           switch (Dim)
            {
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
         if(wsElstat) wsElstat--; else if(ocra1<1300) electrostat();
#endif
         if(pvT[0]<199){
           if(timerElst[1]){
             byte = Relay((int)pvT[0] - spT1[Step], Hyst);  // охладитель
             switch (byte)// --------- ОХЛАДИТЕЛЬ -------------------------------------
                {
                  case ON:  CN1 = ON;  break;
                  case OFF: CN1 = OFF; break;
                };
             byte = OFF;
           }
           else  byte = Relay((int)spT1[Step] - pvT[0], Hyst);  // нагреватель
         }
         else if(ds18b20>1){if(pvT[1]<199) byte = Relay((int)spT2[Step] - pvT[1], Hyst); else byte = OFF;}// если датчик потерян то опираемся на температура среды
         else byte = OFF;
         if(pvT[1]<199 && ds18b20>1) InsideHeatON = Relay((int)spT2[Step] - pvT[1], 0); // температура среды
         if(InsideHeatON == OFF) {byte = OFF; pwTriac0 = 0;}
#ifndef ELECTROSTAT
         else {
           pwTriac0 = UpdatePID();                          // нагреватель 
           if(pwTriac0) CN4 = ON;
         }
#endif
         if(timerElst[1]==0){
             switch (byte)// --------- НАГРЕВАТЕЛЬ -------------------------------------
              {
                case ON:  CN1 = ON;  break;
                case OFF: CN1 = OFF; break;
              };
         }
         CN2 = ON;    // таймер включен
       }
//---- ПРОДУВКА -----------------------------------------------------------------------------------------------
      else if(Cooling)// продувка
       {
         CN1 = OFF; CN2 = ON; ElStat = OFFmosfet; TIMSK=0x49; // Compare A: Off;
         if(--countsec<0)
          {
            countsec=59; displ=0; 
            if(Timer) Timer--;
            else if(Program) nextPrg();
            else {Cooling=OFF; Step=0; countsec=0; lock=30;}
          }
       }
      else if(Program && waitStart==0) nextPrg();
//---- ВСЕ ОТКЛЮЧЕНО ------------------------------------------------------------------------------------------
      else {CN1 = OFF; CN2 = OFF; ElStat = OFFmosfet; TIMSK=0x49;}// все отключено;  Compare A: Off;    
      if(waitStart) {LeftPrg(); if(--waitStart==0){LastProg=Program; if(Program>3) Program<<=4; nextPrg();}} 
      else if(mode==0) display();
     };
//------ Конец проверки каждую 1 сек. -----------------------------------------------------
  };
}
