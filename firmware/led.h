#define ON 1
#define OFF 0

class LED {
    private:
        uint8_t m_pin=0;
        uint8_t m_state=0;
    
    public:
        LED(uint8_t pin);
        void on();
        void off();
        void toggle();
        uint8_t getState();
};
