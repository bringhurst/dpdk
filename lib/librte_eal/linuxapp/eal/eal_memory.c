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
/*   BSD LICENSE
 *
 *   Copyright(c) 2013 6WIND.
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
 *     * Neither the name of 6WIND S.A. nor the names of its
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

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <sys/file.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <rte_log.h>
#include <rte_memory.h>
#include <rte_memzone.h>
#include <rte_launch.h>
#include <rte_tailq.h>
#include <rte_eal.h>
#include <rte_eal_memconfig.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_common.h>
#include <rte_string_fns.h>

#include "eal_private.h"
#include "eal_internal_cfg.h"
#include "eal_filesystem.h"
#include "eal_hugepages.h"

/**
 * @file
 * Huge page mapping under linux
 *
 * To reserve a big contiguous amount of memory, we use the hugepage
 * feature of linux. For that, we need to have hugetlbfs mounted. This
 * code will create many files in this directory (one per page) and
 * map them in virtual memory. For each page, we will retrieve its
 * physical address and remap it in order to have a virtual contiguous
 * zone as well as a physical contiguous zone.
 */


#define RANDOMIZE_VA_SPACE_FILE "/proc/sys/kernel/randomize_va_space"

/*
 * Check whether address-space layout randomization is enabled in
 * the kernel. This is important for multi-process as it can prevent
 * two processes mapping data to the same virtual address
 * Returns:
 *    0 - address space randomization disabled
 *    1/2 - address space randomization enabled
 *    negative error code on error
 */
static int
aslr_enabled(void)
{
	char c;
	int retval, fd = open(RANDOMIZE_VA_SPACE_FILE, O_RDONLY);
	if (fd < 0)
		return -errno;
	retval = read(fd, &c, 1);
	close(fd);
	if (retval < 0)
		return -errno;
	if (retval == 0)
		return -EIO;
	switch (c) {
		case '0' : return 0;
		case '1' : return 1;
		case '2' : return 2;
		default: return -EINVAL;
	}
}

/*
 * Try to mmap *size bytes in /dev/zero. If it is succesful, return the
 * pointer to the mmap'd area and keep *size unmodified. Else, retry
 * with a smaller zone: decrease *size by hugepage_sz until it reaches
 * 0. In this case, return NULL. Note: this function returns an address
 * which is a multiple of hugepage size.
 */
static void *
get_virtual_area(size_t *size, size_t hugepage_sz)
{
	void *addr;
	int fd;
	long aligned_addr;

	RTE_LOG(INFO, EAL, "Ask a virtual area of 0x%zu bytes\n", *size);

	fd = open("/dev/zero", O_RDONLY);
	if (fd < 0){
		RTE_LOG(ERR, EAL, "Cannot open /dev/zero\n");
		return NULL;
	}
	do {
		addr = mmap(NULL, (*size) + hugepage_sz, PROT_READ, MAP_PRIVATE, fd, 0);
		if (addr == MAP_FAILED)
			*size -= hugepage_sz;
	} while (addr == MAP_FAILED && *size > 0);

	if (addr == MAP_FAILED) {
		close(fd);
		RTE_LOG(INFO, EAL, "Cannot get a virtual area\n");
		return NULL;
	}

	munmap(addr, (*size) + hugepage_sz);
	close(fd);

	/* align addr to a huge page size boundary */
	aligned_addr = (long)addr;
	aligned_addr += (hugepage_sz - 1);
	aligned_addr &= (~(hugepage_sz - 1));
	addr = (void *)(aligned_addr);

	RTE_LOG(INFO, EAL, "Virtual area found at %p (size = 0x%zx)\n",
		addr, *size);

	return addr;
}

/*
 * Mmap all hugepages of hugepage table: it first open a file in
 * hugetlbfs, then mmap() hugepage_sz data in it. If orig is set, the
 * virtual address is stored in hugepg_tbl[i].orig_va, else it is stored
 * in hugepg_tbl[i].final_va. The second mapping (when orig is 0) tries to
 * map continguous physical blocks in contiguous virtual blocks.
 */
static int
map_all_hugepages(struct hugepage *hugepg_tbl,
		struct hugepage_info *hpi, int orig)
{
	int fd;
	unsigned i;
	void *virtaddr;
	void *vma_addr = NULL;
	size_t vma_len = 0;

