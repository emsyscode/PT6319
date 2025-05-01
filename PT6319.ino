/****************************************************/
/* This is only one example of code structure       */
/* OFFCOURSE this code can be optimized, but        */
/* the idea is let it so simple to be easy catch    */
/* where we can do changes and look to the results  */
/****************************************************/
#define diodeLED 13 // Only to debug
#define VFD_data 8  // 
#define VFD_clk 9 // 
#define VFD_stb 10 // Must be pulsed to LCD fetch data of bus

//#define AdjustPins    PIND // before is C, but I'm use port C to VFC Controle signals

/*Global Variables Declarations*/
bool clockWakeUp = false;
bool clockAlarm = false;
bool setAlarm = false;
uint8_t myByte= 0x00;   // this variable is only related with swapLed1.
unsigned char hours = 0;
unsigned char minutes = 0;

uint8_t numberDigits = 0b00000010;  //this define number of digits used! On this panel is 5 grids
                                    //I don't find a complet datasheet it can help me on this definitions.
                                    //I used from the PT6315 by process through trial and error!
unsigned char secs;

unsigned char secU;
unsigned char secD;
unsigned char minU;
unsigned char minD;
unsigned char houU;
unsigned char houD;

unsigned char wakeSecsU;
unsigned char wakeSecsD;
unsigned char wakeMinuU;
unsigned char wakeMinuD;
unsigned char wakeHouU;
unsigned char wakeHouD;

unsigned char wakeSecs = 0;
unsigned char wakeMinutes = 0;
unsigned char wakeHours = 0;

unsigned int letter[54] ={
  //This not respect the normal table for 7segm like "hgfedcba"  // 
  //Because the display is only 7 segments, is not possible represent all char's.
  ////..decgfbah....76543210.........
      0b01111110, 0b00000000, //A // 
      0b11111000, 0b00000000, //B // 
      0b11001010, 0b00000000, //C  // 
      0b11110100, 0b00000000, //D  // 
      0b11011010, 0b00000000, //E  // 
      0b01011010, 0b00000000, //F  // 
      0b00000000, 0b00000000, //G  // 
      0b00000000, 0b00000000, //H  // 
      0b00000000, 0b00000000, //I  // 
      0b00000000, 0b00000000, //J  // 
      0b00000000, 0b00000000, //K  // 
      0b00000000, 0b00000000, //L  // 
      0b00000000, 0b00000000, //M  // 
      0b00000000, 0b00000000, //N  // 
      0b00000000, 0b00000000, //O  // 
      0b00000000, 0b00000000, //P  // 
      0b00000000, 0b00000000, //Q  // 
      0b00000000, 0b00000000, //R  // 
      0b00000000, 0b00000000, //S  // 
      0b00000000, 0b00000000, //T  // 
      0b00000000, 0b00000000, //U  // 
      0b00000000, 0b00000000, //V  // empty display
      0b00000000, 0b00000000, //X  // 
      0b00000000, 0b00000000, //Z  // 
      0b00000000, 0b00000000, //W  // empty display
      0b00000000, 0b00000000, //W  // empty display
  };
  unsigned int numbers[12][2] ={
  //This not respect the normal table for 7segm like "hgfedcba"  // 
  ////...decgfbah....76543210.........
      {0b11101110, 0b00000000}, //0  // 
      {0b00100100, 0b00000000}, //1  // 
      {0b11010110, 0b00000000}, //2  // 
      {0b10110110, 0b00000000}, //3  // 
      {0b00111100, 0b00000000}, //4  // 
      {0b10111010, 0b00000000}, //5  // 
      {0b11111010, 0b00000000}, //6  // 
      {0b00100110, 0b00000000}, //7  // 
      {0b11111110, 0b00000000}, //8  // 
      {0b00111110, 0b00000000}, //9  // 
      {0b00000000, 0b00000000}, //10 // empty display
  };
