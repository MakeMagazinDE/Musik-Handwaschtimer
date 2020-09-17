/*
 * Make-Magazin, https://www.heise.de/make/
 * 
 * Demo fuer Multi Function Shield
 * 
 * 7-Segment-Display mit Timer/IRQ als Handwasch- oder Kuechentimer
 */

#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

#define NUMBER_OF_SONGS 10
SoftwareSerial mySoftwareSerial(6, 9); // RX, TX on Arduino
DFRobotDFPlayerMini mp3;
bool randomGeneratorInitialized = false;

const uint8_t LATCH = 4;      // Pinbelegung Schieberegister
const uint8_t CLK = 7;
const uint8_t DATA = 8; 
const uint8_t SEGMENT_MAP[] = {0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0X80,0X90};    // Segmente, die leuchten sollen pro Zahlwert (Low-Aktiv), & 0x7F Verknüpfen fuer Dezimalpunkt
const uint8_t SEGMENT_SELECT[] = {0xF1,0xF2,0xF4,0xF8};                               // Ziffernposition (gemeinsame Anode, LSB)
const uint8_t Led4 = 10;
const uint8_t Led3 = 11;
const uint8_t Led2 = 12;
const uint8_t Led1 = 13;
const uint8_t BuzzerPin = 3;
const uint8_t SensorPin = 5;
const uint8_t Select = A1;
const uint8_t Minus = A2;
const uint8_t Plus = A3;
int val = 0;
int Zeit = 30;
int Veraenderung = 0;

volatile uint8_t ActDigit = 0;  // fuer interne Aufgaben zur Displayanzeige bei IRQ
uint16_t DisplayValue = 0;      // auf dem Display anzuzeigender Wert. Kann jederzeit geaendert werden und wird sofort dargestellt

/*
 * @brief   Ausgabe einer Zahl an einer Position auf dem Display
 * @param   welche Position (0= links, 3=rechts)
 *          Zahlwert (0..9)
 * @return  none
 */
void WriteNumberToSegment(uint8_t digit, uint8_t value)
{
  digitalWrite(LATCH,LOW);                                // Uebernahme-Takt: Ausgang Aus
  shiftOut(DATA, CLK, MSBFIRST, SEGMENT_MAP[value]);      // Segmente passend zum Zahlwert rausschieben
  shiftOut(DATA, CLK, MSBFIRST, SEGMENT_SELECT[digit]);   // Welche Stelle leuchten soll hinterher schieben
  digitalWrite(LATCH,HIGH);                               // Uebernahme-Takt: Ausgang Ein
}

/*
 * @brief   IRQ-Routine wird bei erreichen des Timer-Vergleichswertes ausgeloest. Ausgabe auf dem Display
 * @param   Zahl 0..9999
 * @return  none
 */
ISR(TIMER1_COMPA_vect)          
{
  switch (++ActDigit)   // es wird bei jedem IRQ eine der 4 digits dargestellt.
  {
    case 1 : WriteNumberToSegment(0, DisplayValue / 1000); break;     
    case 2 : WriteNumberToSegment(1, (DisplayValue / 100) % 10); break; 
    case 3 : WriteNumberToSegment(2, (DisplayValue / 10) % 10); break; 
    case 4 : WriteNumberToSegment(3, DisplayValue % 10); ActDigit = 0; break; 
  }
} 


void initMp3(void)
{
  mySoftwareSerial.begin(9600);
  if (!mp3.begin(mySoftwareSerial, false, false)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true);
  }
  Serial.println(F("Delay for DFPlayer init ..."));
  delay(3000);
  Serial.println(F("DFPlayer Mini online."));
  mp3.setTimeOut(500); //Set serial communication time out 500ms
  mp3.volume(15);  //Set volume value
}


void setup ()
{
  pinMode(LATCH,OUTPUT);
  pinMode(CLK,OUTPUT);
  pinMode(DATA,OUTPUT);
  pinMode(Led4,OUTPUT);
  pinMode(Led3,OUTPUT);
  pinMode(Led2,OUTPUT);
  pinMode(Led1,OUTPUT);
  pinMode(BuzzerPin,OUTPUT);
  pinMode(Select, INPUT);    // I/O-Pins als Eingang
  pinMode(Minus, INPUT);    
  pinMode(Plus, INPUT);    
  pinMode(SensorPin,INPUT);

  digitalWrite(BuzzerPin,HIGH);
  Serial.begin(9600);
  
  TCCR1A = 0;                                           // Register loeschen
  OCR1A = 1000;                                         // Vergleichswert x = (CPU / (2 x Teiler x f)) - 1
  TCCR1B |= (1 << CS10) | (1 << CS11) | (1 << WGM12);   // CTC-Mode, Teiler = 64
  TIMSK1 |= (1 << OCIE1A);                              // Output Compare A Match Interrupt Enable
  
  initMp3();
  
  sei();                                                // IRQ Enable
}

