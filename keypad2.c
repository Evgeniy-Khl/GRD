void setOCRA(unsigned int val){
 unsigned int temp;
 temp = 62500/val;       // Clock value: 62500 kHz / 250 Hz = 250 => ocra1
 OCR1A = temp;
}

void setOCRB(unsigned int val){
 unsigned int temp;
 temp = (OCR1A*val)/100;    // Заполнение 50% ->  ocra1 * 50% = 125 => ocrb1
 OCR1B = temp;
}

unsigned char checkkey(void){
  static unsigned char key, count;
  if (key==keydata) ++count;
  else if (key==6){count=10; key=keydata;}
  else {count=0; key=keydata;};
  if(count>3)
   {
    count = 0;
    if(key<5)        // если нажата любая кнопка
     {
      BeepT = 25;
      if(mode)     // режим РЕДАКТИРОВАНИЯ
       {
        waitmode=5; // удерживаем режим установок
        switch (key)
          {
           case KEY_MD:
             {
              waitkey=WAITCOUNT;
              if (++mode>maxOwner) mode=1;
              switch (mode)
                {
                 case 1: buf=Step; break;
                 case 2: buf=SpTmr[Step];  break;     // tr  продолжительность нагрева (время работы таймера)
                 case 3: buf=spT1[Step];   break;     // У01 (С) Требуемая температура по цифровому датчику №1
                 case 4: buf=spT2[Step];   break;     // У02 (С) Требуемая температура по цифровому датчику №2
                 case 5: buf=spT3[Step];   break;     // У03 (С) Требуемая температура или (%) Требуемая влажность
                 case 6: buf=timerMain[0]; break;     // У04 (сек.) Задержка включения электростатики.
                 case 7: buf=numPrg;       break;     // У05 Количество используемых программ
                };
             } break;
           case KEY_UP: buf++; if (waitkey) waitkey--;
                        switch (mode){
                          case 1: buf = buf&3; break;
                          case 2: if(buf>900) buf=900; break;
                          case 3: if(buf>maxSP[Step]) buf=maxSP[Step]; break;
                          case 4: if(buf>maxSP[Step]) buf=maxSP[Step]; break;
                          case 5: if(buf>maxSP[Step]) buf=maxSP[Step]; break;
                          case 7: buf = buf&7; break;
                          case 17: if(buf>maxSP[Step]/4) buf=maxSP[Step]/4; break;
                          case 18: if(buf>maxSP[Step]/8) buf=maxSP[Step]/8; break;
                          case 19: if(buf>2) buf=2; break;
                          case 22: if(buf>99) buf=99; break;
                          case 23: if(buf>130) buf=130; break;
                          case 28: if(buf>500) buf=500; break;
                          case 29: if(buf>95) buf=95; break;
                          case 30: if(buf>99) buf=99; break;
                          case 31: if(buf>99) buf=99; break;
                        }
             break;
           case KEY_ST:
             {
              waitkey=WAITCOUNT; ++mode;
              if(maxUser == MAXUSER0){if(mode==20 || mode==21) mode = 22;}// исключаем пункты П04 и П05
              if (mode>maxUser||mode<16) mode=16;
              switch (mode)
                {
                 case 16: buf=0; break;              // П0 сброс параметров
                 case 17: buf=OvHeat; break;         // П01 максимально допустимое отклонение температуры
                 case 18: buf=Hyst; break;           // П02 гистерезис терморегулятора
                 case 19: buf=DimTmr; break;         // П03 Размерность таймера: 0 - секунды(c); 1 - минуты(n); 2 - часы(h)
                 case 20: buf=timerMain[1]; break;   // П04-Tаймеры электростатики [1]-работа;
                 case 21: buf=timerMain[2]; break;   // П05-Tаймеры электростатики [2]-пауза;
                 case 22: buf=timeCool[Step]; break; // П06 Время продувки в минутах.
                 case 23: buf=maxSP[Step]; break;    // П07 Ограничение максимальной температуры.
                 case 24: buf=Priority; break;       // П08 Приоритет датчиков
                 case 25: buf=reverse; break;        // П09 Pеверсный режим реле (нагрев / охлаждение)
                 case 26: buf=koff[0]; break;        // П10 Пропорциональный кофф.
                 case 27: buf=koff[1]; break;        // П11 Интегральный кофф.
                 case 28: buf=ocra1; break;          // П12 частота импульсов
                 case 29: buf=ocrb1; break;          // П13 скважность
                 case 30: buf=rider[0]; break;       // П14 Коррекция датчика (нижняя точка)
                 case 31: buf=rider[1]; break;       // П15 Коррекция датчика (верхняя точка)
                };
             } break;
           case KEY_DW: buf--; if (waitkey) waitkey--;
                        switch (mode){
//                          case 3: if(buf<20) buf=20; break;
//                          case 4: if(buf<20) buf=20; break;
//                          case 5: if(buf<20) buf=20; break;
                          case 26: if(buf<1) buf=1; break;
                          case 27: if(buf<100)  buf=100;  break;
                          case 28: if(buf<20) buf=20; break;
                          case 29: if(buf<5)  buf=5;  break;
                          case 30: if(buf<-99) buf=-99; break;
                          case 31: if(buf<-99) buf=-99; break;
                          default: if(buf<0) buf=0;
                        }
             break;
           default: waitkey=WAITCOUNT;
          }; 
       }
      else          // режим ИЗМЕРЕНИЯ
       {
        switch (key)
          {
           case KEY_MD: mode=1; buf=Step; waitmode=5; SetUp=1; break;
           case KEY_UP: 
                        if(Heat==OFF){
                          if(waitStart){if(++Program>numPrg) Program=0;} 
                          else Program=LastProg; 
                          waitStart=5; Check=1; waitkey=80; wsElstat=timerMain[0]; pvTimer=1;
                        }
            break; // && errors==0
           case KEY_ST: lock=0; Check=1; waitmode=255;
                        if(Sensor || ds18b20>1) if(++displ>2) displ=0; 
            break;
           case KEY_DW:
                if(Heat==ON||Cooling==ON){Timer=0;lock=3;Dim=0;countsec=0;countmin=0;Program=0;Step=0;BeepT=255;Heat=OFF;Cooling=OFF;}
            break;
          };
       };
      if(mode>2&&mode<16)
       {
        displ_buffer[3] = YY;
        displ_buffer[4] = digit[((mode-2)&0x0F)/10];
        displ_buffer[5] = digit[((mode-2)&0x0F)%10];
       }
      else if(mode&0x10)
       {
        displ_buffer[3] = PT;
        displ_buffer[4] = digit[(mode&0x0F)/10];
        displ_buffer[5] = digit[(mode&0x0F)%10];
        }
     }
    else waitkey=WAITCOUNT;
   };
  return key;
}

