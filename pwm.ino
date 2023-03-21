/* ---------------------------------------------------------------- *
 * Project : PWM signal analysis                                    *
 * Authors : Puzo Andrea                                            *
 * Course  : General Electronics Laboratory                         *
 * Board   : Arduino Micro                                          *
 * Copyright (c) 2023, Andpuv                                       *
 * ---------------------------------------------------------------- */

#define PIN_PWM_OUTPUT  3  // pwm output
#define PIN_PWM_INPUT   A0 // pwm filter
#define PIN_PWM_DIGITAL 8  // pwm digital
#define PIN_PWM_ANALOG  A1 // pwm analog

#define N_ANALOG  100
#define N_DIGITAL 500
#define N_SAMPLES 1000

// current state
int state = 0 ;

// command string
String com_str ;

int pwm_val = 127 ;

// used by edge function
int cur_val = 0 ;
int lst_val = 0 ;
int n_edge = -1 ;
unsigned int edge_time [3] ;

// to get a real value of memory usage we need to consider also
// the memory allocated at runtime on the stack, this macro shows
// stack usage
# define _DEBUG                                                  \
  Serial.println("### DEBUG START") ;                            \
  Serial.println(String("... STACK ADDR  = ") + SP) ;            \
  Serial.println(String("... STACK SIZE  = ") + (RAMEND - SP)) ; \
  Serial.println("### DEBUG END") ;

void setup ()
{
  // initialize the serial connection
  
  Serial.begin(9600) ;
  
  while (!Serial) {
    // connecting...  
  }
  
  Serial.println("START") ;

  // initialize the PIN mode
  
  pinMode( PIN_PWM_OUTPUT  , OUTPUT ) ;
  pinMode( PIN_PWM_INPUT   , INPUT  ) ;
  pinMode( PIN_PWM_DIGITAL , INPUT  ) ;
  pinMode( PIN_PWM_ANALOG  , INPUT  ) ;
}

void loop ()
{
  // scan the input from the serial port
  
  scan_input() ;

  // select the state and execute the command
  
  if (1 == state) {
    pwm_set() ;
  } else if (2 == state) {
    pwm_analog_read() ;
  } else if (3 == state) {
    _DEBUG
    pwm_pulse_read() ;
  } else if (4 == state) {
    _DEBUG
    pwm_meas_analog() ;
  } else if (5 == state) {
    _DEBUG
    pwm_meas_digital() ;
  } else if (6 == state) {
    _DEBUG
    pwm_meas_edge() ;
  } else if (7 == state) {
    analog_sample() ;
  } else if (8 == state) {
    digital_sample() ;
  }
}

void scan_input ()
{
  com_str = "" ;

  // check that there is data in the serial bus
  if (Serial.available() > 0) {
    delay(10) ;

    // read the input from the serial port
    
    for (;;) {
      char c = (char)Serial.read() ;
      
      if (isAscii(c)) {
        com_str += c ;
      }
      
      if (c == '\n')
        break ;
    }
     
    // parse the input
    
    if (com_str.startsWith("stop")) {
      // stop the execution
      state = 0 ;
    } else if (com_str.startsWith("set ")) { // at least one whitespace
      // set the pwm value
      state = 1 ;
    } else if (com_str.startsWith("read")) {
      // read the filtered value
      state = 2 ;
    } else if (com_str.startsWith("pulse")) {
      // pwm measurement using pulse
      state = 3 ;
    } else if (com_str.startsWith("analog")) {
      // pwm measurement using analog port
      state = 4 ;
    } else if (com_str.startsWith("digital")) {
      // pwm measurement using digital port
      state = 5 ;
    } else if (com_str.startsWith("edge")) {
      // pwm measurement using the rising/falling edges
      state = 6 ;
    } else if (com_str.startsWith("asample")) {
      // measure the analog sampling frequency
      state = 7 ;
    } else if (com_str.startsWith("dsample")) {
      // measure the digital sampling frequency
      state = 8 ;
    } else {
      Serial.println("ERROR: UNDEFINED STATE") ;
    }
  }
}

