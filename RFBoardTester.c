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
int CMD_LED_GREEN_ALL[10] = {0,0,1,0,1,0,1,1,1,1};
int CMD_LED_ARGON_CLEAR_RED[10] = {0,0,1,0,1,1,0,0,0,0};
int CMD_LED_ARGON_RED_ALL[10] = {0,0,1,0,1,1,1,1,1,1};
int CMD_LED_ARGON_AMBER_ALL[10] = {0,0,1,1,1,0,1,1,1,1};
int CMD_LED_CLEAR_OVERRIDE[10] = {0,0,1,0,0,1,0,0,0,0};
int CMD_CONFIG_HORIZONTAL[10] = {0,0,0,0,0,1,0,0,0,0};
int CMD_CONFIG_VERTICAL[10] = {0,0,0,0,0,1,0,0,0,1};

// Boron specific commands
int CMD_LED_BORON_POWER_RED[10] = {0,0,1,0,1,1,1,0,0,0};
int CMD_LED_BORON_POWER_GREEN_GLOWBLNK[10] = {0,0,1,0,1,1,1,1,1,1};

// Init pico GPIO
void initPins()
{
    gpio_init(PIN_GP0_CLOCK);
    gpio_set_dir(PIN_GP0_CLOCK, GPIO_IN);
    gpio_pull_up(PIN_GP0_CLOCK);    // Necessary to initiate clock cycling

    gpio_init(PIN_GP1_DATA);
    gpio_set_dir(PIN_GP1_DATA, GPIO_OUT);
    gpio_pull_up(PIN_GP1_DATA);
}

// Init pico LED
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

// Set LED on/off on the pico
void setPicoLED(bool enable) 
{
    #if defined(PICO_DEFAULT_LED_PIN)
        gpio_put(PICO_DEFAULT_LED_PIN, enable);
    #elif defined(CYW43_WL_GPIO_LED_PIN)
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, enable);
    #endif
}

// Blocking wait for clock state to change. Half-cycle.
void blockingWaitClockChange(bool prevClock)
{
    while (prevClock == gpio_get(PIN_GP0_CLOCK)) {}
}

// Blinks pico rapidly 3 times for HIGH, 1 long blink for LOW
void blinkDebugHighLow(bool condition)
{
    if (condition)
    {
        for (int i = 0; i < 3; i++)
        {
            setPicoLED(true);
            sleep_ms(100);
            setPicoLED(false);
            sleep_ms(100);
        }
    } else {
        setPicoLED(true);
        sleep_ms(1000);
        setPicoLED(false);
        sleep_ms(1000);
    }
}

// Send command to RF board
void sendCommand(int command[10])
{
    int prevClock = 0;  // Start on clock low
    gpio_put(PIN_GP1_DATA, LOW);    // Signal start SMC->FPM communication

    for (int i = 0; i < 10; i++) 
    {   
        blockingWaitClockChange(prevClock);
        prevClock = gpio_get(PIN_GP0_CLOCK);
        gpio_put(PIN_GP1_DATA, command[i] == 0 ? LOW : HIGH);   // Yes I know ternary could be simplified to binary operation here but I like the readability.

        blockingWaitClockChange(prevClock);
        prevClock = gpio_get(PIN_GP0_CLOCK);   
    }

    // Wait a full clock cycle after issuing command.
    // This is necessary to chain more than one command consecutively
    blockingWaitClockChange(prevClock);
    prevClock = gpio_get(PIN_GP0_CLOCK);   
    blockingWaitClockChange(prevClock);
    prevClock = gpio_get(PIN_GP0_CLOCK);

    gpio_put(PIN_GP1_DATA, HIGH);   // Signal command complete
}

// turns on pico LED at start of command and turns it off once send is complete.
void sendCommandAndDebug(int command[10])
{
    setPicoLED(true);
    sendCommand(command);
    setPicoLED(false);
}

// Sets green leds to on/off (upper_left, upper_right, lower_left, lower_right)
void setGreenLEDs(bool q1, bool q2, bool q3, bool q4)
{
    int builtCommand[10] = {0,0,1,0,1,0,0,0,0,0};
    builtCommand[6] = !q4 ? 0 : 1;
    builtCommand[7] = !q3 ? 0 : 1;
    builtCommand[8] = !q2 ? 0 : 1;
    builtCommand[9] = !q1 ? 0 : 1;

    sendCommand(builtCommand);
}

// Sets red leds to on/off (upper_left, upper_right, lower_left, lower_right)
// Only works on Argon RF boards, as Boron only have a single red power light
void setRedArgonLEDs(bool q1, bool q2, bool q3, bool q4)
{
    int builtCommand[10] = {0,0,1,0,1,1,0,0,0,0};
    builtCommand[6] = !q4 ? 0 : 1;
    builtCommand[7] = !q3 ? 0 : 1;
    builtCommand[8] = !q2 ? 0 : 1;
    builtCommand[9] = !q1 ? 0 : 1;

    sendCommand(builtCommand);
}

