#include <SevSeg.h>
#include <TM1637.h>

// currently using an Ardiuno Mega

const auto Potentiometer = A0;
const auto AirMeter = A1;
const auto Compressor = 40;
const auto Valve = 41;

const byte numDigits = 4;
const byte digitPins[] = { 53, 51, 52, 50 };
const byte segmentPins[] = { 23, 24, 33, 31, 30, 22, 34, 32 };

TM1637 tm1637(2, 3);

SevSeg ss;

void setup() 
{
  pinMode(Compressor, OUTPUT);
  pinMode(Valve, OUTPUT);
  
  ss.begin(COMMON_CATHODE, numDigits, digitPins, segmentPins);

  tm1637.init();
  tm1637.setBrightness(5);
  
  Serial.begin(115200);
}

enum class State
{
  Idle,
  Wait,
  Measure,
  Inflate,
  Deflate,
  Done
};

State state = State::Idle;
float setPressure = 0.0f;
int lastStateChange = millis();
auto lastPot = 0.0f;

const float PressureDelta = 0.05;


void loop() 
{
  ss.refreshDisplay();

  const auto pressure = analogRead(AirMeter) * 2.06f / 1023; // 30 psi = 5V for my transducer
  auto newState = state;

  auto pot = analogRead(Potentiometer) * 2.0f / 1023.0f; // 2 bar max for kart tyre
  if (abs(pot - lastPot) < 0.02) 
     pot = lastPot;
  else
     lastPot = pot;

  switch(state) {
  case State::Idle:
    setPressure = pot;
    ss.setNumber(int(setPressure * 100));
    tm1637.dispNumber(int(setPressure * 100));

    if (pressure > 0.1) 
      newState = State::Wait;    
    break;
    
  case State::Wait:
    if (pressure < 0.1) {
      newState = State::Idle;
    } else if (millis() > lastStateChange + 500) {
      newState = State::Measure;
    }
    break;

  case State::Measure:
    ss.setNumber(pressure);
    if (pressure > setPressure + PressureDelta)
      newState = State::Deflate;
    else if (pressure < setPressure - PressureDelta)
      newState = State::Inflate;
    else 
      newState = State::Done;
    break;

  case State::Inflate:
    ss.setNumber(pressure);
    digitalWrite(Compressor, HIGH);
    if (pressure + PressureDelta > setPressure) {
      digitalWrite(Compressor, LOW);
      newState = State::Wait;
    }
    break;

  case State::Deflate:
    ss.setNumber(pressure);
    digitalWrite(Valve, HIGH);
    if (millis() > lastStateChange + 300) {
      digitalWrite(Valve, LOW);
      newState = State::Wait;
    }
    break;

  case State::Done:
    if ((millis() - lastStateChange) % 200 > 100)
      ss.setNumber(pressure);
    else
      ss.blank();
    
    if (pressure < 0.1 && millis() > lastStateChange + 5000)
      newState = State::Idle;
      
    break;
  }
   
  if (newState != state) {
    lastStateChange = millis();
    Serial.print(int(newState));
    Serial.print(' ');
    Serial.print(setPressure);
    Serial.print(' ');
    Serial.println(pressure);
  }
}
