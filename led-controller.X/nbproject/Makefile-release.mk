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
ifeq "$(wildcard nbproject/Makefile-local-release.mk)" "nbproject/Makefile-local-release.mk"
include nbproject/Makefile-local-release.mk
endif
endif

# Environment
MKDIR=mkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=release
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
	${MAKE}  -f nbproject/Makefile-release.mk dist/${CND_CONF}/${IMAGE_TYPE}/led-controller.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

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
${OBJECTDIR}/source/main.o: source/main.c  .generated_files/flags/release/cd2c52f15dafdf64903cb6cb5867874fcf74ccf1 .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/main.o.d 
	@${RM} ${OBJECTDIR}/source/main.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/main.o.d" -o ${OBJECTDIR}/source/main.o source/main.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/kernel.o: source/kernel.c  .generated_files/flags/release/afd4ec9458f046c8d9366045f92b4850a3dd8a1f .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/kernel.o.d 
	@${RM} ${OBJECTDIR}/source/kernel.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/kernel.o.d" -o ${OBJECTDIR}/source/kernel.o source/kernel.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/sys.o: source/sys.c  .generated_files/flags/release/d915a8f2aae112e2274e7b875a2976dd74f8377f .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/sys.o.d 
	@${RM} ${OBJECTDIR}/source/sys.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/sys.o.d" -o ${OBJECTDIR}/source/sys.o source/sys.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/config_word.o: source/config_word.c  .generated_files/flags/release/badcfe4170d01d24b7a8dbfdd3853f204a433e70 .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/config_word.o.d 
	@${RM} ${OBJECTDIR}/source/config_word.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/config_word.o.d" -o ${OBJECTDIR}/source/config_word.o source/config_word.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/timer.o: source/timer.c  .generated_files/flags/release/52884cbfc67f147a00fe186a33b60f74e9f03c18 .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/timer.o.d 
	@${RM} ${OBJECTDIR}/source/timer.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/timer.o.d" -o ${OBJECTDIR}/source/timer.o source/timer.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/assert.o: source/assert.c  .generated_files/flags/release/38c511653acb57e1adabf89ad396d9aacde89c40 .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/assert.o.d 
	@${RM} ${OBJECTDIR}/source/assert.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/assert.o.d" -o ${OBJECTDIR}/source/assert.o source/assert.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/print.o: source/print.c  .generated_files/flags/release/a9e4d65ebcc617ea9d0dc5cb3a0c05b893045e2b .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/print.o.d 
	@${RM} ${OBJECTDIR}/source/print.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/print.o.d" -o ${OBJECTDIR}/source/print.o source/print.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/dma.o: source/dma.c  .generated_files/flags/release/bb4e2ceb248e2c7732b2f22373827b8669543bd .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/dma.o.d 
	@${RM} ${OBJECTDIR}/source/dma.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/dma.o.d" -o ${OBJECTDIR}/source/dma.o source/dma.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/spi.o: source/spi.c  .generated_files/flags/release/c089cd7b030db777705d6f9ba3e05eb37f6c9e94 .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/spi.o.d 
	@${RM} ${OBJECTDIR}/source/spi.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/spi.o.d" -o ${OBJECTDIR}/source/spi.o source/spi.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/tlc5940.o: source/tlc5940.c  .generated_files/flags/release/13fd8d8fdcd835c3cb96451818fe3c1731242633 .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/tlc5940.o.d 
	@${RM} ${OBJECTDIR}/source/tlc5940.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/tlc5940.o.d" -o ${OBJECTDIR}/source/tlc5940.o source/tlc5940.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/pwm.o: source/pwm.c  .generated_files/flags/release/ef478aa4813727b6e22674c3e9b7b20fb368c333 .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/pwm.o.d 
	@${RM} ${OBJECTDIR}/source/pwm.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/pwm.o.d" -o ${OBJECTDIR}/source/pwm.o source/pwm.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/layer.o: source/layer.c  .generated_files/flags/release/cab8df95fd7c21e1a3401cb813854c895751d5c6 .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/layer.o.d 
	@${RM} ${OBJECTDIR}/source/layer.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/layer.o.d" -o ${OBJECTDIR}/source/layer.o source/layer.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/rs485.o: source/rs485.c  .generated_files/flags/release/ae535fcafdc06ce7e863cc037ceaa20743774dc2 .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/rs485.o.d 
	@${RM} ${OBJECTDIR}/source/rs485.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/rs485.o.d" -o ${OBJECTDIR}/source/rs485.o source/rs485.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/bus.o: source/bus.c  .generated_files/flags/release/507ef257fcfd543372eb321ff2c4719d318a712a .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/bus.o.d 
	@${RM} ${OBJECTDIR}/source/bus.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/bus.o.d" -o ${OBJECTDIR}/source/bus.o source/bus.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/crc16.o: source/crc16.c  .generated_files/flags/release/2ec8be01e63ff4c7aede3185f6e14798f3b4c67a .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/crc16.o.d 
	@${RM} ${OBJECTDIR}/source/crc16.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/crc16.o.d" -o ${OBJECTDIR}/source/crc16.o source/crc16.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/bus_func_impl.o: source/bus_func_impl.c  .generated_files/flags/release/7eaf95a53efba883770517ad72a385e0173dd6c5 .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/bus_func_impl.o.d 
	@${RM} ${OBJECTDIR}/source/bus_func_impl.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/bus_func_impl.o.d" -o ${OBJECTDIR}/source/bus_func_impl.o source/bus_func_impl.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
