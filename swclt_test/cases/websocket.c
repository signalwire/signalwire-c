/*
 * Copyright (c) 2026 SignalWire, Inc
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

#include "swclt_test.h"
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/objects.h>
#include <openssl/x509.h>

#define WSS_TEST_WAIT_MS 3000
#define WS_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

/* 0x81 = FIN | TEXT, 12 = payload length, no mask */
static const uint8_t TWO_FRAMES[] = {
	0x81, 12, 'f', 'i', 'r', 's', 't', ' ', 'f', 'r', 'a', 'm', 'e', '.',
	0x81, 12, 's', 'e', 'c', 'o', 'n', 'd', ' ', 'f', 'r', 'a', 'm', 'e'
};
static const char *TWO_FRAMES_TEXT[] = { "first frame.", "second frame" };

typedef struct wss_test_srv {
	ks_socket_t sock;
	SSL_CTX *ssl_ctx;
	int with_handshake;		/* frames go in the same write as the 101 */
	volatile int client_up;
	volatile int done;
} wss_test_srv_t;

typedef struct wss_test_rx {
	ks_mutex_t *mutex;
	int count;
	char text[2][32];
} wss_test_rx_t;

static int srv_write(SSL *ssl, ks_socket_t sock, const void *data, ks_size_t len)
{
	ks_size_t off = 0;

	while (off < len) {
		if (ssl) {
			int n = SSL_write(ssl, (const char *)data + off, (int)(len - off));

			if (n <= 0) {
				return 0;
			}
			off += (ks_size_t)n;
		} else {
			ks_size_t n = len - off;

			if (ks_socket_send(sock, (char *)data + off, &n) != KS_STATUS_SUCCESS || n == 0) {
				return 0;
			}
			off += n;
		}
	}

	return 1;
}

static ks_ssize_t srv_read(SSL *ssl, ks_socket_t sock, void *data, ks_size_t len)
{
	if (!(ssl && SSL_pending(ssl) > 0) && ks_wait_sock(sock, 2000, KS_POLL_READ) <= 0) {
		return -1;
	}

	if (ssl) {
		int n = SSL_read(ssl, data, (int)len);

		return n > 0 ? n : -1;
	} else {
		ks_size_t n = len;

		if (ks_socket_recv(sock, data, &n) != KS_STATUS_SUCCESS || n == 0) {
			return -1;
		}

		return (ks_ssize_t)n;
	}
}

static void *wss_test_server_thread(ks_thread_t *thread, void *thread_data)
{
	wss_test_srv_t *srv = (wss_test_srv_t *)thread_data;
	ks_socket_t sock;
	SSL *ssl = NULL;
	char req[4096] = "", key[128] = "", src[256], out[1024];
	unsigned char sha[SHA_DIGEST_LENGTH], accept_key[64] = "";
	ks_size_t got = 0;
	char *k, *e;
	int n, sanity;

	(void)thread;

	if ((sock = accept(srv->sock, NULL, NULL)) == KS_SOCK_INVALID) {
		return NULL;
	}

	if (srv->ssl_ctx) {
		if (!(ssl = SSL_new(srv->ssl_ctx))) {
			goto end;
		}
		SSL_set_fd(ssl, (int)sock);
		if (SSL_accept(ssl) != 1) {
			ks_log(KS_LOG_ERROR, "Test server: TLS handshake failed");
			goto end;
		}
	}

	while (got < sizeof(req) - 1 && !strstr(req, "\r\n\r\n")) {
		ks_ssize_t r = srv_read(ssl, sock, req + got, sizeof(req) - 1 - got);

		if (r <= 0) {
			goto end;
		}
		got += (ks_size_t)r;
		req[got] = '\0';
	}

	if (!(k = strstr(req, "Sec-WebSocket-Key: ")) || !(e = strstr(k, "\r\n"))) {
		goto end;
	}
	k += strlen("Sec-WebSocket-Key: ");
	ks_snprintf(key, sizeof(key), "%.*s", (int)(e - k), k);
	ks_snprintf(src, sizeof(src), "%s%s", key, WS_GUID);
	SHA1((const unsigned char *)src, strlen(src), sha);
	EVP_EncodeBlock(accept_key, sha, SHA_DIGEST_LENGTH);

	n = ks_snprintf(out, sizeof(out),
		"HTTP/1.1 101 Switching Protocols\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Accept: %s\r\n"
		"\r\n", (char *)accept_key);

	if (srv->with_handshake) {
		memcpy(out + n, TWO_FRAMES, sizeof(TWO_FRAMES));
		if (!srv_write(ssl, sock, out, n + sizeof(TWO_FRAMES))) {
			goto end;
		}
	} else {
		if (!srv_write(ssl, sock, out, n)) {
			goto end;
		}
		for (sanity = 300; !srv->client_up && sanity > 0; sanity--) {
			ks_sleep_ms(10);
		}
		if (!srv_write(ssl, sock, TWO_FRAMES, sizeof(TWO_FRAMES))) {
			goto end;
		}
	}

	for (sanity = 1000; !srv->done && sanity > 0; sanity--) {
		ks_sleep_ms(10);
	}

end:
	if (ssl) {
		SSL_shutdown(ssl);
		SSL_free(ssl);
	}
	ks_socket_close(&sock);

	return NULL;
}

