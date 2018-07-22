/**
 * This code controls motor and responds to the clicker.  
 *
 * Number of Sensors: Barcode Reader, Picture Clicker, Motor start/stop.
 * 
 * PIN      configuration:
 * ---      --------------
 *  7       Reset pin, connect to 'Reset' pin on arduino
 *  4       Motor start/stop
 *  2       Clicker on/off
 *
 * Copyright (c) 2017 Mellowain
 * This software is released under the MIT license. See the attached LICENSE file for details.
 */
#include <Stepper.h>

/* 
 *  Calcualte steps per revolution = 360/step angle
 *  For Sampler its: 360/15 degree = 24 steps.
 */
const int stepsPerRevolution = 24;

// Initialize the stepper library on pins 8 through 11:
Stepper myStepper(stepsPerRevolution, 8, 9, 10, 11);

 

// Pins
const int MOTOR_SWITCH                = 4;        /* Motor start/stop */
const int CLICKER_ON                  = 0xF1;     /* Clicker on/off */
const int CLICKER_OFF                 = 0xF0;


// Messages between Arduino-PC
#define CARD_RESET                    ('r')
#define MOTOR_START                   ('1')
#define MOTOR_STOP                    ('2')


#define MAX_ARGC                      (5)
#define MAX_ARG_LEN                   (64)



#define DEBUG  // DEBUG or NODEBUG
#ifdef DEBUG
#define DBG_VAL(val,param)                  do {                           \
                                                 Serial.print(val, param); \
                                               } while(0);
                                  
#define DBG_MSG(msg)                        do {                           \
                                                 Serial.println(msg);      \
                                               } while(0);

#define DBG_MSG_VAL(pre_msg,val,post_msg)   do {                           \
                                                 Serial.print(pre_msg);    \
                                                 Serial.print(val);        \
                                                 Serial.println(post_msg); \
                                               } while(0);
                                    
#else
/* Disable all logs on serial console */
#define DBG_MSG_VAL(pre_msg,val,post_msg)
#define DBG_MSG(msg)
#define DBG_VAL(val,param)
#endif  // DEBUG ends

  

/* Software Reset function @ address 0 */
void (*resetCall)(void) = 0; 



/*
 * read into buffer until terminator received
 * timeout if not received within timeout_period mS
 * returns true if received within timeout period
 * returns false if not received, or no terminator received but buffer full
 */
bool read_string_until (char *data, 
                        const unsigned int length, 
                        const unsigned long timeout_period, 
                        const char terminator) 
{
                          
  unsigned int index = 0;
  unsigned long start_time = millis();
  
  while (index < length) 
  {
    /* check if time is up */
    if (millis() - start_time >= timeout_period) 
    {
       return false;  // no data in timeout period
    }

    /* if data, add to buffer */     
    if (Serial.available() > 0) 
    {
      char r = Serial.read();
      if (r == terminator) 
      {
        data [index] = 0;  // terminating null byte
        DBG_MSG("Command complete");
        return true;
      }
        data [index++] = r;
    }  
  }  
  return false;  // filled up without terminator
}


int motor_state = LOW;
int clicker_curr_state = LOW;
int stepCount = 0;         // number of steps the motor has taken

void setup() {
  Serial.begin(9600);
  // Initialize all the global variables, specially post warm reboot
  motor_state = LOW;
  clicker_curr_state = LOW;
  stepCount = 0;
  delay(500);
  DBG_MSG("Sampler is initalized!");
}


void loop() {   
  
  /* Commands based triggers */
  if(Serial.available()) 
  {
    char msg[128] = {0};

    /* Capture the incoming message */
    if (read_string_until(msg, sizeof(msg), 500, '!')) 
    {
      DBG_MSG_VAL("Rx: ", msg, "");
            
      /* Splitting string into tokens */
      char *token;
      char msg_tok[MAX_ARGC][MAX_ARG_LEN] = {0};
      byte idx = 0;
      token = strtok(msg, " ,-:;");
      while(token != NULL) 
      {
        strncat(msg_tok[idx], token, MAX_ARG_LEN - 1);
        idx++;
        token = strtok(NULL, " ,-:;");
      }
#if 0
#ifdef DEBUG
      for(idx = 0; idx < 5; idx++) 
      {
          DBG_MSG_VAL("Tokens: ", msg_tok[idx], "");
      }
#endif 
#endif
      
      /* Parsing starts here */
      if (strncmp(msg_tok[0], "START", sizeof(msg_tok)) == 0) 
      {
          DBG_MSG("Start motor");
          motor_state = HIGH;
      }
      else if (strncmp(msg_tok[0], "STOP", sizeof(msg_tok)) == 0) 
      {
          DBG_MSG("Stop motor");
          motor_state = LOW;
      }
      else if(strncmp(msg_tok[0], "ACK", sizeof(msg_tok)) == 0) 
      {
          DBG_MSG("Clicker done taking pictures"); 
          clicker_curr_state = LOW;;
      }
      else if(strncmp(msg_tok[0], "RESET", sizeof(msg_tok)) == 0) 
      {
          DBG_MSG("Card Reset called"); 
          motor_state = LOW;
          clicker_curr_state = LOW;
          stepCount = 0;
      }
      else if(strncmp(msg_tok[0], "REBOOT", sizeof(msg_tok)) == 0) 
      {
          DBG_MSG("Card Reboot called"); 
          setup();
      }
      else 
      {
          DBG_MSG("ERROR: Invalid command!");           
      }
    } 
  }
  
  
  if(motor_state) {
    
    if (clicker_curr_state)
    {
      DBG_MSG_VAL("Clicker ON, Steps: ", stepCount, "");
      byte buf[1] = {CLICKER_ON};
      Serial.write(buf, 1);
    }
    else
    {
      myStepper.step(1);
      DBG_MSG("clicker armed again");
      stepCount++;
      clicker_curr_state = HIGH;
    }
  }
}