else
${OBJECTDIR}/source/main.o: source/main.c  .generated_files/flags/release/eb4cf44cb25d4d719c35713895a50612cede8bb8 .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/main.o.d 
	@${RM} ${OBJECTDIR}/source/main.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/main.o.d" -o ${OBJECTDIR}/source/main.o source/main.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/kernel.o: source/kernel.c  .generated_files/flags/release/81797c08c9783ee4193bf5e18bdf2cbf76497245 .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/kernel.o.d 
	@${RM} ${OBJECTDIR}/source/kernel.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/kernel.o.d" -o ${OBJECTDIR}/source/kernel.o source/kernel.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/sys.o: source/sys.c  .generated_files/flags/release/800ca744b0f66daa5c027dbb10d1ff10517650b1 .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/sys.o.d 
	@${RM} ${OBJECTDIR}/source/sys.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/sys.o.d" -o ${OBJECTDIR}/source/sys.o source/sys.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/config_word.o: source/config_word.c  .generated_files/flags/release/48dc79c067202e9ad73beb378cfef382d555d6e7 .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/config_word.o.d 
	@${RM} ${OBJECTDIR}/source/config_word.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/config_word.o.d" -o ${OBJECTDIR}/source/config_word.o source/config_word.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/timer.o: source/timer.c  .generated_files/flags/release/7e49a5741415183a3c361099ebb936dec3d25865 .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/timer.o.d 
	@${RM} ${OBJECTDIR}/source/timer.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/timer.o.d" -o ${OBJECTDIR}/source/timer.o source/timer.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/assert.o: source/assert.c  .generated_files/flags/release/7bc233336b788571b72b0945aa9894562b46e735 .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/assert.o.d 
	@${RM} ${OBJECTDIR}/source/assert.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/assert.o.d" -o ${OBJECTDIR}/source/assert.o source/assert.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/print.o: source/print.c  .generated_files/flags/release/9fbca2ce26819ce321ae3c6678323dad4310d5a4 .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/print.o.d 
	@${RM} ${OBJECTDIR}/source/print.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/print.o.d" -o ${OBJECTDIR}/source/print.o source/print.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/dma.o: source/dma.c  .generated_files/flags/release/3a8b69c12292c5a0f4b85a8ef228e6e879a33356 .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/dma.o.d 
	@${RM} ${OBJECTDIR}/source/dma.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/dma.o.d" -o ${OBJECTDIR}/source/dma.o source/dma.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/spi.o: source/spi.c  .generated_files/flags/release/b7ebacc0d7b2a9c90fc666617308d92f9eca80e6 .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/spi.o.d 
	@${RM} ${OBJECTDIR}/source/spi.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/spi.o.d" -o ${OBJECTDIR}/source/spi.o source/spi.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/tlc5940.o: source/tlc5940.c  .generated_files/flags/release/6ffa7cb5e00c2534ac92bca3d6750e252fd2a625 .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/tlc5940.o.d 
	@${RM} ${OBJECTDIR}/source/tlc5940.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/tlc5940.o.d" -o ${OBJECTDIR}/source/tlc5940.o source/tlc5940.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/pwm.o: source/pwm.c  .generated_files/flags/release/cf7afd12ecaab915b99fd49217fd63571f61f176 .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/pwm.o.d 
	@${RM} ${OBJECTDIR}/source/pwm.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/pwm.o.d" -o ${OBJECTDIR}/source/pwm.o source/pwm.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/layer.o: source/layer.c  .generated_files/flags/release/e3d933f2e503be003e8d8741e35a93e270f567ea .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/layer.o.d 
	@${RM} ${OBJECTDIR}/source/layer.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/layer.o.d" -o ${OBJECTDIR}/source/layer.o source/layer.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/rs485.o: source/rs485.c  .generated_files/flags/release/d40edd669c30b5b4fb4206989b528550204fab46 .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/rs485.o.d 
	@${RM} ${OBJECTDIR}/source/rs485.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/rs485.o.d" -o ${OBJECTDIR}/source/rs485.o source/rs485.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/bus.o: source/bus.c  .generated_files/flags/release/7ddbfc551e57024c156f3402393d2391c86f6a5b .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/bus.o.d 
	@${RM} ${OBJECTDIR}/source/bus.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/bus.o.d" -o ${OBJECTDIR}/source/bus.o source/bus.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/crc16.o: source/crc16.c  .generated_files/flags/release/9dce8d20c8a06b04f87ed925e13a48af1a7c4c63 .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/crc16.o.d 
	@${RM} ${OBJECTDIR}/source/crc16.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/crc16.o.d" -o ${OBJECTDIR}/source/crc16.o source/crc16.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
