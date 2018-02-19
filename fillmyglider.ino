/*
fillmyglider

Process:
1) select which pumps are to be used
2) press start
3) pumps are started
4) every 10 liter increment is signalled with morse code.
5) if flow is interrupted for some seconds, a warning is buzzed
6) press stop when finished
7) pumps are stopped, total amount is signalled with morse code.
8) press start again if needed. Amount starts with zero.

*/


/* Types *******************************************************/

enum state_enum
{
  STATE_INIT,
  STATE_START_FILLING,
  STATE_FILLING,
  STATE_FINISHING
};

/* Constants and constand variables ****************************/

const int PIN_OUT_PUMP1    = 4;
const int PIN_OUT_PUMP2    = 5;
const int PIN_OUT_LED      = 6;
const int PIN_OUT_BUZZER   = 7;
const int PIN_IN_FLOWMETER = 2;   /* only pins 2 and 3 are external interrupts */
const int PIN_IN_STARTSTOP = 3;
const int PIN_IN_PUMP1     = 8;
const int PIN_IN_PUMP2     = 9;

const int MORSE_DI                   = 50;
const int MORSE_DAH                  = 150;
const unsigned long MORSE_BFO        = 800;

const unsigned long DEBOUNCE_TIME    = 400L;

const int PULSES_PER_LITER = 450;

const int MAX_NO_CHANGE_SECONDS = 5;

const boolean DEBUG        = true;

/* Variables ***************************************************/

enum state_enum state;
boolean startstop_pressed;
unsigned long impulses;
unsigned long last_debounce_time;



/* Functions ***************************************************/

void setup()
{
  state = STATE_INIT;
  startstop_pressed = false;
  last_debounce_time = 0;

  pinMode(PIN_OUT_PUMP1, OUTPUT);
  pinMode(PIN_OUT_PUMP2, OUTPUT);
  pinMode(PIN_OUT_LED, OUTPUT);
  pinMode(PIN_OUT_BUZZER, OUTPUT);
  pinMode(PIN_IN_FLOWMETER, INPUT_PULLUP);
  pinMode(PIN_IN_STARTSTOP, INPUT_PULLUP);
  pinMode(PIN_IN_PUMP1, INPUT_PULLUP);
  pinMode(PIN_IN_PUMP2, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_IN_STARTSTOP), i_startstop, FALLING);

  if (DEBUG)
  {
    Serial.begin(9600);
    Serial.println("Fillmyglider V0.1");
  }


}

void loop()
{
  int liters;
  int temp;
  static int liters_before;
  static unsigned long impulses_before;
  static unsigned long millis_last_flow;
  static unsigned long millis_start;
  enum state_enum new_state;

  switch (state)
  {
    case STATE_INIT:
      if (startstop_pressed)
      {
        startstop_pressed = false;
        impulses = 0;
        impulses_before = 0;
        liters_before = 0;
        millis_last_flow = 0;
        millis_start = millis();
        attachInterrupt(digitalPinToInterrupt(PIN_IN_FLOWMETER), i_flowcounter, RISING);
        if(digitalRead(PIN_IN_PUMP1))
        {
          digitalWrite(PIN_OUT_PUMP1, 1);
        }
        if(digitalRead(PIN_IN_PUMP2))
        {
          digitalWrite(PIN_OUT_PUMP2, 1);
        }
        tone(PIN_OUT_BUZZER, 1000, 300);
        if (DEBUG)
        {
          Serial.println("Change to STATE_START_FILLING");
        }
        new_state = STATE_START_FILLING;
      } else
      {
        new_state = STATE_INIT;
      }
    break;

    case STATE_START_FILLING:
      /* wait 5 seconds to allow water flow to start */
      if ((millis() - millis_start) > 5000L)
      {
        if (DEBUG)
        {
          Serial.println("Change to STATE_FILLING");
        }
        new_state = STATE_FILLING;
      } else
      {
        new_state = STATE_START_FILLING;
      }
    break;

    case STATE_FILLING:
      /* If Start/Stop button is pressed, finish pumping */
      if (startstop_pressed)
      {
        startstop_pressed = false;
        if (DEBUG)
        {
          Serial.println("Change to STATE_FINISHING");
        }
        tone(PIN_OUT_BUZZER, 1000, 300);
        new_state = STATE_FINISHING;
      } else
      {          
        /* If water is flowing */
        if (impulses > impulses_before)
        {
          millis_last_flow = millis();        
          liters = impulses_to_liters(impulses);
          if (DEBUG)
          {
            Serial.print("Impulse: ");
            Serial.print(impulses, DEC);
            Serial.print("Liters: ");
            Serial.print(liters, DEC);
            Serial.print("Liters_before: ");
            Serial.println(liters_before, DEC);
          }
 
          /* If liters has reached a new manifold of 10, audio output */
          for (int i = 1; i < 5; i++)
          {
            if ( (liters >= (i*10)) && (liters_before < (i*10)) )
            {
              morse(i);
            }
          }
          liters_before = liters;
        } else
        {
          /* if no water flowing for more than 5 seconds */
          if ((millis() - millis_last_flow) > 5000 )
          {
            if ((millis() % 500L) == 0)
            {
              tone(PIN_OUT_BUZZER, 1000, 250);
            }
          }
        }
        impulses_before = impulses;
        new_state = STATE_FILLING;
      }
    break;

    case STATE_FINISHING:
      digitalWrite(PIN_OUT_PUMP1, 0);
      digitalWrite(PIN_OUT_PUMP2, 0);
      delay(2000);
      detachInterrupt(digitalPinToInterrupt(PIN_IN_FLOWMETER));
      liters = impulses_to_liters(impulses);
      if (liters >= 10)
      {
        temp = liters / 10;
        morse(liters / 10);
        morse(liters - (temp * 10));
      } else
      {
        morse(liters);
      }
      
      if (DEBUG)
      {
          Serial.println("Change to STATE_INIT");
      }
      new_state = STATE_INIT;
    break;

    default:
      if (DEBUG)
      {
          Serial.println("Undefined state");
      }
      new_state = STATE_INIT;
    break;
    
  }
  state = new_state;

}

