// Port B initialization
PORTB=0xFF;// State7=1 State6=1 State5=1 State4=1 State3=1 State2=1 State1=1 State0=1
DDRB=0xFF; // Func7=Out Func6=Out Func5=Out Func4=Out Func3=Out Func2=Out Func1=Out Func0=Out

// Port C initialization
PORTC=0x00;// State6=T State5=T State4=0 State3=0 State2=0 State1=0 State0=0 
DDRC=0x1F;// Func6=In Func5=In Func4=Out Func3=Out Func2=Out Func1=Out Func0=Out 

// Port D initialization
PORTD=0x3F;// State7=T State6=T State5=1 State4=1 State3=1 State2=1 State1=1 State0=1 
DDRD=0x3F;// Func7=In Func6=In Func5=Out Func4=Out Func3=Out Func2=Out Func1=Out Func0=Out 

// Timer/Counter 0 initialization
TCCR0=0x03;                             // Clock value: clk/64 Clock source: (Clock value: 62,500 kHz)
#ifdef ELECTROSTAT
// Timer/Counter 1 initialization
TCCR1B=0x0B;// Mode: CTC top=OCR1A; Clock value: 62,500 kHz
OCR1A =ocra1;
OCR1B =ocrb1;
#endif 
// Timer/Counter 2 initialization
TCCR2=0x05;                             // Clock value: clk/128 Clock source: (Clock value: 31,250 kHz)

// External Interrupt(s) initialization
// INT0: Off
// INT1: Off
MCUCR=0x00;

// Timer(s)/Counter(s) Interrupt(s) initialization
TIMSK=0x49; // TimerOVF0 - ON; TimerOVF2 - ON; Compare A Match Interrupt: Off; Compare B Match Interrupt: On

// Analog Comparator initialization
ACSR=0x80;                              // Analog Comparator: Off
SFIOR=0x00;                             // Analog Comparator Input Capture by Timer/Counter 1: Off

#ifdef KTY84
// ADC initialization
ADMUX=ADC_VREF_TYPE;                    // ADC Clock frequency: 62,500 kHz
ADCSRA=0x86;                            // ADC Voltage Reference: AVCC pin
#endif

w1_init();                              // 1 Wire Bus initialization
ds18b20 = w1_search(0xf0,familycode);   // detect how many DS18b20 devices are connected to the 1 Wire bus
if(ds18b20 > 3) ds18b20 = 3;	    // если датчиков много то оставляем только два
if(ds18b20){displ_buffer[3]=digit[ds18b20]; w1_init(); w1_write(0xCC); w1_write(0x44);}// если датчики обнаружены - 1 Wire Bus initialization
if(ds18b20 == 2) {if(Priority) permutation (0, 1);}
else if(ds18b20 == 3)
 {
   switch (Priority)
    {                                        // 1-2-3
        case 1: permutation (0, 1); break;   // 2-1-3
        case 2: permutation (0, 2); break;   // 3-2-1
        case 3: permutation (1, 2); break;   // 1-3-2
        case 4: permutation (0, 2); permutation (0, 1); break;   // 2-3-1
        case 5: permutation (0, 1); permutation (0, 2); break;   // 3-1-2
    };
 }

// Global enable interrupts
#asm("sei")

#ifdef KTY84
if (ds18b20 < 3){
  buf = read_adc(6);
  if(buf<1000){Sensor=1; displ_buffer[5]=0xf9; PVold1 = PVold2 = buf;} // если датчик обнаружен 
}
#else
if (ds18b20 < 3){
  delay_ms(1000);                             // задержка на заряд конденсаторов DHT
  Sensor=reaDHT();                            // detect DHT-21/11
  if(Sensor) displ_buffer[4]=0xf9;            // если датчик обнаружен
}
#endif

BeepT=50;
delay_ms(2000);
#ifdef ELECTROSTAT
  pvTimer=1;               // для бысрого срабатывания функции electrostat()ж
  wsElstat = timerElst[0]; // задержка старта
#endif
maxUser = MAXUSER0;
maxOwner = MAXOWNER0;
if(Timer && ds18b20)
 {
  ElStat=OFFmosfet; InsideHeatON=ON; Heat=ON; BeepT=255;
  if(SpTmr[Step]){countsec=59; countmin=59; counthum=0;}
  else {ToInsideHeat=ON; countsec=0;}  // устанавливаем отсчет по температуре продукта.
 }
else BeepT=100;

// Watchdog Timer initialization
#pragma optsize-
WDTCR=0x1F;
WDTCR=0x0F;     // Watchdog Timer Prescaler: OSC/2048k
#ifdef _OPTIMIZE_SIZE_
#pragma optsize+
#endif
