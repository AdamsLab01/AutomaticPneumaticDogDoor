//// LIBRAIES ////
#include <WiFi.h>
#include <Espalexa.h>
#include <millisDelay.h>

//// STATES ////
#define sIdle 0
#define sEspalexa 1
#define sDogDoorOpen 2
#define sDogDoorClose 3

int State = sIdle;

//// PIN NAMES ////
const int OnBoardLED = 2;
const int OutBreakBeam = 13;
const int DogDoorRly = 18;
const int OutPIR = 19;
const int InBreakBeam = 21;
const int RedLightRly = 22;
const int YellowLightRly = 23;
const int GreenLightRly = 25;
const int BuzzerRly = 26;
const int OpenButton = 27;
const int CloseButton = 32;

//// WIFI SETTINGS ////
const char* ssid = "YOUR_SSID_HERE";
const char* password = "YOUR_PASSWORD_HERE";

//// ESPALEXA AND WIFI STUFF ////
bool connectWifi();

void DogDoorChanged(EspalexaDevice* dev); // Espalexa callback function

EspalexaDevice* DogDoor; // Create espalexa device

bool wifiConnected = false;

Espalexa espalexa;

//// VARS ////
bool OnBoardLED_ON = false;
millisDelay OnBoardLED_Delay;
const unsigned long OnBoardLED_Time = 500;

millisDelay EspalexaDelay;
const unsigned long EspalexaTime = 1;

millisDelay BuzzerGreenOnDelay;
millisDelay BuzzerGreenOffDelay;
const unsigned long BuzzerGreenOnTime = 1000;
const unsigned long BuzzerGreenOffTime = 2000;

millisDelay BuzzerYellowOnDelay;
millisDelay BuzzerYellowOffDelay;
const unsigned long BuzzerYellowOnTime = 500;
const unsigned long BuzzerYellowOffTime = 2000;
bool BuzzerYellowFirstRun = true;

millisDelay BuzzerRedOnDelay;
millisDelay BuzzerRedOffDelay;
const unsigned long BuzzerRedOnTime = 100;
const unsigned long BuzzerRedOffTime = 1000;
bool BuzzerRedFirstRun = true;

millisDelay DoorOpenDelay;
millisDelay DoorClosedDelay;
const unsigned long DoorOpenTime = 15000;
const unsigned long DoorClosedTime = 15000;

//// SETUP ////

void setup() {
  //// PIN MODES ////
  pinMode(OnBoardLED, OUTPUT);
  pinMode(OutBreakBeam, INPUT_PULLUP);
  pinMode(DogDoorRly, OUTPUT);
  pinMode(OutPIR, INPUT);
  pinMode(InBreakBeam, INPUT_PULLUP);
  pinMode(RedLightRly, OUTPUT);
  pinMode(YellowLightRly, OUTPUT);
  pinMode(GreenLightRly, OUTPUT);
  pinMode(BuzzerRly, OUTPUT);
  pinMode(OpenButton, INPUT_PULLUP);
  pinMode(CloseButton, INPUT_PULLUP);

  //// CONNECT TO WIFI ////
  Serial.begin(115200);

  wifiConnected = connectWifi();

  if (!wifiConnected) {
    while (1) {
      Serial.println("Cannot connect to WiFi. Please check settings and reset the ESP.");
      delay(2500);
    }
  }

  //// ESPALEXA DEVICE SETUP ////
  DogDoor = new EspalexaDevice("Doggie Door", DogDoorChanged, EspalexaDeviceType::onoff); // Doggie Door is the key words used with Alexa "Alexa open *Doggie Door*"

  espalexa.addDevice(DogDoor);

  espalexa.begin();

  OnBoardLED_ON = false;
  OnBoardLED_Delay.start(OnBoardLED_Time);

  EspalexaDelay.start(EspalexaTime);

  BuzzerGreenOnDelay.start(BuzzerGreenOnTime);
  BuzzerGreenOffDelay.start(BuzzerGreenOffTime);

  DoorOpenDelay.start(DoorOpenTime);
  DoorClosedDelay.start(DoorClosedTime);

  BuzzerYellowOnDelay.start(BuzzerYellowOnTime);
  BuzzerYellowOffDelay.start(BuzzerYellowOffTime);

  BuzzerRedOnDelay.start(BuzzerRedOnTime);
  BuzzerRedOffDelay.start(BuzzerRedOffTime);

  DogDoor -> setValue(0);
}

//// LOOP ////

void loop() {
  switch (State) {
    case sIdle:
      fIdle();
      break;

    case sDogDoorOpen:
      fDogDoorOpen();
      break;

    case sDogDoorClose:
      fDogDoorClose();
      break;
  }
}

//// FUNCTIONS ////

void fIdle() {
  BuzzerYellowFirstRun = true;
  BuzzerRedFirstRun = true;

  espalexa.loop();
  fWait();

  if (digitalRead(OutPIR) == HIGH) {
    fOutMotion();
  }

  else {
    digitalWrite(GreenLightRly, LOW);
    digitalWrite(BuzzerRly, LOW);
  }

  if (digitalRead(OpenButton) == LOW) {
     DoorOpenDelay.start(DoorOpenTime);
    digitalWrite(GreenLightRly, LOW);
    digitalWrite(BuzzerRly, LOW);
    State = sDogDoorOpen;
  }

  if (DogDoor -> getValue() == 255) {
    DoorOpenDelay.start(DoorOpenTime);
    digitalWrite(GreenLightRly, LOW);
    digitalWrite(BuzzerRly, LOW);
    State = sDogDoorOpen;
  }
  fBlinkOnBoardLED();
}