	for (i = 0; i < hpi->num_pages[0]; i++) {
		size_t hugepage_sz = hpi->hugepage_sz;

		if (orig) {
			hugepg_tbl[i].file_id = i;
			hugepg_tbl[i].size = hugepage_sz;
			eal_get_hugefile_path(hugepg_tbl[i].filepath,
					sizeof(hugepg_tbl[i].filepath), hpi->hugedir,
					hugepg_tbl[i].file_id);
			hugepg_tbl[i].filepath[sizeof(hugepg_tbl[i].filepath) - 1] = '\0';
		}
#ifndef RTE_ARCH_X86_64
		/* for 32-bit systems, don't remap 1G pages, just reuse original
		 * map address as final map address.
		 */
		else if (hugepage_sz == RTE_PGSIZE_1G){
			hugepg_tbl[i].final_va = hugepg_tbl[i].orig_va;
			hugepg_tbl[i].orig_va = NULL;
			continue;
		}
#endif
		else if (vma_len == 0) {
			unsigned j, num_pages;

			/* reserve a virtual area for next contiguous
			 * physical block: count the number of
			 * contiguous physical pages. */
			for (j = i+1; j < hpi->num_pages[0] ; j++) {
				if (hugepg_tbl[j].physaddr !=
				    hugepg_tbl[j-1].physaddr + hugepage_sz)
					break;
			}
			num_pages = j - i;
			vma_len = num_pages * hugepage_sz;

			/* get the biggest virtual memory area up to
			 * vma_len. If it fails, vma_addr is NULL, so
			 * let the kernel provide the address. */
			vma_addr = get_virtual_area(&vma_len, hpi->hugepage_sz);
			if (vma_addr == NULL)
				vma_len = hugepage_sz;
		}

		/* try to create hugepage file */
		fd = open(hugepg_tbl[i].filepath, O_CREAT | O_RDWR, 0755);
		if (fd < 0) {
			RTE_LOG(ERR, EAL, "%s(): open failed: %s\n", __func__,
					strerror(errno));
			return -1;
		}

		virtaddr = mmap(vma_addr, hugepage_sz, PROT_READ | PROT_WRITE,
				MAP_SHARED, fd, 0);
		if (virtaddr == MAP_FAILED) {
			RTE_LOG(ERR, EAL, "%s(): mmap failed: %s\n", __func__,
					strerror(errno));
			close(fd);
			return -1;
		}

		if (orig) {
			hugepg_tbl[i].orig_va = virtaddr;
			memset(virtaddr, 0, hugepage_sz);
		}
		else {
			hugepg_tbl[i].final_va = virtaddr;
		}

		/* set shared flock on the file. */
		if (flock(fd, LOCK_SH | LOCK_NB) == -1) {
			RTE_LOG(ERR, EAL, "%s(): Locking file failed:%s \n",
				__func__, strerror(errno));
			close(fd);
			return -1;
		}

		close(fd);

		vma_addr = (char *)vma_addr + hugepage_sz;
		vma_len -= hugepage_sz;
	}
	return 0;
}

/* Unmap all hugepages from original mapping. */
static int
unmap_all_hugepages_orig(struct hugepage *hugepg_tbl, struct hugepage_info *hpi)
{
	unsigned i;
	for (i = 0; i < hpi->num_pages[0]; i++) {
		if (hugepg_tbl[i].orig_va) {
			munmap(hugepg_tbl[i].orig_va, hpi->hugepage_sz);
			hugepg_tbl[i].orig_va = NULL;
		}
	}
	return 0;
}

/*
 * For each hugepage in hugepg_tbl, fill the physaddr value. We find
 * it by browsing the /proc/self/pagemap special file.
 */
static int
find_physaddr(struct hugepage *hugepg_tbl, struct hugepage_info *hpi)
{
	int fd;
	unsigned i;
	uint64_t page;
	unsigned long virt_pfn;
	int page_size;

	/* standard page size */
	page_size = getpagesize();

	fd = open("/proc/self/pagemap", O_RDONLY);
	if (fd < 0) {
		RTE_LOG(ERR, EAL, "%s(): cannot open /proc/self/pagemap: %s\n",
			__func__, strerror(errno));
		return -1;
	}

	for (i = 0; i < hpi->num_pages[0]; i++) {
		off_t offset;
		virt_pfn = (unsigned long)hugepg_tbl[i].orig_va /
			page_size;
		offset = sizeof(uint64_t) * virt_pfn;
		if (lseek(fd, offset, SEEK_SET) == (off_t) -1) {
			RTE_LOG(ERR, EAL, "%s(): seek error in /proc/self/pagemap: %s\n",
					__func__, strerror(errno));
			close(fd);
			return -1;
		}
		if (read(fd, &page, sizeof(uint64_t)) < 0) {
			RTE_LOG(ERR, EAL, "%s(): cannot read /proc/self/pagemap: %s\n",
					__func__, strerror(errno));
			close(fd);
			return -1;
		}

		/*
		 * the pfn (page frame number) are bits 0-54 (see
		 * pagemap.txt in linux Documentation)
		 */
		hugepg_tbl[i].physaddr = ((page & 0x7fffffffffffffULL) * page_size);
	}
	close(fd);
	return 0;
}

