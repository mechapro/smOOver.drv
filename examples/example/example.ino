// Example of driving a stepper motor using
// an Arduino and the AccelStepper library 
// via the smOOver.drv stepper motor driver
// available from mechapro.de

// standard libraries
#include <stdint.h>

// Arduino APIs
#include <SPI.h>

// third party libraries
#include <AccelStepper.h>
// http://www.airspayce.com/mikem/arduino/AccelStepper/AccelStepper_8h_source.html

// this demo
#include "smOOver.h"

// some pins we will use
// Arduino  -> smOOver.drv 10-pin Header
//    GND   -> 10 GND
//  2       ->  7 STEP
//  3       ->  8 DIR
//  7       ->  9 ENBL
//  6       ->  5 C5_SREG
// 10 CS    ->  4 CS_EXT
// 11 MOSI  ->  3 MOSI_EXT
// 12 MISO  ->  2 MISO_EXT
// 13 SCK   ->  1 SCK_EXT

const uint8_t pin_cs_sreg = 6; // for mode selection, must be low initially
const uint8_t pin_step = 2; // for mode selection, must be high initially
const uint8_t pin_dir = 3; // part of step-dir interface
const uint8_t pin_enbl = 7; // enables/disables current to motor
// static pin assignment
// CS   10 (can be changed)
// MOSI 11
// MISO 12
// SCK  13


SmOOver smoover(pin_cs_sreg, pin_step, pin_enbl /*, pin_chipselect is 10 */);

// these functions are for the AccelStepper library
// they will create step-dir pulses to drive the motor

void forwardstep()
{  
  digitalWrite(pin_dir, HIGH);
  digitalWrite(pin_step, LOW);
  delayMicroseconds(2); // 1.9us min
  digitalWrite(pin_step, HIGH);
}
 
void backwardstep()
{  
  digitalWrite(pin_dir, LOW);
  digitalWrite(pin_step, LOW);
  delayMicroseconds(2); // 1.9us min
  digitalWrite(pin_step, HIGH);
}

// setting up the AccelStepper library with custom actuator functions
AccelStepper stepper(forwardstep, backwardstep);

// general initialization
void setup()
{
  // you should only give V+ to smOOver.drv once this setup has happened
  // if the mode selection pins are undefined (Arduino unpowered),
  // the smOOver.drv will start up in another mode
  
  // pin_step is already set up (output) by SmOOver() for mode selection
	pinMode(pin_dir, OUTPUT);

  // communication with smOOver.drv, but possibly other peripherals too!
  SPI.begin();

  // we will have a little command line you can use to interact
  Serial.begin(9600);

  // AccelStepper settings
 	stepper.setMaxSpeed(10000); // limited by our calling stepper.run() in loop()
	stepper.setAcceleration(2000); // configures a square root ramp table

  Serial.println("ready.");
}

// helper function to see register values clearly
void print_bin(uint16_t val, uint8_t n)
{
  while (n--)
  {
    Serial.print((val >> n) & 1);
    if (n > 0 && n % 4 == 0) Serial.print("_");
  }
}

void loop()
{
  // AccelStepper: generate step-dir pulse, if due
  stepper.run();

  // a small command line processor
  if (!Serial.available()) return;
  
  String command = Serial.readStringUntil('\n');
  String arg;
  int split = command.indexOf(' ');
  bool have_arg = (split >= 0);

  if (have_arg)
  {
    arg = command.substring(split+1);
    command = command.substring(0, split);
  }

  Serial.print("> ");
  Serial.print(command);
  Serial.print(" ");
  Serial.print(arg);
  Serial.println(" ...");

  // AccelStepper-related commands
  
  if (command == "stop") { stepper.stop(); }

  if (command == "speed")
  {
    if (have_arg)
    {
      uint16_t speed_target = arg.toInt();
      stepper.setMaxSpeed(speed_target);
    }
    uint16_t speed_actual = stepper.maxSpeed();
    Serial.print("max speed == ");
    Serial.println(speed_actual);
  }

  if (command == "accel")
  {
    if (have_arg)
    {
      uint16_t accel_target = arg.toInt();
      stepper.setAcceleration(accel_target);
      Serial.print("acceleration set to ");
      Serial.println(accel_target);
    }
    else
    {
      Serial.println("sorry, AccelStepper does not remember that setting");
      // "Calling setAcceleration() is expensive, since it requires a square root to be calculated."
    }
  }

  if (command == "rel")
  {
    int16_t steps_rel = 1;
    if (have_arg) steps_rel = arg.toInt();
    stepper.move(steps_rel);
    Serial.print("moving ");
    Serial.print(steps_rel);
    Serial.println(" steps");
  }

  if (command == "abs")
  {
    int16_t steps_abs = 0;
    if (have_arg) steps_abs = arg.toInt();
    stepper.moveTo(steps_abs);
    Serial.print("moving to ");
    Serial.print(steps_abs);
    Serial.println(" steps absolute");
  }

  // smOOver.drv-related commands

  // these wiggle the ENABLE pin that controls power to the motor
  if (command == "en")
  {
    float amps_actual = smoover.get_current();
    if (amps_actual > 1.0)
    {
      Serial.print("WARNING: you have configured ");
      Serial.print(amps_actual, 1);
      Serial.println(" Ampere!");
    }
    smoover.enable_current(true);
    Serial.println("motor current enabled");
  }
  if (command == "dis")
  {
    smoover.enable_current(false);
    Serial.println("motor current disabled");
  }

  // get and set current (in Ampere)
  if (command == "cur")
  {
    if (have_arg)
    {
      float amps_target = arg.toFloat();
      float amps_actual = smoover.set_current(amps_target);
      Serial.print("current := ");
      Serial.print(amps_actual, 1);
      Serial.println(" A");
    }
    else
    {
      float amps_actual = smoover.get_current();
      Serial.print("current == ");
      Serial.print(amps_actual, 1);
      Serial.println(" A");
    }
  }

  // configure number of microsteps
  // 1..256 are allowed (powers of two)
  if (command == "micro")
  {
    if (have_arg)
    {
      int microsteps_target = arg.toInt();
      int microsteps_actual = smoover.set_microsteps(microsteps_target);
      Serial.print("microsteps := ");
      Serial.println(microsteps_actual);
    }
    else
    {
      int microsteps_actual = smoover.get_microsteps();
      Serial.print("microsteps == ");
      Serial.println(microsteps_actual);
    }
  }

  // read raw register values
  if (command == "reg")
  {
    uint8_t reg = have_arg ? arg.toInt() : 7; // 7 = status register
    uint16_t regval = smoover.get_register(reg);
    Serial.print("register ");
    Serial.print(reg);
    Serial.print(" == ");
    print_bin(regval, 16);
    Serial.println("");
  }

  return;
}