void pwm_set ()
{
  // ignore the first characters: `set `
  String arg = com_str.substring(4) ;

  // trim the string
  
  unsigned int i ;

  for (i = 0 ; i < arg.length() ; ++i) {
    if (' ' != arg.charAt(i))
      break ;  
  }

  // ignore the first `i` charcters of the string
  pwm_val = arg.substring(i).toInt() ;

  // check the range of the value
  if (pwm_val < 0 || 255 < pwm_val) {
    pwm_val = 127 ;
    Serial.println("PWM VALUE IS OUT OF RANGE, SET TO 127") ;
  }

  // write the value to the pwm output port
  analogWrite(PIN_PWM_OUTPUT, pwm_val) ;

  Serial.println(String("SET PWM VALUE TO ") + String(pwm_val)) ;

  // reset the state
  state = 0 ;
}

void pwm_analog_read ()
{
  // read the value from the filter
  Serial.println(String(">>> FILTER = ") + String(analogRead(PIN_PWM_INPUT))) ;
  
  // this helps the user to read the value
  delay(500) ;
}

void pwm_pulse_read ()
{
  // local variables are initialized here

  _DEBUG // display the stack size

  // measure the digital pulse
  
  unsigned long ht = pulseIn(PIN_PWM_DIGITAL, HIGH) ;
  unsigned long lt = pulseIn(PIN_PWM_DIGITAL, LOW) ;

  // calculate the pwm information
  
  unsigned long T = ht + lt ;
  float freq = 1000000.0 / (float)T ;
  float dc = (100.0 * (float)ht) / (float)T ;

  Serial.println(">>> PULSE") ;
  Serial.println(String("... PERIOD (us)    = ") + String(T)) ;
  Serial.println(String("... FREQ (Hz)      = ") + String(freq)) ;
  Serial.println(String("... DUTY CYCLE (%) = ") + String(dc)) ;

  // reset the state
  state = 0 ;
}

void pwm_meas_analog ()
{
  // local variables are initialized here

  _DEBUG // display the stack size

  // measure the analog values
  
  int meas [N_ANALOG] ;
  int i ;
  unsigned long t ;

  t = micros() ;
  for (i = 0 ; i < N_ANALOG ; ++i) {
    meas[i] = analogRead(PIN_PWM_ANALOG) ;
  }
  t = micros() - t ;
  
  float T = (float)t / N_ANALOG ;

  // --- algorithm ---
  // int hc = 0 ;
  // int lc = 0 ;
  // int rs = 0 ;
  // int fs = 0 ;
  // for (i = 1 ; i < N_ANALOG ; i++) {
  //   if (meas[i - 1] < 500 && 500 < meas[i]) {
  //     if (hc == 0) {
  //       rs = 1 ;
  //       fs = 0 ;
  //     } else {
  //       rs = 0 ;
  //       fs = 0 ;
  //     }
  //   }
  //   if (500 < meas[i - 1] && meas[i] < 500) {
  //     if (lc == 0) {
  //       rs = 0 ;
  //       fs = 1 ;
  //     } else {
  //       rs = 0 ;
  //       fs = 0 ;
  //     }
  //   }
  //   if (rs == 1) hc++ ;
  //   if (fs == 1) lc++ ;
  // }

  int hc = 0, lc = 0, status = 0 ;
  
  for (i = 1 ; i < N_ANALOG ; ++i) {
    // fewer branches, more speed
    if (meas[i - 1] < 500 && 500 < meas[i]) {
      status = 1 * (0 == hc) ;
    } if (500 < meas[i - 1] && meas[i] < 500) {
      status = 2 * (0 == lc) ;
    }
    hc += 1 == status ;
    lc += 2 == status ;
  }

  Serial.println(">>> `meas` = " + String((unsigned int)meas) + String(", ") + sizeof(meas)) ;

  Serial.println(">>> ANALOG") ;
  Serial.println(String("... N HIGH         = ") + String(hc)) ;
  Serial.println(String("... N LOW          = ") + String(lc)) ;

  float ht = (float)hc * T ;
  float lt = (float)lc * T ;

  // calculate the pwm information
  
  T = ht + lt ;

  float freq = 1000000.0 / T ;
  float dc = (100.0 * (float)ht) / T ;

  Serial.println(String("... PERIOD (us)    = ") + String(T)) ;
  Serial.println(String("... FREQ (Hz)      = ") + String(freq)) ;
  Serial.println(String("... DUTY CYCLE (%) = ") + String(dc)) ;

  // reset the state
  state = 0 ;
}

