#include "config.h"

LED::LED(uint8_t pin){
    m_pin=pin;
    pinMode(m_pin, OUTPUT);
    off();
};

// invert LED
void LED::toggle(){
    if(m_state==OFF){
        on();
    } else {
        off();
    }
}

void LED::on(){
  m_state = ON;
  Serial.println("Switch ON");
  digitalWrite(m_pin, ON);
}

void LED::off(){
  m_state = OFF;
  Serial.println("Switch OFF");
  digitalWrite(m_pin, OFF);
}

uint8_t LED::getState(){
    return m_state;
}
