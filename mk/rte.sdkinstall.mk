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

ifdef O
ifeq ("$(origin O)", "command line")
$(error "Cannot use O= with install target")
endif
endif

# Targets to install can be specified in command line. It can be a
# target name or a name containing jokers "*". Example:
# x86_64-default-*-gcc
ifndef T
T=*
endif

#
# install: build sdk for all supported targets
#
INSTALL_CONFIGS := $(patsubst $(RTE_SRCDIR)/config/defconfig_%,%,\
	$(wildcard $(RTE_SRCDIR)/config/defconfig_$(T)))
INSTALL_TARGETS := $(addsuffix _install,\
	$(filter-out %~,$(INSTALL_CONFIGS)))

.PHONY: install
install: $(INSTALL_TARGETS)

%_install:
	@echo ================== Installing $*
	$(Q)$(MAKE) config T=$* O=$*
	$(Q)$(MAKE) all O=$*

#
# uninstall: remove all built sdk
#
UNINSTALL_TARGETS := $(addsuffix _uninstall,\
	$(filter-out %~,$(INSTALL_CONFIGS)))

.PHONY: uninstall
uninstall: $(UNINSTALL_TARGETS)

%_uninstall:
	@echo ================== Uninstalling $*
	$(Q)rm -rf $*


