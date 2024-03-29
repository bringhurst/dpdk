#   BSD LICENSE
# 
#   Copyright(c) 2010-2013 Intel Corporation. All rights reserved.
#   All rights reserved.
# 
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions
#   are met:
# 
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#     * Neither the name of Intel Corporation nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
# 
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#
# Common to rte.lib.mk, rte.app.mk, rte.obj.mk
#

SRCS-all := $(SRCS-y) $(SRCS-n) $(SRCS-)

# convert source to obj file
src2obj = $(strip $(patsubst %.c,%.o,\
	$(patsubst %.S,%_s.o,$(1))))

# add a dot in front of the file name
dotfile = $(strip $(foreach f,$(1),\
	$(join $(dir $f),.$(notdir $f))))

# convert source/obj files into dot-dep filename (does not
# include .S files)
src2dep = $(strip $(call dotfile,$(patsubst %.c,%.o.d, \
		$(patsubst %.S,,$(1)))))
obj2dep = $(strip $(call dotfile,$(patsubst %.o,%.o.d,$(1))))

# convert source/obj files into dot-cmd filename
src2cmd = $(strip $(call dotfile,$(patsubst %.c,%.o.cmd, \
		$(patsubst %.S,%_s.o.cmd,$(1)))))
obj2cmd = $(strip $(call dotfile,$(patsubst %.o,%.o.cmd,$(1))))

OBJS-y := $(call src2obj,$(SRCS-y))
OBJS-n := $(call src2obj,$(SRCS-n))
OBJS-  := $(call src2obj,$(SRCS-))
OBJS-all := $(filter-out $(SRCS-all),$(OBJS-y) $(OBJS-n) $(OBJS-))

DEPS-y := $(call src2dep,$(SRCS-y))
DEPS-n := $(call src2dep,$(SRCS-n))
DEPS-  := $(call src2dep,$(SRCS-))
DEPS-all := $(DEPS-y) $(DEPS-n) $(DEPS-)
DEPSTMP-all := $(DEPS-all:%.d=%.d.tmp)

CMDS-y := $(call src2cmd,$(SRCS-y))
CMDS-n := $(call src2cmd,$(SRCS-n))
CMDS-  := $(call src2cmd,$(SRCS-))
CMDS-all := $(CMDS-y) $(CMDS-n) $(CMDS-)

-include $(DEPS-y) $(CMDS-y)

# command to compile a .c file to generate an object
ifeq ($(USE_HOST),1)
C_TO_O = $(HOSTCC) -Wp,-MD,$(call obj2dep,$(@)).tmp $(HOST_CFLAGS) \
	$(CFLAGS_$(@)) $(HOST_EXTRA_CFLAGS) -o $@ -c $<
C_TO_O_STR = $(subst ','\'',$(C_TO_O)) #'# fix syntax highlight
C_TO_O_DISP = $(if $(V),"$(C_TO_O_STR)","  HOSTCC $(@)")
else
C_TO_O = $(CC) -Wp,-MD,$(call obj2dep,$(@)).tmp $(CFLAGS) \
	$(CFLAGS_$(@)) $(EXTRA_CFLAGS) -o $@ -c $<
C_TO_O_STR = $(subst ','\'',$(C_TO_O)) #'# fix syntax highlight
C_TO_O_DISP = $(if $(V),"$(C_TO_O_STR)","  CC $(@)")
endif
C_TO_O_CMD = "cmd_$@ = $(C_TO_O_STR)"
C_TO_O_DO = @set -e; \
	echo $(C_TO_O_DISP); \
	$(C_TO_O) && \
	echo $(C_TO_O_CMD) > $(call obj2cmd,$(@)) && \
	sed 's,'$@':,dep_'$@' =,' $(call obj2dep,$(@)).tmp > $(call obj2dep,$(@)) && \
	rm -f $(call obj2dep,$(@)).tmp

# return an empty string if string are equal
compare = $(strip $(subst $(1),,$(2)) $(subst $(2),,$(1)))

# return a non-empty string if the dst file does not exist
file_missing = $(call compare,$(wildcard $@),$@)

