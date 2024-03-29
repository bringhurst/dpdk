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

#include <math.h>
#include "rte_red.h"
#include <rte_random.h>

#ifdef __INTEL_COMPILER
#pragma warning(disable:2259) /* conversion may lose significant bits */
#endif

#define DIM(x) (sizeof(x)/sizeof(x[0]))

static int rte_red_init_done = 0;     /**< Flag to indicate that global initialisation is done */
uint32_t rte_red_rand_val = 0;        /**< Random value cache */
uint32_t rte_red_rand_seed = 0;       /**< Seed for random number generation */

/**
 * table[i] = log2(1-Wq) * Scale * -1
 *       Wq = 1/(2^i)
 */
uint16_t rte_red_log2_1_minus_Wq[RTE_RED_WQ_LOG2_NUM];

/**
 * table[i] = 2^(i/16) * Scale
 */
uint16_t rte_red_pow2_frac_inv[16];

/**
 * @brief Initialize tables used to compute average
 *        queue size when queue is empty.
 */
static void
__rte_red_init_tables(void)
{
	uint32_t i = 0;
	double scale = 0.0;
	double table_size = 0.0;

	scale = (double)(1 << RTE_RED_SCALING);
	table_size = (double)(DIM(rte_red_pow2_frac_inv));

	for (i = 0; i < DIM(rte_red_pow2_frac_inv); i++) {
		double m = (double)i;
		
		rte_red_pow2_frac_inv[i] = (uint16_t) round(scale / pow(2, m / table_size));
	}
	
	scale = 1024.0;

	RTE_RED_ASSERT(RTE_RED_WQ_LOG2_NUM == DIM(rte_red_log2_1_minus_Wq));

	for (i = RTE_RED_WQ_LOG2_MIN; i <= RTE_RED_WQ_LOG2_MAX; i++) {
		double n = (double)i;
		double Wq = pow(2, -n);
		uint32_t index = i - RTE_RED_WQ_LOG2_MIN;
		
		rte_red_log2_1_minus_Wq[index] = (uint16_t) round(-1.0 * scale * log2(1.0 - Wq));
		/**
		* Table entry of zero, corresponds to a Wq of zero
		* which is not valid (avg would remain constant no
		* matter how long the queue is empty). So we have
		* to check for zero and round up to one.
		*/
		if (rte_red_log2_1_minus_Wq[index] == 0) {
			rte_red_log2_1_minus_Wq[index] = 1;
		}
	}
}

int
rte_red_rt_data_init(struct rte_red *red)
{
	if (red == NULL)
		return -1;

	red->avg = 0;
	red->count = 0;
	red->q_time = 0;
	return 0;
}

int
rte_red_config_init(struct rte_red_config *red_cfg,
	const uint16_t wq_log2,
	const uint16_t min_th,
	const uint16_t max_th,
	const uint16_t maxp_inv)
{
	if (red_cfg == NULL) {
		return -1;
	}
	if (max_th > RTE_RED_MAX_TH_MAX) {
		return -2;
	}
	if (min_th >= max_th) {
		return -3;
	}
	if (wq_log2 > RTE_RED_WQ_LOG2_MAX) {
		return -4;
	}
	if (wq_log2 < RTE_RED_WQ_LOG2_MIN) {
		return -5;
	}
	if (maxp_inv < RTE_RED_MAXP_INV_MIN) {
		return -6;
	}
	if (maxp_inv > RTE_RED_MAXP_INV_MAX) {
		return -7;
	}
	
	/**
	 *  Initialize the RED module if not already done
	 */
	if (!rte_red_init_done) {
		rte_red_rand_seed = rte_rand();
		rte_red_rand_val = rte_fast_rand();
		__rte_red_init_tables();
		rte_red_init_done = 1;
	}

	red_cfg->min_th = ((uint32_t) min_th) << (wq_log2 + RTE_RED_SCALING);
	red_cfg->max_th = ((uint32_t) max_th) << (wq_log2 + RTE_RED_SCALING);
	red_cfg->pa_const = (2 * (max_th - min_th) * maxp_inv) << RTE_RED_SCALING;
	red_cfg->maxp_inv = maxp_inv;
	red_cfg->wq_log2 = wq_log2;

	return 0;
}
