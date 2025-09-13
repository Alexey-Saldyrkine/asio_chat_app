################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/asio_chat_app.cpp 

CPP_DEPS += \
./src/asio_chat_app.d 

OBJS += \
./src/asio_chat_app.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++-14 -DNCURSES_NOMACROS -I/home/alexey/Documents/GitHub/cereal/include -O0 -g3 -Wall -c -fmessage-length=0 -std=c++23 -fcoroutines -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/asio_chat_app.d ./src/asio_chat_app.o

.PHONY: clean-src

