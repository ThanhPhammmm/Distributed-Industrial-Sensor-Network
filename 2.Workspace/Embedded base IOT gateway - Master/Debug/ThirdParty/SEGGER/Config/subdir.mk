################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ThirdParty/SEGGER/Config/SEGGER_SYSVIEW_Config_FreeRTOS.c 

OBJS += \
./ThirdParty/SEGGER/Config/SEGGER_SYSVIEW_Config_FreeRTOS.o 

C_DEPS += \
./ThirdParty/SEGGER/Config/SEGGER_SYSVIEW_Config_FreeRTOS.d 


# Each subdirectory must supply rules for building sources it contributes
ThirdParty/SEGGER/Config/%.o ThirdParty/SEGGER/Config/%.su ThirdParty/SEGGER/Config/%.cyclo: ../ThirdParty/SEGGER/Config/%.c ThirdParty/SEGGER/Config/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I"C:/Users/LENOVO/Documents/1.Personal/11.Embedded Projects/1.Embedded base IOT gateway/2.Workspace/Embedded base IOT gateway - Master/ThirdParty/FreeRTOS/portable/GCC/ARM_CM4F" -I../Core/Inc -I"C:/Users/LENOVO/Documents/1.Personal/11.Embedded Projects/1.Embedded base IOT gateway/2.Workspace/Embedded base IOT gateway - Master/ThirdParty/SEGGER/Config" -I"C:/Users/LENOVO/Documents/1.Personal/11.Embedded Projects/1.Embedded base IOT gateway/2.Workspace/Embedded base IOT gateway - Master/ThirdParty/SEGGER/OS" -I"C:/Users/LENOVO/Documents/1.Personal/11.Embedded Projects/1.Embedded base IOT gateway/2.Workspace/Embedded base IOT gateway - Master/ThirdParty/SEGGER/SEGGER" -I"C:/Users/LENOVO/Documents/1.Personal/11.Embedded Projects/1.Embedded base IOT gateway/2.Workspace/Embedded base IOT gateway - Master/ThirdParty/FreeRTOS" -I"C:/Users/LENOVO/Documents/1.Personal/11.Embedded Projects/1.Embedded base IOT gateway/2.Workspace/Embedded base IOT gateway - Master/ThirdParty/FreeRTOS/include" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-ThirdParty-2f-SEGGER-2f-Config

clean-ThirdParty-2f-SEGGER-2f-Config:
	-$(RM) ./ThirdParty/SEGGER/Config/SEGGER_SYSVIEW_Config_FreeRTOS.cyclo ./ThirdParty/SEGGER/Config/SEGGER_SYSVIEW_Config_FreeRTOS.d ./ThirdParty/SEGGER/Config/SEGGER_SYSVIEW_Config_FreeRTOS.o ./ThirdParty/SEGGER/Config/SEGGER_SYSVIEW_Config_FreeRTOS.su

.PHONY: clean-ThirdParty-2f-SEGGER-2f-Config

