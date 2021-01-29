#include <LiquidCrystal.h>
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

int pulsePin = A10;               
int blinkPin = 22;
int blinkbzr = 24;              
int lcd_key     = 0;
int adc_key_in  = 0;
int c_button = 0;
String txt = "";

volatile int BPM;                  
volatile int Signal;               
volatile int IBI = 600;            
volatile boolean Pulse = false;  
volatile boolean QS = false;      

byte heart[8] = {B00000,
  B01010,
  B11111,
  B11111,
  B11111,
  B01110,
  B00100,
  B00000
}; 


static boolean serialVisual = true;   

volatile int rate[10];                  
volatile unsigned long sampleCounter = 0;       
volatile unsigned long lastBeatTime = 0;   
volatile int P = 512;             
volatile int T = 512;         
volatile int thresh = 750; //525        
volatile int amp = 100;      
volatile boolean firstBeat = true;     
volatile boolean secondBeat = false;  

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

void setup()
{
  pinMode(blinkPin,OUTPUT);
  pinMode(blinkbzr, OUTPUT);        
  Serial.begin(115200);          
  interruptSetup();                                     
  lcd.begin(16, 2);           
  lcd.setCursor(0,0);
  lcd.print("SELECT TO START");
  lcd.createChar(1, heart);
}


void loop()
{
   serialOutput(); 
   
  if (QS == true) 
    {     
      serialOutputWhenBeatHappens();     
      QS = false;
    }
     
  delay(20);
}


void interruptSetup()
{     
  // Initializes Timer2 to throw an interrupt every 2mS.
  TCCR2A = 0x02;     // DISABLE PWM ON DIGITAL PINS 3 AND 11, AND GO INTO CTC MODE
  TCCR2B = 0x06;     // DON'T FORCE COMPARE, 256 PRESCALER 
  OCR2A = 0X7C;      // SET THE TOP OF THE COUNT TO 124 FOR 500Hz SAMPLE RATE
  TIMSK2 = 0x02;     // ENABLE INTERRUPT ON MATCH BETWEEN TIMER2 AND OCR2A
  sei();             // MAKE SURE GLOBAL INTERRUPTS ARE ENABLED      
} 

void serialOutput()
{   // Decide How To Output Serial. 
 if (serialVisual == true)
  {  
     arduinoSerialMonitorVisual('-', Signal);
  } 
 else
  {
      sendDataToSerial('S', Signal);    
   }        
}

void serialOutputWhenBeatHappens()
{    
  lcd_key = read_LCD_buttons();
  if(lcd_key == btnSELECT){
    c_button = 1;
  }
 if (serialVisual == true)
   {            
     Serial.print(" Heart-Beat Found ");  
     Serial.print("BPM: ");
     Serial.println(BPM);
   }
 else
   {
     sendDataToSerial('B',BPM);   
//     sendDataToSerial('Q',IBI);   
   } 

  
  if(c_button){
      if(BPM > 300){
        txt = "TIDAK TERDETEKSI";
        BPM = 0;
      }
      else if(BPM > 100){
        txt = " (TV)";
      }
      else if(BPM > 60){
        txt = " (N)";
      }
      else if(BPM < 60 && BPM > 1){ 
        txt = " (B)";
      }else{
         txt = "TIDAK TERDETEKSI";
      }

     lcd.clear();
     lcd.setCursor(2,0);
     lcd.print("Heart Monitor");
      
     lcd.setCursor(0,1);
     lcd.print("BPM : ");
     lcd.setCursor(4,1);
     lcd.write(1);
     lcd.setCursor(6,1);
     lcd.print(BPM);
     lcd.setCursor(9,1);
     lcd.print(txt);
  }else{
    c_button = 0;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("SELECT TO START");
  }

}

void arduinoSerialMonitorVisual(char symbol, int data )
{    
  const int sensorMin = 0;  
  const int sensorMax = 1024; 
  int sensorReading = data;
  int range = map(sensorReading, sensorMin, sensorMax, 0, 11);
}


void sendDataToSerial(char symbol, int data )
{
   Serial.print(symbol);
   Serial.println(data);                
}

ISR(TIMER2_COMPA_vect)
{  
  cli();                                    
  Signal = analogRead(pulsePin);             
  sampleCounter += 2;                        
  int N = sampleCounter - lastBeatTime;    
                                            
  if(Signal < thresh && N > (IBI/5)*3) 
    {      
      if (Signal < T)
      {                        
        T = Signal;
      }
    }

  if(Signal > thresh && Signal > P)
    {         
      P = Signal;                           
    }                                     

  //  NOW IT'S TIME TO LOOK FOR THE HEART BEAT
  if (N > 200)
  {                                 
    if ( (Signal > thresh) && (Pulse == false) && (N > (IBI/5)*3) )
      {        
        Pulse = true;                              
        digitalWrite(blinkPin,HIGH);               
        digitalWrite(blinkbzr,HIGH); 
        IBI = sampleCounter - lastBeatTime;       
        lastBeatTime = sampleCounter;              
  
        if(secondBeat)
        {                       
          secondBeat = false;              
          for(int i=0; i<=9; i++)
          {             
            rate[i] = IBI;                      
          }
        }
  
        if(firstBeat)
        {                         
          firstBeat = false;                  
          secondBeat = true;                
          sei();
          c_button = 0;
          delay(1000);                           
          return;                           
        }   
      word runningTotal = 0;                    

      for(int i=0; i<=8; i++)
        {          
          rate[i] = rate[i+1];               
          runningTotal += rate[i];        
        }

      rate[9] = IBI;                        
      runningTotal += rate[9];            
      runningTotal /= 10;                
      BPM = 60000/runningTotal;            
      QS = true;                   
      // QS FLAG IS NOT CLEARED INSIDE THIS ISR
    }                       
  }

  if (Signal < thresh && Pulse == true)
    {  
      digitalWrite(blinkPin,LOW); 
      digitalWrite(blinkbzr,LOW);           
      Pulse = false;                        
      amp = P - T;                         
      thresh = amp/2 + T;         
      P = thresh;        
      T = thresh;
    }

  if (N > 2500)
    {                        
      thresh = 512;                      
      P = 512;                           
      T = 512;                           
      lastBeatTime = sampleCounter;         
      firstBeat = true;          
      secondBeat = false;        
    }

  sei();                               
}

int read_LCD_buttons()
{
 adc_key_in = analogRead(0);   
 
 if (adc_key_in > 1000) return btnNONE; 
 // For V1.1 us this threshold
 if (adc_key_in < 50)   return btnRIGHT;  
 if (adc_key_in < 250)  return btnUP; 
 if (adc_key_in < 450)  return btnDOWN; 
 if (adc_key_in < 650)  return btnLEFT; 
 if (adc_key_in < 850)  return btnSELECT;  

 return btnNONE;  
}
