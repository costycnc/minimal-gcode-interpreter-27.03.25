// Ultra-lightweight G-Code Interpreter for foam cutting
// Version with timer interrupt - by CostyCNC
// https://costycnc.it

volatile char serialChar;
volatile int currentX = 0, targetX = 0, currentY = 0, targetY = 0;
volatile byte stepCounterX = 0, stepCounterY = 0;
volatile byte directionX = 0, directionY = 0;
volatile byte isMoving = 0;
bool isAbsoluteMode = 1;

// Stepper motor phase sequence
const byte stepSequence[] = {1, 3, 2, 6, 4, 12, 8, 9}; 

void setup() {
  // Configure motor pins
  DDRD |= 0b00111100;  // Pins 2-5 as output (X axis)
  DDRC |= 0b00001111;  // Pins A0-A3 as output (Y axis)
  
  // Configure Timer1 for Compare Match (CTC) at 1kHz
  TCCR1A = 0;
  TCCR1B = (1 << WGM12) | (1 << CS11); // CTC mode, prescaler 8
  OCR1A = 1999; // 1kHz frequency (16MHz / 8 / 2000)
  TIMSK1 = (1 << OCIE1A);
  
  Serial.begin(115200);
  Serial.println("Grbl CostyCNC-ISR Connected");
}

ISR(TIMER1_COMPA_vect) {
  if (isMoving) {
    // Control Motor X
    if (currentX != targetX) {
      PORTD = (PORTD & 0b11000011) | ((stepSequence[stepCounterX & 7] << 2) & 0b00111100);
      stepCounterX += directionX;
      currentX += directionX;
    }
    
    // Control Motor Y
    if (currentY != targetY) {
      PORTC = (PORTC & 0b11110000) | (stepSequence[stepCounterY & 7] & 0b00001111);
      stepCounterY += directionY;
      currentY += directionY;
    }
    
    if (currentX == targetX && currentY == targetY) {
      isMoving = 0;
    }
  }
}

void process_gcode(String cmd) {
  if (cmd.startsWith("G90")) isAbsoluteMode = 1;
  else if (cmd.startsWith("G91")) isAbsoluteMode = 0;
  else if (cmd.startsWith("G92")) currentX = targetX = currentY = targetY = 0;
  else if (cmd.startsWith("X")) {
    targetX = cmd.substring(1).toFloat() * 100;
    if (!isAbsoluteMode) targetX += currentX;
  }
  else if (cmd.startsWith("Y")) {
    targetY = cmd.substring(1).toFloat() * 100;
    if (!isAbsoluteMode) targetY += currentY;
  }
  
  // Calculate directions
  directionX = (targetX > currentX) ? 1 : -1;
  directionY = (targetY > currentY) ? 1 : -1;
  
  isMoving = 1;
  Serial.println("ok");
}

void loop() {
  static String commandBuffer;
  
  while (Serial.available()) {
    serialChar = Serial.read();
    
    if (serialChar == '\r') {
      process_gcode(commandBuffer);
      commandBuffer = "";
    } 
    else if (serialChar != ' ') {
      commandBuffer += serialChar;
    }
  }
  
  // Motor Timeout handling
  static unsigned long lastMoveTime = 0;
  if (isMoving) {
    lastMoveTime = millis();
  }
  else if (millis() - lastMoveTime > 2000) {
    PORTD &= 0b11000011; // Disable X motor
    PORTC &= 0b11110000; // Disable Y motor
  }
}