# return a non-empty string if cmdline changed
cmdline_changed = $(call compare,$(cmd_$@),$(1))

# return a non-empty string if a dependency file does not exist
depfile_missing = $(call compare,$(wildcard $(dep_$@)),$(dep_$@))

# return an empty string if no prereq is newer than target
#     - $^ -> names of all the prerequisites
#     - $(wildcard $^) -> every existing prereq
#     - $(filter-out $(wildcard $^),$^) -> every prereq that don't
#       exist (filter-out removes existing ones from the list)
#     - $? -> names of all the prerequisites newer than target
depfile_newer = $(strip $(filter-out FORCE,$? \
	$(filter-out $(wildcard $^),$^)))

# return 1 if parameter is a non-empty string, else 0
boolean = $(if $1,1,0)

#
# Compile .c file if needed
# Note: dep_$$@ is from the .d file and DEP_$$@ can be specified by
# user (by default it is empty)
#
.SECONDEXPANSION:
%.o: %.c $$(wildcard $$(dep_$$@)) $$(DEP_$$(@)) FORCE
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	$(if $(D),\
		@echo -n "$< -> $@ " ; \
		echo -n "file_missing=$(call boolean,$(file_missing)) " ; \
		echo -n "cmdline_changed=$(call boolean,$(call cmdline_changed,$(C_TO_O_STR))) " ; \
		echo -n "depfile_missing=$(call boolean,$(depfile_missing)) " ; \
		echo "depfile_newer=$(call boolean,$(depfile_newer))")
	$(if $(or \
		$(file_missing),\
		$(call cmdline_changed,$(C_TO_O_STR)),\
		$(depfile_missing),\
		$(depfile_newer)),\
		$(C_TO_O_DO))

# command to assemble a .S file to generate an object
ifeq ($(USE_HOST),1)
S_TO_O = $(CPP) $(HOST_CPPFLAGS) $($(@)_CPPFLAGS) $(HOST_EXTRA_CPPFLAGS) $< $(@).tmp && \
	$(HOSTAS) $(HOST_ASFLAGS) $($(@)_ASFLAGS) $(HOST_EXTRA_ASFLAGS) -o $@ $(@).tmp
S_TO_O_STR = $(subst ','\'',$(S_TO_O)) #'# fix syntax highlight
S_TO_O_DISP =  $(if $(V),"$(S_TO_O_STR)","  HOSTAS $(@)")
else
S_TO_O = $(CPP) $(CPPFLAGS) $($(@)_CPPFLAGS) $(EXTRA_CPPFLAGS) $< -o $(@).tmp && \
	$(AS) $(ASFLAGS) $($(@)_ASFLAGS) $(EXTRA_ASFLAGS) -o $@ $(@).tmp
S_TO_O_STR = $(subst ','\'',$(S_TO_O)) #'# fix syntax highlight
S_TO_O_DISP =  $(if $(V),"$(S_TO_O_STR)","  AS $(@)")
endif

S_TO_O_CMD = "cmd_$@ = $(S_TO_O_STR)"
S_TO_O_DO = @set -e; \
	echo $(S_TO_O_DISP); \
	$(S_TO_O) && \
	echo $(S_TO_O_CMD) > $(call obj2cmd,$(@))

#
# Compile .S file if needed
# Note: DEP_$$@ can be specified by user (by default it is empty)
#
%_s.o: %.S $$(DEP_$$@) FORCE
	@[ ! -d $(dir $@) ] || mkdir -p $(dir $@)
	$(if $(D),\
		@echo -n "$< -> $@ " ; \
		echo -n "file_missing=$(call boolean,$(file_missing)) " ; \
		echo -n "cmdline_changed=$(call boolean,$(call cmdline_changed,$(S_TO_O_STR))) " ; \
		echo -n "depfile_missing=$(call boolean,$(depfile_missing)) " ; \
		echo "depfile_newer=$(call boolean,$(depfile_newer)) ")
	$(if $(or \
		$(file_missing),\
		$(call cmdline_changed,$(S_TO_O_STR)),\
		$(depfile_missing),\
		$(depfile_newer)),\
		$(S_TO_O_DO))
