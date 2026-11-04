//*****************************************************************************
//
// joystick.c - BoosterPack MKII joystick read with ADC0 (polling)
//
// Pins (BoosterPack 2): X = J1.2 = PD2 = AIN13, Y = J3.26 = PK1 = AIN17,
//                       SEL = J1.5 = PD4 (low when pressed)
// ADC: 12 bit -> 0 .. 4095, centre about 2048.
// The stick counts as moved above 3200 / below 900 and as back in the centre
// between 1400 and 2700 (hysteresis, so it does not flicker at the border).
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"

#include "joystick.h"

#define ADC_HIGH        3200   // above: stick moved to this side
#define ADC_LOW          900   // below: stick moved to the other side
#define CENTRE_HIGH     2700   // inside CENTRE_LOW .. CENTRE_HIGH = centre
#define CENTRE_LOW      1400

static JoyDir lastDir = JOY_NONE;

void joystick_init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD) ||
           !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOK) ||
           !SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0)) {}

    GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_2);          // X
    GPIOPinTypeADC(GPIO_PORTK_BASE, GPIO_PIN_1);          // Y
    GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, GPIO_PIN_4);    // SEL
    GPIOPadConfigSet(GPIO_PORTD_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA,
                     GPIO_PIN_TYPE_STD_WPU);              // pull-up for safety

    // ADC clock from the 16 MHz PIOSC (independent of the PLL)
    ADCClockConfigSet(ADC0_BASE, ADC_CLOCK_SRC_PIOSC | ADC_CLOCK_RATE_FULL, 1);

    // sequence 1, started by software: step 0 = X, step 1 = Y (last step)
    ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_CH13);
    ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_CH17 | ADC_CTL_IE | ADC_CTL_END);
    ADCSequenceEnable(ADC0_BASE, 1);
    ADCIntClear(ADC0_BASE, 1);
}

// one measurement of both axes (takes a few microseconds)
static void readAxes(uint32_t *x, uint32_t *y)
{
    uint32_t values[2];

    ADCProcessorTrigger(ADC0_BASE, 1);
    while (!ADCIntStatus(ADC0_BASE, 1, false)) {}   // wait until both are done
    ADCIntClear(ADC0_BASE, 1);
    ADCSequenceDataGet(ADC0_BASE, 1, values);

    *x = values[0];
    *y = values[1];
}

JoyDir joystick_direction(void)
{
    uint32_t x, y;
    readAxes(&x, &y);

    bool xCentre = (x > CENTRE_LOW) && (x < CENTRE_HIGH);
    bool yCentre = (y > CENTRE_LOW) && (y < CENTRE_HIGH);

    if (xCentre && yCentre)
    {
        lastDir = JOY_NONE;                  // clearly back in the centre
    }
    else if (y > ADC_HIGH)
    {
        lastDir = JOY_UP;
    }
    else if (y < ADC_LOW)
    {
        lastDir = JOY_DOWN;
    }
    else if (x > ADC_HIGH)
    {
        lastDir = JOY_RIGHT;
    }
    else if (x < ADC_LOW)
    {
        lastDir = JOY_LEFT;
    }
    // between the thresholds: keep the last direction (hysteresis)

    return lastDir;
}

bool joystick_isPressed(void)
{
    return GPIOPinRead(GPIO_PORTD_BASE, GPIO_PIN_4) == 0;   // active low
}

void joystick_raw(int32_t *x, int32_t *y)
{
    uint32_t rx, ry;
    readAxes(&rx, &ry);
    *x = (int32_t)rx - 2048;     // -2048 .. +2047, 0 = centre
    *y = (int32_t)ry - 2048;
}