static SSL_CTX *create_server_ssl_context(void)
{
	EVP_PKEY_CTX *kctx;
	EVP_PKEY *pkey = NULL;
	X509 *cert;
	SSL_CTX *ctx;

	REQUIRE(kctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL));
	REQUIRE(EVP_PKEY_keygen_init(kctx) > 0);
	REQUIRE(EVP_PKEY_CTX_set_ec_paramgen_curve_nid(kctx, NID_X9_62_prime256v1) > 0);
	REQUIRE(EVP_PKEY_keygen(kctx, &pkey) > 0);
	EVP_PKEY_CTX_free(kctx);

	REQUIRE(cert = X509_new());
	ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);
	X509_gmtime_adj(X509_getm_notBefore(cert), 0);
	X509_gmtime_adj(X509_getm_notAfter(cert), 3600);
	X509_set_pubkey(cert, pkey);
	X509_NAME_add_entry_by_txt(X509_get_subject_name(cert), "CN", MBSTRING_ASC, (const unsigned char *)"localhost", -1, -1, 0);
	X509_set_issuer_name(cert, X509_get_subject_name(cert));
	REQUIRE(X509_sign(cert, pkey, EVP_sha256()) > 0);

	REQUIRE(ctx = SSL_CTX_new(TLS_server_method()));
	REQUIRE(SSL_CTX_use_certificate(ctx, cert) == 1);
	REQUIRE(SSL_CTX_use_PrivateKey(ctx, pkey) == 1);

	X509_free(cert);
	EVP_PKEY_free(pkey);

	return ctx;
}

static ks_status_t __on_incoming_frame(swclt_wss_t *wss, swclt_frame_t **frame, void *cb_data)
{
	wss_test_rx_t *rx = (wss_test_rx_t *)cb_data;

	(void)wss;

	ks_mutex_lock(rx->mutex);
	if (rx->count < 2) {
		ks_snprintf(rx->text[rx->count], sizeof(rx->text[0]), "%.*s", (int)(*frame)->len, (char *)(*frame)->data);
	}
	rx->count++;
	ks_mutex_unlock(rx->mutex);

	return KS_STATUS_SUCCESS;
}

static int run_case(ks_pool_t *pool, const char *name, SSL_CTX *server_ctx, SSL_CTX *client_ctx, int with_handshake)
{
	wss_test_srv_t srv = { 0 };
	wss_test_rx_t rx = { 0 };
	ks_thread_t *server_thread = NULL;
	swclt_wss_t *wss = NULL;
	ks_sockaddr_t addr;
	socklen_t addr_len = sizeof(addr.v.v4);
	int waited, count = 0, ok;

	srv.ssl_ctx = server_ctx;
	srv.with_handshake = with_handshake;

	REQUIRE(!ks_mutex_create(&rx.mutex, KS_MUTEX_FLAG_DEFAULT, pool));

	REQUIRE(ks_addr_set(&addr, "127.0.0.1", 0, AF_INET) == KS_STATUS_SUCCESS);
	REQUIRE((srv.sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) != KS_SOCK_INVALID);
	REQUIRE(ks_addr_bind(srv.sock, &addr) == KS_STATUS_SUCCESS);
	REQUIRE(getsockname(srv.sock, (struct sockaddr *)&addr.v.v4, &addr_len) == 0);
	REQUIRE(listen(srv.sock, 1) == 0);
	REQUIRE(!ks_thread_create(&server_thread, wss_test_server_thread, &srv, pool));

	REQUIRE(!swclt_wss_connect(&wss, __on_incoming_frame, &rx, NULL, NULL,
		"127.0.0.1", (short)ks_addr_get_port(&addr), "websocket_test", 2000, client_ctx));
	srv.client_up = 1;

	for (waited = 0; waited < WSS_TEST_WAIT_MS; waited += 10) {
		ks_mutex_lock(rx.mutex);
		count = rx.count;
		ks_mutex_unlock(rx.mutex);
		if (count >= 2) {
			break;
		}
		ks_sleep_ms(10);
	}

	srv.done = 1;
	swclt_wss_destroy(&wss);
	ks_socket_shutdown(srv.sock, 2);
	ks_socket_close(&srv.sock);
	ks_thread_join(server_thread);
	ks_thread_destroy(&server_thread);

	ok = count == 2 && !strcmp(rx.text[0], TWO_FRAMES_TEXT[0]) && !strcmp(rx.text[1], TWO_FRAMES_TEXT[1]);
	if (ok) {
		ks_log(KS_LOG_INFO, "%s: PASS, %d of 2 frames within %d ms", name, count, waited);
	} else {
		ks_log(KS_LOG_ERROR, "%s: FAIL, %d of 2 frames within %d ms", name, count, waited);
	}

	ks_mutex_destroy(&rx.mutex);

	return ok;
}

void test_websocket(ks_pool_t *pool)
{
	SSL_CTX *server_ctx = create_server_ssl_context();
	SSL_CTX *client_ctx = NULL;
	int tls_ok, plain_ok;

	REQUIRE(!swclt_ssl_create_context(NULL, NULL, NULL, &client_ctx));

	tls_ok = run_case(pool, "two frames in one TLS record", server_ctx, client_ctx, 0);
	plain_ok = run_case(pool, "two frames with the handshake response", NULL, NULL, 1);

	swclt_ssl_destroy_context(&client_ctx);
	SSL_CTX_free(server_ctx);

	REQUIRE(tls_ok);
	REQUIRE(plain_ok);
}
