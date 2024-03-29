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

#ifndef _RTE_HEXDUMP_H_
#define _RTE_HEXDUMP_H_

/**
 * @file
 * Simple API to dump out memory in a special hex format.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
* Dump out memory in a special hex dump format.
*
* @param title
* 		If not NULL this string is printed as a header to the output.
* @param buf
* 		This is the buffer address to print out.
* @param len
* 		The number of bytes to dump out
* @return
* 		None.
*/

extern void
rte_hexdump(const char * title, const void * buf, unsigned int len);

/**
* Dump out memory in a hex format with colons between bytes.
*
* @param title
* 		If not NULL this string is printed as a header to the output.
* @param buf
* 		This is the buffer address to print out.
* @param len
* 		The number of bytes to dump out
* @return
* 		None.
*/

void
rte_memdump(const char * title, const void * buf, unsigned int len);


#ifdef __cplusplus
}
#endif

#endif /* _RTE_HEXDUMP_H_ */
