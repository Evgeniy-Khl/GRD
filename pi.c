unsigned char Relay(signed int err, unsigned char hst) // [n] ����� � 1 ��� 2
{
unsigned char x=2;
 if (err>hst) x = ON;         // ��������
 if (err <= 0) x = OFF;       // ���������
 return x;
}

#ifndef ELECTROSTAT
unsigned char humidifier()
{
 unsigned char val;
  if(timerHum[0]==0 || timerHum[1]==0){
    if(ds18b20==3) val = Relay((int)spT3[Step] - pvT[2], Hyst); // ������ ������ - ������ ���������
    else if(Sensor) val = Relay((int)spT3[Step] - pvRH, Hyst);  // DHT-21 - ������ ���������
  }
  else {
     val=CN3;
     if(val) {if(--counthum==0) {val=OFF; counthum=timerHum[0];}} // ������������ �����
     else {if(--counthum==0) {val=ON; counthum=timerHum[1];}}     // ������������ �������
  }
  if(pvT[0]<timerHum[0]) val=OFF;        // ������ ���������� ��� ������ ������������
  return val;
}
#endif

unsigned char UpdatePID(void)
{
 signed int err;
 float pPart, Ud;
 static float iPart;
  err = (int)spT1[Step]*10 - pvT0;
  pPart = (float) err * koff[0];                  // ������ ���������������� �����
//---- ������� ����������� pPart ---------------
  if (pPart < 0) pPart = 0;
  else if (pPart > 200) pPart = 200;             // ������� �����������
//----------------------------------------------
  iPart += (float) koff[0] / koff[1] * err;      // ���������� ������������ �����
  Ud = pPart + iPart;                            // ����� ���������� �� �����������
//---- ������� ����������� Ud ------------------
  if (Ud < 0) Ud = 0;
  else if (Ud > 200) Ud = 200;                   // ������� �����������
  iPart = Ud - pPart;                            // "��������������" ��������
  err = Ud;
  return err;
};


