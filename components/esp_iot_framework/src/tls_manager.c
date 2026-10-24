/* SPDX-FileCopyrightText: 2026 AmakeSasha
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Path: ./components/esp_iot_framework/src/tls_manager.c
 * 
 * Copyright 2026 AmakeSasha
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "sdkconfig.h"

#ifdef CONFIG_EIF_ENABLE_TLS

#include <stdio.h>
#include <string.h>
#include <esp_log.h>
#include <esp_random.h>
#include <mbedtls/pk.h>
#include <mbedtls/oid.h>
#include <mbedtls/error.h>
#include <mbedtls/version.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/x509_crt.h>

#include "private_include/macros.h"
#include "private_include/internal.h"

#define KEY_PEM_LEN   384
#define CERT_BUF_SIZE 1024

/* --- */

#define TAG "EIF TLS"

static void log_mbedtls_error(const char * const ctx_msg, int error_code) {
    if (error_code != 0) {
        char err_buf[100] = {0};
        mbedtls_strerror(error_code, err_buf, sizeof(err_buf));
        EIF_LOG_E("%s failed: %s (-0x%04X)", ctx_msg, err_buf, -error_code);
    }
}

static int generate_ecc_keypair(uint8_t * const key_pem, size_t * const key_len) {
    int ret = 0;
    uint8_t rand_pool[32] = {0};
    mbedtls_pk_context key = {0};
    mbedtls_entropy_context entropy = {0};
    mbedtls_ctr_drbg_context ctr_drbg = {0};

    mbedtls_pk_init(&key);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    esp_fill_random(rand_pool, sizeof(rand_pool));

    EIF_TRY_INT(mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func,
                                      &entropy, rand_pool, sizeof(rand_pool)));
    EIF_TRY_INT(mbedtls_pk_setup(&key, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY)));
    EIF_TRY_INT(mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, mbedtls_pk_ec(key),
                                    mbedtls_ctr_drbg_random, &ctr_drbg));
    EIF_TRY_INT(mbedtls_pk_write_key_pem(&key, key_pem, KEY_PEM_LEN));

    *key_len = strnlen((const char *)key_pem, KEY_PEM_LEN) + 1U;

    EIF_LOG_I("ECC keypair generated. PEM length: %u bytes", (unsigned int)*key_len);

cleanup:
    mbedtls_pk_free(&key);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);

    if (ret != 0) {
        log_mbedtls_error("ECC keypair generation", ret);
    }
    return ret;
}

