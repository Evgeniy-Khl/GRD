//#define TUNING    170
char temperature_check(void)
{
  char item, *ptr_to_ram, byte, crc, try=0, err=0;
  int tempT; 
  for (item=0; item < ds18b20;)
   {
     w1_init(); // 1 Wire Bus initialization
     w1_write(0x55); // Load MATCH ROM [55H] comand
     ptr_to_ram = &familycode[item][0];
     for (byte=0; byte < 8; byte++) w1_write(*ptr_to_ram++); // Load cont. byte
     ptr_to_ram = data.ds_buffer;
     w1_write(0xBE); // Read Scratchpad command [BE]
     for (byte=0; byte < 8; byte++)
      {
        *ptr_to_ram = w1_readnew(); // Read cont. byt
        *ptr_to_ram++;
      }
     crc = w1_readnew(); // Read CRC byte
     ptr_to_ram = data.ds_buffer;
     if (w1_dow_crc8(ptr_to_ram, 8)==crc){
       try = 0; tempT = data.intval;
       if(tempT<0) {tempT = - tempT; tempT = tempT*10/16; tempT = - tempT;}
       else if(tempT==0) tempT = 2000;// значение "0" считается ОШИБКОЙ 
       else tempT = tempT*10/16;     // перевод в десятичное значение !!
       if(item==0) pvT0 = tempT;     // точное значение температуры для ПИД регулятора
       pvT[item] = (tempT+5)/10;     // округление до целого
       //----- Коректировка датчика DS18B20 ------------------------------------------------------------------------------------------
//#pragma warn-
//       if(data.ds_buffer[2]==TUNING) pvT[item] +=(signed char)data.ds_buffer[3]; // корекция показаний датчика
//#pragma warn+
       //-----------------------------------------------------------------------------------------------------------------------------
     }
     else if(++try > 2)
      {
        if(++wait[item] > 3){wait[item] = 3; pvT[item] = 199; err|=(1<<item);}// (199) если ошибка более X раз то больше не опрашиваем
        try = 0;
      }
     else delay_ms(2);           
     if(try==0) item++;
   };
  w1_init(); // 1 Wire Bus initialization
  w1_write(0xCC); // Load Skip ROM [CCH] command
  w1_write(0x44); // Load Convert T [44H] command
  return err;
}

void startPrg(void)
{
  BeepT=255; InsideHeatON=ON; Heat=ON; Cooling=OFF; countsec=0; counthum=Humid[0]; errors=0;
  if(SpTmr[Step]){Timer=SpTmr[Step]; countsec=59; countmin=59; counthum=0; Dim=DimTmr; ToInsideHeat=OFF;}
  else {ToInsideHeat=ON; countsec=0; Timer=0; Dim=0;}
}

void electrostat(void) // [TIM1_COMPA (0x10)] CN4 = ONmosfet;    [TIM1_COMPB (0x08)] CN4 = OFFmosfet;   [1]-работа; [2]-пауза;
{
  if(ElStat==ONmosfet){if(--pvTimer==0) {pvTimer=timerElst[2]; ElStat = OFFmosfet; TIMSK&=0xEF;}}// TimerOVF0-On; TimerOVF2-On; Compare A: Off; Compare B: On
  else {if(--pvTimer==0) {pvTimer=timerElst[1]; ElStat = ONmosfet; TIMSK|=0x10;}};// TimerOVF0-On; TimerOVF2-On; Compare A: On; Compare B: On
}

void nextPrg(void)
{
 switch (Program)
  {
    case 0x01: Step = 0x01; Program=0; break; // 1__  ?????
    case 0x02: Step = 0x02; Program=0; break; // _2_  ?????
    case 0x03: Step = 0x03; Program=0; break; // __3  ?????
                 
    case 0x40: Step = 0x01; Program=0x02; break; // 12_  ?????
    case 0x50: Step = 0x01; Program=0x03; break; // 1_3  ?????
    case 0x60: Step = 0x02; Program=0x03; break; // _23  ?????
    case 0x70: Step = 0x01; Program=0x60; break; // 123  ?????
    default:   Step = 0;    Program=0;
  };
 startPrg();
}

void permutation (char a, char b){
unsigned char i, buff;
  for (i=0;i<9;i++) {
     buff = familycode[a][i];
     familycode[a][i] = familycode[b][i];
     familycode[b][i] = buff; 
  };
}