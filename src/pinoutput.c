#include "soc_AM335x.h"
#include "gpio_v2.h"
#include "gpio.h"

void configGPIO1(int pin, int dir) {
	GPIO1ModuleClkConfig();
	GPIO1PinMuxSetup(pin);
	GPIOModuleEnable(SOC_GPIO_1_REGS);
	GPIOModuleReset(SOC_GPIO_1_REGS);
	GPIODirModeSet(SOC_GPIO_1_REGS, pin, dir);
}

void setGPIO1Pin(int pin, int value) {
	value ? HWREG(SOC_GPIO_1_REGS + GPIO_SETDATAOUT) = (1 << pin) : HWREG(SOC_GPIO_1_REGS + GPIO_CLEARDATAOUT) = (1 << pin);
}