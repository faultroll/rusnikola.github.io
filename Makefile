
prfx   ?= 
cxx    := $(prfx)g++
ar     := $(prfx)ar
ranlib := $(prfx)ranlib
strip  := $(prfx)strip

name    := memory_tracker
srcs    := # $(wildcard *.cc)
objs    := $(patsubst %.cc,%.o,$(filter %.cc, $(srcs)))
deps    := $(patsubst %.o,%.d,$(objs))
libs    := -lpthread # -latomic
cflags  := -I. -I./rideables -I./trackers
ldflags := 
common_cflags  := -Os -Wall -Wextra -std=c++11 -fPIC -fpermissive
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

%.o : %.cc
	@$(cxx) $(common_cflags) $(cflags) -c $< -o $@ -MMD -MF $*.d -MP
	$(info $(cxx) -c $(notdir $<) -o $(notdir $@))
%.o : %.cpp
	@$(cxx) $(common_cflags) $(cflags) -c $< -o $@ -MMD -MF $*.d -MP
	$(info $(cxx) -c $(notdir $<) -o $(notdir $@))

-include $(deps)
