################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/DeviceManager.c \
../Core/Src/ProtocolTask.c \
../Core/Src/SlaveRegistry.c \
../Core/Src/alarm.c \
../Core/Src/button.c \
../Core/Src/lcd_driver.c \
../Core/Src/lcd_ui.c \
../Core/Src/main.c \
../Core/Src/rs485_driver.c \
../Core/Src/stm32f4xx_hal_msp.c \
../Core/Src/stm32f4xx_hal_timebase_tim.c \
../Core/Src/stm32f4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/system_state.c \
../Core/Src/system_stm32f4xx.c \
../Core/Src/upstream.c \
../Core/Src/watchdog.c 

OBJS += \
./Core/Src/DeviceManager.o \
./Core/Src/ProtocolTask.o \
./Core/Src/SlaveRegistry.o \
./Core/Src/alarm.o \
./Core/Src/button.o \
./Core/Src/lcd_driver.o \
./Core/Src/lcd_ui.o \
./Core/Src/main.o \
./Core/Src/rs485_driver.o \
./Core/Src/stm32f4xx_hal_msp.o \
./Core/Src/stm32f4xx_hal_timebase_tim.o \
./Core/Src/stm32f4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/system_state.o \
./Core/Src/system_stm32f4xx.o \
./Core/Src/upstream.o \
./Core/Src/watchdog.o 

C_DEPS += \
./Core/Src/DeviceManager.d \
./Core/Src/ProtocolTask.d \
./Core/Src/SlaveRegistry.d \
./Core/Src/alarm.d \
./Core/Src/button.d \
./Core/Src/lcd_driver.d \
./Core/Src/lcd_ui.d \
./Core/Src/main.d \
./Core/Src/rs485_driver.d \
./Core/Src/stm32f4xx_hal_msp.d \
./Core/Src/stm32f4xx_hal_timebase_tim.d \
./Core/Src/stm32f4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/system_state.d \
./Core/Src/system_stm32f4xx.d \
./Core/Src/upstream.d \
./Core/Src/watchdog.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I"C:/Users/LENOVO/Documents/1.Personal/11.Embedded Projects/1.Embedded base IOT gateway/2.Workspace/Embedded base IOT gateway - Master/ThirdParty/FreeRTOS/portable/GCC/ARM_CM4F" -I"C:/Users/LENOVO/Documents/1.Personal/11.Embedded Projects/1.Embedded base IOT gateway/2.Workspace/Embedded base IOT gateway - Master/ThirdParty/SEGGER/Config" -I"C:/Users/LENOVO/Documents/1.Personal/11.Embedded Projects/1.Embedded base IOT gateway/2.Workspace/Embedded base IOT gateway - Master/ThirdParty/SEGGER/OS" -I"C:/Users/LENOVO/Documents/1.Personal/11.Embedded Projects/1.Embedded base IOT gateway/2.Workspace/Embedded base IOT gateway - Master/ThirdParty/SEGGER/SEGGER" -I../Core/Inc -I"C:/Users/LENOVO/Documents/1.Personal/11.Embedded Projects/1.Embedded base IOT gateway/2.Workspace/Embedded base IOT gateway - Master/ThirdParty/FreeRTOS" -I"C:/Users/LENOVO/Documents/1.Personal/11.Embedded Projects/1.Embedded base IOT gateway/2.Workspace/Embedded base IOT gateway - Master/ThirdParty/FreeRTOS/include" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/DeviceManager.cyclo ./Core/Src/DeviceManager.d ./Core/Src/DeviceManager.o ./Core/Src/DeviceManager.su ./Core/Src/ProtocolTask.cyclo ./Core/Src/ProtocolTask.d ./Core/Src/ProtocolTask.o ./Core/Src/ProtocolTask.su ./Core/Src/SlaveRegistry.cyclo ./Core/Src/SlaveRegistry.d ./Core/Src/SlaveRegistry.o ./Core/Src/SlaveRegistry.su ./Core/Src/alarm.cyclo ./Core/Src/alarm.d ./Core/Src/alarm.o ./Core/Src/alarm.su ./Core/Src/button.cyclo ./Core/Src/button.d ./Core/Src/button.o ./Core/Src/button.su ./Core/Src/lcd_driver.cyclo ./Core/Src/lcd_driver.d ./Core/Src/lcd_driver.o ./Core/Src/lcd_driver.su ./Core/Src/lcd_ui.cyclo ./Core/Src/lcd_ui.d ./Core/Src/lcd_ui.o ./Core/Src/lcd_ui.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/rs485_driver.cyclo ./Core/Src/rs485_driver.d ./Core/Src/rs485_driver.o ./Core/Src/rs485_driver.su ./Core/Src/stm32f4xx_hal_msp.cyclo ./Core/Src/stm32f4xx_hal_msp.d ./Core/Src/stm32f4xx_hal_msp.o ./Core/Src/stm32f4xx_hal_msp.su ./Core/Src/stm32f4xx_hal_timebase_tim.cyclo ./Core/Src/stm32f4xx_hal_timebase_tim.d ./Core/Src/stm32f4xx_hal_timebase_tim.o ./Core/Src/stm32f4xx_hal_timebase_tim.su ./Core/Src/stm32f4xx_it.cyclo ./Core/Src/stm32f4xx_it.d ./Core/Src/stm32f4xx_it.o ./Core/Src/stm32f4xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/system_state.cyclo ./Core/Src/system_state.d ./Core/Src/system_state.o ./Core/Src/system_state.su ./Core/Src/system_stm32f4xx.cyclo ./Core/Src/system_stm32f4xx.d ./Core/Src/system_stm32f4xx.o ./Core/Src/system_stm32f4xx.su ./Core/Src/upstream.cyclo ./Core/Src/upstream.d ./Core/Src/upstream.o ./Core/Src/upstream.su ./Core/Src/watchdog.cyclo ./Core/Src/watchdog.d ./Core/Src/watchdog.o ./Core/Src/watchdog.su

.PHONY: clean-Core-2f-Src

