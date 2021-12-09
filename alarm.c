void alarm(void)
{
signed char cn, bias;
signed int err;
  for (cn=0; cn<ALARMCNT; cn++)
   {
     bias= sp[cn].alarm;
     err = outErr[cn];
     if (abs(err) < bias) t_OK[cn] = 1;         // вышли на заданную температуру
     if (t_OK[0]==0) t_OK[1] = 0;               // отключение тревоги по 2 каналу !!!!!!!!
     if (t_OK[cn])
      {
       if (err > bias)                     // ПЕРЕОХЛАЖДЕНИЕ
        {
          t_OK[cn] = 2;                         // мигают цифры
          Alarm = ON;                           // включить тревогу
          error |= (cn+1)<<4;                   // включить сигнал АВАРИЯ
        }
       else if (err < -bias)               // ПЕРЕГРЕВ
        {
          t_OK[cn] = 3;                         // мигают цифры
          Alarm = ON;                           // включить тревогу
          error |= (cn+1)<<4;                   // включить сигнал АВАРИЯ
        };
      };
   };
   if (Alarm & !SetUp)
    {
      if (disableBeep > 0) --disableBeep;       // если сирена заблокирована
      else BeepT = 255;                         // длительность звукового сигнала
    }
   else disableBeep = 0;
}