static int generate_self_signed_cert(
    uint8_t *key_pem, size_t key_len,
    uint8_t *cert_out, size_t *cert_len_out
) {
    static const unsigned char eku_server_auth[] = {
        0x30, 0x0a, /* Sequence */
        0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x01  /* ServerAuth */
    };

    const eif_config_t * const cfg = eif_config_get();

    int ret = 0;
    mbedtls_mpi serial = {0};
    uint8_t rand_pool[64] = {0}; /* 32B seed + 32B serial */
    mbedtls_pk_context key = {0};
    mbedtls_x509write_cert crt = {0};
    mbedtls_entropy_context entropy = {0};
    mbedtls_ctr_drbg_context ctr_drbg = {0};
    /* @note Array size includes the maximum hostname length + 3B for the "CN=" prefix */
    char subject[MDNS_HOSTNAME_FULL_MAX_LEN + 3U] = {0};
    /* @note +4U for ASN.1 sequence and DNS name header fields */
    unsigned char san_der[MDNS_HOSTNAME_FULL_MAX_LEN + 4U] = {0};

    mbedtls_pk_init(&key);
    mbedtls_x509write_crt_init(&crt);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_mpi_init(&serial);
    esp_fill_random(rand_pool, sizeof(rand_pool));

    /* 1. Load Key */
    #if defined(MBEDTLS_VERSION_NUMBER) && (MBEDTLS_VERSION_NUMBER >= 0x03000000)
        EIF_TRY_INT(mbedtls_pk_parse_key(&key, key_pem, key_len, NULL, 0, NULL, NULL));
    #else
        EIF_TRY_INT(mbedtls_pk_parse_key(&key, key_pem, key_len, NULL, 0));
    #endif

    /* 2. Seed DRBG */
    EIF_TRY_INT(mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func,
                                      &entropy, rand_pool, 32U)); /* 32B seed */

    /* 3. Setup Cert Info */
    mbedtls_x509write_crt_set_subject_key(&crt, &key);
    mbedtls_x509write_crt_set_issuer_key(&crt, &key);

    (void)snprintf(subject, sizeof(subject), "CN=%s", cfg->mdns_hostname);
    mbedtls_x509write_crt_set_subject_name(&crt, subject);
    mbedtls_x509write_crt_set_issuer_name(&crt, subject);

    mbedtls_x509write_crt_set_validity(&crt, "19700101000000", "21700101000000");
    mbedtls_x509write_crt_set_version(&crt, MBEDTLS_X509_CRT_VERSION_3);
    mbedtls_x509write_crt_set_md_alg(&crt, MBEDTLS_MD_SHA256);
    /* Set as end-entity certificate (not a CA, path length is ignored) */
    mbedtls_x509write_crt_set_basic_constraints(&crt, 0, -1);
    mbedtls_x509write_crt_set_key_usage(&crt, MBEDTLS_X509_KU_DIGITAL_SIGNATURE
                                        | MBEDTLS_X509_KU_KEY_ENCIPHERMENT);

    /* 4. Random Serial */
    #if defined(MBEDTLS_VERSION_NUMBER) && (MBEDTLS_VERSION_NUMBER >= 0x03000000)
        EIF_TRY_INT(mbedtls_x509write_crt_set_serial_raw(&crt, &rand_pool[32], 32U));
    #else
        EIF_TRY_INT(mbedtls_mpi_read_binary(&serial, &rand_pool[32], 32U)); /* 32B serial */
        EIF_TRY_INT(mbedtls_x509write_crt_set_serial(&crt, &serial));
    #endif

    /* 5. Set SAN */
    int dns_str_len = snprintf((char *)&san_der[4], sizeof(san_der) - 4U, "%s.local",
                               cfg->mdns_hostname);
    /* @note Constructing a manual ASN.1 DER structure for Subject Alternative Name (SAN).
     *       Layout: 
     *       [0] Sequence tag
     *       [1] Sequence length -> dns_str_len + 1B (DNS Name tag) + 1B (String length)
     *       [2] DNS Name tag
     *       [3] String length */
    san_der[0] = 0x30U;
    san_der[1] = (unsigned char)(dns_str_len + 2U);
    san_der[2] = 0x82U;
    san_der[3] = (unsigned char)dns_str_len;

    EIF_TRY_INT(mbedtls_x509write_crt_set_extension(&crt, MBEDTLS_OID_SUBJECT_ALT_NAME,
                                                    MBEDTLS_OID_SIZE(MBEDTLS_OID_SUBJECT_ALT_NAME),
                                                    0, san_der, (size_t)dns_str_len + 4));

    /* 6. Set Extended Key Usage (Server Auth) */
    EIF_TRY_INT(mbedtls_x509write_crt_set_extension(&crt, MBEDTLS_OID_EXTENDED_KEY_USAGE,
                                                    MBEDTLS_OID_SIZE(MBEDTLS_OID_EXTENDED_KEY_USAGE),
                                                    0, eku_server_auth, sizeof(eku_server_auth)));

    #if defined(MBEDTLS_VERSION_NUMBER) && (MBEDTLS_VERSION_NUMBER >= 0x03000000)
        #if defined(MBEDTLS_SHA1_C)
            mbedtls_x509write_crt_set_subject_key_identifier(&crt);
            mbedtls_x509write_crt_set_authority_key_identifier(&crt);
        #endif
    #else
        mbedtls_x509write_crt_set_subject_key_identifier(&crt);
        mbedtls_x509write_crt_set_authority_key_identifier(&crt);
    #endif

    /* 7. Write PEM */
    EIF_TRY_INT(mbedtls_x509write_crt_pem(&crt, cert_out, CERT_BUF_SIZE,
                                          mbedtls_ctr_drbg_random, &ctr_drbg));

    *cert_len_out = strnlen((char *)cert_out, CERT_BUF_SIZE) + 1U;
    EIF_LOG_I("Certificate generated for mDNS name '%s'", cfg->mdns_hostname);

cleanup:
    mbedtls_platform_zeroize(rand_pool, sizeof(rand_pool));
    mbedtls_pk_free(&key);
    mbedtls_x509write_crt_free(&crt);
    mbedtls_mpi_free(&serial);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    if (ret != 0) {
        log_mbedtls_error("Self-signed certificate generation", ret);
    }
    
    return ret;
}

/* --- */

esp_err_t tls_create_creds(
    char * * const  cert_pem, size_t * const cert_len,
    char * * const key_pem,   size_t * const key_len
) {
    esp_err_t ret = ESP_OK;

    EIF_CHECK_NOT_NULL(cert_pem, ESP_ERR_INVALID_ARG);
    EIF_CHECK_NOT_NULL(key_pem,  ESP_ERR_INVALID_ARG);

    if (*cert_pem != NULL) {
        vPortFree(*cert_pem);
        *cert_pem = NULL;
    }
    if (*key_pem != NULL) {
        vPortFree(*key_pem);
        *key_pem = NULL;
    }

    *cert_pem = (char *)pvPortMalloc((size_t)CERT_BUF_SIZE);
    EIF_CHECK_NOT_NULL(*cert_pem, ESP_ERR_NO_MEM);
    
    *key_pem = (char *)pvPortMalloc((size_t)KEY_PEM_LEN);
    EIF_CHECK_NOT_NULL(*key_pem, ESP_ERR_NO_MEM);

    EIF_TRY(generate_ecc_keypair((uint8_t *)*key_pem, key_len));
    EIF_TRY(generate_self_signed_cert((uint8_t *)*key_pem, *key_len,
                                      (uint8_t *)*cert_pem, cert_len));

    EIF_LOG_I("TLS credentials generated");

cleanup:
    if (ret != ESP_OK) {
        EIF_LOG_E("'TLS credentials' generate failed , err: %s", esp_err_to_name(ret));
        
        if (*cert_pem != NULL) {
            mbedtls_platform_zeroize(*cert_pem, (size_t)CERT_BUF_SIZE);
            vPortFree(*cert_pem);
            *cert_pem = NULL;
        }
        if (*key_pem != NULL) {
            mbedtls_platform_zeroize(*key_pem, (size_t)KEY_PEM_LEN);
            vPortFree(*key_pem);
            *key_pem = NULL;
        }
    }
    return ret;
}

#endif