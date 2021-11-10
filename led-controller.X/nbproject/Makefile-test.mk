#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Include project Makefile
ifeq "${IGNORE_LOCAL}" "TRUE"
# do not include local makefile. User is passing all local related variables already
else
include Makefile
# Include makefile containing local settings
ifeq "$(wildcard nbproject/Makefile-local-test.mk)" "nbproject/Makefile-local-test.mk"
include nbproject/Makefile-local-test.mk
endif
endif

# Environment
MKDIR=mkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=test
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=elf
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/led-controller.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/led-controller.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
endif

ifeq ($(COMPARE_BUILD), true)
COMPARISON_BUILD=-mafrlcsj
else
COMPARISON_BUILD=
endif

ifdef SUB_IMAGE_ADDRESS

else
SUB_IMAGE_ADDRESS_COMMAND=
endif

# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}

# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Source Files Quoted if spaced
SOURCEFILES_QUOTED_IF_SPACED=source/main.c source/kernel.c source/sys.c source/config_word.c source/timer.c source/assert.c source/print.c source/dma.c source/spi.c source/tlc5940.c source/pwm.c source/layer.c source/rs485.c source/bus.c source/crc16.c source/bus_func_impl.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/source/main.o ${OBJECTDIR}/source/kernel.o ${OBJECTDIR}/source/sys.o ${OBJECTDIR}/source/config_word.o ${OBJECTDIR}/source/timer.o ${OBJECTDIR}/source/assert.o ${OBJECTDIR}/source/print.o ${OBJECTDIR}/source/dma.o ${OBJECTDIR}/source/spi.o ${OBJECTDIR}/source/tlc5940.o ${OBJECTDIR}/source/pwm.o ${OBJECTDIR}/source/layer.o ${OBJECTDIR}/source/rs485.o ${OBJECTDIR}/source/bus.o ${OBJECTDIR}/source/crc16.o ${OBJECTDIR}/source/bus_func_impl.o
POSSIBLE_DEPFILES=${OBJECTDIR}/source/main.o.d ${OBJECTDIR}/source/kernel.o.d ${OBJECTDIR}/source/sys.o.d ${OBJECTDIR}/source/config_word.o.d ${OBJECTDIR}/source/timer.o.d ${OBJECTDIR}/source/assert.o.d ${OBJECTDIR}/source/print.o.d ${OBJECTDIR}/source/dma.o.d ${OBJECTDIR}/source/spi.o.d ${OBJECTDIR}/source/tlc5940.o.d ${OBJECTDIR}/source/pwm.o.d ${OBJECTDIR}/source/layer.o.d ${OBJECTDIR}/source/rs485.o.d ${OBJECTDIR}/source/bus.o.d ${OBJECTDIR}/source/crc16.o.d ${OBJECTDIR}/source/bus_func_impl.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/source/main.o ${OBJECTDIR}/source/kernel.o ${OBJECTDIR}/source/sys.o ${OBJECTDIR}/source/config_word.o ${OBJECTDIR}/source/timer.o ${OBJECTDIR}/source/assert.o ${OBJECTDIR}/source/print.o ${OBJECTDIR}/source/dma.o ${OBJECTDIR}/source/spi.o ${OBJECTDIR}/source/tlc5940.o ${OBJECTDIR}/source/pwm.o ${OBJECTDIR}/source/layer.o ${OBJECTDIR}/source/rs485.o ${OBJECTDIR}/source/bus.o ${OBJECTDIR}/source/crc16.o ${OBJECTDIR}/source/bus_func_impl.o

# Source Files
SOURCEFILES=source/main.c source/kernel.c source/sys.c source/config_word.c source/timer.c source/assert.c source/print.c source/dma.c source/spi.c source/tlc5940.c source/pwm.c source/layer.c source/rs485.c source/bus.c source/crc16.c source/bus_func_impl.c



CFLAGS=
ASFLAGS=
LDLIBSOPTIONS=

