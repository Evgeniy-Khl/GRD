void dispLEFT(signed int val, unsigned char ext)
{
  if(ext){displ_buffer[0] = BL; displ_buffer[1] = EE;}
  else if((val>124)&&(mode==0))
   {
     displ_buffer[0] = DEF;
     displ_buffer[1] = DEF;
   }
  else if(val>99)
   {
     displ_buffer[0] = digit[(val/100)&0x0F];
     displ_buffer[1] = digit[((val/10)%10)&0x0F];
   }
  else if(val>=0)
   {
     displ_buffer[0] = BL;
     displ_buffer[1] = digit[(val/10)&0x0F];
   }
  else
   {
     val=-val;
     displ_buffer[0] = DEF;
     displ_buffer[1] = digit[(val/10)&0x0F];
   }
  if((val>124)&&(mode==0)) displ_buffer[2] = DEF; else displ_buffer[2] = digit[(val%10)&0x0F];
}

void dispRIGHT(signed int val, unsigned char dm)
{
 unsigned char chr, comma=0xFF;
  switch (dm)
   {
    case 0: chr=CC; break;
    case 1: chr=NN; break;
    case 2: chr=HH; break;
    case 3: chr=PT; break;
    case 4: chr=Pct; break;
    default:chr=BL;
   };
  if(Cooling) chr=OO;
  if(Heat==OFF&&Cooling==OFF&&mode==0&&val==0)// && Step==0
   {
     displ_buffer[3] = 0xc0;  // OFF
     displ_buffer[4] = FF;
     displ_buffer[5] = FF;
   }
/*  else if(val==0 && mode==0 && Step>0)
   {
     displ_buffer[3] = PP;  // P
     displ_buffer[4] = digit[Step];// номер шага
     displ_buffer[5] = BL;
   }*/
  else if(val==-1 && mode==0)
   {
     displ_buffer[3] = EE;  // E
     displ_buffer[4] = 0x99;// 4
     displ_buffer[5] = BL;
   }
  else if(val<10)
   {
     displ_buffer[3] = BL;
     displ_buffer[4] = digit[(val)&0x0F];
     if(SetUp) displ_buffer[5] = chr;
     else if(countsec & 1 && dm) displ_buffer[5] = BL;
     else displ_buffer[5] = chr;
   }
  else if(val<100)
   {
     displ_buffer[3] = digit[(val/10)&0x0F];
     displ_buffer[4] = digit[(val%10)&0x0F];
     if(SetUp) displ_buffer[5] = chr;
     else if(countsec & 1 && dm) displ_buffer[5] = BL;
     else displ_buffer[5] = chr;
   }
  else if(val<1000)
   {
     if(countsec & 1 && dm) comma=0x7F;               // запятая мигает только для минут и часов
     displ_buffer[3] = digit[(val/100)&0x0F];
     displ_buffer[4] = digit[(val/10)%10&0x0F];
     displ_buffer[5] = digit[(val%10)&0x0F]&comma;      // запятая;
   }
}

void Ltmr()
{
  displ_buffer[0] = BL; displ_buffer[1] = TT; displ_buffer[2] = RS;
}

void Lstp()
{
  displ_buffer[0] = PT; displ_buffer[1] = RT; displ_buffer[2] = GE;
}

void LeftPrg()
{
//  if(buf>7) buf=0;
  switch (Program)
   {
    case 1: displ_buffer[0]=BL;    displ_buffer[1]=BL;   displ_buffer[2]=0xf9;   break; // 1
    case 2: displ_buffer[0]=BL;    displ_buffer[1]=BL;   displ_buffer[2]=0xa4;   break; // 2
    case 3: displ_buffer[0]=BL;    displ_buffer[1]=BL;   displ_buffer[2]=0xb0;   break; // 3
    case 4: displ_buffer[0]=0xf9;  displ_buffer[1]=0xa4; displ_buffer[2]=D_F;    break; // 12_
    case 5: displ_buffer[0]=0xf9;  displ_buffer[1]=D_F;  displ_buffer[2]=0xb0;   break; // 1_3
    case 6: displ_buffer[0]=D_F;   displ_buffer[1]=0xa4; displ_buffer[2]=0xb0;   break; // _23
    case 7: displ_buffer[0]=0xf9;  displ_buffer[1]=0xa4; displ_buffer[2]=0xb0;   break; // 123
    default: displ_buffer[0]=BL;   displ_buffer[1]=BL;   displ_buffer[2]=0xc0;          // 0
   };
  if(Program<4) {displ_buffer[3] = PT; displ_buffer[4] = RT; displ_buffer[5] = GE;}
  else {displ_buffer[3] = SS; displ_buffer[4] = EE; displ_buffer[5] = RT;}
}

void display()
{
  if(errors && (countsec&1)){{dispLEFT(errors,1); if(Timer) dispRIGHT(Timer,Dim); else dispRIGHT(countsec,Dim);}}
  else     // нет ошибок
   {
     if(displ==1){dispLEFT(pvT[1],0); displ_buffer[3] = DD; displ_buffer[4] = 0xa4; displ_buffer[5] = BL;}// d2
     else if(displ==2){dispLEFT(pvT[2],0); displ_buffer[3] = DD; displ_buffer[4] = 0xb0; displ_buffer[5] = BL;}// d3
     else if(ToInsideHeat)  // если отсчет по температуре продукта.
     { 
       dispLEFT(pvT[0],0);
       if(ElStat == ONmosfet) dispRIGHT(Timer,4);
       else if(Timer) dispRIGHT(Timer,1);
       else dispRIGHT(countsec,0);
     }
     else 
      {
        if(countsec > DISPLprg){dispRIGHT(Step,10); Lstp();}
        else
         {
           dispLEFT(pvT[0],0);
           if(ElStat == ONmosfet) dispRIGHT(Timer,4);
           else if(Timer) dispRIGHT(Timer,Dim);
           else dispRIGHT(countsec,Dim);
         }
      }
   }
}

void display_setup()
{
  if(mode==1) {if(buf<0) buf=0; else if(buf>3) buf=3; dispRIGHT(buf, 10); Lstp();}
  else if(mode==2) {if(buf<0) buf=0; dispRIGHT(buf, DimTmr); Ltmr();}
  else {dispLEFT(buf,0);}
}