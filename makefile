A5_NAME := itpdk_demo
A5_TARGET= ${A5_NAME}
A5_LIBS   := -lfixapi -lsecitpdk
A5_INC		:= -I./src

ifeq "$(LOSAP_BUILDTYPE)" "debug"
BUILD_TYPE=Debug
CXXFLAGS=-O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -std=c++11 -fvisibility=hidden
else
BUILD_TYPE=Release
CXXFLAGS=-O2 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -std=c++11 -fvisibility=hidden
endif

A5_LIBDIR := ./lib
A5_LIBPATH := -L./lib
A5_OUTDIR  := ./out
# A5_OUTDIR=${LOSAP_OUT_DIR}/${A5_NAME}/${BUILD_TYPE}

SRCDIR=src
CPP_SRCS=$(wildcard $(SRCDIR)/*.cpp )
NODIR_FILE=$(notdir $(CPP_SRCS)) 
OBJS=$(patsubst %.cpp,${A5_OUTDIR}/%.o,$(NODIR_FILE) )
CPP_DEPS=$(patsubst %.cpp,${A5_OUTDIR}/%.d,$(NODIR_FILE) )
-include $(CPP_DEPS)

# Each subdirectory must supply rules for building sources it contributes
${A5_OUTDIR}/%.o: $(SRCDIR)/%.cpp
	@echo '[$(A5_NAME)|$(BUILD_TYPE)] Compile $< '	
	@g++ ${A5_INC} ${CXXFLAGS} -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	

all: PRJ_REQ ${A5_OUTDIR}/${A5_TARGET}
# Tool invocations
${A5_OUTDIR}/${A5_TARGET}: $(OBJS)	
	@echo '[$(A5_NAME)|$(BUILD_TYPE)] Linking...'
	@g++ ${A5_LIBPATH} -zdefs -o $@ $(OBJS) ${A5_LIBS} -pg
	@echo '[$(A5_NAME)|$(BUILD_TYPE)] Building Success --> $@'
	@cp ${A5_LIBDIR}/* ${A5_OUTDIR}/

PRJ_REQ:
	-@if [ ! -d $(A5_OUTDIR)  ]; then mkdir -p $(A5_OUTDIR); fi

clean:
	-$(RM) ${A5_OUTDIR}/itpdk_demo
	-$(RM) ${A5_OUTDIR}/*.o
	-$(RM) ${A5_OUTDIR}/*.d
	-@echo ''

.PHONY: all clean dependents PRJ_REQ
