#include "pico/stdlib.h"

#ifdef CYW43_WL_GPIO_LED_PIN
    #include "pico/cyw43_arch.h"
#endif

bool LOW = false;
bool HIGH = true;
int PIN_GP0_CLOCK = 0;
int PIN_GP1_DATA = 1;
int CMD_LED_INIT_NOANIM[10] = {0,0,1,0,0,0,0,1,0,0};
int CMD_LED_INIT_NOANIM_NOPWR[10] = {0,0,1,0,0,0,1,0,0,0};
int CMD_LED_INIT_NOANIM_BLNKPWR[10] = {0,0,1,0,0,0,1,1,0,0};    // Consistent blinking, not a single blink
int CMD_LED_INIT_ANIM[10] = {0,0,1,0,0,0,0,1,0,1};
int CMD_LED_INIT_ANIM_NOPWR[10] = {0,0,1,0,0,0,1,0,0,1};
int CMD_LED_INIT_ANIM_BLNKPWR[10] = {0,0,1,0,0,0,1,1,0,1};
int CMD_LED_CLEAR_GREEN[10] = {0,0,1,0,1,0,0,0,0,0};
int CMD_LED_CLEAR_RED[10] = {0,0,1,0,1,1,0,0,0,0};
int CMD_LED_GREEN_ALL[10] = {0,0,1,0,1,0,1,1,1,1};
int CMD_LED_RED_ALL[10] = {0,0,1,0,1,1,1,1,1,1};
int CMD_LED_AMBER_ALL[10] = {0,0,1,1,1,0,1,1,1,1};
int CMD_LED_CLEAR_OVERRIDE[10] = {0,0,1,0,0,1,0,0,0,0};
int CMD_CONFIG_ARGON_HORIZONTAL[10] = {0,0,0,0,0,1,0,0,0,0};
int CMD_CONFIG_ARGON_VERTICAL[10] = {0,0,0,0,0,1,0,0,0,1};


void initPins()
{
    gpio_init(PIN_GP0_CLOCK);
    gpio_set_dir(PIN_GP0_CLOCK, GPIO_IN);
    gpio_pull_up(PIN_GP0_CLOCK);

    gpio_init(PIN_GP1_DATA);
    gpio_set_dir(PIN_GP1_DATA, GPIO_OUT);
    gpio_pull_up(PIN_GP1_DATA);
}

int initLED(void) 
{
    #if defined(PICO_DEFAULT_LED_PIN)
        gpio_init(PICO_DEFAULT_LED_PIN);
        gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

        return PICO_OK;
    #elif defined(CYW43_WL_GPIO_LED_PIN)
        return cyw43_arch_init();
    #endif
}

void setPicoLED(bool led_on) 
{
    #if defined(PICO_DEFAULT_LED_PIN)
        gpio_put(PICO_DEFAULT_LED_PIN, led_on);
    #elif defined(CYW43_WL_GPIO_LED_PIN)
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
    #endif
}

void blockingWaitClockChange(bool prevClock)
{
    while (prevClock == gpio_get(PIN_GP0_CLOCK)) {}
}

void blockingWaitClockCycle(bool prevClock)
{

}

// Blinks rapidly 3 times for HIGH, 1 long blink for LOW
void blinkDebugHighLow(bool condition)
{
    if (condition)
    {
        setPicoLED(true);
        sleep_ms(100);
        setPicoLED(false);
        sleep_ms(100);
        setPicoLED(true);
        sleep_ms(100);
        setPicoLED(false);
        sleep_ms(100);
        setPicoLED(true);
        sleep_ms(100);
        setPicoLED(false);
        sleep_ms(100);
    } else {
        setPicoLED(true);
        sleep_ms(1000);
        setPicoLED(false);
        sleep_ms(1000);
    }
}

