/*
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Folder: ./components/esp_iot_framework_core/src
 * File: tls_manager.c
 * Library: esp_iot_framework_core
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

#include "esp_iot_framework_core_macros.h"
#include "core_internal.h"
#include "esp_iot_framework_core.h"

/* --- */

#define TLS_ERR_GEN_KEY    "Failed to generate ECC keypair"
#define TLS_ERR_GEN_CERT   "Failed to generate self-signed certificate"
#define TLS_ERR_TLS        "Failed to generate TLS credentials"
#define TLS_ERR_SAVE       "Saving '%s' failed"
#define TLS_ERR_ALLOCATE   "Failed to allocate %d bytes for '%s'"

#define TLS_MSG_KEY_GEN_OK  "ECC key pair generated. Size: %u bytes"
#define TLS_MSG_CERT_GEN_OK "Certificate generated successfully for: %s"
#define TLS_MSG_CREATE_OK   "Creation of `tls credentials` completed successfully"

#define KEY_PEM_LEN   384
#define CERT_BUF_SIZE 1024

/* --- */

static esp_err_t eif_generate_ecc_keypair(
    uint8_t * const key_pem, size_t * const key_len
) {
    EIF_TAG_WITH_UNUSED "Key-Gen";

    int ret = 0;
    mbedtls_pk_context key = {0};
    mbedtls_entropy_context entropy = {0};
    mbedtls_ctr_drbg_context ctr_drbg = {0};
    uint8_t seed[32] = {0};

    mbedtls_pk_init(&key);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    esp_fill_random(seed, sizeof(seed));

    EIF_IF_OK_CHECK_MBEDTLS_ERR(ret, mbedtls_ctr_drbg_seed(
        &ctr_drbg, mbedtls_entropy_func, &entropy, seed, sizeof(seed)
    ), "drbg_seed");

    EIF_IF_OK_CHECK_MBEDTLS_ERR(ret, mbedtls_pk_setup(
        &key, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY)
    ), "pk_setup");

    EIF_IF_OK_CHECK_MBEDTLS_ERR(ret, mbedtls_ecp_gen_key(
        MBEDTLS_ECP_DP_SECP256R1, mbedtls_pk_ec(key),
        mbedtls_ctr_drbg_random, &ctr_drbg
    ), "ecp_gen");

    EIF_IF_OK_CHECK_MBEDTLS_ERR(ret, mbedtls_pk_write_key_pem(
        &key, key_pem, KEY_PEM_LEN), "write_priv_key");
    if (ret == 0) {
        *key_len = eif_strnlen((const char *)key_pem, KEY_PEM_LEN) + 1U;
        EIF_LOG_I(TLS_MSG_KEY_GEN_OK, *key_len);
    }


    /* Cleanup */
    mbedtls_pk_free(&key);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    return (ret == 0) ? ESP_OK : ESP_FAIL;
}

