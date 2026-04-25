/*
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp_iot_framework
 * Folder: src
 * File: cert_manager.c
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
#include "esp_log.h"
#include "esp_random.h"
#include "mbedtls/pk.h"
#include "mbedtls/oid.h"
#include "mbedtls/error.h"
#include "mbedtls/version.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/x509_crt.h"

#include "macros.h"
#include "core_internal.h"
#include "esp_iot_framework.h"

/* --- */

#define _ERR_MBEDTLS       "MbedTLS %s failed: %s (-0x%04x)"
#define _ERR_GEN_KEY       "Failed to generate ECC keypair"
#define _ERR_GEN_CERT      "Failed to generate self-signed certificate"
#define _ERR_SAN_SET       "Failed to set Subject Alternative Name (DNS: %s)"
#define _ERR_SPAWN_TASK    "Failed to spawn [%s]. Free heap: %zu bytes"

#define _MSG_KEY_GEN_OK    "ECC key pair generated. Size: %zu bytes"
#define _MSG_CERT_GEN_OK   "Certificate generated successfully for: %s"
#define _MSG_RECREATE_DONE "TLS credentials recreated. System will restart..."
#define _MSG_RESTARTING    "Restarting..."

#define KEY_PEM_LEN   384
#define PUB_DER_LEN   256
#define CERT_BUF_SIZE 1024

/* --- */

#define TAG "tls_recreate_task"

void tls_recreate_task(void* arg) {
    vTaskDelay(pdMS_TO_TICKS(500)); 
    
    CHECK_ESP_ERR_T(E, nvs_tls_creds_create_and_save(), 
        {}, goto cleanup, "");
    CORE_LOG(I, _MSG_RECREATE_DONE);
    vTaskDelay(pdMS_TO_TICKS(500));
    CORE_LOG(I, _MSG_RESTARTING);
    
    int result = xTaskCreate(reboot_task, "reboot_task",
        CONFIG_EIF_REBOOT_TASK_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
    if (result != pdPASS) {
        CORE_LOG(E, _ERR_SPAWN_TASK, "reboot_task", (size_t)esp_get_free_heap_size());
    }
cleanup:
    vTaskDelete(NULL);
}

#undef TAG

/* --- */

#define TAG "Key-Gen"

static esp_err_t generate_ecc_keypair(
    uint8_t *key_pem, size_t *key_len
) {
    int ret = -1;
    mbedtls_pk_context key;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;

    mbedtls_pk_init(&key);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    uint8_t seed[32];
    esp_fill_random(seed, sizeof(seed));

    CHECK_MBEDTLS_ERR(
        mbedtls_ctr_drbg_seed(
            &ctr_drbg, mbedtls_entropy_func,
            &entropy, seed, sizeof(seed)
        ), "drbg_seed");

    CHECK_MBEDTLS_ERR(
        mbedtls_pk_setup(
            &key, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY)
        ), "pk_setup");

    CHECK_MBEDTLS_ERR(
        mbedtls_ecp_gen_key(
            MBEDTLS_ECP_DP_SECP256R1, mbedtls_pk_ec(key), 
            mbedtls_ctr_drbg_random, &ctr_drbg
        ), "ecp_gen");

    CHECK_MBEDTLS_ERR(
        mbedtls_pk_write_key_pem(
            &key, key_pem, KEY_PEM_LEN
        ), "write_priv_key");
    *key_len = strlen((char *)key_pem) + 1;

    CORE_LOG(I, _MSG_KEY_GEN_OK, *key_len);
    ret = 0;

cleanup:
    mbedtls_pk_free(&key);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    return (ret == 0) ? ESP_OK : ESP_FAIL;
}

#undef TAG
#define TAG "Cert-Gen"

static const unsigned char eku_server_auth[] = { 
    0x30, 0x0a, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x01 };