/*
 * Parse /proc/self/numa_maps to get the NUMA socket ID for each huge
 * page.
 */
static int
find_numasocket(struct hugepage *hugepg_tbl, struct hugepage_info *hpi)
{
	int socket_id;
	char *end, *nodestr;
	unsigned i, hp_count = 0;
	uint64_t virt_addr;
	char buf[BUFSIZ];
	char hugedir_str[PATH_MAX];
	FILE *f;

	f = fopen("/proc/self/numa_maps", "r");
	if (f == NULL) {
		RTE_LOG(INFO, EAL, "cannot open /proc/self/numa_maps,"
				" consider that all memory is in socket_id 0\n");
		return 0;
	}

	rte_snprintf(hugedir_str, sizeof(hugedir_str),
			"%s/", hpi->hugedir);

	/* parse numa map */
	while (fgets(buf, sizeof(buf), f) != NULL) {

		/* ignore non huge page */
		if (strstr(buf, " huge ") == NULL &&
				strstr(buf, hugedir_str) == NULL)
			continue;

		/* get zone addr */
		virt_addr = strtoull(buf, &end, 16);
		if (virt_addr == 0 || end == buf) {
			RTE_LOG(ERR, EAL, "%s(): error in numa_maps parsing\n", __func__);
			goto error;
		}

		/* get node id (socket id) */
		nodestr = strstr(buf, " N");
		if (nodestr == NULL) {
			RTE_LOG(ERR, EAL, "%s(): error in numa_maps parsing\n", __func__);
			goto error;
		}
		nodestr += 2;
		end = strstr(nodestr, "=");
		if (end == NULL) {
			RTE_LOG(ERR, EAL, "%s(): error in numa_maps parsing\n", __func__);
			goto error;
		}
		end[0] = '\0';
		end = NULL;

		socket_id = strtoul(nodestr, &end, 0);
		if ((nodestr[0] == '\0') || (end == NULL) || (*end != '\0')) {
			RTE_LOG(ERR, EAL, "%s(): error in numa_maps parsing\n", __func__);
			goto error;
		}

		/* if we find this page in our mappings, set socket_id */
		for (i = 0; i < hpi->num_pages[0]; i++) {
			void *va = (void *)(unsigned long)virt_addr;
			if (hugepg_tbl[i].orig_va == va) {
				hugepg_tbl[i].socket_id = socket_id;
				hp_count++;
			}
		}
	}

	if (hp_count < hpi->num_pages[0])
		goto error;

	fclose(f);
	return 0;

error:
	fclose(f);
	return -1;
}

/*
 * Sort the hugepg_tbl by physical address (lower addresses first). We
 * use a slow algorithm, but we won't have millions of pages, and this
 * is only done at init time.
 */
static int
sort_by_physaddr(struct hugepage *hugepg_tbl, struct hugepage_info *hpi)
{
	unsigned i, j;
	int smallest_idx;
	uint64_t smallest_addr;
	struct hugepage tmp;

	for (i = 0; i < hpi->num_pages[0]; i++) {
		smallest_addr = 0;
		smallest_idx = -1;

		/*
		 * browse all entries starting at 'i', and find the
		 * entry with the smallest addr
		 */
		for (j=i; j< hpi->num_pages[0]; j++) {

			if (smallest_addr == 0 ||
			    hugepg_tbl[j].physaddr < smallest_addr) {
				smallest_addr = hugepg_tbl[j].physaddr;
				smallest_idx = j;
			}
		}

		/* should not happen */
		if (smallest_idx == -1) {
			RTE_LOG(ERR, EAL, "%s(): error in physaddr sorting\n", __func__);
			return -1;
		}

		/* swap the 2 entries in the table */
		memcpy(&tmp, &hugepg_tbl[smallest_idx], sizeof(struct hugepage));
		memcpy(&hugepg_tbl[smallest_idx], &hugepg_tbl[i],
				sizeof(struct hugepage));
		memcpy(&hugepg_tbl[i], &tmp, sizeof(struct hugepage));
	}
	return 0;
}

