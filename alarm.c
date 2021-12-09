void alarm(void)
{
signed char cn, bias;
signed int err;
  for (cn=0; cn<ALARMCNT; cn++)
   {
     bias= sp[cn].alarm;
     err = outErr[cn];
     if (abs(err) < bias) t_OK[cn] = 1;         // ����� �� �������� �����������
     if (t_OK[0]==0) t_OK[1] = 0;               // ���������� ������� �� 2 ������ !!!!!!!!
     if (t_OK[cn])
      {
       if (err > bias)                     // ��������������
        {
          t_OK[cn] = 2;                         // ������ �����
          Alarm = ON;                           // �������� �������
          error |= (cn+1)<<4;                   // �������� ������ ������
        }
       else if (err < -bias)               // ��������
        {
          t_OK[cn] = 3;                         // ������ �����
          Alarm = ON;                           // �������� �������
          error |= (cn+1)<<4;                   // �������� ������ ������
        };
      };
   };
   if (Alarm & !SetUp)
    {
      if (disableBeep > 0) --disableBeep;       // ���� ������ �������������
      else BeepT = 255;                         // ������������ ��������� �������
    }
   else disableBeep = 0;
}