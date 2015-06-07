#include <SD.h>
File dataFile;
const int chipSelect = 4;
const int redLed = A0;
const int power = A1;
const int sensor = A2;
const int gnd = A3;
const int gnd2 = A4;

//Clock is at 2MHz
// 5000 Hz = 2 000 000 / 400
long int timer1_period = 200; 
int write_buffer=false;
//Work with timer 1 since it's 16 bit

void timer1_init()//makes timer 1 a 5000Hz counter
{
  //up to ICR1, PWM mode (mode 14)
  TCCR1B  |= (1<<WGM13);    
  TCCR1B  &= ~(1<<WGM12);   
  TCCR1A  |= (1<<WGM11);
  TCCR1A  |= (1<<WGM10);
  
  // set ICR1 to get a period of 20 ms
  ICR1=timer1_period;

  // initialize the duty cycle to be 50%  why?
  OCR1AH= timer1_period>>8;           
  OCR1AL= timer1_period&=255;
  
  //choose the mode to be set at OCR1A, clear at rollover       
  TCCR1A  |= (1<<(COM1A1));   
  TCCR1A  &= ~(1<<(COM1A0));
  TCCR1A  |= (1<<(COM1B1));   
  TCCR1A  &= ~(1<<(COM1B0));

        
  //set the frequency by modifying the prescaler /8. 
  TCCR1B  &= ~(1<<(CS12));
  TCCR1B  |= (1<<(CS11));
  TCCR1B  &= ~(1<<(CS10));
  
  //enable interrupt
  TIMSK1 |= (1<<(TOIE1));

}


const uint8_t BUFFER_SIZE = 512;
uint8_t BUFFER[2][BUFFER_SIZE];
int active_buff=0;
int buffer_index=0;





const int LED_pin = A0;
bool led_status = 0;
long int ledcnt = 0;
void setup()
{

  Serial.begin(9600);
  
  Serial.println("Starting up");

  pinMode(LED_pin, OUTPUT);
  pinMode(10, OUTPUT);

  Serial.println("Starting SD card");
  
  if(!SD.begin(chipSelect)){
    Serial.println("Initialization failed. Exiting");
    return;
  }
  
  Serial.println("Finished initializing. Opening file");
  
  dataFile = SD.open("DATA.TXT", FILE_WRITE);
  
  if(dataFile)
  {
    Serial.println("Opened successfully");
  }else{
    Serial.println("Error opening file");
  }
  

  //Initialize ICR
  timer1_init();

  //enable interrupts
//  interrupts();
  sei(); //global interrupts
  
  
  
  
}

void loop()
{
  
  
 // if(write_buffer)
 //   save_data(BUFFER[1-active_buffer]);
 
   if(write_buffer)
  {
    Serial.print("Writing ");
    
    //write whole buffer:
    dataFile = SD.open("DATA.TXT", FILE_WRITE);
    if(dataFile){
       Serial.print("data (");
      int numbits = dataFile.write(BUFFER[1-active_buff],BUFFER_SIZE);
      dataFile.close();
      Serial.print(numbits);
      Serial.println(" bytes)");
      
    }else{
      Serial.println(" failed"); 
    }
    write_buffer = false;
  }  
  
  
  
  if(ledcnt > 5000)
  {
    ledcnt = 0;
    digitalWrite(LED_pin, led_status);
    led_status = 1-led_status;
  } 
}


//Clunky, but this is TIMER1_OVF
ISR(TIMER1_OVF_vect)
{
 //add data to BUFFER[active_buffer]
  digitalWrite(LED_pin,1);
  
  //micros() is fast & uses Timer0. Need to convert from unsigned long.
  uint16_t time = (uint16_t) micros();
  //low byte
  BUFFER[active_buff][buffer_index] = (byte)time;
  buffer_index++;
  //high byte
  BUFFER[active_buff][buffer_index] = (time >> 8);
  buffer_index++;
  
  //analogRead takes 100micros, so this might be a problem
  //low byte
  uint16_t val = analogRead(sensor);
  BUFFER[active_buff][buffer_index] =  (byte)val;
  buffer_index++;  
  //high byte
  BUFFER[active_buff][buffer_index] = (val >> 8);
  buffer_index++;

  //Check if we need to switch buffers
  if(buffer_index >= BUFFER_SIZE)
  {
    buffer_index = 0;
    active_buff = 1-active_buff;
    write_buffer = true;
  }

  ledcnt++;
}
