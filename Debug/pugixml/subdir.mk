################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../pugixml/pugixml.cpp 

OBJS += \
./pugixml/pugixml.o 

CPP_DEPS += \
./pugixml/pugixml.d 


# Each subdirectory must supply rules for building sources it contributes
pugixml/%.o: ../pugixml/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -O2 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