void loop()
{
  unsigned int songNumber;
  
  DisplayValue = Zeit;
  if (!digitalRead(Select))    // wenn Taster gedrueckt ist, dann ... (Low-Aktiv => gedrueckt = LOW)
  {  
    if (Veraenderung == 0)
    {
      Veraenderung = 1;
    }
    else
    {
      Veraenderung = Veraenderung * 10;
      if (Veraenderung > 1000)
         Veraenderung = 0;
    }    
    delay(100);
  }
  
  switch (Veraenderung)
  {
     case 0:
      digitalWrite(Led1,HIGH);
      digitalWrite(Led2,HIGH);
      digitalWrite(Led3,HIGH);
      digitalWrite(Led4,HIGH);
     break;

     case 1:
      digitalWrite(Led1,LOW);
      digitalWrite(Led2,HIGH);
      digitalWrite(Led3,HIGH);
      digitalWrite(Led4,HIGH);
      if (!digitalRead(Minus))
      {
        Zeit = Zeit - Veraenderung;
      }
      if (!digitalRead(Plus))
      {
        Zeit = Zeit + Veraenderung;
      }
      if (Zeit < 0)
         Zeit = 0;
      DisplayValue = Zeit;
      delay(200);
     break;

     case 10:
      digitalWrite(Led1,HIGH);
      digitalWrite(Led2,LOW);
      digitalWrite(Led3,HIGH);
      digitalWrite(Led4,HIGH);
      if (!digitalRead(Minus))
      {
        Zeit = Zeit - Veraenderung;
      }
      if (!digitalRead(Plus))
      {
        Zeit = Zeit + Veraenderung;
      }
      if (Zeit < 0)
         Zeit = 0;
      DisplayValue = Zeit;
      delay(200);
     break;

     case 100:
      digitalWrite(Led1,HIGH);
      digitalWrite(Led2,HIGH);
      digitalWrite(Led3,LOW);
      digitalWrite(Led4,HIGH);
      if (!digitalRead(Minus))
      {
        Zeit = Zeit - Veraenderung;
      }
      if (!digitalRead(Plus))
      {
        Zeit = Zeit + Veraenderung;
      }
      if (Zeit < 0)
         Zeit = 0;
      DisplayValue = Zeit;
      delay(200);
     break;

     case 1000:
      digitalWrite(Led1,HIGH);
      digitalWrite(Led2,HIGH);
      digitalWrite(Led3,HIGH);
      digitalWrite(Led4,LOW);
      if (!digitalRead(Minus))
      {
        Zeit = Zeit - Veraenderung;
      }
      if (!digitalRead(Plus))
      {
        Zeit = Zeit + Veraenderung;
      }
     if (Zeit < 0)
         Zeit = 0;
     DisplayValue = Zeit;
      delay(200);
     break;
  }
  Serial.print(Veraenderung);
  Serial.print("    ");
  Serial.print(Zeit);
  Serial.println("   ");  
  DisplayValue = Zeit;
  val = digitalRead(SensorPin);
  if (val == HIGH)
  {
    digitalWrite(BuzzerPin,HIGH);
  }
  else
  {
    if(!randomGeneratorInitialized)
    {
       randomSeed(millis()); // use elapsed time as first start value for random generator
       randomGeneratorInitialized = true;
    }
    songNumber = random(1, NUMBER_OF_SONGS+1);
    mp3.play(songNumber);
    /*
    digitalWrite(BuzzerPin,LOW);
    delay(100);
    digitalWrite(BuzzerPin,HIGH);
    */
    for(int i=Zeit;i>0;i=i-1)
    {
      DisplayValue = i;
      delay(1000);
    }
    DisplayValue = 0;
    mp3.stop();
    /*
    digitalWrite(BuzzerPin,LOW);
    delay(2000);
    digitalWrite(BuzzerPin,HIGH);
    */
  }
}