############# Tool locations ##########################################
# If you copy a project from one host to another, the path where the  #
# compiler is installed may be different.                             #
# If you open this project with MPLAB X in the new host, this         #
# makefile will be regenerated and the paths will be corrected.       #
#######################################################################
# fixDeps replaces a bunch of sed/cat/printf statements that slow down the build
FIXDEPS=fixDeps

.build-conf:  ${BUILD_SUBPROJECTS}
ifneq ($(INFORMATION_MESSAGE), )
	@echo $(INFORMATION_MESSAGE)
endif
	${MAKE}  -f nbproject/Makefile-test.mk dist/${CND_CONF}/${IMAGE_TYPE}/led-controller.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=32MX330F064H
MP_LINKER_FILE_OPTION=,--script="linker/p32MX330F064H.ld"
# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assembleWithPreprocess
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/source/main.o: source/main.c  .generated_files/flags/test/ba52af1d453163c77b981e178fdddf6f908734cd .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/main.o.d 
	@${RM} ${OBJECTDIR}/source/main.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/main.o.d" -o ${OBJECTDIR}/source/main.o source/main.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/kernel.o: source/kernel.c  .generated_files/flags/test/de451d1f43e227913fd55501beac8bdca68189cd .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/kernel.o.d 
	@${RM} ${OBJECTDIR}/source/kernel.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/kernel.o.d" -o ${OBJECTDIR}/source/kernel.o source/kernel.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/sys.o: source/sys.c  .generated_files/flags/test/ba4f16828a97db0c0ca5866d9b5d5a3b9bcaa4e9 .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/sys.o.d 
	@${RM} ${OBJECTDIR}/source/sys.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/sys.o.d" -o ${OBJECTDIR}/source/sys.o source/sys.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/config_word.o: source/config_word.c  .generated_files/flags/test/ec4f813fc29caabc35fcd621e3aed071a9c4d94e .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/config_word.o.d 
	@${RM} ${OBJECTDIR}/source/config_word.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/config_word.o.d" -o ${OBJECTDIR}/source/config_word.o source/config_word.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/timer.o: source/timer.c  .generated_files/flags/test/ca7f7da5f0bfef656197acef8dbcba4c975b1afd .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/timer.o.d 
	@${RM} ${OBJECTDIR}/source/timer.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/timer.o.d" -o ${OBJECTDIR}/source/timer.o source/timer.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/assert.o: source/assert.c  .generated_files/flags/test/cea16404a675bd03d9307d38bd00499c0c872cb1 .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/assert.o.d 
	@${RM} ${OBJECTDIR}/source/assert.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/assert.o.d" -o ${OBJECTDIR}/source/assert.o source/assert.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/print.o: source/print.c  .generated_files/flags/test/37047dc4d812bf424fc8152ea19acf6b4d59361d .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/print.o.d 
	@${RM} ${OBJECTDIR}/source/print.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/print.o.d" -o ${OBJECTDIR}/source/print.o source/print.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/dma.o: source/dma.c  .generated_files/flags/test/8cb424a52b35f02dd2480ad99db50526868d294f .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/dma.o.d 
	@${RM} ${OBJECTDIR}/source/dma.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/dma.o.d" -o ${OBJECTDIR}/source/dma.o source/dma.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/spi.o: source/spi.c  .generated_files/flags/test/43884c1850745fc7481572a1a30b84b1d794ef8f .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/spi.o.d 
	@${RM} ${OBJECTDIR}/source/spi.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/spi.o.d" -o ${OBJECTDIR}/source/spi.o source/spi.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/tlc5940.o: source/tlc5940.c  .generated_files/flags/test/5473a934ebc86866823ecf25c6fd55f35e65c021 .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/tlc5940.o.d 
	@${RM} ${OBJECTDIR}/source/tlc5940.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/tlc5940.o.d" -o ${OBJECTDIR}/source/tlc5940.o source/tlc5940.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/pwm.o: source/pwm.c  .generated_files/flags/test/fa937cb654efea8578c71d0f5734acc6d90ab02c .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/pwm.o.d 
	@${RM} ${OBJECTDIR}/source/pwm.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/pwm.o.d" -o ${OBJECTDIR}/source/pwm.o source/pwm.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/layer.o: source/layer.c  .generated_files/flags/test/edeb37583da229af187be0ad00cc5cca9781dc6f .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/layer.o.d 
	@${RM} ${OBJECTDIR}/source/layer.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/layer.o.d" -o ${OBJECTDIR}/source/layer.o source/layer.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/rs485.o: source/rs485.c  .generated_files/flags/test/e036ab13f3ac1aa0ffb1c23c7c2565086691b57b .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/rs485.o.d 
	@${RM} ${OBJECTDIR}/source/rs485.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/rs485.o.d" -o ${OBJECTDIR}/source/rs485.o source/rs485.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/bus.o: source/bus.c  .generated_files/flags/test/cf28e3ac959b44014dc72c3f5e5737ba780009ff .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/bus.o.d 
	@${RM} ${OBJECTDIR}/source/bus.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/bus.o.d" -o ${OBJECTDIR}/source/bus.o source/bus.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/crc16.o: source/crc16.c  .generated_files/flags/test/588efb37741ffe2a269d40cfdf36f109ef9acdef .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/crc16.o.d 
	@${RM} ${OBJECTDIR}/source/crc16.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/crc16.o.d" -o ${OBJECTDIR}/source/crc16.o source/crc16.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/bus_func_impl.o: source/bus_func_impl.c  .generated_files/flags/test/6a9a6af43eb79b6b9ff9453b718bd17208778c77 .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/bus_func_impl.o.d 
	@${RM} ${OBJECTDIR}/source/bus_func_impl.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/bus_func_impl.o.d" -o ${OBJECTDIR}/source/bus_func_impl.o source/bus_func_impl.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
