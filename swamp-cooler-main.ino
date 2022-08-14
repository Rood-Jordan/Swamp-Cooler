#include <config.h>
#include <ds3231.h>
#include <Wire.h>

#include <LiquidCrystal.h>
#include <Stepper.h>
#include <dht.h>

#define DHT_PIN 2

//note to use the pins SDA/SCL pins 20 and 21

volatile unsigned char* my_admux = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* ADC_DATA = (unsigned int*) 0x78;

volatile unsigned char* waterPin = (unsigned char*) 0x61; //water level input port  A0
volatile unsigned char* powerWaterPin = (unsigned char*) 0x07; //power


volatile unsigned char* port_b = (unsigned char*) 0x25; //input port 12
volatile unsigned char* ddr_b = (unsigned char*) 0x24; 
volatile unsigned char* pin_b = (unsigned char*) 0x23;


  //Vent system Calls
  volatile unsigned char* port_k = (unsigned char*) 0x108;//Vent port calls
  volatile unsigned char* ddr_k = (unsigned char*) 0x107;//only need first 4 pins for output
  int curangle;//Need to have this for the vent system to work; dont ask why.
  //I tried it and it worked. if its not called this early itll set it to 0 always

volatile unsigned char *pin_k = (unsigned char*) 0x106;

volatile unsigned char* pin_f = (unsigned char*) 0x2F;
volatile unsigned char* ddr_f = (unsigned char*) 0x30;
volatile unsigned char* port_f = (unsigned char*) 0x31;

volatile unsigned char* pin_d = (unsigned char*) 0x29;
volatile unsigned char* ddr_d = (unsigned char*) 0x2A;
volatile unsigned char* port_d = (unsigned char*) 0x2B;

LiquidCrystal lcd(22, 23, 24, 25, 26, 27);
dht DHT;

struct ts t;

void setup() {
  Serial.begin(9600);
  
  SetupADC();

  Wire.begin();
  DS3231_init(DS3231_CONTROL_INTCN);
  t.hour = 1;
  t.sec = 0;
  t.min = 30;
  DS3231_set(t);

  //*ddr_d &= 0x00;
  //pinMode(21, INPUT);
  //EIMSK |= (1 << INT0);
  //EICRA |= (1 << ISC01);
  
  lcd.begin(16, 2);
  
  *ddr_b = B00001111; //set PB3-PB4 to input for pushbuttons

  //*ddr_b |= 0x03; //set fan pins to output

  *ddr_k |= 0x0F; //setup motor pins for vent output

  *ddr_f = B00001111; //set fan and led pins to output
//*port_d |= 0xff;

}

int toggleState = false;
int lcdState = false;
int stopButton = false;

void loop() {

  int num = readIn(5);
  //Serial.println(num);
  
  if(num == 0){
    Serial.print("Disabled\n");
    disabledState();
  }
  else{
      Serial.print("System on!\n");
      if(temperature() <= 27){
        idleState();
      }
      if(temperature() > 27){
        runningState();
      }
      if(readIn(6) == 0){
         reset();
      }
      if(Waterlevel() < 350){
        error();
      }         
  }

}

void displayTime(){
  DS3231_get(&t);
  Serial.print(t.hour);
  Serial.print(":");
  if(t.min < 10){
     Serial.print("0");
     Serial.print(t.min);
  }
  else{
    Serial.print(t.min);
  }
  Serial.print(".");
  if(t.sec < 10){
     Serial.print("0");
     Serial.print(t.sec);
  }
  else{
    Serial.print(t.sec);
  }
 Serial.print("\n");
  
}

void disabledState(){

    *port_f |= B00000010; //setting yellow led to on
    lcd.setCursor(0, 0);
    lcd.print("DISABLED ");
   // Serial.println(readIn(5));
   readIn(5);
    while(readIn(5) < 500){
      Ventgobrr();

    }
    *port_f &= B11111101; //setting yellow led to off
    lcd.clear();         
  
}

void idleState(){
  float temp, humid;
  Serial.print("Changed to idle state at ");
  displayTime();
 *port_f |= B00000001; //setting green led to on
  while(temperature() <= 27){
 //   Serial.println(Waterlevel());
    temp = temperature();
    humid = humidity();
    lcdFunction(temp, humid);
   // Serial.println(readIn(6));
    Ventgobrr(); 
    //Serial.println(readIn(5));
  readIn(5);
    if(readIn(5) < 500){
      lcd.clear();
      *port_f &= B11111110;
      disabledState();
      *port_f |= B00000001;      
    }
    Waterlevel();
    //Serial.println(Waterlevel());
    if(Waterlevel() < 350){
      *port_f &= B11111110;
      lcd.clear();
      error();
      *port_f |= B00000001;
    }

  }
   //  Serial.print("end while\n");
  *port_f &= B11111110; //setting green led to off
  lcd.clear();
}

void runningState(){
  float temp, humid;
  Serial.print("Changed to running state at ");
  displayTime();
  *port_b |= B00000010;
  *port_f |= B01001000; //setting blue led to on and fan on
  while(temperature() > 27){
    //check switch input
    //look for state changes
    //changing vent angle
    temp = temperature();
    humid = humidity();
    lcdFunction(temp, humid);
    Ventgobrr();
    //Serial.println(readIn(5));
        readIn(5);
        if(readIn(5) < 128){
      *port_b &= B11111101;
      *port_f &= B00000000;
      lcd.clear();
      disabledState();
      *port_f |= B01001000;
      *port_b |= B00000010;
    }
    Waterlevel();
    if(Waterlevel() < 350){
      *port_b &= B11111101;
      *port_f &= B00000000;
      lcd.clear();
      error();
      *port_f |= B01001000;
      *port_b |= B00000010;
    }


  }
  *port_b &= B11111101;
  *port_f &= B11110111; //setting blue led to off and fan off
  lcd.clear();

}