void sendCommand(int command[10])
{
    int prevClock = 1;  // Start on clock high
    gpio_put(PIN_GP1_DATA, LOW);    // Signal start of command

    for (int i = 0; i < 10; i++) 
    {   
        blockingWaitClockChange(prevClock); // Blocking wait for clock state to change
        prevClock = gpio_get(PIN_GP0_CLOCK);
        gpio_put(PIN_GP1_DATA, command[i] == 0 ? LOW : HIGH);   // Yes I know ternary could be simplified to binary operation here but I like the readability.

        blockingWaitClockChange(prevClock);
        prevClock = gpio_get(PIN_GP0_CLOCK);   
    }

    gpio_put(PIN_GP1_DATA, HIGH);   // Signal command complete

    // Wait a full clock cycle after issuing command.
    // This is necessary to chain more than one command consecutively
    blockingWaitClockChange(prevClock);
    prevClock = gpio_get(PIN_GP0_CLOCK);   
    blockingWaitClockChange(prevClock);
    prevClock = gpio_get(PIN_GP0_CLOCK);
}

// turns on pico LED at start of command and turns it off once send is complete.
void sendCommandAndDebug(int command[10])
{
    setPicoLED(true);
    sendCommand(command);
    setPicoLED(false);
}

void setGreenLEDs(bool q1, bool q2, bool q3, bool q4)
{
    int builtCommand[10] = {0,0,1,0,1,0,0,0,0,0,};;
    builtCommand[6] = !q4 ? 0 : 1;
    builtCommand[7] = !q3 ? 0 : 1;
    builtCommand[8] = !q2 ? 0 : 1;
    builtCommand[9] = !q1 ? 0 : 1;

    sendCommand(builtCommand);
}

void setRedLEDs(bool q1, bool q2, bool q3, bool q4)
{
    int builtCommand[10] = {0,0,1,0,1,1,0,0,0,0};
    builtCommand[6] = !q4 ? 0 : 1;
    builtCommand[7] = !q3 ? 0 : 1;
    builtCommand[8] = !q2 ? 0 : 1;
    builtCommand[9] = !q1 ? 0 : 1;

    sendCommand(builtCommand);
}

void playSpinningRGYAnimation()
{
    int delay = 75;
    int clearDelay = 25;
    // Green circle nofill
    setGreenLEDs(true, false, false, false);
    sleep_ms(delay);
    setGreenLEDs(false, true, false, false);
    sleep_ms(delay);
    setGreenLEDs(false, false, false, true);
    sleep_ms(delay);
    setGreenLEDs(false, false, true, false);
    sleep_ms(delay);

    // Red circle nofill
    setGreenLEDs(false, false, false, false);
    setRedLEDs(true, false, false, false);
    sleep_ms(delay);
    setRedLEDs(false, true, false, false);
    sleep_ms(delay);
    setRedLEDs(false, false, false, true);
    sleep_ms(delay);
    setRedLEDs(false, false, true, false);
    sleep_ms(delay);

    // Green circle fill
    sendCommand(CMD_LED_CLEAR_RED);
    setGreenLEDs(true, false, false, false);
    sleep_ms(delay);
    setGreenLEDs(true, true, false, false);
    sleep_ms(delay);
    setGreenLEDs(true, true, false, true);
    sleep_ms(delay);
    setGreenLEDs(true, true, true, true);
    sleep_ms(delay);

    // Red circle fill
    setGreenLEDs(false, true, true, true);
    setRedLEDs(true, false, false, false);
    sleep_ms(delay);
    setGreenLEDs(false, false, true, true);
    setRedLEDs(true, true, false, false);
    sleep_ms(delay);
    setGreenLEDs(false, false, true, false);
    setRedLEDs(true, true, false, true);
    sleep_ms(delay);
    sendCommand(CMD_LED_CLEAR_GREEN);
    setRedLEDs(true, true, true, true);
    sleep_ms(delay);

    // Yellow circle fill
    setGreenLEDs(true, false, false, false);
    sleep_ms(delay);
    setGreenLEDs(true, true, false, false);
    sleep_ms(delay);
    setGreenLEDs(true, true, false, true);
    sleep_ms(delay);
    setGreenLEDs(true, true, true, true);
    sleep_ms(delay);

    // Off circle
    setGreenLEDs(false, true, true, true);
    setRedLEDs(false, true, true, true);
    sleep_ms(clearDelay);
    setGreenLEDs(false, false, true, true);
    setRedLEDs(false, false, true, true);
    sleep_ms(clearDelay);
    setGreenLEDs(false, false, true, false);
    setRedLEDs(false, false, true, false);
    sleep_ms(clearDelay);


    sendCommand(CMD_LED_CLEAR_GREEN);
    sendCommand(CMD_LED_CLEAR_RED);
}

