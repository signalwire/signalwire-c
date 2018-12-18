/*
 * Copyright (c) 2018 SignalWire, Inc
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "signalwire-client-c/client.h"

SWCLT_DECLARE(ks_status_t) swclt_ssl_create_context(const char *private_key_path, const char *client_cert_path, const char *cert_chain_path, SSL_CTX **sslP)
{
    const SSL_METHOD *method = NULL;
	SSL_CTX *ssl;

#if OPENSSL_VERSION_NUMBER >= 0x10100000
    method = TLS_client_method();
#else
    method = TLSv1_2_client_method();
#endif
	if (!method) {
		ks_log(KS_LOG_ERROR, "Failed to allocate method, returning status: %lu", KS_STATUS_NO_MEM);
		return KS_STATUS_NO_MEM;
	}

    if (!(ssl = SSL_CTX_new(method))) {
		ks_log(KS_LOG_ERROR, "Failed to allocate ssl context returning status: %lu", KS_STATUS_NO_MEM);
		return KS_STATUS_NO_MEM;
	}

    SSL_CTX_set_options(ssl, SSL_OP_NO_SSLv2);
    SSL_CTX_set_options(ssl, SSL_OP_NO_SSLv3);
    SSL_CTX_set_options(ssl, SSL_OP_NO_TLSv1);
    SSL_CTX_set_options(ssl, SSL_OP_NO_TLSv1_1);

    SSL_CTX_set_options(ssl, SSL_OP_NO_COMPRESSION);

    if (cert_chain_path) {
        if (!SSL_CTX_use_certificate_chain_file(ssl, cert_chain_path)) {
			SSL_CTX_free(ssl);
			ks_log(KS_LOG_WARNING, "Failed to load ssl certificate chain: %s", cert_chain_path);
            return KS_STATUS_INVALID_ARGUMENT;
        }
        if (!SSL_CTX_load_verify_locations(ssl, cert_chain_path, NULL)) {
			ks_log(KS_LOG_WARNING, "Failed to verify ssl certificate chain: %s", cert_chain_path);
			SSL_CTX_free(ssl);
            return KS_STATUS_INVALID_ARGUMENT;
        }
    }

    if (private_key_path && client_cert_path) {
        if (!SSL_CTX_use_certificate_file(ssl, client_cert_path, SSL_FILETYPE_PEM)) {
            ks_log(KS_LOG_ERROR, "SSL certificate file error: %s", client_cert_path);
			SSL_CTX_free(ssl);
            return KS_STATUS_INVALID_ARGUMENT;
        }

        if (!SSL_CTX_use_PrivateKey_file(ssl, private_key_path, SSL_FILETYPE_PEM)) {
            ks_log(KS_LOG_ERROR, "SSL private key file error: %s", private_key_path);
			SSL_CTX_free(ssl);
            return KS_STATUS_INVALID_ARGUMENT;
        }

        if (!SSL_CTX_check_private_key(ssl)) {
            ks_log(KS_LOG_ERROR, "SSL failed to verify private key file error: %s", private_key_path);
			SSL_CTX_free(ssl);
            return KS_STATUS_INVALID_ARGUMENT;
        }

        SSL_CTX_set_cipher_list(ssl,"HIGH:!DSS:!aNULL@STRENGTH");
    }

	*sslP = ssl;

	ks_log(KS_LOG_DEBUG, "Successfully created ssl context");

	return KS_STATUS_SUCCESS;
}

SWCLT_DECLARE(void) swclt_ssl_destroy_context(SSL_CTX **ctx)
{
	if (!ctx || !*ctx)
		return;

	SSL_CTX_free(*ctx);
	*ctx = NULL;
}