/*
 * Uses mmap to create a shared memory area for storage of data
 * Used in this file to store the hugepage file map on disk
 */
static void *
create_shared_memory(const char *filename, const size_t mem_size)
{
	void *retval;
	int fd = open(filename, O_CREAT | O_RDWR, 0666);
	if (fd < 0)
		return NULL;
	if (ftruncate(fd, mem_size) < 0) {
		close(fd);
		return NULL;
	}
	retval = mmap(NULL, mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);
	return retval;
}

/*
 * this copies *active* hugepages from one hugepage table to another.
 * destination is typically the shared memory.
 */
static int
copy_hugepages_to_shared_mem(struct hugepage * dst, int dest_size,
		const struct hugepage * src, int src_size)
{
	int src_pos, dst_pos = 0;

	for (src_pos = 0; src_pos < src_size; src_pos++) {
		if (src[src_pos].final_va != NULL) {
			/* error on overflow attempt */
			if (dst_pos == dest_size)
				return -1;
			memcpy(&dst[dst_pos], &src[src_pos], sizeof(struct hugepage));
			dst_pos++;
		}
	}
	return 0;
}

/*
 * unmaps hugepages that are not going to be used. since we originally allocate
 * ALL hugepages (not just those we need), additional unmapping needs to be done.
 */
static int
unmap_unneeded_hugepages(struct hugepage *hugepg_tbl,
		struct hugepage_info *hpi,
		unsigned num_hp_info)
{
	unsigned socket, size;
	int page, nrpages = 0;

	/* get total number of hugepages */
	for (size = 0; size < num_hp_info; size++)
		for (socket = 0; socket < RTE_MAX_NUMA_NODES; socket++)
			nrpages += internal_config.hugepage_info[size].num_pages[socket];

	for (size = 0; size < num_hp_info; size++) {
		for (socket = 0; socket < RTE_MAX_NUMA_NODES; socket++) {
			unsigned pages_found = 0;
			/* traverse until we have unmapped all the unused pages */
			for (page = 0; page < nrpages; page++) {
				struct hugepage *hp = &hugepg_tbl[page];

				/* find a page that matches the criteria */
				if ((hp->size == hpi[size].hugepage_sz) &&
						(hp->socket_id == (int) socket)) {

					/* if we skipped enough pages, unmap the rest */
					if (pages_found == hpi[size].num_pages[socket]) {
						munmap(hp->final_va, hp->size);
						hp->final_va = NULL;
					}
					/* lock the page and skip */
					else
						pages_found++;

				} /* match page */
			} /* foreach page */
		} /* foreach socket */
	} /* foreach pagesize */

	return 0;
}

static inline uint64_t
get_socket_mem_size(int socket)
{
	uint64_t size = 0;
	unsigned i;

	for (i = 0; i < internal_config.num_hugepage_sizes; i++){
		struct hugepage_info *hpi = &internal_config.hugepage_info[i];
		if (hpi->hugedir != NULL)
			size += hpi->hugepage_sz * hpi->num_pages[socket];
	}

	return (size);
}

/*
 * This function is a NUMA-aware equivalent of calc_num_pages.
 * It takes in the list of hugepage sizes and the
 * number of pages thereof, and calculates the best number of
 * pages of each size to fulfill the request for <memory> ram
 */