void saveset(void)
{
 switch (mode)
   {
    case 1: Step = buf&3; break;
    case 2: if(buf>900) buf=900; else if(buf<0) buf=0; SpTmr[Step] = buf; break;
    case 3: spT1[Step]=buf; break;
    case 4: spT2[Step]=buf; break;
    case 5: spT3[Step]=buf; break;
    case 6: timerMain[0]=buf; break;
    case 7: numPrg=buf&7; break;
    //------------------------------------ П00 -------------------------------------------------------------------
    case 16: if(buf) {maxOwner=MAXOWNER1; maxUser=MAXUSER1;}// разрешение вводить У04, У05, П04, П05
             break;//--------------------- Меню специалиста ---------------------------------------------------------
    case 17: if(buf>maxSP[Step]/4) buf=maxSP[Step]/4; else if(buf<0) buf=0; OvHeat = buf; break;// П01-максимально допустимое отклонение температуры 
    case 18: if(buf>maxSP[Step]/8) buf=maxSP[Step]/8; else if(buf<0) buf=0; Hyst = buf; break;  // П02-гистерезис терморегулятора
    case 19: if(buf>2) buf=2; else if(buf<0) buf=0; DimTmr = buf; break;            // П03-Размерность таймера
    case 20: if(buf>240) buf=240; else if(buf<0) buf=0; timerMain[1] = buf; break;  // П04-Tаймеры электростатики [1]-работа;
    case 21: if(buf>240) buf=240; else if(buf<0) buf=0; timerMain[2] = buf; break;  // П05-Tаймеры электростатики [2]-пауза;
    case 22: if(buf>99) buf=99; else if(buf<0) buf=0; timeCool[Step] = buf; break;  // П06-Время продувки в минутах.
    case 23: if(buf>130) buf=130; else if(buf<0) buf=0; maxSP[Step] = buf; break;   // П07-Ограничение максимальной температуры.
    case 24: Priority = buf&7; break;                                               // П08-Приоритет датчиков
    case 25: reverse  = buf&1; break;                                               // П09-инверсия реле нагрева / охлаждения
    case 26: if(buf<1)   buf=1;   koff[0] = buf; break;                             // П10 Пропорциональный кофф.
    case 27: if(buf<100) buf=100; koff[1] = buf; break;                             // П11 Интегральный кофф.
    case 28: if(buf<20) buf=20; ocra1 = buf; setOCRA(buf); setOCRB(ocrb1); break;   // П12 частота импульсов  от 20 до 511       
    case 29: if(buf<5)  buf=5;  ocrb1 = buf; setOCRB(buf); break;                   // П13 скважность
    case 30: if(buf>99) buf=99; else if(buf<-99) buf=-99; rider[0]=buf; break;      // П14-Коррекция датчика температуры (Bias)
    case 31: if(buf>99) buf=99; else if(buf<-99) buf=-99; rider[1]=buf; break;      // П15-Коррекция датчика влажности   (koff)
   }; 
 mode=0; SetUp=0;
}