static esp_err_t eif_generate_self_signed_cert(
    uint8_t *key_pem, size_t key_len,
    uint8_t *cert_out, size_t *cert_len_out,
    const char *dns_name
) {
    EIF_TAG_WITH_UNUSED "Cert-Gen";

    #define F_ASN1_TAG_SEQUENCE   0x30U
    #define F_ASN1_TAG_DNS_NAME   0x82U
    #define F_SAN_HEADER_SIZE     4U
    #define F_RANDOM_SEED_SIZE    32U
    #define F_SERIAL_NUM_SIZE     32U
    #define F_SUBJECT_BUF_SIZE    128U
    #define F_SAN_EXTRA_SPACE     32U
    #define F_SAN_TYPE_DNS_OFFSET 2U
    #define F_SAN_DATA_OFFSET     4U

    #define F_TIME_START "19700101000000"
    #define F_TIME_END   "21700101000000"

    static const unsigned char eku_server_auth[] = {
        0x30, 0x14, // Sequence
        0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x01, // Server Auth
        0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x02  // Client Auth
    };

    int ret = 0;
    size_t name_len = 0;
    mbedtls_mpi serial = {0};
    mbedtls_pk_context key = {0};
    mbedtls_x509write_cert crt = {0};
    mbedtls_entropy_context entropy = {0};
    char subject[F_SUBJECT_BUF_SIZE] = {0};
    uint8_t seed[F_RANDOM_SEED_SIZE] = {0};
    mbedtls_ctr_drbg_context ctr_drbg = {0};
    uint8_t serial_buf[F_SERIAL_NUM_SIZE] = {0};
    char temp_name[MDNS_HOSTNAME_FULL_MAX_LEN] = {0};
    unsigned char san_der[MDNS_HOSTNAME_FULL_MAX_LEN + F_SAN_EXTRA_SPACE] = {0};

    mbedtls_pk_init(&key);
    mbedtls_x509write_crt_init(&crt);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_mpi_init(&serial);
    esp_fill_random(seed, sizeof(seed));
    esp_fill_random(serial_buf, sizeof(serial_buf));

    /* 1. Load Key */
    #if defined(MBEDTLS_VERSION_NUMBER) && (MBEDTLS_VERSION_NUMBER >= 0x03000000)
        EIF_IF_OK_CHECK_MBEDTLS_ERR(ret, mbedtls_pk_parse_key(
            &key, key_pem, key_len, NULL, 0, NULL, NULL), "parse_key");
    #else
        EIF_IF_OK_CHECK_MBEDTLS_ERR(ret, mbedtls_pk_parse_key(
            &key, key_pem, key_len, NULL, 0), "parse_key");
    #endif

    /* 2. Seed DRBG */
    EIF_IF_OK_CHECK_MBEDTLS_ERR(ret,
        mbedtls_ctr_drbg_seed(
            &ctr_drbg, mbedtls_entropy_func,
            &entropy, seed, sizeof(seed)
        ), "drbg_seed");

    /* 3. Setup Cert Info */
    if (ret == 0) {
        mbedtls_x509write_crt_set_subject_key(&crt, &key);
        mbedtls_x509write_crt_set_issuer_key(&crt, &key);

        size_t dns_len = eif_strnlen(dns_name, sizeof(subject));
        size_t total_len = dns_len + 4U;

        if (total_len > sizeof(subject)) {
            ret = 1;
        } else {
            (void)memcpy(subject, "CN=", 3U);
            (void)memcpy(&subject[3U], dns_name, dns_len);
            subject[total_len - 1U] = '\0';
        }
    }

    if (ret == 0) {
        mbedtls_x509write_crt_set_subject_name(&crt, subject);
        mbedtls_x509write_crt_set_issuer_name(&crt, subject);

        mbedtls_x509write_crt_set_validity(&crt, F_TIME_START, F_TIME_END);
        mbedtls_x509write_crt_set_version(&crt, MBEDTLS_X509_CRT_VERSION_3);
        mbedtls_x509write_crt_set_md_alg(&crt, MBEDTLS_MD_SHA256);

        mbedtls_x509write_crt_set_basic_constraints(&crt, 0, -1);
        mbedtls_x509write_crt_set_key_usage(&crt,
            MBEDTLS_X509_KU_DIGITAL_SIGNATURE | MBEDTLS_X509_KU_KEY_ENCIPHERMENT);

    /* 4. Random Serial */
        mbedtls_mpi_read_binary(&serial, serial_buf, sizeof(serial_buf));
        #if defined(MBEDTLS_VERSION_NUMBER) && (MBEDTLS_VERSION_NUMBER >= 0x03000000)
            EIF_IF_OK_CHECK_MBEDTLS_ERR(ret, mbedtls_x509write_crt_set_serial_raw(
                &crt, serial_buf, sizeof(serial_buf)), "set_serial");
        #else
            mbedtls_mpi_read_binary(&serial, serial_buf, sizeof(serial_buf));
            EIF_IF_OK_CHECK_MBEDTLS_ERR(ret, mbedtls_x509write_crt_set_serial(
                &crt, &serial), "set_serial");
        #endif
    }

    /* 5. Set SAN */
    if (ret == 0) {
        size_t dns_len = eif_strnlen(dns_name, sizeof(temp_name));
        size_t total_len = dns_len + 7U;

        if (total_len > sizeof(temp_name)) {
            ret = 1;
        } else {
            (void)memcpy(temp_name, dns_name, dns_len);
            (void)memcpy(&temp_name[dns_len], ".local", 7U);
        }
    }
    if (ret == 0) {
        name_len = eif_strnlen(temp_name, MDNS_HOSTNAME_FULL_MAX_LEN);
        san_der[0] = F_ASN1_TAG_SEQUENCE;
        san_der[1] = (unsigned char)(name_len + F_SAN_TYPE_DNS_OFFSET);
        san_der[2] = F_ASN1_TAG_DNS_NAME;
        san_der[3] = (unsigned char)name_len;
        (void)memcpy(&san_der[F_SAN_HEADER_SIZE], temp_name, name_len);
    }

    EIF_IF_OK_CHECK_MBEDTLS_ERR(ret,
        mbedtls_x509write_crt_set_extension(
            &crt, MBEDTLS_OID_SUBJECT_ALT_NAME,
            MBEDTLS_OID_SIZE(MBEDTLS_OID_SUBJECT_ALT_NAME),
            0, san_der, name_len + F_SAN_DATA_OFFSET
        ), "set_san");

    /* 6. Set Extended Key Usage (Server Auth) */
    EIF_IF_OK_CHECK_MBEDTLS_ERR(ret,
        mbedtls_x509write_crt_set_extension(
            &crt, MBEDTLS_OID_EXTENDED_KEY_USAGE,
            MBEDTLS_OID_SIZE(MBEDTLS_OID_EXTENDED_KEY_USAGE),
            0, eku_server_auth, sizeof(eku_server_auth)
        ), "set_extension_key");

    if (ret == 0) {
        #if defined(MBEDTLS_VERSION_NUMBER) && (MBEDTLS_VERSION_NUMBER < 0x03000000)
            mbedtls_x509write_crt_set_subject_key_identifier(&crt);
            mbedtls_x509write_crt_set_authority_key_identifier(&crt);
        #else
            #if defined(MBEDTLS_SHA1_C)
                mbedtls_x509write_crt_set_subject_key_identifier(&crt);
                mbedtls_x509write_crt_set_authority_key_identifier(&crt);
            #endif
        #endif
    }

    /* 7. Write PEM */
    EIF_IF_OK_CHECK_MBEDTLS_ERR(ret,
        mbedtls_x509write_crt_pem(
            &crt, cert_out, CERT_BUF_SIZE,
            mbedtls_ctr_drbg_random, &ctr_drbg
        ), "write_pem");

    if (ret == 0) {
        *cert_len_out = eif_strnlen((char *)cert_out, CERT_BUF_SIZE) + 1U;
        EIF_LOG_I(TLS_MSG_CERT_GEN_OK, dns_name);
    }

    /* Cleanup */
    (void)memset(seed, 0, sizeof(seed));
    (void)memset(serial_buf, 0, sizeof(serial_buf));

    mbedtls_pk_free(&key);
    mbedtls_x509write_crt_free(&crt);
    mbedtls_mpi_free(&serial);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    return (ret == 0) ? ESP_OK : ESP_FAIL;
}

