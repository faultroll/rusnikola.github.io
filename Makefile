
prfx   ?= 
cc     := $(prfx)gcc
cxx    := $(prfx)g++
ar     := $(prfx)ar
ranlib := $(prfx)ranlib
strip  := $(prfx)strip

name    := mtracker
srcs    := test.cpp rideables/BonsaiTree.cpp gettid.c # $(wildcard *.c) $(wildcard *.cpp)
srcs    += mtracker.c \
           trackers/mtracker_base.c trackers/mtracker_rcu.c
# srcs    += $(wildcard features/*.c)
srcs    += trackers/ssmem/src/ssmem.c trackers/mtracker_ssmem.c
srcs    += cutest/CuTest.c
objs    := $(patsubst %.c,%.o,$(filter %.c, $(srcs))) \
           $(patsubst %.cpp,%.o,$(filter %.cpp, $(srcs)))
deps    := $(patsubst %.o,%.d,$(objs))
libs    := -lpthread # -latomic
cflags  := -I. -I./rideables -I./trackers -Wno-unused-parameter
cflags  += -I./features -DWEBRTC_POSIX
cflags  += -I./trackers/ssmem/include
cflags  += -I./cutest
ldflags := 
common_cflags  := -Os -Wall -Wextra -fPIC
common_ldflags := -Wl,--gc-sections -Wl,--as-needed -Wl,--export-dynamic


targets := $(name).elf
all : $(targets)

clean : 
	rm -f $(targets)
	rm -f $(objs) $(deps)

lib$(name).so : $(objs)
	@$(cxx) -shared $(common_ldflags) $(ldflags) $^ -o $@ $(libs)
	@$(strip) --strip-all $@
	$(info $(cxx) -shared $(notdir $^) -o $(notdir $@))

lib$(name).a : $(objs)
	@$(ar) -crD $@ $^
	@$(ranlib) -D $@
	@$(strip) --strip-unneeded $@
	$(info $(ar) -crD $(notdir $@) $(notdir $^))

$(name).elf : $(objs)
	@$(cxx) $(common_ldflags) $(ldflags) $^ -o $@ $(libs)
	@$(strip) --strip-all $@
	$(info $(cxx) $(notdir $^) -o $(notdir $@))

%.o : %.c
	@$(cc) $(common_cflags) $(cflags) -std=c11 -D_POSIX_C_SOURCE=200809L -Wpedantic -c $< -o $@ -MMD -MF $*.d -MP
	$(info $(cc) -c $(notdir $<) -o $(notdir $@))
%.o : %.cpp
	@$(cxx) $(common_cflags) $(cflags) -std=c++11 -fpermissive -c $< -o $@ -MMD -MF $*.d -MP
	$(info $(cxx) -c $(notdir $<) -o $(notdir $@))

-include $(deps)