// Spinning clockwise animation for Argon RF boards
void playArgonSpinningRGYAnimation()
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
    setRedArgonLEDs(true, false, false, false);
    sleep_ms(delay);
    setRedArgonLEDs(false, true, false, false);
    sleep_ms(delay);
    setRedArgonLEDs(false, false, false, true);
    sleep_ms(delay);
    setRedArgonLEDs(false, false, true, false);
    sleep_ms(delay);

    // Green circle fill
    sendCommand(CMD_LED_ARGON_CLEAR_RED);
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
    setRedArgonLEDs(true, false, false, false);
    sleep_ms(delay);
    setGreenLEDs(false, false, true, true);
    setRedArgonLEDs(true, true, false, false);
    sleep_ms(delay);
    setGreenLEDs(false, false, true, false);
    setRedArgonLEDs(true, true, false, true);
    sleep_ms(delay);
    sendCommand(CMD_LED_CLEAR_GREEN);
    setRedArgonLEDs(true, true, true, true);
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
    setRedArgonLEDs(false, true, true, true);
    sleep_ms(clearDelay);
    setGreenLEDs(false, false, true, true);
    setRedArgonLEDs(false, false, true, true);
    sleep_ms(clearDelay);
    setGreenLEDs(false, false, true, false);
    setRedArgonLEDs(false, false, true, false);
    sleep_ms(clearDelay);

    sendCommand(CMD_LED_CLEAR_GREEN);
    sendCommand(CMD_LED_ARGON_CLEAR_RED);
}


// LED testing animation for Argon RF boards
void playArgonRGYAnimation()
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
    setRedArgonLEDs(true, false, false, false);
    sleep_ms(delay);
    setGreenLEDs(false, false, true, true);
    setRedArgonLEDs(true, true, false, false);
    sleep_ms(delay);
    setGreenLEDs(false, false, false, true);
    setRedArgonLEDs(true, true, true, false);
    sleep_ms(delay);
    setGreenLEDs(false, false, false, false);
    setRedArgonLEDs(true, true, true, true);
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
    setRedArgonLEDs(false, true, true, true);
    sleep_ms(delay);
    setGreenLEDs(false, false, true, true);
    setRedArgonLEDs(false, false, true, true);
    sleep_ms(delay);
    setGreenLEDs(false, false, false, true);
    setRedArgonLEDs(false, false, false, true);
    sleep_ms(delay);
    setGreenLEDs(false, false, false, false);
    setRedArgonLEDs(false, false, false, false);
    sleep_ms(delay);
}

// Alternating X (/ \) animation for Argon RF boards
void playArgonAltXAnimation()
{
    int delay = 125;
    setGreenLEDs(true, false, false, true);
    setRedArgonLEDs(false, true, true, false);
    sleep_ms(delay);
    setGreenLEDs(false, true, true, false);
    setRedArgonLEDs(true, false, false, true);
    sleep_ms(delay);    
    
    setGreenLEDs(true, false, false, true);
    setRedArgonLEDs(false, true, true, false);
    sleep_ms(delay);
    setGreenLEDs(false, true, true, false);
    setRedArgonLEDs(true, false, false, true);
    sleep_ms(delay);

    sendCommand(CMD_LED_GREEN_ALL);
    setRedArgonLEDs(false, true, true, false);
    sleep_ms(delay);
    setRedArgonLEDs(true, false, false, true);
    sleep_ms(delay);     
    setRedArgonLEDs(false, true, true, false);
    sleep_ms(delay);
    setRedArgonLEDs(true, false, false, true);
    sleep_ms(delay);

    sendCommand(CMD_LED_CLEAR_GREEN);
    sendCommand(CMD_LED_ARGON_CLEAR_RED);
}

// LED testing animation for Boron RF boards
void playBoronRGYAnimation()
{
    int delay = 125;
    sendCommand(CMD_LED_INIT_NOANIM);
    sleep_ms(delay);
    setGreenLEDs(true, false, false, false);
    sleep_ms(delay);
    setGreenLEDs(false, true, false, false);
    sleep_ms(delay);
    setGreenLEDs(false, false, false, true);
    sleep_ms(delay);
    setGreenLEDs(false, false, true, false);
    sleep_ms(delay);
    setGreenLEDs(true, true, true, true);
    sleep_ms(delay);
    setGreenLEDs(false, false, false, false);
    sleep_ms(delay);

    sendCommand(CMD_LED_BORON_POWER_RED);
    sleep_ms(delay);
    setGreenLEDs(true, false, false, false);
    sleep_ms(delay);
    setGreenLEDs(false, true, false, false);
    sleep_ms(delay);
    setGreenLEDs(false, false, false, true);
    sleep_ms(delay);
    setGreenLEDs(false, false, true, false);
    sleep_ms(delay);
    setGreenLEDs(true, true, true, true);
    sleep_ms(delay);
    setGreenLEDs(false, false, false, false);
    sleep_ms(delay);
}

int main() 
{
    int rc = initLED();
    hard_assert(rc == PICO_OK);

    blinkDebugHighLow(false);   // Debug light to signal start of program
    initPins(); // GPIO init
    sendCommand(CMD_LED_INIT_NOANIM);
    sendCommand(CMD_CONFIG_HORIZONTAL);

    while (true)
    {
        playArgonRGYAnimation();
        //playBoronRGYAnimation();
    }
}