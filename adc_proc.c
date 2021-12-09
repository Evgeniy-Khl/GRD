#define BIAS            0

// Read the AD conversion result
unsigned int read_adc(unsigned char adc_input)
{
 ADMUX=adc_input | (ADC_VREF_TYPE & 0xff);
 delay_us(10);                   // Delay needed for the stabilization of the ADC input voltage
 ADCSRA|=0x40;                   // Start the AD conversion
 while ((ADCSRA & 0x10)==0);     // Wait for the AD conversion to complete
 ADCSRA|=0x10;
 return ADCW;
}
/*
unsigned int calc_t(unsigned int adc)// ������ ����� ���
{
  if(adc<480) adc = 480;
  if(adc>999) {adc=999; errors|=4;}  // ��������� 999 ��� ����������� ������ �������
  else
   {
    adc -= 480; adc += rider[0];     // �������� ������ �������
    adc *= 100; adc /= (300-rider[1]);
   }
  return adc;
}
*/
/*
 ������ ����������
 1. ��� 0  ���.�. ��������� ��� = 310 ��.
 2. ��� 22 ���.�. ��������� ��� = 379 ��.
 3. ��� 100���.�. ��������� ��� = 700 ��.
 ������� ������� -> y = 4x + 310;  x = (y-310)/4 ��� x = (y-310)*100/400
*/
#ifdef KTY84

unsigned int calc_t(signed int adc) // ��� ��� ����� ����� ���
{
  if(adc>800) {adc=999; errors|=4;}  // ��������� 999 ��� ����������� ������ �������
  else
   {
    adc -= 310; adc += rider[0];     // �������� ������ �������
    if(adc<0) adc=0;
    adc *= 100; adc /=(unsigned int)(400-rider[1]);
   }
  return adc;
}

unsigned int LowPassF2(unsigned int PV)
{
float val;
  val = A1*PVold1-A2*PVold2+A3*PV;
  PVold2 = PVold1;
  PVold1 = val;
  return val;
};

#endif 