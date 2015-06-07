//This is a test
#include <SD.h>
File dataFile;

const int redLed = A0;
const int power = A1;
const int sensor = A2;
const int gnd = A3;
const int gnd2 = A4;
const int chipSelect = 4;
const int BUFFER_SIZE = 512; 
uint8_t BUFFER[2][BUFFER_SIZE];
int active_buff=0;
int buffer_index=0;
int write_buffer=false;
void timer1_init(); // Counter for interrupts



//Clock is at 2MHz
// 5000 Hz = 2 000 000 / 400
long int timer1_period = 200; 
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






bool led_status = 0;
long int ledcnt = 0;
void setup()
{

  Serial.begin(9600);
  
  Serial.println("Starting up");

  pinMode(10, OUTPUT);
  pinMode(redLed, OUTPUT);
  pinMode(power, OUTPUT);
  digitalWrite(power, HIGH);
  pinMode(gnd, OUTPUT);
  digitalWrite(gnd, LOW);
  pinMode(gnd2, OUTPUT);
  digitalWrite(gnd2, LOW);


  Serial.println("Starting SD card");
  
  if(!SD.begin(chipSelect)){
    Serial.println("Initialization failed. Exiting");
    return;
  }
  
  Serial.println("Finished initializing. Opening file");
  
  dataFile = SD.open("data.txt", FILE_WRITE);
  
  if(dataFile)
  {
    Serial.println("Opened successfully");
    dataFile.close();
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
  if(write_buffer)
  {
    Serial.println("Writing data");
    
    //write whole buffer:
    dataFile = SD.open("data.txt", FILE_WRITE);
    if(dataFile){
      dataFile.write(BUFFER[1-active_buff],BUFFER_SIZE);
      dataFile.close();
    }
    write_buffer = false;
  }  
  
  Serial.println(".");
  //Use the LED to indicate activity
  if (buffer_index > 0 && BUFFER[active_buff][buffer_index-1] > 30){
    digitalWrite(redLed, HIGH);
  }
  
  //delay might be a bad idea here.
//  delay(1);
//  digitalWrite(redLed, LOW);

  

  
  if(ledcnt > 5000)
  {
    ledcnt = 0;
    digitalWrite(redLed, led_status);
    led_status = 1-led_status;
  } 
}


//Clunky, but this is TIMER1_OVF
ISR(TIMER1_OVF_vect)
{

  //add data to BUFFER[active_buffer]
  
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