static int
calc_num_pages_per_socket(uint64_t * memory,
		struct hugepage_info *hp_info,
		struct hugepage_info *hp_used,
		unsigned num_hp_info)
{
	unsigned socket, j, i = 0;
	unsigned requested, available;
	int total_num_pages = 0;
	uint64_t remaining_mem, cur_mem;
	uint64_t total_mem = internal_config.memory;

	if (num_hp_info == 0)
		return -1;

	for (socket = 0; socket < RTE_MAX_NUMA_NODES && total_mem != 0; socket++) {
		/* if specific memory amounts per socket weren't requested */
		if (internal_config.force_sockets == 0) {
			/* take whatever is available */
			memory[socket] = RTE_MIN(get_socket_mem_size(socket),
					total_mem);
		}
		/* skips if the memory on specific socket wasn't requested */
		for (i = 0; i < num_hp_info && memory[socket] != 0; i++){
			hp_used[i].hugedir = hp_info[i].hugedir;
			hp_used[i].num_pages[socket] = RTE_MIN(
					memory[socket] / hp_info[i].hugepage_sz,
					hp_info[i].num_pages[socket]);

			cur_mem = hp_used[i].num_pages[socket] *
					hp_used[i].hugepage_sz;

			memory[socket] -= cur_mem;
			total_mem -= cur_mem;

			total_num_pages += hp_used[i].num_pages[socket];

			/* check if we have met all memory requests */
			if (memory[socket] == 0)
				break;

			/* check if we have any more pages left at this size, if so
			 * move on to next size */
			if (hp_used[i].num_pages[socket] == hp_info[i].num_pages[socket])
				continue;
			/* At this point we know that there are more pages available that are
			 * bigger than the memory we want, so lets see if we can get enough
			 * from other page sizes.
			 */
			remaining_mem = 0;
			for (j = i+1; j < num_hp_info; j++)
				remaining_mem += hp_info[j].hugepage_sz *
				hp_info[j].num_pages[socket];

			/* is there enough other memory, if not allocate another page and quit */
			if (remaining_mem < memory[socket]){
				cur_mem = RTE_MIN(memory[socket],
						hp_info[i].hugepage_sz);
				memory[socket] -= cur_mem;
				total_mem -= cur_mem;
				hp_used[i].num_pages[socket]++;
				total_num_pages++;
				break; /* we are done with this socket*/
			}
		}
		/* if we didn't satisfy all memory requirements per socket */
		if (memory[socket] > 0) {
			/* to prevent icc errors */
			requested = (unsigned) (internal_config.socket_mem[socket] /
					0x100000);
			available = requested -
					((unsigned) (memory[socket] / 0x100000));
			RTE_LOG(INFO, EAL, "Not enough memory available on socket %u! "
					"Requested: %uMB, available: %uMB\n", socket,
					requested, available);
			return -1;
		}
	}

	/* if we didn't satisfy total memory requirements */
	if (total_mem > 0) {
		requested = (unsigned) (internal_config.memory / 0x100000);
		available = requested - (unsigned) (total_mem / 0x100000);
		RTE_LOG(INFO, EAL, "Not enough memory available! Requested: %uMB,"
				" available: %uMB\n", requested, available);
		return -1;
	}
	return total_num_pages;
}

/*
 * Prepare physical memory mapping: fill configuration structure with
 * these infos, return 0 on success.
 *  1. map N huge pages in separate files in hugetlbfs
 *  2. find associated physical addr
 *  3. find associated NUMA socket ID
 *  4. sort all huge pages by physical address
 *  5. remap these N huge pages in the correct order
 *  6. unmap the first mapping
 *  7. fill memsegs in configuration with contiguous zones
 */