void error(){
  Serial.print("Changed to error state at ");
  displayTime();
  *port_f &= B00000000; //turn fan off
  *port_f |= B00000100; //setting red led to on
  lcd.setCursor(0, 0);
  lcd.print(" Water level is "); 
  lcd.setCursor(0, 1);
  lcd.print("    too low! ");
  //Serial.println(readIn(6));
  readIn(6);
  while(readIn(6) < 150){
    
      //Serial.println("waiting for switch and water level");
      //*port_f &= B11111011; //setting red led to off
      //reset();
    
  }
  *port_f &= B11111011; //setting red led to off
  lcd.clear();
}

void reset(){
  lcd.clear();
  idleState();
}


void SetupADC(){
  *my_ADCSRA |= 0x80; 
  *my_ADCSRA &= 0B11011111; 
  *my_ADCSRA &= 0B11110111; 
  *my_ADCSRA &= 0B11111000;
  
  *my_ADCSRB &= 0B11110111;
  *my_ADCSRB &= 0B11111000;
  
  *my_admux &= 0B01111111;
  *my_admux |= 0B01000000;
  *my_admux &= 0B11011111;
  *my_admux &= 0B11100000;
}

unsigned int readIn(unsigned char num){
   
  *my_admux &= 0B11100000;

  *my_ADCSRB &= 0B11110111;
 
  if(num > 7)
  {
    num -= 8;
    *my_ADCSRB |= 0B00001000;
  }
  
  *my_admux  += num;
  *my_ADCSRA |= 0B01000000;
  while((*my_ADCSRA & 0B01000000) != 0);

  return *ADC_DATA;
}

int Waterlevel() {
 
  *powerWaterPin |= 0x01 << 7;
  unsigned int value = analogRead(7); // to be tested
  *powerWaterPin &= ~(0x01 << 7); 
  return value;
}


float temperature(){
  DHT.read11(DHT_PIN);
  return DHT.temperature;
}

float humidity(){
  DHT.read11(DHT_PIN);
  return DHT.humidity;
}

void lcdFunction(float tempDisplay, float humDisplay){
  //display humidity
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Humidity: ");
  lcd.setCursor(10, 0);
  lcd.print(humDisplay);
  lcd.setCursor(15, 0);
  lcd.print("%");
  
  //display temperature
  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.setCursor(6, 1);
  lcd.print(tempDisplay); 
  lcd.setCursor(13, 1);
  lcd.print("C");
  delay(1000);
}

//Begin Vent Calls

void Ventgobrr(){//This is the main call to allow for a vent angle change
  int angle;
  int desangle;

 if(Serial.available() != 0){
    angle = Serial.parseInt();
    if(angle != 0){//This will remove a parseint() issue,
      //where itll input a 0 if nothing is in for a while
      //this consequently makes it so you cant have a 0 degree vent
      //so you need to call at lowest 1 step
    desangle = angle;
    curangle = Ventangle(curangle,desangle);//Calls the main ventangle math
    Serial.print("Moved to an angle of roughly ");
    Serial.print(curangle);
    Serial.println(" degrees");
  }
 }
}

int Ventangle(int current, int desired){//while not a perfect math solution, 128 steps
  int modstep = current - desired;      //in 90  degrees is close. i dont want to deal with 
  if (modstep < 0){                     //trunkation issues
    coclockwi(-modstep);          //counter clockwise for increasing angle
    }
  if (modstep > 0){               //clockwise for decreasing angle
    clockwi(modstep);
    }
current = desired;
return (current);
}

void onestepfo(){ //Note that beacuse im &0x0Z we cant use something else in K unless we change it
//write(1,0,0,0);
    *port_k &= 0x00;
    *port_k |= 0x01;
  delay(5);
//write(1,1,0,0);
    *port_k &= 0x00;
    *port_k |= 0x03;
  delay(5);
//write(0,1,0,0);
    *port_k &= 0x00;
    *port_k |= 0x02;
  delay(5);
//write(0,1,1,0);
    *port_b &= 0x00;
    *port_k |= 0x06;
  delay(5);
//write(0,0,1,0);
    *port_k &= 0x00;
    *port_k |= 0x04;
  delay(5);
//write(0,0,1,1);
    *port_k &= 0x00;
    *port_k |= 0x0C;
  delay(5);
//write(0,0,0,1);
    *port_k &= 0x00;
    *port_k |= 0x08;
  delay(5);
//write(1,0,0,1);
    *port_k &= 0x00;
    *port_k |= 0x09;
  delay(5);
}

void onestepba(){//Same deal here as onestepfo
    *port_k &= 0x00;
    *port_k |= 0x09;
  delay(5);
    *port_k &= 0x00;
    *port_k |= 0x08;
  delay(5);
    *port_k &= 0x00;
    *port_k |= 0x0C;
  delay(5);
    *port_k &= 0x00;
    *port_k |= 0x04;
  delay(5);
    *port_k &= 0x00;
    *port_k |= 0x06;
  delay(5);
    *port_k &= 0x00;
    *port_k |= 0x02;
  delay(5);
    *port_k &= 0x00;
    *port_k |= 0x03;
  delay(5);
    *port_k &= 0x00;
    *port_k |= 0x01;
  delay(5);
  }

void clockwi(int steps){ //basic step increaser
  int i = 0;
while(i < steps){
onestepfo();
i++;
}}
void coclockwi(int steps){ //basic step increaser
  int i = 0;
while(i < steps){
onestepba();
i++;
}}

//End Vent calls
