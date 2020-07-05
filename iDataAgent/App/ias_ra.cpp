/*
 * Copyright (C) 2011-2017 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */



#include "attestation_service.h"
#include "sample_libcrypto.h"
#include "ecp.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include <string.h>
#include "ias_ra.h"

//This whole file is used as simulation of the interfaces to be
// delivered an attestation server. 


#define UNUSED(expr) do { (void)(expr); } while (0)

#if !defined(SWAP_ENDIAN_DW)
    #define SWAP_ENDIAN_DW(dw)	((((dw) & 0x000000ff) << 24)                \
    | (((dw) & 0x0000ff00) << 8)                                            \
    | (((dw) & 0x00ff0000) >> 8)                                            \
    | (((dw) & 0xff000000) >> 24))
#endif
#if !defined(SWAP_ENDIAN_32B)
    #define SWAP_ENDIAN_32B(ptr)                                            \
{\
    unsigned int temp = 0;                                                  \
    temp = SWAP_ENDIAN_DW(((unsigned int*)(ptr))[0]);                       \
    ((unsigned int*)(ptr))[0] = SWAP_ENDIAN_DW(((unsigned int*)(ptr))[7]);  \
    ((unsigned int*)(ptr))[7] = temp;                                       \
    temp = SWAP_ENDIAN_DW(((unsigned int*)(ptr))[1]);                       \
    ((unsigned int*)(ptr))[1] = SWAP_ENDIAN_DW(((unsigned int*)(ptr))[6]);  \
    ((unsigned int*)(ptr))[6] = temp;                                       \
    temp = SWAP_ENDIAN_DW(((unsigned int*)(ptr))[2]);                       \
    ((unsigned int*)(ptr))[2] = SWAP_ENDIAN_DW(((unsigned int*)(ptr))[5]);  \
    ((unsigned int*)(ptr))[5] = temp;                                       \
    temp = SWAP_ENDIAN_DW(((unsigned int*)(ptr))[3]);                       \
    ((unsigned int*)(ptr))[3] = SWAP_ENDIAN_DW(((unsigned int*)(ptr))[4]);  \
    ((unsigned int*)(ptr))[4] = temp;                                       \
}
#endif

// This is the ECDSA NIST P-256 private key used to sign platform_info_blob.
// This private
// key and the public key in SDK untrusted KElibrary should be a temporary key
// pair. For production parts an attestation server will sign the platform_info_blob with the
// production private key and the SDK untrusted KE library will have the public
// key for verifcation.

static const sample_ec256_private_t g_rk_priv_key =
{{
    0x63,0x2c,0xd4,0x02,0x7a,0xdc,0x56,0xa5,
    0x59,0x6c,0x44,0x3e,0x43,0xca,0x4e,0x0b,
    0x58,0xcd,0x78,0xcb,0x3c,0x7e,0xd5,0xb9,
    0xf2,0x91,0x5b,0x39,0x0d,0xb3,0xb5,0xfb
}};

static sample_spid_t g_sim_spid = {"Service X"};


// Simulates the attestation server function for verifying the quote produce by
// the ISV enclave. It doesn't decrypt or verify the quote in
// the simulation.  Just produces the attestaion verification
// report with the platform info blob.
//
// @param p_isv_quote Pointer to the quote generated by the ISV
//                    enclave.
// @param pse_manifest Pointer to the PSE manifest if used.
// @param p_attestation_verification_report Pointer the outputed
//                                          verification report.
//
// @return int

