/*-
 *   BSD LICENSE
 * 
 *   Copyright(c) 2010-2013 Intel Corporation. All rights reserved.
 *   All rights reserved.
 * 
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 * 
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * Holds the structures for the eal internal configuration
 */

#ifndef _EAL_LINUXAPP_INTERNAL_CFG
#define _EAL_LINUXAPP_INTERNAL_CFG

#include <rte_eal.h>

#define MAX_HUGEPAGE_SIZES 3  /**< support up to 3 page sizes */

/*
 * internal configuration structure for the number, size and
 * mount points of hugepages
 */
struct hugepage_info {
	size_t hugepage_sz;   /**< size of a huge page */
	const char *hugedir;    /**< dir where hugetlbfs is mounted */
	uint32_t num_pages[RTE_MAX_NUMA_NODES];
				/**< number of hugepages of that size on each socket */
	int lock_descriptor;    /**< file descriptor for hugepage dir */
};

/**
 * internal configuration
 */
struct internal_config {
	volatile size_t memory;           /**< amount of asked memory */
	volatile unsigned force_nchannel; /**< force number of channels */
	volatile unsigned force_nrank;    /**< force number of ranks */
	volatile unsigned no_hugetlbfs;   /**< true to disable hugetlbfs */
	volatile unsigned no_pci;         /**< true to disable PCI */
	volatile unsigned no_hpet;        /**< true to disable HPET */
	volatile unsigned vmware_tsc_map; /**< true to use VMware TSC mapping
										* instead of native TSC */
	volatile unsigned no_shconf;      /**< true if there is no shared config */
	volatile enum rte_proc_type_t process_type; /* multi-process proc type */
	/* true to try allocating memory on specific sockets */
	volatile unsigned force_sockets;
	volatile uint64_t socket_mem[RTE_MAX_NUMA_NODES]; /**< amount of memory per socket*/
	volatile int syslog_facility;	  /**< facility passed to openlog() */
	const char *hugefile_prefix;      /**< the base filename of hugetlbfs files */
	const char *hugepage_dir;         /**< specific hugetlbfs directory to use */

	unsigned num_hugepage_sizes;      /**< how many sizes on this system */
	struct hugepage_info hugepage_info[MAX_HUGEPAGE_SIZES];
};
extern struct internal_config internal_config; /**< Global EAL configuration. */

#endif