void playRGYAnimation()
{
    int delay = 125;
    setGreenLEDs(true, false, false, false);
    sleep_ms(delay);
    setGreenLEDs(true, true, false, false);
    sleep_ms(delay);
    setGreenLEDs(true, true, true, false);
    sleep_ms(delay);
    setGreenLEDs(true, true, true, true);
    sleep_ms(delay);

    setGreenLEDs(false, true, true, true);
    setRedLEDs(true, false, false, false);
    sleep_ms(delay);
    setGreenLEDs(false, false, true, true);
    setRedLEDs(true, true, false, false);
    sleep_ms(delay);
    setGreenLEDs(false, false, false, true);
    setRedLEDs(true, true, true, false);
    sleep_ms(delay);
    setGreenLEDs(false, false, false, false);
    setRedLEDs(true, true, true, true);
    sleep_ms(delay);

    setGreenLEDs(true, false, false, false);
    sleep_ms(delay);
    setGreenLEDs(true, true, false, false);
    sleep_ms(delay);
    setGreenLEDs(true, true, true, false);
    sleep_ms(delay);
    setGreenLEDs(true, true, true, true);
    sleep_ms(delay);

    setGreenLEDs(false, true, true, true);
    setRedLEDs(false, true, true, true);
    sleep_ms(delay);
    setGreenLEDs(false, false, true, true);
    setRedLEDs(false, false, true, true);
    sleep_ms(delay);
    setGreenLEDs(false, false, false, true);
    setRedLEDs(false, false, false, true);
    sleep_ms(delay);
    setGreenLEDs(false, false, false, false);
    setRedLEDs(false, false, false, false);
    sleep_ms(delay);
}

void playAltXAnimation()
{
    int delay = 250;
    setGreenLEDs(true, false, false, true);
    setRedLEDs(false, true, true, false);
    sleep_ms(delay);
    setGreenLEDs(false, true, true, false);
    setRedLEDs(true, false, false, true);
    sleep_ms(delay);    
    
    setGreenLEDs(true, false, false, true);
    setRedLEDs(false, true, true, false);
    sleep_ms(delay);
    setGreenLEDs(false, true, true, false);
    setRedLEDs(true, false, false, true);
    sleep_ms(delay);

    sendCommand(CMD_LED_GREEN_ALL);
    setRedLEDs(false, true, true, false);
    sleep_ms(delay);
    setRedLEDs(true, false, false, true);
    sleep_ms(delay);     
    setRedLEDs(false, true, true, false);
    sleep_ms(delay);
    setRedLEDs(true, false, false, true);
    sleep_ms(delay);

    sendCommand(CMD_LED_CLEAR_GREEN);
    sendCommand(CMD_LED_CLEAR_RED);
}

int main() 
{
    int rc = initLED();
    hard_assert(rc == PICO_OK);

    blinkDebugHighLow(false);   // Debug light to signal start of program
    initPins(); // GPIO init
    sendCommand(CMD_LED_INIT_NOANIM_NOPWR);
    sendCommand(CMD_CONFIG_ARGON_HORIZONTAL);
    sendCommand(CMD_LED_CLEAR_GREEN);
    sendCommand(CMD_LED_CLEAR_RED);
    while (true)
    {
        playRGYAnimation();
    }
}