static esp_err_t generate_self_signed_cert(
    uint8_t *key_pem, size_t key_len, 
    uint8_t *cert_out, size_t *cert_len_out, 
    const char *dns_name
) {
    int ret = -1;
    mbedtls_pk_context key;
    mbedtls_x509write_cert crt;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_mpi serial;

    mbedtls_pk_init(&key);
    mbedtls_x509write_crt_init(&crt);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_mpi_init(&serial);

    /* 1. Load Key */
    CHECK_MBEDTLS_ERR(
        #if MBEDTLS_VERSION_NUMBER >= 0x03000000
            mbedtls_pk_parse_key(&key, key_pem, key_len, NULL, 0, NULL, NULL),
        #else
            mbedtls_pk_parse_key(&key, key_pem, key_len, NULL, 0),
        #endif
        "parse_key");

    /* 2. Seed DRBG */
    uint8_t seed[32];
    esp_fill_random(seed, sizeof(seed));
    CHECK_MBEDTLS_ERR(
        mbedtls_ctr_drbg_seed(
            &ctr_drbg, mbedtls_entropy_func, 
            &entropy, seed, sizeof(seed)
        ), "drbg_seed");

    /* 3. Setup Cert Info */
    mbedtls_x509write_crt_set_subject_key(&crt, &key);
    mbedtls_x509write_crt_set_issuer_key(&crt, &key);

    char subject[128];
    snprintf(subject, sizeof(subject), "CN=%s", dns_name);
    mbedtls_x509write_crt_set_subject_name(&crt, subject);
    mbedtls_x509write_crt_set_issuer_name(&crt, subject);

    mbedtls_x509write_crt_set_validity(&crt, "19700101000000", "21700101000000");
    mbedtls_x509write_crt_set_version(&crt, MBEDTLS_X509_CRT_VERSION_3);
    mbedtls_x509write_crt_set_md_alg(&crt, MBEDTLS_MD_SHA256);

    mbedtls_x509write_crt_set_basic_constraints(&crt, 0, -1);
    mbedtls_x509write_crt_set_key_usage(&crt, 
        MBEDTLS_X509_KU_DIGITAL_SIGNATURE | MBEDTLS_X509_KU_KEY_ENCIPHERMENT);

    /* 4. Random Serial */
    uint8_t serial_buf[32];
    esp_fill_random(serial_buf, sizeof(serial_buf));
    mbedtls_mpi_read_binary(&serial, serial_buf, sizeof(serial_buf));
    if MBEDTLS_VERSION_NUMBER >= 0x03000000
        CHECK_MBEDTLS_ERR(mbedtls_x509write_crt_set_serial_raw(
            &crt, serial_buf, sizeof(serial_buf)), "set_serial");
    #else
        mbedtls_mpi_read_binary(&serial, serial_buf, sizeof(serial_buf));
        CHECK_MBEDTLS_ERR(mbedtls_x509write_crt_set_serial(
            &crt, &serial), "set_serial");
    #endif

    /* 5. Set SAN */
    unsigned char san_der[MDNS_HOSTNAME_FULL_MAX_LEN + 6 + 32];
    size_t name_len = strlen(dns_name);

    char temp_name[MDNS_HOSTNAME_FULL_MAX_LEN + 6];
    snprintf(temp_name, sizeof(temp_name), "%s.local", dns_name);
    name_len = strlen(temp_name);

    san_der[0] = 0x30; /* Sequence */
    san_der[1] = (unsigned char)(name_len + 2);
    san_der[2] = 0x82; /* DNS Name Tag */
    san_der[3] = (unsigned char)name_len;
    memcpy(&san_der[4], temp_name, name_len);

    CHECK_MBEDTLS_ERR(
        mbedtls_x509write_crt_set_extension(
            &crt, MBEDTLS_OID_SUBJECT_ALT_NAME,
            MBEDTLS_OID_SIZE(MBEDTLS_OID_SUBJECT_ALT_NAME),
            0, san_der, name_len + 4
        ), "set_san");

    /* 6. Set Extended Key Usage (Server Auth) */
    CHECK_MBEDTLS_ERR(
        mbedtls_x509write_crt_set_extension(
            &crt, MBEDTLS_OID_EXTENDED_KEY_USAGE, 
            MBEDTLS_OID_SIZE(MBEDTLS_OID_EXTENDED_KEY_USAGE),
            0, eku_server_auth, sizeof(eku_server_auth)
        ), "set_extension_key");

    mbedtls_x509write_crt_set_subject_key_identifier(&crt);
    mbedtls_x509write_crt_set_authority_key_identifier(&crt);

    /* 7. Write PEM */
    CHECK_MBEDTLS_ERR(
        mbedtls_x509write_crt_pem(
            &crt, cert_out, CERT_BUF_SIZE, 
            mbedtls_ctr_drbg_random, &ctr_drbg
        ), "write_pem");

    *cert_len_out = strlen((char *)cert_out) + 1;
    CORE_LOG(I, _MSG_CERT_GEN_OK, dns_name);
    ret = 0;
cleanup:
    mbedtls_pk_free(&key);
    mbedtls_x509write_crt_free(&crt);
    mbedtls_mpi_free(&serial);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    return (ret == 0) ? ESP_OK : ESP_FAIL;
}

#undef TAG
#define TAG "Cert-Manager"

esp_err_t generate_https_certs(
    char** cert_pem, size_t* cert_len, char** key_pem,  size_t* key_len
) {
    esp_err_t ret = ESP_FAIL;
    const eif_t *cfg = eif_get();

    *cert_pem = calloc(1, CERT_BUF_SIZE);
    *key_pem  = calloc(1, KEY_PEM_LEN);

    if (!*cert_pem || !*key_pem) {
        CORE_LOG(E, "Memory allocation failed for certs");
        goto cleanup;
    }

    CHECK_ESP_ERR_T(E, 
        generate_ecc_keypair((uint8_t *)*key_pem, key_len), 
        {}, goto cleanup, _ERR_GEN_KEY);

    CHECK_ESP_ERR_T(E, generate_self_signed_cert(
        (uint8_t *)*key_pem, *key_len, 
        (uint8_t *)*cert_pem, cert_len, 
        cfg->mdns_hostname
    ), {}, goto cleanup, _ERR_GEN_CERT);

    return ESP_OK;
cleanup:
    free(*cert_pem); *cert_pem = NULL;
    free(*key_pem);  *key_pem = NULL;
    return ret;
}
