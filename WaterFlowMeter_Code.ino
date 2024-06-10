#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Initialize the LCD with the I2C address 0x27 and dimensions 16x2
LiquidCrystal_I2C lcd(0x27, 16, 2);

byte statusLed = 13;
byte sensorInterrupt = 0;  // 0 = digital pin 2
byte sensorPin = 2;
byte buzzerPin = 8;  // Buzzer connected to pin 8

// The hall-effect flow sensor outputs approximately 4.5 pulses per second per litre/minute of flow.
float calibrationFactor = 4.5;

volatile byte pulseCount;  

float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
int cnt = 0;
int limit;
unsigned long oldTime;
bool limitSet = false; // Flag to check if the limit is set

void setup() {
  // Initialize a serial connection for reporting values to the host
  Serial.begin(9600);

  // Set up the status LED line as an output
  pinMode(statusLed, OUTPUT);
  digitalWrite(statusLed, HIGH);  // We have an active-low LED attached

  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, HIGH);

  // Set up the buzzer pin as an output
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);  // Ensure the buzzer is off initially

  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  oldTime = 0;

  // The Hall-effect sensor is connected to pin 2 which uses interrupt 0.
  // Configured to trigger on a FALLING state change (transition from HIGH state to LOW state)
  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);

  // Initialize the LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("D10-WaterFlowSensor");
  lcd.setCursor(0, 1);
  lcd.print("*");
  delay(2000);
  lcd.clear();
}

void loop() {
  // Check if the limit is set
  if (!limitSet) {
    // Prompt user to enter limit via Serial Monitor
    Serial.println("Enter the limit value (mL): ");
    while (Serial.available() == 0) {
      // Wait for user input
    }
    limit = Serial.parseInt(); // Read the input value
    Serial.print("Limit set to: ");
    Serial.println(limit);
    limitSet = true; // Set the flag to true after setting the limit
  }

  if ((millis() - oldTime) > 1000) { // Only process counters once per second
    // Disable the interrupt while calculating flow rate and sending the value to the host
    detachInterrupt(sensorInterrupt);

    // Calculate the flow rate in litres per minute
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;

    // Note the time this processing pass was executed
    oldTime = millis();

    // Convert flow rate to ml/s
    flowMilliLitres = (flowRate / 60) * 1000;
    
    // Calculate the flow rate in ml/s
    float flowRateMlPerSec = flowRate / 60 * 1000;

    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;

    // Print the flow rate for this second in ml/s
    Serial.print("Flow rate: ");
    Serial.print(flowRateMlPerSec, 2); // Print the flow rate in ml/s
    Serial.print(" mL/s");
    Serial.print("\t"); // Print tab space

    // Print the cumulative total of millilitres flowed since starting
    Serial.print("Output Liquid Quantity: ");
    Serial.print(totalMilliLitres);
    Serial.println(" mL");

    // Display on the LCD
    lcd.clear();
    if (totalMilliLitres > 1000) {
      lcd.setCursor(0, 0);
      lcd.print("OVERUSE");
      lcd.print(flowRateMlPerSec, 2);
      lcd.print("mL/s");
      lcd.setCursor(0, 1);
    } else {
      lcd.setCursor(0, 0);
      lcd.print("Flow: ");
      lcd.print(flowRateMlPerSec, 2);
      lcd.print(" mL/s");
      lcd.setCursor(0, 1);
      lcd.print("Total: ");
    }
    lcd.print(totalMilliLitres);
    lcd.print(" mL");

    // Check if the total volume has exceeded the limit
    if (cnt < 1) {
      if (totalMilliLitres > limit) {
        // Activate the buzzer
        digitalWrite(buzzerPin, HIGH);
        cnt++;
        delay(3000);  // Buzzer on for 3 seconds
        digitalWrite(buzzerPin, LOW);
      }
    }

    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;

    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
  }
}

/*
 Interrupt Service Routine
 */
void pulseCounter() {
  // Increment the pulse counter
  pulseCount++;
}
