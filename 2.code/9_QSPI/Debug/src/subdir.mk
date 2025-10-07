################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/hal_entry.c \
../src/hal_warmstart.c \
../src/qspi.c \
../src/user.c 

C_DEPS += \
./src/hal_entry.d \
./src/hal_warmstart.d \
./src/qspi.d \
./src/user.d 

OBJS += \
./src/hal_entry.o \
./src/hal_warmstart.o \
./src/qspi.o \
./src/user.o 

SREC += \
9_QSPI.srec 

MAP += \
9_QSPI.map 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	$(file > $@.in,-mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O2 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-strict-aliasing -Wunused -Wuninitialized -Wall -Wextra -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Wlogical-op -Waggregate-return -Wfloat-equal -g -D_RENESAS_RA_ -D_RA_CORE=CM33 -D_RA_ORDINAL=1 -I"E:/StudyNote/study_RA/2.code/9_QSPI/ra_gen" -I"." -I"E:/StudyNote/study_RA/2.code/9_QSPI/ra_cfg/fsp_cfg/bsp" -I"E:/StudyNote/study_RA/2.code/9_QSPI/ra_cfg/fsp_cfg" -I"E:/StudyNote/study_RA/2.code/9_QSPI/src" -I"E:/StudyNote/study_RA/2.code/9_QSPI/ra/fsp/inc" -I"E:/StudyNote/study_RA/2.code/9_QSPI/ra/fsp/inc/api" -I"E:/StudyNote/study_RA/2.code/9_QSPI/ra/fsp/inc/instances" -I"E:/StudyNote/study_RA/2.code/9_QSPI/ra/arm/CMSIS_6/CMSIS/Core/Include" -I"E:/StudyNote/study_RA/2.code/9_QSPI/ra_cfg/driver" -std=c99 -Wno-stringop-overflow -Wno-format-truncation --param=min-pagesize=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" -x c "$<")
	@echo Building file: $< && arm-none-eabi-gcc @"$@.in"

