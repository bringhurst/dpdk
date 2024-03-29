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

ifeq (,$(wildcard $(RTE_OUTPUT)/.config))
  $(error "need a make config first")
endif
ifeq (,$(wildcard $(RTE_OUTPUT)/Makefile))
  $(error "need a make config first")
endif

# use a "for" in a shell to process dependencies: we don't want this
# task to be run in parallel.
..PHONY: depdirs
depdirs:
	@rm -f $(RTE_OUTPUT)/.depdirs ; \
	for d in $(ROOTDIRS-y); do \
		if [ -f $(RTE_SRCDIR)/$$d/Makefile ]; then \
			[ -d $(BUILDDIR)/$$d ] || mkdir -p $(BUILDDIR)/$$d ; \
			$(MAKE) S=$$d -f $(RTE_SRCDIR)/$$d/Makefile depdirs \
				>> $(RTE_OUTPUT)/.depdirs ; \
		fi ; \
	done

.PHONY: depgraph
depgraph:
	@echo "digraph unix {" ; \
	echo "    size=\"6,6\";" ; \
	echo "    node [color=lightblue2, style=filled];" ; \
	for d in $(ROOTDIRS-y); do \
		echo "    \"root\" -> \"$$d\"" ; \
		if [ -f $(RTE_SRCDIR)/$$d/Makefile ]; then \
			$(MAKE) S=$$d -f $(RTE_SRCDIR)/$$d/Makefile depgraph ; \
		fi ; \
	done ; \
	echo "}"
