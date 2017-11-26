#ifndef	_CRYPTO_H
#define _CRYPTO_H

#ifdef __cplusplus
extern "C" {
#endif

//*******************************************************
// used by client and server
#include "function_strings.h"
#include "polarssl/config.h"
#include "polarssl/net.h"
#include "polarssl/ssl.h"
#include "polarssl/entropy.h"
#include "polarssl/ctr_drbg.h"
#include "crypto_proj_strings.h"
#include "dhExchange.h"
#include "debug.h"

#define SRV_CERT_FILE	"./server.crt"
#define CA_CERT_FILE	"./ca.crt"
#define SRV_KEY_FILE	"./server.key"
#define AES_KEY_SIZE	256

#define CLIENT 1
#define SERVER 2
#define MAX(a,b)	(((a) > (b)) ? (a) : (b))
#define MIN(a,b)	(((a) < (b)) ? (a) : (b))

enum flag {FALSE = 0, TRUE};
extern entropy_context entropy;				// Entropy context
extern ctr_drbg_context ctr_drbg;			// Counter mode deterministic random byte generator context
extern dhm_context *dhm;					// Diffie-Hellman context
extern enum flag rng_initialized;			// Random number generator initialization flag

typedef struct _crypt_context {
	ssl_context	*ssl;
	ssl_session	*ssn;
	int 		*socket;
	enum flag	encrypt;
	aes_context	*aes;
} crypt_context;

crypt_context *crypt_setup_client(int *sockfd );
crypt_context *crypt_setup_server(int *sockfd );
int rng_init();
int crypt_handshake(crypt_context *ioc);
int crypt_read(crypt_context *ioc, unsigned char *buf, size_t bufsize );
int crypt_write(crypt_context *ioc, unsigned char *buf, size_t bufsize );
int	crypt_close_notify(crypt_context *ioc);
void crypt_cleanup(crypt_context *ioc);
int gen_random(unsigned char *output, size_t output_len);
int aes_init(crypt_context *ioc);
int aes_terminate(crypt_context *ioc);
void print_ssl_error(int error);

#ifdef __cplusplus
}
#endif

#endif	//_CRYPTO_H