else
${OBJECTDIR}/source/main.o: source/main.c  .generated_files/flags/test/6913d1453052937591ff4996d912f7419131b9b4 .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/main.o.d 
	@${RM} ${OBJECTDIR}/source/main.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/main.o.d" -o ${OBJECTDIR}/source/main.o source/main.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/kernel.o: source/kernel.c  .generated_files/flags/test/2ffecdeb207d5260717a14a5565fc9a510137bc5 .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/kernel.o.d 
	@${RM} ${OBJECTDIR}/source/kernel.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/kernel.o.d" -o ${OBJECTDIR}/source/kernel.o source/kernel.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/sys.o: source/sys.c  .generated_files/flags/test/18c92575d7c763822f99ade87421c3ce5f6c5fd6 .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/sys.o.d 
	@${RM} ${OBJECTDIR}/source/sys.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/sys.o.d" -o ${OBJECTDIR}/source/sys.o source/sys.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/config_word.o: source/config_word.c  .generated_files/flags/test/90a6d72783f8c5d7dab1c0ac391478b297d28210 .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/config_word.o.d 
	@${RM} ${OBJECTDIR}/source/config_word.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/config_word.o.d" -o ${OBJECTDIR}/source/config_word.o source/config_word.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/timer.o: source/timer.c  .generated_files/flags/test/e670892221d6734500865cc426c78d0c25450303 .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/timer.o.d 
	@${RM} ${OBJECTDIR}/source/timer.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/timer.o.d" -o ${OBJECTDIR}/source/timer.o source/timer.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/assert.o: source/assert.c  .generated_files/flags/test/e44bd3a2410f59a0a208f0793d70f07b68408c51 .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/assert.o.d 
	@${RM} ${OBJECTDIR}/source/assert.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/assert.o.d" -o ${OBJECTDIR}/source/assert.o source/assert.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/print.o: source/print.c  .generated_files/flags/test/2bec1b05c227798687c8567b07810472c537f3fc .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/print.o.d 
	@${RM} ${OBJECTDIR}/source/print.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/print.o.d" -o ${OBJECTDIR}/source/print.o source/print.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/dma.o: source/dma.c  .generated_files/flags/test/99d8dfd95267ab5863c7b8706b3464e39ebd2874 .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/dma.o.d 
	@${RM} ${OBJECTDIR}/source/dma.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/dma.o.d" -o ${OBJECTDIR}/source/dma.o source/dma.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/spi.o: source/spi.c  .generated_files/flags/test/51e2665dbf019ba351d4c75ae62164802a0d219 .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/spi.o.d 
	@${RM} ${OBJECTDIR}/source/spi.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/spi.o.d" -o ${OBJECTDIR}/source/spi.o source/spi.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/tlc5940.o: source/tlc5940.c  .generated_files/flags/test/f4cd5519b547077a19d69178b22be6de3932ffe0 .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/tlc5940.o.d 
	@${RM} ${OBJECTDIR}/source/tlc5940.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/tlc5940.o.d" -o ${OBJECTDIR}/source/tlc5940.o source/tlc5940.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/pwm.o: source/pwm.c  .generated_files/flags/test/df2c24fa9d8a69e149aeebf49d0fca9b06d27b27 .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/pwm.o.d 
	@${RM} ${OBJECTDIR}/source/pwm.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/pwm.o.d" -o ${OBJECTDIR}/source/pwm.o source/pwm.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/layer.o: source/layer.c  .generated_files/flags/test/9c454d187ea5f293f070b887a3feeceba53cb816 .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/layer.o.d 
	@${RM} ${OBJECTDIR}/source/layer.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/layer.o.d" -o ${OBJECTDIR}/source/layer.o source/layer.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/rs485.o: source/rs485.c  .generated_files/flags/test/7e99deb9c5d5eb1708c11b84a7afa809b2b56523 .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/rs485.o.d 
	@${RM} ${OBJECTDIR}/source/rs485.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/rs485.o.d" -o ${OBJECTDIR}/source/rs485.o source/rs485.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/bus.o: source/bus.c  .generated_files/flags/test/9b271d749fa1232de95ef7d927e5237510318b1f .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/bus.o.d 
	@${RM} ${OBJECTDIR}/source/bus.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/bus.o.d" -o ${OBJECTDIR}/source/bus.o source/bus.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/crc16.o: source/crc16.c  .generated_files/flags/test/64de001ccae60f0d6342d177c13a08f34bc38224 .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/crc16.o.d 
	@${RM} ${OBJECTDIR}/source/crc16.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/crc16.o.d" -o ${OBJECTDIR}/source/crc16.o source/crc16.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/bus_func_impl.o: source/bus_func_impl.c  .generated_files/flags/test/aebbb86ce058f54e9f5d018779fdb2c9ac393fd1 .generated_files/flags/test/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/bus_func_impl.o.d 
	@${RM} ${OBJECTDIR}/source/bus_func_impl.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -DLAYER_AUTO_BUFFER_SWAP -DBUS_IGNORE_CRC -MP -MMD -MF "${OBJECTDIR}/source/bus_func_impl.o.d" -o ${OBJECTDIR}/source/bus_func_impl.o source/bus_func_impl.c    -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compileCPP
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
dist/${CND_CONF}/${IMAGE_TYPE}/led-controller.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    linker/p32MX330F064H.ld
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE) -g   -mprocessor=$(MP_PROCESSOR_OPTION)  -o dist/${CND_CONF}/${IMAGE_TYPE}/led-controller.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}          -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)      -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-D=__DEBUG_D,--no-code-in-dinit,--no-dinit-in-serial-mem,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml -mdfp="${DFP_DIR}"
	
else
dist/${CND_CONF}/${IMAGE_TYPE}/led-controller.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   linker/p32MX330F064H.ld
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -mprocessor=$(MP_PROCESSOR_OPTION)  -o dist/${CND_CONF}/${IMAGE_TYPE}/led-controller.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}          -DXPRJ_test=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),--no-code-in-dinit,--no-dinit-in-serial-mem,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml -mdfp="${DFP_DIR}"
	${MP_CC_DIR}/xc32-bin2hex dist/${CND_CONF}/${IMAGE_TYPE}/led-controller.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} 
endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/test
	${RM} -r dist/test

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(shell "${PATH_TO_IDE_BIN}"mplabwildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