/*****************************************************************/
void pt6319_init(void){
  delay(200); //power_up delay
  // Note: Allways the first byte in the input data after the STB go to LOW is interpret as command!!!
  // Configure VFD display (grids)
  cmd_with_stb(numberDigits);//  cmd1 8 grids 20 segm, this is the minimum value of digits at pt6319
  delayMicroseconds(1);
  // turn vfd on, stop key scannig
   cmd_with_stb(0b10001000);//(BIN(01100110)); 
  delayMicroseconds(1);
  // Write to memory display, increment address, normal operation
  cmd_with_stb(0b01000000);//(BIN(01000000));
  delayMicroseconds(1);
  // Address 00H - 15H ( total of 11*2Bytes=176 Bits)
  cmd_with_stb(0b11000000);//(BIN(01100110)); 
  delayMicroseconds(1);
  // set DIMM/PWM to value
  cmd_with_stb((0b10001000) | 7);//0 min - 7 max  )(0b01010000)
  delayMicroseconds(1);
}
/*****************************************************************/
void cmd_without_stb(unsigned char a){
  // send without stb
  unsigned char data = 170; //value to transmit, binary 10101010
  unsigned char mask = 1; //our bitmask
  
  data=a;
  //This don't send the strobe signal, to be used in burst data send
   for (mask = 0b00000001; mask>0; mask <<= 1) { //iterate through bit mask
     digitalWrite(VFD_clk, LOW);
     if (data & mask){ // if bitwise AND resolves to true
        digitalWrite(VFD_data, HIGH);
     }
     else{ //if bitwise and resolves to false
       digitalWrite(VFD_data, LOW);
     }
    delayMicroseconds(5);
    digitalWrite(VFD_clk, HIGH);
    delayMicroseconds(5);
   }
   //digitalWrite(VFD_clk, LOW);
}
/*****************************************************************/
void cmd_with_stb(unsigned char a){
  // send with stb
  unsigned char data = 170; //value to transmit, binary 10101010
  unsigned char mask = 1; //our bitmask
  data=a;
  //This send the strobe signal
  //Note: The first byte input at in after the STB go LOW is interpreted as a command!!!
  digitalWrite(VFD_stb, LOW);
  delayMicroseconds(1);
   for (mask = 0b00000001; mask>0; mask <<= 1) { //iterate through bit mask
     digitalWrite(VFD_clk, LOW);
     delayMicroseconds(1);
     if (data & mask){ // if bitwise AND resolves to true
        digitalWrite(VFD_data, HIGH);
     }
     else{ //if bitwise and resolves to false
       digitalWrite(VFD_data, LOW);
     }
    digitalWrite(VFD_clk, HIGH);
    delayMicroseconds(1);
   }
   digitalWrite(VFD_stb, HIGH);
   delayMicroseconds(1);
}
/*****************************************************************/
void test_VFD(void){
    clear_VFD();
      digitalWrite(VFD_stb, LOW);
      delayMicroseconds(1);
      cmd_with_stb(numberDigits); // cmd 1 // 5 Grids & 16 numbers
      cmd_with_stb(0b01000000); // cmd 2 //Write VFD, Normal operation; Set pulse as 1/16, Auto increment
      cmd_with_stb(0b10001000 | 0x07); // cmd 2 //set on, dimmer to max
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(1);
        cmd_without_stb((0b11001000)); //cmd 3 wich define the start address (00H to 15H)
        // Only here must change the bit to test, first 2 bytes and 1/2 byte of third.
         //for (int i = 0; i < 8 ; i++){ // test base to 20 segm and 8 grids
            // Zone of test, if write 1 on any position of 3 bytes below position, will bright segment corresponding it.
            
            cmd_without_stb(0b00000000); // Data to fill first posição of memory belongs to the digit.
            cmd_without_stb(0b00000000); // Data to fill second posição of memory belongs to the digit.
         //}
      //cmd_without_stb(0b00000001); // cmd1 Here I define the 5 grids and 16 numbers
      //cmd_with_stb((0b10001000) | 7); //cmd 4
      digitalWrite(VFD_stb, HIGH);
      delay(1);
      delay(10);  
}
void test_letter(void){
    clear_VFD();
      digitalWrite(VFD_stb, LOW);
      delayMicroseconds(1);
      cmd_with_stb(numberDigits); // cmd 1 // 5 Grids & 16 numbers
      cmd_with_stb(0b01000000); // cmd 2 //Write VFD, Normal operation; Set pulse as 1/16, Auto increment
      cmd_with_stb(0b10001000 | 0x07); // cmd 2 //set on, dimmer to max
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(1);
        cmd_without_stb((0b11001000)); //cmd 3 wich define the start address (00H to 15H)
        // Only here must change the bit to test, first 2 bytes and 1/2 byte of third.
         //for (int i = 0; i < 8 ; i++){ // test base to 20 segm and 8 grids
            // Zone of test, if write 1 on any position of 3 bytes below position, will bright segment corresponding it.
            cmd_without_stb(letter[4]); // Data to fill first posição of memory belongs to the digit.
            cmd_without_stb(letter[5]); // Data to fill second posição of memory belongs to the digit.
         //}
      //cmd_without_stb(0b00000001); // cmd1 Here I define the 5 grids and 16 numbers
      //cmd_with_stb((0b10001000) | 7); //cmd 4
      digitalWrite(VFD_stb, HIGH);
      delay(1);
      delay(10);  
}
void test_numbers(void){
    clear_VFD();
      digitalWrite(VFD_stb, LOW);
      delayMicroseconds(1);
      cmd_with_stb(numberDigits); // cmd 1 // 5 Grids & 16 numbers
      cmd_with_stb(0b01000000); // cmd 2 //Write VFD, Normal operation; Set pulse as 1/16, Auto increment
      cmd_with_stb(0b10001000 | 0x07); // cmd 2 //set on, dimmer to max
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(1);
        cmd_without_stb((0b11001000)); //cmd 3 wich define the start address (00H to 15H)
        // Only here must change the bit to test, first 2 bytes and 1/2 byte of third.
         //for (int i = 0; i < 8 ; i++){ // test base to 20 segm and 8 grids
            // Zone of test, if write 1 on any position of 3 bytes below position, will bright segment corresponding it.
            cmd_without_stb(numbers[3][0]); // Data to fill first posição of memory belongs to the digit.
            cmd_without_stb(numbers[3][1]); // Data to fill second posição of memory belongs to the digit.
         //}
      //cmd_without_stb(0b00000001); // cmd1 Here I define the 5 grids and 16 numbers
      //cmd_with_stb((0b10001000) | 7); //cmd 4
      digitalWrite(VFD_stb, HIGH);
      delay(1);
      delay(10);  
}
/*****************************************************************/
void clear_VFD(void){
  /*
  Here I clean all registers 
  Could be done only on the number of grid
  to be more fast. The 12 * 3 bytes = 36 registers
  */
      for (uint8_t n=0; n < 12; n=n+3){  //
      //Note we here implement a skip by 3, because the address RAM of each grid is:
      //0, 3, 6, 9, C wich correspond to grids: 0 until 4 (total of 5 grids)
      //by this reason each cycle for will fill the 3 positions of RAM address.
        cmd_with_stb(numberDigits); //       cmd 1 // 8 Grids & 20 numbers is minimum value of digits to the pt6319
        cmd_with_stb(0b01000000); //       cmd 2 //Normal operation; Set pulse as 1/16
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(1);
            cmd_without_stb((0b11000000) | n); // cmd 3 //wich define the start address (00H to 15H)
            cmd_without_stb(0b00000000); // Data to fill first posição of memory belongs to the digit.
            cmd_without_stb(0b00000000); // Data to fill second posição of memory belongs to the digit.
            //cmd_4bitsWithout_stb(0b00000000); // Data to fill third posição of memory belongs to the digit.
            //
            //cmd_with_stb((0b10001000) | 7); //cmd 4
            digitalWrite(VFD_stb, HIGH);
            delayMicroseconds(10);
     }
}
/*****************************************************************/
void updateSetClock(void){
    if (secs >= 60){
      secs = 0;
      minutes++;
    }
    if (minutes >= 60){
      minutes = 0;
      hours++;
    }
    if (hours >= 24){
      hours = 0;
    }
   //*************************************************************
    secU = (secs%10);
    secD = (secs/10);
    sendTo7segDigitClock();
    //*************************************************************
    minU = (minutes%10);
    minD = (minutes/10);
    sendTo7segDigitClock();
    //**************************************************************
    houU = (hours%10);
    houD = (hours/10);
    sendTo7segDigitClock();
    //**************************************************************
}
/******************************************************************/
void updateSetWakeUp(void){
      if (wakeSecs >= 60){
        wakeSecs = 0;
        wakeMinutes++;
      }
      if (wakeMinutes >= 60){
        wakeMinutes = 0;
        wakeHours++;
      }
      if (wakeHours >= 24){
        wakeHours = 0;
      }
    //*************************************************************
    wakeSecsU = (wakeSecs%10);
    wakeSecsD = (wakeSecs/10);
    wakeUpSet();
    //*************************************************************
    wakeMinuU = (wakeMinutes%10);
    wakeMinuD = (wakeMinutes/10);
    wakeUpSet();
    //**************************************************************
    wakeHouU = (wakeHours%10);
    wakeHouD = (wakeHours/10);
    wakeUpSet();
    //**************************************************************
}
/******************************************************************/
void sendTo7segDigitClock(){
  // This block is very important, it solve the difference 
  // between numbers from grid 1 and grid 2(start 8 or 9)
  digitalWrite(VFD_stb, LOW);
  delayMicroseconds(10);
      cmd_with_stb(numberDigits); // cmd 1 // 5 Grids & 16 numbers
      cmd_with_stb(0b01000000); // cmd 2 //Normal operation; Set pulse as 1/16
        //
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(10);
        cmd_without_stb((0b11000000) | 0x00); //cmd 3 wich define the start address (00H to 15H)
        //The second byte of array is not used on this case, by this reason is commented.
        cmd_without_stb(numbers[houU][0]); //cmd_without_stb(numbers[houD][1]);
        cmd_without_stb(numbers[houD][0]); //cmd_without_stb(numbers[houU][1]);
        cmd_without_stb(0x00);// //----------------------------3
        cmd_without_stb(0x00);// //----------------------------4
        cmd_without_stb(0x00);// //----------------------------3
        cmd_without_stb(0x00);// //----------------------------4
        cmd_without_stb(numbers[minU][0]); //cmd_without_stb(numbers[minU][1]);
        cmd_without_stb(numbers[minD][0]); //cmd_without_stb(numbers[minD][1]);
        //
        cmd_without_stb(numbers[secU][0]); //cmd_without_stb(numbers[secU][1]);
        cmd_without_stb(numbers[secD][0]); //cmd_without_stb(numbers[secD][1]);
        //
      digitalWrite(VFD_stb, HIGH);
      delayMicroseconds(10);
      cmd_with_stb((0b10001000) | 7); //cmd 4
      delay(5);      
}
/***************************************************************************/
void wakeUpSet(){
  digitalWrite(VFD_stb, LOW);
  delayMicroseconds(10);
      cmd_with_stb(numberDigits); // cmd 1 // 8 Grids 
      cmd_with_stb(0b01000000); // cmd 2 //Normal operation; Set pulse as 1/16
      //
      digitalWrite(VFD_stb, LOW);
      delayMicroseconds(10);
      cmd_without_stb((0b11000000) | 0x00); //cmd 3 wich define the start address (00H to 15H)
      //The second byte of array is not used on this case, by this reason is commented.
        cmd_without_stb(numbers[wakeHouU][0]); //cmd_without_stb(numbers[houD][1]);
        cmd_without_stb(numbers[wakeHouD][0]); //cmd_without_stb(numbers[houU][1]);
        cmd_without_stb(0x00);// //----------------------------3
        cmd_without_stb(0x00);// //----------------------------4
        cmd_without_stb(0x00);// //----------------------------3
        cmd_without_stb(0x00);// //----------------------------4
        cmd_without_stb(numbers[wakeMinuU][0]); //cmd_without_stb(numbers[minU][1]);
        cmd_without_stb(numbers[wakeMinuD][0]); //cmd_without_stb(numbers[minD][1]);
        cmd_without_stb(numbers[wakeSecsU][0]); //cmd_without_stb(numbers[secU][1]);
        cmd_without_stb(numbers[wakeSecsD][0]); //cmd_without_stb(numbers[secD][1]);
        //
      digitalWrite(VFD_stb, HIGH);
      delayMicroseconds(10);
      cmd_with_stb((0b10001000) | 7); //cmd 4
      delay(5); 
}
void readButtonsClock(){
  // Pay attenntion to the Pin: 8. This is the pin will be used as INPUT-PULLUP comming from Arduino
  uint8_t val = 0x00;       // variable to store the read value
  uint8_t data = 0x00;
  uint8_t array[9] = {0,0,0,0,0,0,0,0};
  uint8_t mask = 0x00; //our bitmask
  
    digitalWrite(VFD_stb, LOW);
    delayMicroseconds(2);
    cmd_without_stb(0b01000010); // cmd 2 //Read Keys;Normal operation; Set pulse as 1/16
    digitalWrite(VFD_clk, LOW); //This line is importante because the clk go from LOW to HIGH! Reverse from write data to out.
    pinMode(8, INPUT_PULLUP);  // Important this point! Here I'm changing the direction of the pin to INPUT data.
    delayMicroseconds(2);
          for (int8_t z = 0; z < 2; z++){
            for (mask=0b10000000; mask >=1; mask = mask >> 1) { //iterate through bit mask
                        for (int8_t h = 8; h >0; h--) {
                            digitalWrite(VFD_clk, HIGH);  // Remember wich the read data happen when the clk go from LOW to HIGH! Reverse from write data to out.
                            delayMicroseconds(2);
                            val = digitalRead(8);
                                if (val & mask){ // if bitwise AND resolves to true
                                  array[h] = 1;
                                }
                                else{ //if bitwise and resolves to false
                                array[h] = 0;
                                }
                          digitalWrite(VFD_clk, LOW);
                          delayMicroseconds(2);
                        } 
            }
                Serial.print(z);
                Serial.print(" - ");
                          
                      for (uint8_t bits = 8 ; bits > 0; bits--) {
                          Serial.print(array[bits]);
                      }
                            if ((z==1) && (array[1] == 1)){
                                  if(clockAlarm == false){
                                  hours++;
                                  }
                                  else{
                                  wakeHours++;
                                  }
                            }
                            if ((z==1) && (array[2] == 1)){
                                if(clockAlarm == false){
                                hours--;
                                }
                                else{
                                wakeHours--;
                                }
                            }
                            if ((z==1) && (array[3] == 1)){
                                if(clockAlarm == false){
                                minutes++;
                                }
                                else{
                                wakeMinutes++;
                                }  
                            }
                            if ((z==1) && (array[4] == 1)){
                                if(clockAlarm == false){
                                minutes--;
                                }
                                else{
                                wakeMinutes--;
                                }  
                            }
                            if((z==1) && (array[5] == 1)){
                                clockAlarm = !clockAlarm;
                                digitalWrite(2, !digitalRead(2));
                            }
                            if((z==1) && (array[6] == 1)){
                              if(setAlarm == true){
                                setAlarm = false;
                                digitalWrite(3, LOW);
                              }
                              else{
                                setAlarm = true;
                                digitalWrite(3, HIGH);
                              }
                              wakeSecs=00;
                            }
                            if((z==1) && (array[7] == 1)){
                              //Not used yet.
                            }        
                    Serial.println();
            }  // End of "for" of "z"
        Serial.println();

  digitalWrite(VFD_stb, HIGH);
  delayMicroseconds(2);
  cmd_with_stb((0b10001000) | 7); //cmd 4
  delayMicroseconds(2);
  pinMode(8, OUTPUT);  // Important this point!  // Important this point! Here I'm changing the direction of the pin to OUTPUT data.
  delay(2); 
}
void compareTime(){
  //This is to the situation where the LED port of Driver are used/connected.
  //I have implemented a port at pin 3 to e one alarm output.
  if (setAlarm == true){
    //Serial.print(secs, DEC); Serial.println(wakeSecs, DEC); //Only to debug.
      if((hours == wakeHours) && (minutes == wakeMinutes) && (secs == wakeSecs)){
        swapLedAlarm(0b00011011);
        digitalWrite(3, LOW);
      }
      else{
        swapLedAlarm(0b00011110);
      }
  }
}
void swapLedAlarm(unsigned char alarmLED){
  unsigned char myByte = alarmLED;
    digitalWrite(VFD_stb, LOW);
    delayMicroseconds(20);
    cmd_without_stb(0b01000001);  //Write Data to LED Port. (pt6319 have 5 LED pins: 50,49,48,47,46)
    delayMicroseconds(20);
    //myByte = (statusAlarm);  // The buzzer of this panel is LED3
    cmd_without_stb(myByte);
    delayMicroseconds(20);
    digitalWrite(VFD_stb, HIGH);
    delayMicroseconds(20);
}
void offAllLEDs(){
  //The status On of LED happen applaing the GND on pin of LED. The common is to VCC.
    digitalWrite(VFD_stb, LOW);
    delayMicroseconds(20);
    cmd_without_stb(0b01000001);  //Write Data to LED Port. (pt6319 have 5 LED pins: 50,49,48,47,46)
    delayMicroseconds(20);
    myByte ^=(0b00011111);  // Here is only to invert bit of led 1.
    cmd_without_stb(myByte);
    delayMicroseconds(20);
    digitalWrite(VFD_stb, HIGH);
    delayMicroseconds(20);
}
void onAllLEDs(){
  //The status On of LED happen applaing the GND on pin of LED. The common is to VCC.
    digitalWrite(VFD_stb, LOW);
    delayMicroseconds(20);
    cmd_without_stb(0b01000001);  //Write Data to LED Port. (pt6319 have 5 LED pins: 50,49,48,47,46)
    delayMicroseconds(20);
    myByte ^=(0b00000000);  // Here is only to invert bit of led 1.
    cmd_without_stb(myByte);
    delayMicroseconds(20);
    digitalWrite(VFD_stb, HIGH);
    delayMicroseconds(20);
}
void setup() {
  // put your setup code here, to run once:
  pinMode(diodeLED, OUTPUT);
  pinMode(2, OUTPUT);
  //pinMode(diodeLED, OUTPUT);
  
  Serial.begin(115200);
  minutes =0x00;
  hours = 0x00;

    /*CS12  CS11 CS10 DESCRIPTION
    0        0     0  Timer/Counter1 Disabled 
    0        0     1  No Prescaling
    0        1     0  Clock / 8
    0        1     1  Clock / 64
    1        0     0  Clock / 256
    1        0     1  Clock / 1024
    1        1     0  External clock source on T1 pin, Clock on Falling edge
    1        1     1  External clock source on T1 pin, Clock on rising edge
  */
  // initialize timer1 
  cli();           // disable all interrupts
  //initialize timer1 
  //noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;// This initialisations is very important, to have sure the trigger take place!!!
  TCNT1  = 0;
  // Use 62499 to generate a cycle of 1 sex 2 X 0.5 Secs (16MHz / (2*256*(1+62449) = 0.5
  OCR1A = 62499;            // compare match register 16MHz/256/2Hz
  //OCR1A = 1000; //Change to low value to accelarate the clock to debug!  // compare match register 16MHz/256/2Hz
  TCCR1B |= (1 << WGM12);   // CTC mode
  TCCR1B |= ((1 << CS12) | (0 << CS11) | (0 << CS10));    // 256 prescaler 
  TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
 
  
  // Note: this counts is done to a Arduino 1 with Atmega 328... Is possible you need adjust
  // a little the value 62499 upper or lower if the clock have a delay or advnce on hours.
    
  //  a=0x33;
  //  b=0x01;

  CLKPR=(0x80);
  //Set PORT
  DDRD = 0xFF;  // IMPORTANT: from pin 0 to 7 is port D, from pin 8 to 13 is port B
  PORTD=0x00;
  DDRB =0xFF;
  PORTB =0x00;

  pt6319_init();

  test_VFD();

  clear_VFD();

  //only here I active the enable of interrupts to allow run the test of VFD
  //interrupts();             // enable all interrupts
  sei();
}
void loop() {
  // You can comment untill while cycle to avoid the test running.
  // Can use this cycle to teste all numbers of VFD
  delay(200);
          for(uint8_t i = 0x00; i < 2; i++){
              clear_VFD();
              //test_VFD();
              //test_letter();
              test_numbers();
              delay(500);
              clear_VFD();
              delay(500);
              //Serial.println(secs);
          }
    while(1){
              if(clockAlarm == false){
                  updateSetClock();
                  delay(200);
              }
              else{
                      updateSetWakeUp();
                      delay(200);
                  } 
            readButtonsClock();
            compareTime();      
          }  
    }
ISR(TIMER1_COMPA_vect)   {  //This is the interrupt request
      secs++;
} 
