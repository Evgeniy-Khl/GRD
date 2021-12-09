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
  if(Humid[0]==0 || Humid[1]==0){
    if(ds18b20==3) val = Relay((int)spT3[Step] - pvT[2], Hyst); // ������ ������ - ������ ���������
    else if(Sensor) val = Relay((int)spT3[Step] - pvRH, Hyst);  // DHT-21 - ������ ���������
  }
  else {
     val=CN3;
     if(val) {if(--counthum==0) {val=OFF; counthum=Humid[0];}} // ������������ �����
     else {if(--counthum==0) {val=ON; counthum=Humid[1];}}     // ������������ �������
  }
  if(pvT[0]<HumMin) val=OFF;        // ������ ���������� ��� ������ ������������
  return val;
}
#endif

unsigned char UpdatePID(void)
{
 signed int err;
 float pPart, Ud;
 static float iPart;
  err = (int)spT1[Step]*10 - pvT0;
  pPart = (float) err * ocra1;                  // ������ ���������������� �����
//---- ������� ����������� pPart ---------------
  if (pPart < 0) pPart = 0;
  else if (pPart > 200) pPart = 200;          // ������� �����������
//----------------------------------------------
  iPart += (float) ocra1 / ocrb1 * err;    // ���������� ������������ �����
  Ud = pPart + iPart;                               // ����� ���������� �� �����������
//---- ������� ����������� Ud ------------------
  if (Ud < 0) Ud = 0;
  else if (Ud > 200) Ud = 200;                // ������� �����������
  iPart = Ud - pPart;                               // "��������������" ��������
  err = Ud;
  return err;
};