int ias_verify_attestation_evidence(
    sample_quote_t *p_isv_quote,
    uint8_t* pse_manifest,
    ias_att_report_t* p_attestation_verification_report)
{
    int ret = 0;
    sample_ecc_state_handle_t ecc_state = NULL;

    //unused parameters
    UNUSED(pse_manifest);

    if((NULL == p_isv_quote) ||
        (NULL == p_attestation_verification_report))
    {
        return -1;
    }
    //Decrypt the Quote signature and verify.

    p_attestation_verification_report->id = 0x12345678;
    p_attestation_verification_report->status = IAS_QUOTE_OK;
    p_attestation_verification_report->revocation_reason =
        IAS_REVOC_REASON_NONE;
    p_attestation_verification_report->info_blob.sample_epid_group_status =
        0 << IAS_EPID_GROUP_STATUS_REVOKED_BIT_POS
        | 0 << IAS_EPID_GROUP_STATUS_REKEY_AVAILABLE_BIT_POS;
    p_attestation_verification_report->info_blob.sample_tcb_evaluation_status =
        0 << IAS_TCB_EVAL_STATUS_CPUSVN_OUT_OF_DATE_BIT_POS
        | 0 << IAS_TCB_EVAL_STATUS_ISVSVN_OUT_OF_DATE_BIT_POS;
    p_attestation_verification_report->info_blob.pse_evaluation_status =
        0 << IAS_PSE_EVAL_STATUS_ISVSVN_OUT_OF_DATE_BIT_POS
        | 0 << IAS_PSE_EVAL_STATUS_EPID_GROUP_REVOKED_BIT_POS
        | 0 << IAS_PSE_EVAL_STATUS_PSDASVN_OUT_OF_DATE_BIT_POS
        | 0 << IAS_PSE_EVAL_STATUS_SIGRL_OUT_OF_DATE_BIT_POS
        | 0 << IAS_PSE_EVAL_STATUS_PRIVRL_OUT_OF_DATE_BIT_POS;
    memset(p_attestation_verification_report->
                info_blob.latest_equivalent_tcb_psvn, 0, PSVN_SIZE);
    memset(p_attestation_verification_report->info_blob.latest_pse_isvsvn,
           0, ISVSVN_SIZE);
    memset(p_attestation_verification_report->info_blob.latest_psda_svn,
           0, PSDA_SVN_SIZE);
    memset(p_attestation_verification_report->info_blob.performance_rekey_gid,
           0, GID_SIZE);

    // @TODO: Product signing algorithm still TBD.  May be RSA2048 signing.
    // Generate the Service providers ECCDH key pair.
    do {
        ret = sample_ecc256_open_context(&ecc_state);
        if (SAMPLE_SUCCESS != ret) {
            fprintf(stderr, "\nError, cannot get ECC cotext in [%s].",
                    __FUNCTION__);
            ret = -1;
            break;
        }
        // Sign
        ret = sample_ecdsa_sign(
                (uint8_t *)&p_attestation_verification_report->
                    info_blob.sample_epid_group_status,
                sizeof(ias_platform_info_blob_t) - sizeof(sample_ec_sign256_t),
                (sample_ec256_private_t *)&g_rk_priv_key,
                (sample_ec256_signature_t *)&p_attestation_verification_report->
                    info_blob.signature,
                ecc_state);
        if (SAMPLE_SUCCESS != ret) {
            fprintf(stderr, "\nError, sign ga_gb fail in [%s].", __FUNCTION__);
            ret = SP_INTERNAL_ERROR;
            break;
        }
        SWAP_ENDIAN_32B(p_attestation_verification_report->
                            info_blob.signature.x);
        SWAP_ENDIAN_32B(p_attestation_verification_report->
                            info_blob.signature.y);

    }while (0);
    if (ecc_state) {
        sample_ecc256_close_context(ecc_state);
    }
    p_attestation_verification_report->pse_status = IAS_PSE_OK;

    // For now, don't simulate the policy reports.
    p_attestation_verification_report->policy_report_size = 0;
    return(ret);
}


// Simulates retrieving the SIGRL for upon the SP request. 
//
// @param gid Group ID for the EPID key.
// @param p_sig_rl_size Pointer to the output value of the full
//                      SIGRL size in bytes. (including the
//                      signature).
// @param p_sig_rl Pointer to the output of the SIGRL.
//
// @return int

int ias_get_sigrl(
    const sample_epid_group_id_t gid,
    uint32_t *p_sig_rl_size,
    uint8_t **p_sig_rl)
{
    int ret = 0;

    UNUSED(gid);

    do {

        if (NULL == p_sig_rl || NULL == p_sig_rl_size) {
            ret = -1;
            break;
        }
        *p_sig_rl_size = 0;
        *p_sig_rl = NULL;
        // we should try to get sig_rl from an attestation server
        break;
    }while (0);

    return(ret);
}


// Used to simulate the enrollment function of an attestation server.  It only
// gives back the SPID right now. In production, the enrollment
// occurs out of context from an attestation attempt and only
// occurs once.
//
//
// @param sp_credentials
// @param p_spid
// @param p_authentication_token
//
// @return int

int ias_enroll(
    int sp_credentials,
    sample_spid_t *p_spid,
    int *p_authentication_token)
{
    UNUSED(sp_credentials);
    UNUSED(p_authentication_token);

    if (NULL != p_spid) {
        memcpy_s(p_spid, sizeof(sample_spid_t), &g_sim_spid,
                 sizeof(sample_spid_t));
    } else {
        return(1);
    }
    return(0);
}


