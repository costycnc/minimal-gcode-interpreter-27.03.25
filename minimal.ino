// Interprete GCode ultra-leggero per taglio polistirolo
// Versione con timer interrupt - by CostyCNC
// https://www.costycnc.it

volatile char c;
volatile int x0=0, x1=0, y0=0, y1=0;
volatile byte step_x=0, step_y=0;
volatile byte dir_x=0, dir_y=0;
volatile byte moving=0;

const byte xx[] = {1,3,2,6,4,12,8,9}; // Sequenza passi

void setup() {
  // Configura pin motori
  DDRD |= B00111100;  // Pin 2-5 come output (X)
  DDRC |= B00001111;  // Pin A0-A3 come output (Y)
  
  // Configura timer1 per interrupt (1kHz)
  TCCR1A = 0;
  TCCR1B = (1 << WGM12) | (1 << CS11); // CTC, prescaler 8
  OCR1A = 1999; // 1kHz (16MHz/8/2000)
  TIMSK1 = (1 << OCIE1A);
  
  Serial.begin(115200);
  Serial.println("Grbl CostyCNC-ISR");
}

ISR(TIMER1_COMPA_vect) {
  if(moving) {
    // Controllo motore X
    if(x0 != x1) {
      PORTD = (PORTD & B11000011) | ((xx[step_x & 7] << 2) & B00111100);
      step_x += dir_x;
      x0 += dir_x;
    }
    
    // Controllo motore Y
    if(y0 != y1) {
      PORTC = (PORTC & B11110000) | (xx[step_y & 7] & B00001111);
      step_y += dir_y;
      y0 += dir_y;
    }
    
    if(x0 == x1 && y0 == y1) moving = 0;
  }
}

void process_gcode(String cmd) {
  if(cmd.startsWith("G90")) absolute = 1;
  else if(cmd.startsWith("G91")) absolute = 0;
  else if(cmd.startsWith("G92")) x0 = x1 = y0 = y1 = 0;
  else if(cmd.startsWith("X")) {
    x1 = cmd.substring(1).toFloat() * 100;
    if(!absolute) x1 += x0;
  }
  else if(cmd.startsWith("Y")) {
    y1 = cmd.substring(1).toFloat() * 100;
    if(!absolute) y1 += y0;
  }
  
  // Calcolo direzioni
  dir_x = (x1 > x0) ? 1 : -1;
  dir_y = (y1 > y0) ? 1 : -1;
  
  moving = 1;
  Serial.println("ok");
}

void loop() {
  static String buffer;
  
  while(Serial.available()) {
    c = Serial.read();
    
    if(c == '\r') {
      process_gcode(buffer);
      buffer = "";
    } 
    else if(c != ' ') {
      buffer += c;
    }
  }
  
  // Timeout motori
  static unsigned long last_move = 0;
  if(moving) last_move = millis();
  else if(millis() - last_move > 2000) {
    PORTD &= B11000011; // Disabilita X
    PORTC &= B11110000; // Disabilita Y
  }
}