${OBJECTDIR}/source/bus_func_impl.o: source/bus_func_impl.c  .generated_files/flags/release/2ecf528a905094f092cda5d4d73f424ce127f767 .generated_files/flags/release/dd3623c89e4677f90469d78ff507bf2f23022ec
	@${MKDIR} "${OBJECTDIR}/source" 
	@${RM} ${OBJECTDIR}/source/bus_func_impl.o.d 
	@${RM} ${OBJECTDIR}/source/bus_func_impl.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"include" -funroll-loops -fno-common -D_SYS_CLK=80000000 -D_PB_DIV=1 -MP -MMD -MF "${OBJECTDIR}/source/bus_func_impl.o.d" -o ${OBJECTDIR}/source/bus_func_impl.o source/bus_func_impl.c    -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -std=c99 -mdfp="${DFP_DIR}"  
	
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
	${MP_CC} $(MP_EXTRA_LD_PRE) -g   -mprocessor=$(MP_PROCESSOR_OPTION)  -o dist/${CND_CONF}/${IMAGE_TYPE}/led-controller.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}          -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)      -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-D=__DEBUG_D,--no-code-in-dinit,--no-dinit-in-serial-mem,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml -mdfp="${DFP_DIR}"
	
else
dist/${CND_CONF}/${IMAGE_TYPE}/led-controller.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   linker/p32MX330F064H.ld
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -mprocessor=$(MP_PROCESSOR_OPTION)  -o dist/${CND_CONF}/${IMAGE_TYPE}/led-controller.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}          -DXPRJ_release=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),--no-code-in-dinit,--no-dinit-in-serial-mem,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml -mdfp="${DFP_DIR}"
	${MP_CC_DIR}/xc32-bin2hex dist/${CND_CONF}/${IMAGE_TYPE}/led-controller.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} 
endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/release
	${RM} -r dist/release

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(shell "${PATH_TO_IDE_BIN}"mplabwildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