void pwm_meas_digital ()
{
  // local variables are initialized here

  _DEBUG // display the stack size

  // measure the digital values
  
  int meas [N_DIGITAL] ;
  int i ;
  unsigned long t ;

  t = micros() ;
  for (i = 0 ; i < N_DIGITAL ; ++i) {
    meas[i] = digitalRead(PIN_PWM_DIGITAL) ;
  }
  t = micros() - t ;
  
  float T = (float)t / N_DIGITAL ;

  // --- algorithm ---
  // int hc = 0 ;
  // int lc = 0 ;
  // int rs = 0 ;
  // int fs = 0 ;
  // for (i = 1 ; i < N_ANALOG ; i++) {
  //   if (0 == meas[i - 1] && 1 == meas[i]) {
  //     if (hc == 0) {
  //       rs = 1 ;
  //       fs = 0 ;
  //     } else {
  //       rs = 0 ;
  //       fs = 0 ;
  //     }
  //   }
  //   if (1 == meas[i - 1] && 0 == meas[i]) {
  //     if (lc == 0) {
  //       rs = 0 ;
  //       fs = 1 ;
  //     } else {
  //       rs = 0 ;
  //       fs = 0 ;
  //     }
  //   }
  //   if (rs == 1) hc++ ;
  //   if (fs == 1) lc++ ;
  // }

  int hc = 0, lc = 0, status = 0 ;

  for (i = 1 ; i < N_DIGITAL ; ++i) {
    // fewer branches, more speed
    if (0 == meas[i - 1] && 1 == meas[i]) {
      status = 1 * (0 == hc) ;
    } else if (1 == meas[i - 1] && 0 == meas[i]) {
      status = 2 * (0 == lc) ;
    }
    hc += 1 == status ;
    lc += 2 == status ;
  }

  Serial.println(">>> DIGITAL") ;
  Serial.println(String("... N HIGH         = ") + String(hc)) ;
  Serial.println(String("... N LOW          = ") + String(lc)) ;

  float ht = (float)hc * T ;
  float lt = (float)lc * T ;

  // calculate the pwm information
  
  T = ht + lt ;

  float freq = 1000000.0 / T ;
  float dc = (100.0 * (float)ht) / T ;

  Serial.println(String("... PERIOD (us)    = ") + String(T)) ;
  Serial.println(String("... FREQ (Hz)      = ") + String(freq)) ;
  Serial.println(String("... DUTY CYCLE (%) = ") + String(dc)) ;

  // reset the state
  state = 0 ;
}

