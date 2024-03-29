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

#ifndef _RTE_CPUFLAGS_H_
#define _RTE_CPUFLAGS_H_

/**
 * @file
 * Simple API to determine available CPU features at runtime.
 */

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Enumeration of all CPU features supported
 */
enum rte_cpu_flag_t {
	/* (EAX 01h) ECX features*/
	RTE_CPUFLAG_SSE3 = 0,               /**< SSE3 */
	RTE_CPUFLAG_PCLMULQDQ,              /**< PCLMULQDQ */
	RTE_CPUFLAG_DTES64,                 /**< DTES64 */
	RTE_CPUFLAG_MONITOR,                /**< MONITOR */
	RTE_CPUFLAG_DS_CPL,                 /**< DS_CPL */
	RTE_CPUFLAG_VMX,                    /**< VMX */
	RTE_CPUFLAG_SMX,                    /**< SMX */
	RTE_CPUFLAG_EIST,                   /**< EIST */
	RTE_CPUFLAG_TM2,                    /**< TM2 */
	RTE_CPUFLAG_SSSE3,                  /**< SSSE3 */
	RTE_CPUFLAG_CNXT_ID,                /**< CNXT_ID */
	RTE_CPUFLAG_FMA,                    /**< FMA */
	RTE_CPUFLAG_CMPXCHG16B,             /**< CMPXCHG16B */
	RTE_CPUFLAG_XTPR,                   /**< XTPR */
	RTE_CPUFLAG_PDCM,                   /**< PDCM */
	RTE_CPUFLAG_PCID,                   /**< PCID */
	RTE_CPUFLAG_DCA,                    /**< DCA */
	RTE_CPUFLAG_SSE4_1,                 /**< SSE4_1 */
	RTE_CPUFLAG_SSE4_2,                 /**< SSE4_2 */
	RTE_CPUFLAG_X2APIC,                 /**< X2APIC */
	RTE_CPUFLAG_MOVBE,                  /**< MOVBE */
	RTE_CPUFLAG_POPCNT,                 /**< POPCNT */
	RTE_CPUFLAG_TSC_DEADLINE,           /**< TSC_DEADLINE */
	RTE_CPUFLAG_AES,                    /**< AES */
	RTE_CPUFLAG_XSAVE,                  /**< XSAVE */
	RTE_CPUFLAG_OSXSAVE,                /**< OSXSAVE */
	RTE_CPUFLAG_AVX,                    /**< AVX */
	RTE_CPUFLAG_F16C,                   /**< F16C */
	RTE_CPUFLAG_RDRAND,                 /**< RDRAND */

	/* (EAX 01h) EDX features */
	RTE_CPUFLAG_FPU,                    /**< FPU */
	RTE_CPUFLAG_VME,                    /**< VME */
	RTE_CPUFLAG_DE,                     /**< DE */
	RTE_CPUFLAG_PSE,                    /**< PSE */
	RTE_CPUFLAG_TSC,                    /**< TSC */
	RTE_CPUFLAG_MSR,                    /**< MSR */
	RTE_CPUFLAG_PAE,                    /**< PAE */
	RTE_CPUFLAG_MCE,                    /**< MCE */
	RTE_CPUFLAG_CX8,                    /**< CX8 */
	RTE_CPUFLAG_APIC,                   /**< APIC */
	RTE_CPUFLAG_SEP,                    /**< SEP */
	RTE_CPUFLAG_MTRR,                   /**< MTRR */
	RTE_CPUFLAG_PGE,                    /**< PGE */
	RTE_CPUFLAG_MCA,                    /**< MCA */
	RTE_CPUFLAG_CMOV,                   /**< CMOV */
	RTE_CPUFLAG_PAT,                    /**< PAT */
	RTE_CPUFLAG_PSE36,                  /**< PSE36 */
	RTE_CPUFLAG_PSN,                    /**< PSN */
	RTE_CPUFLAG_CLFSH,                  /**< CLFSH */
	RTE_CPUFLAG_DS,                     /**< DS */
	RTE_CPUFLAG_ACPI,                   /**< ACPI */
	RTE_CPUFLAG_MMX,                    /**< MMX */
	RTE_CPUFLAG_FXSR,                   /**< FXSR */
	RTE_CPUFLAG_SSE,                    /**< SSE */
	RTE_CPUFLAG_SSE2,                   /**< SSE2 */
	RTE_CPUFLAG_SS,                     /**< SS */
	RTE_CPUFLAG_HTT,                    /**< HTT */
	RTE_CPUFLAG_TM,                     /**< TM */
	RTE_CPUFLAG_PBE,                    /**< PBE */

	/* (EAX 06h) EAX features */
	RTE_CPUFLAG_DIGTEMP,                /**< DIGTEMP */
	RTE_CPUFLAG_TRBOBST,                /**< TRBOBST */
	RTE_CPUFLAG_ARAT,                   /**< ARAT */
	RTE_CPUFLAG_PLN,                    /**< PLN */
	RTE_CPUFLAG_ECMD,                   /**< ECMD */
	RTE_CPUFLAG_PTM,                    /**< PTM */

	/* (EAX 06h) ECX features */
	RTE_CPUFLAG_MPERF_APERF_MSR,        /**< MPERF_APERF_MSR */
	RTE_CPUFLAG_ACNT2,                  /**< ACNT2 */
	RTE_CPUFLAG_ENERGY_EFF,             /**< ENERGY_EFF */

	/* (EAX 07h, ECX 0h) EBX features */
	RTE_CPUFLAG_FSGSBASE,               /**< FSGSBASE */
	RTE_CPUFLAG_BMI1,                   /**< BMI1 */
	RTE_CPUFLAG_AVX2,                   /**< AVX2 */
	RTE_CPUFLAG_SMEP,                   /**< SMEP */
	RTE_CPUFLAG_BMI2,                   /**< BMI2 */
	RTE_CPUFLAG_ERMS,                   /**< ERMS */
	RTE_CPUFLAG_INVPCID,                /**< INVPCID */

	/* (EAX 80000001h) ECX features */
	RTE_CPUFLAG_LAHF_SAHF,              /**< LAHF_SAHF */
	RTE_CPUFLAG_LZCNT,                  /**< LZCNT */

	/* (EAX 80000001h) EDX features */
	RTE_CPUFLAG_SYSCALL,                /**< SYSCALL */
	RTE_CPUFLAG_XD,                     /**< XD */
	RTE_CPUFLAG_1GB_PG,                 /**< 1GB_PG */
	RTE_CPUFLAG_RDTSCP,                 /**< RDTSCP */
	RTE_CPUFLAG_EM64T,                  /**< EM64T */

	/* (EAX 80000007h) EDX features */
	RTE_CPUFLAG_INVTSC,                 /**< INVTSC */

	/* The last item */
	RTE_CPUFLAG_NUMFLAGS,               /**< This should always be the last! */
};


/**
 * Function for checking a CPU flag availability
 *
 * @param flag
 *     CPU flag to query CPU for
 * @return
 *     1 if flag is available
 *     0 if flag is not available
 *     -ENOENT if flag is invalid
 */
int
rte_cpu_get_flag_enabled(enum rte_cpu_flag_t flag);

/**
 * This function checks that the currently used CPU supports the CPU features
 * that were specified at compile time. It is called automatically within the
 * EAL, so does not need to be used by applications.
 */
void
rte_cpu_check_supported(void);

#ifdef __cplusplus
}
#endif


#endif /* _RTE_CPUFLAGS_H_ */