static inline void if_ok_erase_it(
    esp_err_t ret, char * const buf, size_t size, const char * const name
) {
    EIF_TAG_WITH_UNUSED "TLS manager";
    
    if ((ret == ESP_OK) && (buf != NULL)) {
        (void)memset((void *)buf, 0, size);
    } else if (ret == ESP_OK) {
        EIF_LOG_E(TLS_ERR_ALLOCATE, (int)CERT_BUF_SIZE, name);
    } else { ; }
}

static esp_err_t eif_tls_create_creds(
    char * * const  cert_pem, size_t * const cert_len,
    char * * const key_pem,   size_t * const key_len
) {
    EIF_TAG_WITH_UNUSED "TLS manager";

    esp_err_t ret = ESP_OK;
    const eif_core_t * const cfg = eif_core_get();

    *cert_pem = (char *)pvPortMalloc((size_t)CERT_BUF_SIZE);
    EIF_IF_OK_CHECK_NOT_NULL(ret, *key_pem, ESP_ERR_NO_MEM);
    if_ok_erase_it(ret, *cert_pem, (size_t)CERT_BUF_SIZE, "cert_pem_buf");
    
    if (ret == ESP_OK) {
        *key_pem = (char *)pvPortMalloc((size_t)KEY_PEM_LEN);
        EIF_IF_OK_CHECK_NOT_NULL(ret, *key_pem, ESP_ERR_NO_MEM);
        if_ok_erase_it(ret, *key_pem, (size_t)KEY_PEM_LEN, "key_pem_buf");
    } 

    EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_generate_ecc_keypair(
        (uint8_t *)*key_pem, key_len), TLS_ERR_GEN_KEY);
    EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_generate_self_signed_cert(
        (uint8_t *)*key_pem, *key_len,
        (uint8_t *)*cert_pem, cert_len,
        cfg->mdns_hostname), TLS_ERR_GEN_CERT);

    /* Cleanup */
    if (ret != ESP_OK) {
        if (*cert_pem != NULL) {
            if_ok_erase_it(ESP_OK, *cert_pem, (size_t)CERT_BUF_SIZE, "cert_pem_buf");
            vPortFree(*cert_pem);
            *cert_pem = NULL;
        }
        if (*key_pem != NULL) {
            if_ok_erase_it(ESP_OK, *key_pem,  (size_t)KEY_PEM_LEN, "key_pem_buf");
            vPortFree(*key_pem);
            *key_pem = NULL;
        }
    }
    return ret;
}

esp_err_t eif_tls_create_creds_and_nvs_save(void) {
    EIF_TAG_WITH_UNUSED "TLS manager";

    esp_err_t ret = ESP_OK;
    char *cert_out = NULL;
    char *key_out = NULL;
    size_t cert_len = 0;
    size_t key_len = 0;

    EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_tls_create_creds(
        &cert_out, &cert_len, &key_out, &key_len), TLS_ERR_TLS);
    EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_nvs_value_save(
        EIF_NVS_KEY_TLS_CERT, cert_out, 1, CERT_BUF_SIZE, false
    ), TLS_ERR_SAVE, EIF_NVS_KEY_TLS_CERT);
    EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_nvs_value_save(
        EIF_NVS_KEY_TLS_PRIV_KEY, key_out, 1, KEY_PEM_LEN, false
    ), TLS_ERR_SAVE, EIF_NVS_KEY_TLS_PRIV_KEY);

    if (ret == ESP_OK) {
        EIF_LOG_I(TLS_MSG_CREATE_OK);
    }

    /* Cleanup */
    if (cert_out != NULL) {
        if_ok_erase_it(ESP_OK, cert_out, (size_t)CERT_BUF_SIZE, "cert_out_buf");
        vPortFree(cert_out);
    }
    if (key_out != NULL) {
        if_ok_erase_it(ESP_OK, key_out, (size_t)KEY_PEM_LEN, "key_out_buf");
        vPortFree(key_out);
    }
    return ret;
}