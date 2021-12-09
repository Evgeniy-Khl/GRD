
#define dhtport PORTD.7
#define dhtddr DDRD.7
#define dhtpin PIND.7

unsigned char starDHT(void)
{
   unsigned char flag=0;
   dhtport = 0;
   dhtddr = 1;   // MCU pull down
//#if MDFAMILY==1
//   delay_ms(30); // MCU Sends out Start Signal to DHT and pull down voltage for at least 18ms to let DHT11 detect the signal.
//#else
   delay_ms(10); // MCU Sends out Start Signal to DHT and pull down voltage for at least 18ms to let DHT21 detect the signal.
//#endif
   dhtddr = 0;   // MCU pull up
   delay_us(60); // wait for DHT respond 20-40uS
   if(!dhtpin)
   {
      while(!dhtpin) {flag++;} // low-voltage-level response signal & keeps it for 80us
      if(flag<10) return 0;
      else {flag=0; while(dhtpin) {flag++;}}  // hi-voltage-level response signal & keeps it for 80us
      if(flag<10) return 0;
      else return 1;
   }
   else return 0;
}

unsigned char reaDHT(void)
{
 unsigned char i, j, flag=0, tem[5];
 if(starDHT())
  {
    for(i=0; i<5; i++)
     {
       tem[i]=0;
       for(j=0; j<8; j++)             
        {
          tem[i]<<= 1;
//          delay_us(30);   // When DHT is sending data to MCU, every bit of data begins with the 50us low-voltage-level
                          // and the length of the following high-voltage-level signal determines whether data bit is "0" or "1"
          // Global disable interrupts
          #asm("cli")
          while(!dhtpin) {flag++;}// ожидаем фронт сигнала
          delay_us(32);   //26-28u voltage-length means data "0" / 70u voltage-length means data "1"
          flag=0;
          while(dhtpin) {flag++;}// ожидаем спад сигнала
          if(flag>10) tem[i]|= 1;// data "1"
          // Global enable interrupts
          #asm("sei")
        }
     }
    flag=tem[0]+tem[1]+tem[2]+tem[3];
    if(flag==tem[4])
     {
#ifdef DHT11
      pvRH=(int)tem[0]; pvT[2]=(int)tem[2];
#else
      pvRH =(int)(tem[0]*256+tem[1])/10; pvT[2] =(int)(tem[2]*256+tem[3])/10;
#endif
      pvT[2] += rider[0];                // коррекция датчика температуры
      pvRH += rider[1];                  // коррекция датчика влажности
      if(pvRH>100) pvRH=100; else if (pvRH<0) pvRH=0;
      return 1;
     }
    else return 0;  // НЕ верная CRC датчика влажности
  }
 else return 0;     // потерян датчик влажности
}