void pwm_meas_edge ()
{
  // local variables are initialized here

  _DEBUG // display the stack size

  // measure the digital values
  
  cur_val = digitalRead(PIN_PWM_DIGITAL) ;

  if (-1 == n_edge) {
    lst_val = cur_val ;
    ++n_edge ;
  }

  // --- algorithm ---
  // if (0 == n_edge || 2 == n_edge) {
  //   if (0 == lst_val && 1 == cur_val) {
  //     edge_time[n_edge] = micros() ;
  //     ++n_edge ;
  //   }
  // } else if (1 == n_edge) {
  //   if (1 == lst_val && 0 == cur_val) {
  //     edge_time[n_edge] = micros() ;
  //     ++n_edge ;
  //   }
  // }

  if ( // fewer branches, more speed
    ((0 == n_edge || 2 == n_edge) && (0 == lst_val && 1 == cur_val)) ||
    ((1 == n_edge)                && (1 == lst_val && 0 == cur_val))
  ) {
    edge_time[n_edge] = micros() ;
    ++n_edge ;
  }

  lst_val = cur_val ;

  // calculate the pwm information

  if (3 == n_edge) {
    float T = (float)(edge_time[2] - edge_time[0]) ;
    float freq = 1000000.0 / T ;
    float dc = (100.0 * (float)(edge_time[1] - edge_time[0])) / T ;

    Serial.println(">>> TIME") ;
    Serial.println(String("... PERIOD (us)    = ") + String(T)) ;
    Serial.println(String("... FREQ (Hz)      = ") + String(freq)) ;
    Serial.println(String("... DUTY CYCLE (%) = ") + String(dc)) ;

    lst_val = 0 ;
    cur_val = 0 ;
    n_edge = -1 ;

    // reset the state
    state = 0 ;
  }
}

void analog_sample ()
{
  // sample the analog port

  int meas [N_SAMPLES] ;
  int i ;
  unsigned long t ;

  t = micros() ;
  for (i = 0 ; i < N_SAMPLES ; ++i) {
    // memory storage takes some time,
    // we have to verify that this is negligible
    meas[i] = analogRead(PIN_PWM_ANALOG) ;
  }
  t = micros() - t ;
  
  float T = (float)t / N_SAMPLES ;
  float n = 1000.0 / T ;

  Serial.println(">>> ANALOG SAMPLE (MEMORY)") ;
  Serial.println(String("... SAMPLE TIME (us) = ") + String(T)) ;
  Serial.println(String("... N MEAS PER ms    = ") + String(n)) ;

  // ... without memory

  t = micros() ;
  for (i = 0 ; i < N_SAMPLES ; ++i) {
    analogRead(PIN_PWM_DIGITAL) ;
  }
  t = micros() - t ;
  
  T = (float)t / N_SAMPLES ;
  n = 1000.0 / T ;

  Serial.println(">>> ANALOG SAMPLE (NO MEMORY)") ;
  Serial.println(String("... SAMPLE TIME (us) = ") + String(T)) ;
  Serial.println(String("... N MEAS PER ms    = ") + String(n)) ;

  // reset the state
  state = 0 ;
}

void digital_sample ()
{
  // sample the digital port

  int meas [N_SAMPLES] ;
  int i ;
  unsigned long t ;

  t = micros() ;
  for (i = 0 ; i < N_SAMPLES ; ++i) {
    // memory storage takes some time,
    // we have to verify that this is negligible
    meas[i] = digitalRead(PIN_PWM_DIGITAL) ;
  }
  t = micros() - t ;
  
  float T = (float)t / N_SAMPLES ;
  float n = 1000.0 / T ;

  Serial.println(">>> DIGITAL SAMPLE (MEMORY)") ;
  Serial.println(String("... SAMPLE TIME (us) = ") + String(T)) ;
  Serial.println(String("... N MEAS PER ms    = ") + String(n)) ;

  // ... without memory

  t = micros() ;
  for (i = 0 ; i < N_SAMPLES ; ++i) {
    digitalRead(PIN_PWM_DIGITAL) ;
  }
  t = micros() - t ;
  
  T = (float)t / N_SAMPLES ;
  n = 1000.0 / T ;

  Serial.println(">>> DIGITAL SAMPLE (NO MEMORY)") ;
  Serial.println(String("... SAMPLE TIME (us) = ") + String(T)) ;
  Serial.println(String("... N MEAS PER ms    = ") + String(n)) ;

  // reset the state
  state = 0 ;
}