static int
rte_eal_hugepage_init(void)
{
	struct rte_mem_config *mcfg;
	struct hugepage *hugepage, *tmp_hp = NULL;
	struct hugepage_info used_hp[MAX_HUGEPAGE_SIZES];

	uint64_t memory[RTE_MAX_NUMA_NODES];

	unsigned hp_offset;
	int i, j, new_memseg;
	int nrpages, total_pages = 0;
	void *addr;

	memset(used_hp, 0, sizeof(used_hp));

	/* get pointer to global configuration */
	mcfg = rte_eal_get_configuration()->mem_config;

	/* for debug purposes, hugetlbfs can be disabled */
	if (internal_config.no_hugetlbfs) {
		addr = malloc(internal_config.memory);
		mcfg->memseg[0].phys_addr = (phys_addr_t)(uintptr_t)addr;
		mcfg->memseg[0].addr = addr;
		mcfg->memseg[0].len = internal_config.memory;
		mcfg->memseg[0].socket_id = 0;
		return 0;
	}


	/* calculate total number of hugepages available. at this point we haven't
	 * yet started sorting them so they all are on socket 0 */
	for (i = 0; i < (int) internal_config.num_hugepage_sizes; i++) {
		/* meanwhile, also initialize used_hp hugepage sizes in used_hp */
		used_hp[i].hugepage_sz = internal_config.hugepage_info[i].hugepage_sz;

		total_pages += internal_config.hugepage_info[i].num_pages[0];
	}

	/*
	 * allocate a memory area for hugepage table.
	 * this isn't shared memory yet. due to the fact that we need some
	 * processing done on these pages, shared memory will be created
	 * at a later stage.
	 */
	tmp_hp = malloc(total_pages * sizeof(struct hugepage));
	if (tmp_hp == NULL)
		goto fail;

	memset(tmp_hp, 0, total_pages * sizeof(struct hugepage));

	hp_offset = 0; /* where we start the current page size entries */

	/* map all hugepages and sort them */
	for (i = 0; i < (int)internal_config.num_hugepage_sizes; i ++){
		struct hugepage_info *hpi;

		/*
		 * we don't yet mark hugepages as used at this stage, so
		 * we just map all hugepages available to the system
		 * all hugepages are still located on socket 0
		 */
		hpi = &internal_config.hugepage_info[i];

		if (hpi->num_pages == 0)
			continue;

		/* map all hugepages available */
		if (map_all_hugepages(&tmp_hp[hp_offset], hpi, 1) < 0){
			RTE_LOG(DEBUG, EAL, "Failed to mmap %u MB hugepages\n",
					(unsigned)(hpi->hugepage_sz / 0x100000));
			goto fail;
		}

		/* find physical addresses and sockets for each hugepage */
		if (find_physaddr(&tmp_hp[hp_offset], hpi) < 0){
			RTE_LOG(DEBUG, EAL, "Failed to find phys addr for %u MB pages\n",
					(unsigned)(hpi->hugepage_sz / 0x100000));
			goto fail;
		}

		if (find_numasocket(&tmp_hp[hp_offset], hpi) < 0){
			RTE_LOG(DEBUG, EAL, "Failed to find NUMA socket for %u MB pages\n",
					(unsigned)(hpi->hugepage_sz / 0x100000));
			goto fail;
		}

		if (sort_by_physaddr(&tmp_hp[hp_offset], hpi) < 0)
			goto fail;

		/* remap all hugepages */
		if (map_all_hugepages(&tmp_hp[hp_offset], hpi, 0) < 0){
			RTE_LOG(DEBUG, EAL, "Failed to remap %u MB pages\n",
					(unsigned)(hpi->hugepage_sz / 0x100000));
			goto fail;
		}

		/* unmap original mappings */
		if (unmap_all_hugepages_orig(&tmp_hp[hp_offset], hpi) < 0)
			goto fail;

		/* we have processed a num of hugepages of this size, so inc offset */
		hp_offset += hpi->num_pages[0];
	}

	/* clean out the numbers of pages */
	for (i = 0; i < (int) internal_config.num_hugepage_sizes; i++)
		for (j = 0; j < RTE_MAX_NUMA_NODES; j++)
			internal_config.hugepage_info[i].num_pages[j] = 0;

	/* get hugepages for each socket */
	for (i = 0; i < total_pages; i++) {
		int socket = tmp_hp[i].socket_id;

		/* find a hugepage info with right size and increment num_pages */
		for (j = 0; j < (int) internal_config.num_hugepage_sizes; j++) {
			if (tmp_hp[i].size ==
					internal_config.hugepage_info[j].hugepage_sz) {
				internal_config.hugepage_info[j].num_pages[socket]++;
			}
		}
	}

	/* make a copy of socket_mem, needed for number of pages calculation */
	for (i = 0; i < RTE_MAX_NUMA_NODES; i++)
		memory[i] = internal_config.socket_mem[i];

	/* calculate final number of pages */
	nrpages = calc_num_pages_per_socket(memory,
			internal_config.hugepage_info, used_hp,
			internal_config.num_hugepage_sizes);

	/* error if not enough memory available */
	if (nrpages < 0)
		goto fail;

	/* reporting in! */
	for (i = 0; i < (int) internal_config.num_hugepage_sizes; i++) {
		for (j = 0; j < RTE_MAX_NUMA_NODES; j++) {
			if (used_hp[i].num_pages[j] > 0) {
				RTE_LOG(INFO, EAL,
						"Requesting %u pages of size %uMB"
						" from socket %i\n",
						used_hp[i].num_pages[j],
						(unsigned)
							(used_hp[i].hugepage_sz / 0x100000),
						j);
			}
		}
	}

	/* create shared memory */
	hugepage = create_shared_memory(eal_hugepage_info_path(),
					nrpages * sizeof(struct hugepage));

	if (hugepage == NULL) {
		RTE_LOG(ERR, EAL, "Failed to create shared memory!\n");
		goto fail;
	}

	/*
	 * unmap pages that we won't need (looks at used_hp).
	 * also, sets final_va to NULL on pages that were unmapped.
	 */
	if (unmap_unneeded_hugepages(tmp_hp, used_hp,
			internal_config.num_hugepage_sizes) < 0) {
		RTE_LOG(ERR, EAL, "Unmapping and locking hugepages failed!\n");
		goto fail;
	}

	/*
	 * copy stuff from malloc'd hugepage* to the actual shared memory.
	 * this procedure only copies those hugepages that have final_va
	 * not NULL. has overflow protection.
	 */
	if (copy_hugepages_to_shared_mem(hugepage, nrpages,
			tmp_hp, total_pages) < 0) {
		RTE_LOG(ERR, EAL, "Copying tables to shared memory failed!\n");
		goto fail;
	}

	/* free the temporary hugepage table */
	free(tmp_hp);
	tmp_hp = NULL;

	memset(mcfg->memseg, 0, sizeof(mcfg->memseg));
	j = -1;
	for (i = 0; i < nrpages; i++) {
		new_memseg = 0;

		/* if this is a new section, create a new memseg */
		if (i == 0)
			new_memseg = 1;
		else if (hugepage[i].socket_id != hugepage[i-1].socket_id)
			new_memseg = 1;
		else if (hugepage[i].size != hugepage[i-1].size)
			new_memseg = 1;
		else if ((hugepage[i].physaddr - hugepage[i-1].physaddr) !=
		    hugepage[i].size)
			new_memseg = 1;
		else if (((unsigned long)hugepage[i].final_va -
		    (unsigned long)hugepage[i-1].final_va) != hugepage[i].size)
			new_memseg = 1;

		if (new_memseg) {
			j += 1;
			if (j == RTE_MAX_MEMSEG)
				break;

			mcfg->memseg[j].phys_addr = hugepage[i].physaddr;
			mcfg->memseg[j].addr = hugepage[i].final_va;
			mcfg->memseg[j].len = hugepage[i].size;
			mcfg->memseg[j].socket_id = hugepage[i].socket_id;
			mcfg->memseg[j].hugepage_sz = hugepage[i].size;
		}
		/* continuation of previous memseg */
		else {
			mcfg->memseg[j].len += mcfg->memseg[j].hugepage_sz;
		}
		hugepage[i].memseg_id = j;
	}

	if (i < nrpages) {
		RTE_LOG(ERR, EAL, "Can only reserve %d pages "
			"from %d requested\n"
			"Current %s=%d is not enough\n"
			"Please either increase it or request less amount "
			"of memory.\n",
			i, nrpages, RTE_STR(CONFIG_RTE_MAX_MEMSEG),
			RTE_MAX_MEMSEG);
		return (-ENOMEM);
	}
	

	return 0;


fail:
	if (tmp_hp)
		free(tmp_hp);
	return -1;
}