int impulses_to_liters (unsigned long impulses_in)
{
  unsigned long liters;

  liters = impulses_in / PULSES_PER_LITER;
  return (int)liters;
}


void morse(int input)
{
  char morsechar;
  int i = 0;

  if (DEBUG)
  {
    Serial.print("Morse: ");
    Serial.println(input, DEC);
  }

  morsechar = 0x30 + input;

  switch (morsechar)
  {
    case '0':
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DAH);
      delay(2 * MORSE_DAH);
    break;

    case '1':
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(2 * MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DAH);
      delay(MORSE_DAH + MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DAH);
      delay(MORSE_DAH + MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DAH);
      delay(MORSE_DAH + MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DAH);
      delay(2 * MORSE_DAH);
    break;
      
    case '2':
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(2 * MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(2 * MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DAH);
      delay(MORSE_DAH + MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DAH);
      delay(MORSE_DAH + MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DAH);
      delay(2 * MORSE_DAH);
    break;
      
    case '3':
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(2 * MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(2 * MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(2 * MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DAH);
      delay(MORSE_DAH + MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DAH);
      delay(2 * MORSE_DAH);
    break;
      
    case '4':
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(2 * MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(2 * MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(2 * MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(2 * MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DAH);
      delay(2 * MORSE_DAH);
    break;
      
    case '5':
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(2 * MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(2 * MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(2 * MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(2 * MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(MORSE_DI + MORSE_DAH);
    break;
      
    case '6':
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DAH);
      delay(MORSE_DAH + MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(2 * MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(2 * MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(2 * MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(MORSE_DI + MORSE_DAH);
    break;
      
    case '7':
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DAH);
      delay(MORSE_DAH + MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DAH);
      delay(MORSE_DAH + MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(2 * MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(2 * MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(MORSE_DI + MORSE_DAH);
    break;
      
    case '8':
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DAH);
      delay(MORSE_DAH + MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DAH);
      delay(MORSE_DAH + MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DAH);
      delay(MORSE_DAH + MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(2 * MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(MORSE_DI + MORSE_DAH);
    break;
      
    case '9':
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DAH);
      delay(MORSE_DAH + MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DAH);
      delay(MORSE_DAH + MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DAH);
      delay(MORSE_DAH + MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DAH);
      delay(MORSE_DAH + MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(MORSE_DI + MORSE_DAH);
    break;
      
    default:
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(2 * MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(2 * MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DAH);
      delay(MORSE_DAH + MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DAH);
      delay(MORSE_DAH + MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(2 * MORSE_DI);
      tone(PIN_OUT_BUZZER, MORSE_BFO, MORSE_DI);
      delay(MORSE_DI + MORSE_DAH);
    break;
  }
}

// ISR for counting the impulses of the water flow sensor
void i_flowcounter()
{
  impulses++;
}

// ISR for start/stop button
void i_startstop()
{
  if ((millis() - last_debounce_time) > DEBOUNCE_TIME)
  {
    startstop_pressed = true;
    last_debounce_time = millis();
  }

}

