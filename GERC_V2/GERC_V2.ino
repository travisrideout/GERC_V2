/*
 Name:		GERC_V2.ino
 Created:	12/31/2017 11:25:01 AM
 Author:	Travi
*/

/*
Gecko Error/Reset pin
This terminal functions as an ERROR output and as a RESET input. Because this terminal functions as both an input and an output,
some detailed description is necessary. When first testing the G320X, ERR / RES(Terminal 5) was connected to ENC + (term. 7). 
It can be left that way if it is not necessaryto read the state of the ERROR output. Otherwise, the following details are important.

The ERROR output is latched in the “ERROR” state(Terminal 5 = “0”) by the power-on reset circuitry in the G320X. It will stay in this
state indefinitely until it is cleared by applying + 5V to this terminal for at least 1 second.

The  voltage  on  this  terminal  is + 5VDC  when  the  G320X  is  functioning  normally.The  voltage  on  this  terminal  goes  to  0VDC
whenever the FAULT indicator is lit.This output can be used to signal your controller that an error has occurred.

Normally when the G320X is first powered up, it will be necessary to push the momentary switch to START for 1 second. This will
clear  the  power-on  reset  condition  and  extinguish  the  FAULT  LED.The  motor  will  then  be  enabled  and  the  drive  will  begin  to
operate. If at any time after that a condition occurs that causes the G320X to “FAULT out”, such as not being able to complete a
step command, the ERR / RES terminal will go to “0”, signaling to the controller an error has occurred. This will require the operator
to correct the problem that caused the FAULT and then push the switch to “START” for 1 second to re-enable the G320X.

At anytime the operator can push the switch to the “STOP” position to immediately halt the G320X drive.Anytime the G320X is
in the “FAULT” state(FAULT LED lit), all switching action stops, the motor freewheels and is unpowered */

static const uint8_t eStopPin = PIN2;
static const uint8_t xERPin = PIN3;
static const uint8_t yERPin = PIN4;
static const uint8_t enablePin = PIN5;
static const uint8_t ledPin = LED_BUILTIN;

bool eStop = true;
bool xER = false;
bool xERisInput = false;
bool yER = false;
bool yERisInput = false;
bool enable = true;
bool prev_enable = true;
bool led = false;

enum states {
	start,
	error,
	reset,
	eStopped,
	run,	
};

String stateStrings[] = {
	"Start",
	"Error",
	"Reset",
	"E-Stopped",
	"Run",	
};

states mState;
long counter = 0;
unsigned long debounce_time = 0;
uint16_t debouce_value = 250;
bool debouncing = false;

unsigned long blinkTime = 0;
uint16_t blinkRate = 500;

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
	Serial.println("GERC V2");

	pinMode(eStopPin, OUTPUT);
	pinMode(xERPin, OUTPUT);
	pinMode(yERPin, OUTPUT);
	pinMode(enablePin, INPUT);
	pinMode(led, OUTPUT);

	mState = start;
}

// the loop function runs over and over again until power down or reset
void loop() {
	FSM();
}

void FSM() {
	switch (mState) {
	case start:	//initial startup -> disable servos goto error state
		xERisInput = false; 
		yERisInput = false;
		xER = LOW;
		yER = LOW;
		setOutputs();
		printValues();
		delay(1000);		
		mState = error;
		break;
	case error:	// triggers e-stop on BOB		
		eStop = HIGH;
		led = false;
		setOutputs();
		printValues();
		delay(1000);
		mState = eStopped;
		break;
	case reset:	// enable servos, release e-stop		
		xERisInput = false;
		yERisInput = false;
		xER = HIGH;
		yER = HIGH;
		eStop = false;
		led = true;
		setOutputs();
		printValues();
		delay(1000);
		mState = run;
		break;
	case eStopped:	//	
		if (eStop) {
			eStop = false;
			setOutputs();
			printValues();
			delay(1000);
		}
		blinkLed();
		readInputs();
		if (prev_enable == false && enable == true) {
			mState = reset;
		}
		break;
	case run:	//	put ER pins in input mode, read inputs, 
		if (!xERisInput || !yERisInput) {
			xERisInput = true;
			pinMode(xERPin, INPUT_PULLUP);
			yERisInput = true;
			pinMode(yERPin, INPUT_PULLUP);
			printValues();
		}		
		readInputs();
		if (xER == LOW || yER == LOW) {
			mState = error;
		}
		break;
	default:
		break;
	}
}

void readInputs() {
	prev_enable = enable;

	bool temp_enable = digitalRead(enablePin);
	if (temp_enable != enable) {
		if (!debouncing) {
			//Serial.println("debouncing enable input change");
			debouncing = true;
			debounce_time = micros();
		} else if (micros() - debounce_time > debouce_value) {	
			
			enable = temp_enable;	
			debouncing = false;

			/*Serial.print("enable input changed: ");
			Serial.print("prev_enable = ");
			Serial.print(prev_enable);
			Serial.print(", enable = ");
			Serial.println(enable);*/
		}
	}

	if (debouncing&&micros() - debounce_time > debouce_value) {
		debouncing = false;
		//Serial.println("Reset debounce");
	}

	if (xERisInput) {
		xER = digitalRead(xERPin);
	}
	if (yERisInput) {
		yER = digitalRead(yERPin);
	}
}

void setOutputs() {
	digitalWrite(eStopPin, eStop);
	if (!xERisInput) {
		pinMode(xERPin, OUTPUT);
		digitalWrite(xERPin, xER);
	}
	if (!yERisInput) {
		pinMode(yERPin, OUTPUT);
		digitalWrite(yERPin, yER);
	}
	digitalWrite(ledPin, led);
}

void blinkLed() {
	if (millis() - blinkTime > blinkRate) {
		led = !led;
		digitalWrite(ledPin, led);
		blinkTime = millis();
	}
}

void printValues() {
	Serial.print(counter);
	Serial.print(" ");
	Serial.print("State = ");
	Serial.print(stateStrings[mState]);
	Serial.print(", eStopPin = ");
	Serial.print(eStop);
	Serial.print(", xERisInput = ");
	Serial.print(xERisInput);
	Serial.print(", xERPin = ");
	Serial.print(xER);
	Serial.print(", yERisInput = ");
	Serial.print(yERisInput);
	Serial.print(", yERPin = ");
	Serial.print(yER);
	Serial.print(", prev_enable = ");
	Serial.print(prev_enable);
	Serial.print(", enable = ");
	Serial.println(enable);
	counter++;
}