/*
 * uses fstat to report the size of a file on disk
 */
static off_t
getFileSize(int fd)
{
	struct stat st;
	if (fstat(fd, &st) < 0)
		return 0;
	return st.st_size;
}

/*
 * This creates the memory mappings in the secondary process to match that of
 * the server process. It goes through each memory segment in the DPDK runtime
 * configuration and finds the hugepages which form that segment, mapping them
 * in order to form a contiguous block in the virtual memory space
 */
static int
rte_eal_hugepage_attach(void)
{
	const struct rte_mem_config *mcfg = rte_eal_get_configuration()->mem_config;
	const struct hugepage *hp = NULL;
	unsigned num_hp = 0;
	unsigned i, s = 0; /* s used to track the segment number */
	off_t size;
	int fd, fd_zero = -1, fd_hugepage = -1;

	if (aslr_enabled() > 0) {
		RTE_LOG(WARNING, EAL, "WARNING: Address Space Layout Randomization "
				"(ASLR) is enabled in the kernel.\n");
		RTE_LOG(WARNING, EAL, "   This may cause issues with mapping memory "
				"into secondary processes\n");
	}

	fd_zero = open("/dev/zero", O_RDONLY);
	if (fd_zero < 0) {
		RTE_LOG(ERR, EAL, "Could not open /dev/zero\n");
		goto error;
	}
	fd_hugepage = open(eal_hugepage_info_path(), O_RDONLY);
	if (fd_hugepage < 0) {
		RTE_LOG(ERR, EAL, "Could not open %s\n", eal_hugepage_info_path());
		goto error;
	}

	/* map all segments into memory to make sure we get the addrs */
	for (s = 0; s < RTE_MAX_MEMSEG; ++s) {
		void *base_addr;

		/*
		 * the first memory segment with len==0 is the one that
		 * follows the last valid segment.
		 */
		if (mcfg->memseg[s].len == 0)
			break;

		/*
		 * fdzero is mmapped to get a contiguous block of virtual
		 * addresses of the appropriate memseg size.
		 * use mmap to get identical addresses as the primary process.
		 */
		base_addr = mmap(mcfg->memseg[s].addr, mcfg->memseg[s].len,
				 PROT_READ, MAP_PRIVATE, fd_zero, 0);
		if (base_addr == MAP_FAILED ||
		    base_addr != mcfg->memseg[s].addr) {
			RTE_LOG(ERR, EAL, "Could not mmap %llu bytes "
				"in /dev/zero to requested address [%p]\n",
				(unsigned long long)mcfg->memseg[s].len,
				mcfg->memseg[s].addr);
			if (aslr_enabled() > 0) {
				RTE_LOG(ERR, EAL, "It is recommended to "
					"disable ASLR in the kernel "
					"and retry running both primary "
					"and secondary processes\n");
			}
			goto error;
		}
	}

	size = getFileSize(fd_hugepage);
	hp = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd_hugepage, 0);
	if (hp == NULL) {
		RTE_LOG(ERR, EAL, "Could not mmap %s\n", eal_hugepage_info_path());
		goto error;
	}

	num_hp = size / sizeof(struct hugepage);
	RTE_LOG(DEBUG, EAL, "Analysing %u hugepages\n", num_hp);

	s = 0;
	while (s < RTE_MAX_MEMSEG && mcfg->memseg[s].len > 0){
		void *addr, *base_addr;
		uintptr_t offset = 0;

		/*
		 * free previously mapped memory so we can map the
		 * hugepages into the space
		 */
		base_addr = mcfg->memseg[s].addr;
		munmap(base_addr, mcfg->memseg[s].len);

		/* find the hugepages for this segment and map them
		 * we don't need to worry about order, as the server sorted the
		 * entries before it did the second mmap of them */
		for (i = 0; i < num_hp && offset < mcfg->memseg[s].len; i++){
			if (hp[i].memseg_id == (int)s){
				fd = open(hp[i].filepath, O_RDWR);
				if (fd < 0) {
					RTE_LOG(ERR, EAL, "Could not open %s\n",
						hp[i].filepath);
					goto error;
				}
				addr = mmap(RTE_PTR_ADD(base_addr, offset),
						hp[i].size, PROT_READ | PROT_WRITE,
						MAP_SHARED | MAP_FIXED, fd, 0);
				close(fd); /* close file both on success and on failure */
				if (addr == MAP_FAILED) {
					RTE_LOG(ERR, EAL, "Could not mmap %s\n",
						hp[i].filepath);
					goto error;
				}
				offset+=hp[i].size;
			}
		}
		RTE_LOG(DEBUG, EAL, "Mapped segment %u of size 0x%llx\n", s,
				(unsigned long long)mcfg->memseg[s].len);
		s++;
	}
	/* unmap the hugepage config file, since we are done using it */
	munmap((void *)(uintptr_t)hp, size);
	close(fd_zero);
	close(fd_hugepage);
	return 0;

error:
	if (fd_zero >= 0)
		close(fd_zero);
	if (fd_hugepage >= 0)
		close(fd_hugepage);
	return -1;
}

static int
rte_eal_memdevice_init(void)
{
	struct rte_config *config;

	if (rte_eal_process_type() == RTE_PROC_SECONDARY)
		return 0;

	config = rte_eal_get_configuration();
	config->mem_config->nchannel = internal_config.force_nchannel;
	config->mem_config->nrank = internal_config.force_nrank;

	return 0;
}


/* init memory subsystem */
int
rte_eal_memory_init(void)
{
	RTE_LOG(INFO, EAL, "Setting up hugepage memory...\n");
	const int retval = rte_eal_process_type() == RTE_PROC_PRIMARY ?
			rte_eal_hugepage_init() :
			rte_eal_hugepage_attach();
	if (retval < 0)
		return -1;

	if (internal_config.no_shconf == 0 && rte_eal_memdevice_init() < 0)
		return -1;

	return 0;
}