void fDogDoorOpen() {
  DogDoor -> setValue(255);

  espalexa.loop();
  fWait();

  if (BuzzerYellowFirstRun == true) {
    BuzzerYellowOffDelay.finish();
  }

  fOpenSignal();
  digitalWrite(DogDoorRly, HIGH);

  if (digitalRead(InBreakBeam) == LOW || digitalRead(OutBreakBeam) == LOW || digitalRead(OutPIR) == HIGH) {
    DoorOpenDelay.restart();
  }

  if (digitalRead(OpenButton) == LOW) {
    DoorOpenDelay.restart();
  }

  if (DoorOpenDelay.justFinished()) {
    digitalWrite(YellowLightRly, LOW);
    digitalWrite(BuzzerRly, LOW);
    DoorClosedDelay.start(DoorClosedTime);
    State = sDogDoorClose;
  }

  if (DogDoor -> getValue() == 0) {
    digitalWrite(YellowLightRly, LOW);
    digitalWrite(BuzzerRly, LOW);
    DoorClosedDelay.start(DoorClosedTime);
    State = sDogDoorClose;
  }

  if (digitalRead(CloseButton) == LOW) {
    digitalWrite(YellowLightRly, LOW);
    digitalWrite(BuzzerRly, LOW);
    DoorClosedDelay.start(DoorClosedTime);
    State = sDogDoorClose;
  }
  fBlinkOnBoardLED();
}

void fDogDoorClose() {
  espalexa.loop();
  fWait();

  if (BuzzerRedFirstRun == true) {
    BuzzerRedOffDelay.finish();
  }

  fCloseSignal();
  digitalWrite(DogDoorRly, LOW); // Close the dog door.

  if (digitalRead(InBreakBeam) == LOW || digitalRead(OutBreakBeam) == LOW || digitalRead(OutPIR) == HIGH) {
    digitalWrite(RedLightRly, LOW);
    digitalWrite(BuzzerRly, LOW);
    DoorOpenDelay.start(DoorOpenTime);
    State = sDogDoorOpen; // Open the door.
  }

  if (digitalRead(OpenButton) == LOW) {
    digitalWrite(RedLightRly, LOW);
    digitalWrite(BuzzerRly, LOW);
    DoorOpenDelay.start(DoorOpenTime);
    State = sDogDoorOpen; // Open the door.
  }

  if (DoorClosedDelay.justFinished()) {
    digitalWrite(RedLightRly, LOW);
    digitalWrite(BuzzerRly, LOW);
    DogDoor -> setValue(0);
    State = sIdle;
  }
  fBlinkOnBoardLED();
}

void fWait() {
  if (EspalexaDelay.justFinished()) {
    EspalexaDelay.repeat();
  }
}

void fBlinkOnBoardLED() {
  if (OnBoardLED_Delay.justFinished()) {
    OnBoardLED_Delay.repeat();
    OnBoardLED_ON = !OnBoardLED_ON;
    if (OnBoardLED_ON) {
      digitalWrite(OnBoardLED, HIGH);
    } else {
      digitalWrite(OnBoardLED, LOW);
    }
  }
}

void fOutMotion() {
  if (BuzzerGreenOnDelay.justFinished()) {
    BuzzerGreenOnDelay.repeat();
    digitalWrite(BuzzerRly, LOW);
    digitalWrite(GreenLightRly, LOW);
  }

  if (BuzzerGreenOffDelay.justFinished()) {
    BuzzerGreenOffDelay.repeat();
    digitalWrite(BuzzerRly, HIGH);
    digitalWrite(GreenLightRly, HIGH);
  }
}

void fOpenSignal() {
  if (BuzzerYellowOnDelay.justFinished()) {
    digitalWrite(BuzzerRly, LOW);
    digitalWrite(YellowLightRly, LOW);
    BuzzerYellowOffDelay.start(BuzzerYellowOffTime);
  }

  if (BuzzerYellowOffDelay.justFinished()) {
    BuzzerYellowFirstRun = false;
    digitalWrite(BuzzerRly, HIGH);
    digitalWrite(YellowLightRly, HIGH);
    BuzzerYellowOnDelay.start(BuzzerYellowOnTime);
  }
}

void fCloseSignal() {
  if (BuzzerRedOnDelay.justFinished()) {
    digitalWrite(BuzzerRly, LOW);
    digitalWrite(RedLightRly, LOW);
    BuzzerRedOffDelay.start(BuzzerRedOffTime);
  }

  if (BuzzerRedOffDelay.justFinished()) {
    BuzzerRedFirstRun = false;
    digitalWrite(BuzzerRly, HIGH);
    digitalWrite(RedLightRly, HIGH);
    BuzzerRedOnDelay.start(BuzzerRedOnTime);
  }
}

// DogDoor callback function
void DogDoorChanged(EspalexaDevice * DogDoor) {
  if (DogDoor == nullptr) return; //this is good practice, but not required

  //do what you need to do here
  //EXAMPLE
  Serial.print("A changed to ");
  if (DogDoor->getValue()) {
    Serial.println("ON");
  }
  else {
    Serial.println("OFF");
  }
}

bool connectWifi() {
  bool state = true;
  int i = 0;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Connecting to WiFi");

  // Wait for connection
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i > 20) {
      state = false; break;
    }
    i++;
  }
  Serial.println("");
  if (state) {
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println("Connection failed.");
  }
  return state;
}
