#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "crypto_proj_strings.h"
#include "polarssl/x509.h"
#include "polarssl/aes.h"
#include "crypto.h"

#if defined SOLARIS
#define UINT16_MAX (65535U)
#else
#include <stdint.h>
#endif      //SOLARIS

entropy_context entropy;				// Entropy context
ctr_drbg_context ctr_drbg;				// Counter mode deterministic random byte generator context
dhm_context *dhm;						// Diffie-Hellman context
unsigned char iv[16];					// Initialization vector
enum flag rng_initialized = FALSE;		// RNG initialization flag

const char *personalization = "7ddc11c4-5789-44d4-8de4-88c0d23d4029";	// Custom data to add uniqueness
char *my_dhm_P = (char *) my_dhm_P_String;	// The values of these strings are located in crypto_strings.txt
char *my_dhm_G = (char *) my_dhm_G_String;
unsigned char shared_key[AES_KEY_SIZE];

static int	my_set_session(ssl_context * ssl);
static int	my_get_session(ssl_context * ssl);

//*******************************************************
#ifndef DEBUG
#define DEBUG_LEVEL 0
#endif
//*******************************************************
void my_debug(void *ctx, int level, const char *str) {
#ifdef DEBUG
	if (level < dbug_level_)
#else
	if (level < DEBUG_LEVEL)
#endif
	{
		fprintf((FILE *) ctx, "%s", str);
		fflush((FILE *) ctx);
	}
}

//*******************************************************
/*!
 * @brief Initialize random number generator
 * @return
 * @retval < 0 -- error
 * @retval 1 -- success
 */
int rng_init()
{
	int ret = 1;
	unsigned int seed;

	DLX(6, printf( "Initializing RNG.\n"));
	entropy_init(&entropy);
	if ( (ret = ctr_drbg_init(&ctr_drbg, entropy_func, &entropy, (unsigned const char *)personalization, strlen(personalization))) != 0 ) {
		DLX(4, switch (ret) {
					case POLARSSL_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED:	printf("The entropy source failed.\n"); break;
					case POLARSSL_ERR_CTR_DRBG_REQUEST_TOO_BIG:			printf("Too many random requested in single call."); break;
					case POLARSSL_ERR_CTR_DRBG_INPUT_TOO_BIG:			printf("Input too large (Entropy + additional).\n"); break;
					case POLARSSL_ERR_CTR_DRBG_FILE_IO_ERROR:			printf("Read/write error in file\n"); break;
					default:											printf("ERROR: ctr_drbg_init() failed, returned -0x%04x\n", -ret);}
				);
		if ((ret = ctr_drbg_update_seed_file(&ctr_drbg, ".seedfile")) !=0 ) {
			DLX(4, switch (ret) {
				case POLARSSL_ERR_CTR_DRBG_FILE_IO_ERROR:			printf("Failed to open seed file.\n"); break;
				case POLARSSL_ERR_CTR_DRBG_REQUEST_TOO_BIG:			printf("Seed file too big?.\n"); break;
				case POLARSSL_ERR_CTR_DRBG_INPUT_TOO_BIG:			printf("Seed file too big?.\n"); break;
				case POLARSSL_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED:	printf("The entropy source failed.\n"); break;
				default:											printf("ERROR: ctr_drbg_update_seedfile() failed, returned -0x%04x\n", -ret);
				}
			);
		}
	}
	ret = ret < 0 ? ret : 1;
	ctr_drbg_set_prediction_resistance(&ctr_drbg, CTR_DRBG_PR_OFF);	// Turn off prediction resistance
	ctr_drbg_random(&ctr_drbg, (unsigned char *)&seed, sizeof(seed));
	srand(seed);		// Seed system's pseudo random number generator
	rng_initialized = TRUE;
	return ret;
}

/*!
 * gen_random
 * @brief generate random numbers
 * @param output - output buffer
 * @param output_len - length of output buffer
 * @return
 * @retval 0 -- error
 * @retval 1 -- success
 */
int gen_random(unsigned char *output, size_t output_len)
{
	if (!rng_initialized) {
		if (rng_init() < 0) {
			DLX(4, printf( "Failed to initialize random number generator\n"));
			return 0;
		}
	}

	if ((ctr_drbg_random( &ctr_drbg, output, output_len)) != 0) {
		DLX(4, printf( "Failed to generate random number\n"));
	}
	return 1;
}

//*******************************************************
/*!
 *
 * @param ioc -- I/O context
 * @return
 * @retval 0 -- error
 * @retval 1 -- success
 */
int aes_init(crypt_context *ioc) {

	int ret;

	if (ioc == NULL) {
		DLX(4, printf("failed, no I/O context.\n"));
		return 0;
	}

	DLX(4, printf( "Diffie-Hellman Handshake\n"));
	if ((dhm = dhHandshake( ioc->ssl )) == NULL)
	{
		DLX(4, printf("Diffie-Hellman Handshake failed\n"));
		return 0;
	}

	// Extract shared key from DHM context
    if ((ret = mpi_write_binary(&dhm->K, shared_key, AES_KEY_SIZE)) < 0) {
    	DLX(4, printf("mpi_write_binary() failed:"); print_ssl_error(ret));
    	return 0;
    }
    DPB(4, "Shared Key", shared_key, AES_KEY_SIZE);
    md5(shared_key, AES_KEY_SIZE, iv);	// Seed initialization vector with md5 hash of shared key
    DPB(4, "Initialization Vector", iv, sizeof(iv));
    ioc->encrypt = TRUE;
    return 1;
}

int aes_terminate(crypt_context *ioc)
{
	DLX(6, printf("Terminating AES. I/O context: %p\n", ioc));
	if (ioc->aes->nr == 0) {
		DLX(4, printf("failed, AES context is invalid.\n"));
		return 0;
	}
	memset(ioc->aes, 0, sizeof(aes_context));	// Clear the AES context
	dhm_free(dhm);
	free(dhm);
	ioc->encrypt = FALSE;
	return 1;
}

//*******************************************************
/*!
 * int crypt_handshake(crypt_context *ioc)
 * @brief Establish an SSL connection
 * @param ioc -- I/O context
 * @return
 * @retval 0 -- error
 * @retval 1 -- success
 */
int crypt_handshake(crypt_context *ioc) {
		int ret;

		DLX(4, printf("\tPerforming the TLS handshake... \n"));

		while ((ret = ssl_handshake(ioc->ssl)) != 0) {
			if (ret == POLARSSL_ERR_SSL_CONN_EOF)
				DLX(4, printf("\tNormal reset by Blot Proxy: "); print_ssl_error(ret));
				return -1;
			if (ret != POLARSSL_ERR_NET_WANT_WRITE && ret != POLARSSL_ERR_SSL_CONN_EOF) {
				DLX(4, printf("\tTLS handshake failed: "); print_ssl_error(ret));
				return -1;
			}
		}
		DLX(4, printf("\tTLS handshake complete. I/O context: %p, socket: %d\n", ioc, *ioc->socket));

		return 0;
	}

//*******************************************************
/*!
 * int crypt_write(crypt_context *ioc, unsigned char *buf, size_t size)
 * @brief Write data to the encrypted network connection
 * @param ioc -- I/O context
 * @param buf -- buffer to transmit
 * @param size -- size of buffer (<= 65,535 bytes)
 * @return
 * @retval >= 0 -- number of characters written
 * @retval < 0  -- error
 */
int crypt_write(crypt_context *ioc, unsigned char *buf, size_t size) {
	int ret = 0;
	size_t bufsize, sent;
	unsigned char *encbuf = NULL;

	if (size > UINT16_MAX) {	// Check size of write request
		DLX(6, printf("Size to write (%u bytes) is too big. Must be <= %u bytes\n", (unsigned int)size, UINT16_MAX));
		return -1;
	}

	DPB(7, "Buffer to write", buf, size);
	if (ioc->encrypt) {
		DLX(6, printf("AES encrypting write buffer\n"));
		bufsize = ((size+2) % 16) ? (size+2) + (16 - (size+2)%16) : (size+2);	// Compute size of buffers - multiple of 16, including length field
		encbuf = (unsigned char *) calloc(bufsize, sizeof(unsigned char) );		// Allocate encryption buffer
		if (encbuf == NULL) {
				DLX(4, printf("calloc() failed\n"));
				return -1;
		}
	    encbuf[0] = (unsigned char)(size >> 8);	// Insert the data length
	    encbuf[1] = (unsigned char) size;
		memcpy(encbuf+2, buf, size);	// Copy input buffer to padded encryption buffer
		DPB(8, "Buffer before encryption", encbuf, bufsize);
		DPB(9, "Initialization Vector", iv, sizeof(iv));
		DLX(9, printf("ioc->aes->nr = %d\n", ioc->aes->nr));
		aes_setkey_enc(ioc->aes, shared_key, AES_KEY_SIZE);		// Set key for encryption
		if (( ret = aes_crypt_cbc(ioc->aes, AES_ENCRYPT, bufsize, iv, encbuf, encbuf)) < 0) {	// Encrypt the block
			DLX(4, printf("aes_crypt_cbd() failed: "); print_ssl_error(ret));
			return ret;
		}
		DPB(8, "Buffer after encryption", encbuf, bufsize);
	} else {		// If not encrypting, adjust pointers
		encbuf = buf;
		bufsize = size;
	}

	DLX(8, printf("Sending %u bytes\n", (unsigned int)bufsize));
	sent = 0;
	do {	// Write loop
		ret = ssl_write(ioc->ssl, encbuf+sent, bufsize-sent);
		if (ret < 0) {
			if (ret == POLARSSL_ERR_NET_WANT_WRITE)
				continue;

			if (ret == POLARSSL_ERR_SSL_PEER_CLOSE_NOTIFY) {
				DLX(4, printf("Remote closed connection\n"));
				break;
			}

			if (ret == POLARSSL_ERR_NET_CONN_RESET) {
				DLX(4, printf("Connection reset by peer\n"));
				break;
			}

			DLX(4, printf("ssl_write() failed: "); print_ssl_error(ret));
			break;

		} else
			sent += ret;
	} while (sent < bufsize);
	DLX(8, printf("Bytes sent: %u\n", (unsigned int)sent));

	ret = (ret < 0) ? ret : (int)size; 		//Return the number of (unencrypted) bytes sent or the error code
	DLX(6, printf("Return value: 0x%04x\n", ret));
	if (ioc->encrypt)
		free(encbuf);						// Clean-up
	return ret;

}
/*!
 * int crypt_read(ssl_context *ssl, unsigned char *buf, size_t size)
 * @brief Read data from the encrypted network connection
 * @param ioc - I/O context
 * @param buf - read buffer of at least size bytes
 * @param size - maximum size to read
 * @return
 * @retval > 0 - Number of bytes returned in buf
 * @retval 0 - EOF
 * @retval < 0 - Error
 */
//*******************************************************
int crypt_read(crypt_context *ioc, unsigned char *buf, size_t size) {
	int ret = 0, received = 0;
	size_t bufsize;
	unsigned char *encbuf = NULL;

	DLX(6, printf("New read request for %lu bytes\n", (unsigned long)size));
	if (ioc->encrypt) {																// Allocate encryption buffer -- multiple of 16 bytes
		bufsize = ((size+2) % 16) ? (size+2) + (16 - (size+2)%16) : (size+2);	// Buffer size needed must account for 2-byte size field
		encbuf = (unsigned char *) calloc(bufsize, sizeof(unsigned char) );		// Allocate encryption buffer
		if (encbuf == NULL) {
				DLX(4, printf("calloc() failed\n"));
				return -1;
		}
	} else {
		encbuf = buf;
		bufsize = size;
	}

	do {
#ifdef SOLARIS		// "re_read / goto re_read" necessary for Solaris, as the compiler
					// apparently doesn't do the right thing for "continue" statement.
		re_read:
#endif
		// Read data from network
		received = ssl_read(ioc->ssl, encbuf, bufsize);
		switch (received) {		// Process any exceptions
			case POLARSSL_ERR_NET_WANT_READ:
				DLX(4, printf("POLARSSL_ERR_NET_WANT_READ\n"));
#ifdef SOLARIS
				goto re_read;
#endif
				continue;

			case POLARSSL_ERR_SSL_PEER_CLOSE_NOTIFY:
				DLX(4, printf("POLARSSL_ERR_SSL_PEER_CLOSE_NOTIFY\n"));
				ret = received;
				goto Exception;
				break;

			case POLARSSL_ERR_NET_CONN_RESET:
				DLX(4, printf("Connection reset by peer\n"));
				ret = received;
				goto Exception;
				break;

			case 0: // EOF
				DLX(6, printf("EOF\n"));
				return 0;
				break;

			default:
				if (received < 0) {
					DLX(4, printf("ERROR: crypt_read() failed: "); print_ssl_error(received));
					ret = received;
					goto Exception;
				} else
					DLX(6, printf("%d bytes read\n", received));
				break;
		}
	} while (0);

	if (ioc->encrypt) {
		DPB(8, "Buffer before decryption", encbuf, received);
		if ((received % 16) != 0) {
			DLX(6, printf("WARNING: Received data is not a multiple of 16\n"));
			ret = -1;
			goto Exception;
		}
		DLX(8, printf("AES decrypting read buffer\n"));
		DLX(9, printf("ioc->aes->nr = %d\n", ioc->aes->nr));
		DPB(9, "Initialization Vector", iv, sizeof(iv));
		aes_setkey_dec(ioc->aes, shared_key, AES_KEY_SIZE);		// Set key for decryption
		if (( ret = aes_crypt_cbc(ioc->aes, AES_DECRYPT, received, iv, encbuf, encbuf)) < 0) {	// Decrypt the block
			DLX(4, printf("aes_crypt_cbc() failed: "); print_ssl_error(ret));
			return ret;
		}
		DPB(8, "Buffer after decryption", encbuf, received);
		bufsize = (encbuf[0] << 8) + encbuf[1];
		DLX(8, printf("bufsize = %u\n", (unsigned int)bufsize));
		if (bufsize > size)	{	// Data in the embedded length field does not match the length of the data sent
			DLX(4, printf("ERROR: Buffer read (%u) is larger than buffer available (%u)\n", (unsigned int)bufsize, (unsigned int)size));
			free(encbuf);
			return -1;
		}
		else
			memcpy(buf, encbuf+2, bufsize);
		free(encbuf);
	} else {
		bufsize = received;
	}
	DPB(6, "Buffer read", buf, bufsize);
	return bufsize;		// This is the actual number of bytes read

	Exception:
		if (ioc->encrypt) {
			free(encbuf);
		}
		return ret;
}


//*******************************************************
int crypt_close_notify(crypt_context *ioc) {
	DLX(6, printf("I/O context: %p, socket: %d\n", ioc, *ioc->socket));
	if (ioc) {
		if (ioc->ssl) {
			return ssl_close_notify(ioc->ssl);
		}
	}
	return 0;
}

//*******************************************************
/*!
 * crypt_context *crypt_setup_client(int *sockfd)
 * @brief - Create a client crypt context
 * @param sockfd
 * @return - Pointer to the crypt_contex or NULL otherwise
 */
crypt_context *crypt_setup_client(int *sockfd) {
	crypt_context	*ioc;
	ssl_context		*ssl;
	ssl_session		*ssn;
	aes_context		*aes;
	int ret;

	if (! rng_initialized)	// Verify that the RNG is initialized.
		rng_init();

	ioc = (crypt_context *)calloc(1, sizeof(crypt_context));
	DLX(6, printf("I/O context: %p\n", ioc));
	ssl = (ssl_context *)calloc(1, sizeof(ssl_context));
	ssn = (ssl_session *)calloc(1, sizeof(ssl_session));
	aes = (aes_context *)calloc(1, sizeof(aes_context));
	if (ioc == NULL || ssl == NULL || ssn == NULL || aes == NULL) {
		DLX(1, printf("ERROR: Could not allocate IO context\n"));
		return NULL;
	}
	ioc->socket = sockfd;
	ioc->ssl = ssl;
	ioc->ssn = ssn;
	ioc->aes = aes;

	DLX(4, printf("\tInitializing the TLS structure...\n"));
	if ((ret = ssl_init(ioc->ssl)) != 0) {
		DLX(4, print_ssl_error(ret));
		return NULL;
	}
	DLX(4, printf("\tSSL Initialized\n"));

	ssl_set_endpoint(ioc->ssl, SSL_IS_CLIENT);
	ssl_set_authmode(ioc->ssl, SSL_VERIFY_NONE);

	ssl_set_rng(ioc->ssl, ctr_drbg_random, &ctr_drbg);
	ssl_set_dbg(ioc->ssl, my_debug, stdout);
	ssl_set_bio(ioc->ssl, net_recv, sockfd, net_send, sockfd);

	ssl_set_ciphersuites(ioc->ssl, ssl_default_ciphersuites);
	ssl_set_session(ioc->ssl, 1, 600, ioc->ssn);

	return ioc;
}

#if 0
// = ssl_default_ciphers;
	int my_ciphers[] = {
		SSL_EDH_RSA_AES_256_SHA,
		SSL_EDH_RSA_CAMELLIA_256_SHA,
		SSL_EDH_RSA_AES_128_SHA,
		SSL_EDH_RSA_CAMELLIA_128_SHA,
		SSL_EDH_RSA_DES_168_SHA,
		SSL_RSA_AES_256_SHA,
		SSL_RSA_CAMELLIA_256_SHA,
		SSL_RSA_AES_128_SHA,
		SSL_RSA_CAMELLIA_128_SHA,
		SSL_RSA_DES_168_SHA,
		SSL_RSA_RC4_128_SHA,
		SSL_RSA_RC4_128_MD5,
		0
	};
#endif

//*******************************************************
static x509_cert srvcert, ca_chain;
static rsa_context rsa;

//*******************************************************
crypt_context *crypt_setup_server(int *sockfd) {
	crypt_context	*ioc;
	ssl_context		*ssl;
	ssl_session		*ssn;
	aes_context		*aes;
	int ret;
	int certflags;

	DLX(4, printf(" . Loading the server certs and key...\n"));

	memset(&srvcert, 0, sizeof(x509_cert));
	memset(&ca_chain, 0, sizeof(x509_cert));

	ret = x509parse_crtfile(&srvcert, SRV_CERT_FILE);
	if (ret != 0) {
		printf("\t> Error: Invalid or missing server certificate (%s).\n", SRV_CERT_FILE);
		DLX(4, print_ssl_error(ret));
		return NULL;
	}

	ret = x509parse_crtfile(&ca_chain, CA_CERT_FILE);
	if (ret != 0) {
		printf("\t> Error: Invalid or missing CA certificate (%s).\n", CA_CERT_FILE);
		DLX(4, print_ssl_error(ret));
		return NULL;
	}

	ret = x509parse_keyfile(&rsa, SRV_KEY_FILE, NULL);
	if (ret != 0) {
		printf("\t> Error: Invalid or missing server key (%s).\n", SRV_KEY_FILE);
		DLX(4, print_ssl_error(ret));
		return NULL;
	}

	if (x509parse_verify(&srvcert, &ca_chain, NULL, NULL, &certflags, NULL, NULL) != 0) {
		printf("\t> Error: Certificate chain verification failed:");
		if (certflags & BADCERT_EXPIRED)
			printf(" EXPIRED");
		if (certflags & BADCERT_NOT_TRUSTED)
			printf(" NOT TRUSTED");
		printf("\n");
		return NULL;
	}

	if (! rng_initialized)	// Verify that the RNG is initialized.
		rng_init();

	ioc = (crypt_context *)calloc(1, sizeof(crypt_context));
	DLX(6, printf("ioc = %p\n", ioc));
	ssl = (ssl_context *)calloc(1, sizeof(ssl_context));
	ssn = (ssl_session *)calloc(1, sizeof(ssl_session));
	aes = (aes_context *)calloc(1, sizeof(aes_context));
	if (ioc == NULL || ssl == NULL || ssn == NULL || aes == NULL) {
		DLX(1, printf("ERROR: Could not allocate IO context\n"));
		return NULL;
	}
	ioc->socket = sockfd;
	ioc->ssl = ssl;
	ioc->ssn = ssn;
	ioc->aes = aes;

	if ((ret = ssl_init(ioc->ssl)) != 0) {
		DLX(4, print_ssl_error(ret));
		return NULL;
	}

	ssl_set_endpoint(ioc->ssl, SSL_IS_SERVER);
	ssl_set_authmode(ioc->ssl, SSL_VERIFY_NONE);

	ssl_set_rng(ioc->ssl, ctr_drbg_random, &ctr_drbg);
	ssl_set_dbg(ioc->ssl, my_debug, stdout);
	ssl_set_bio(ioc->ssl, net_recv, sockfd, net_send, sockfd);
	ssl_set_scb(ioc->ssl, my_get_session, my_set_session);

	ssl_set_ciphersuites(ioc->ssl, ssl_default_ciphersuites);
	ssl_set_session(ioc->ssl, 1, 0, ioc->ssn);
	ssl_set_ca_chain(ioc->ssl, &ca_chain, NULL, NULL);
	ssl_set_own_cert(ioc->ssl, &srvcert, &rsa);

	if (ssl_set_dh_param(ioc->ssl, my_dhm_P, my_dhm_G) != 0) {
		DLX(1, printf("INTERNAL ERROR: Unable to set DH parameters, check if init_crypto_strings() was called.\n"));
		return NULL;
	}
	DLX(4, printf(" . SSL Server setup complete\n"));
	return ioc;
}

//*******************************************************
/*
 * These session callbacks use a simple chained list
 * to store and retrieve the session information.
 */
	ssl_session *s_list_1st = NULL;
	ssl_session *cur, *prv;

//*******************************************************
static int my_get_session(ssl_context * ssl) {
	time_t t = time(NULL);

	if (ssl->resume == 0)
		return 1;

	cur = s_list_1st;
	prv = NULL;

	while (cur != NULL) {
		prv = cur;
		cur = cur->next;

		if (ssl->timeout != 0 && t - prv->start > ssl->timeout)
			continue;

		if (ssl->session->ciphersuite != prv->ciphersuite || ssl->session->length != prv->length)
			continue;

		if (memcmp(ssl->session->id, prv->id, prv->length) != 0)
			continue;

		memcpy(ssl->session->master, prv->master, 48);
		return 0;
	}

	return 1;
}

//*******************************************************
static int my_set_session(ssl_context * ssl) {
	time_t t = time(NULL);

	cur = s_list_1st;
	prv = NULL;

	while (cur != NULL) {
		if (ssl->timeout != 0 && t - cur->start > ssl->timeout)
			break;			/* expired, reuse this slot */

		if (memcmp(ssl->session->id, cur->id, cur->length) == 0)
			break;			/* client reconnected */

		prv = cur;
		cur = cur->next;
	}

	if (cur == NULL) {
		cur = (ssl_session *) malloc(sizeof(ssl_session));
		if (cur == NULL)
			return 1;

		if (prv == NULL)
			s_list_1st = cur;
		else
			prv->next = cur;
	}

	memcpy(cur, ssl->session, sizeof(ssl_session));

	return 0;
}


//*******************************************************
void crypt_cleanup(crypt_context *ioc) {
	DLX(6, printf("Cleanup I/O context: %p\n", ioc));
	if (ioc) {
		if (ioc->ssl) {
			ssl_free(ioc->ssl);
			free(ioc->ssn);
			free(ioc->ssl);
			ioc->ssn = NULL;
			ioc->ssl = NULL;
		}
		if (ioc->aes) {
			free(ioc->aes);
			ioc->aes = NULL;
		}
		free(ioc);
		ioc = NULL;
	}
	return;
}

#ifdef __cplusplus
}
#endif

//*******************************************************
D(
		void print_ssl_error(int error)
{
	switch(error) {

	case POLARSSL_ERR_X509_FEATURE_UNAVAILABLE:				printf("X509 Error: Feature not available\n");	break;
	case POLARSSL_ERR_X509_CERT_INVALID_PEM:				printf("X509 Certificate Error: Invalid PEM format\n");	break;
	case POLARSSL_ERR_X509_CERT_INVALID_FORMAT:				printf("X509 Certificate Error: Invalid format\n");	break;
	case POLARSSL_ERR_X509_CERT_INVALID_VERSION:			printf("X509 Certificate Error: Invalid version\n");	break;
	case POLARSSL_ERR_X509_CERT_INVALID_SERIAL:				printf("X509 Certificate Error: Invalid serial number\n");	break;

	case POLARSSL_ERR_X509_CERT_INVALID_ALG:				printf("X509 Certificate Error: Invalid algorithm\n");	break;
	case POLARSSL_ERR_X509_CERT_INVALID_NAME:				printf("X509 Certificate Error: Invalid name\n");	break;
	case POLARSSL_ERR_X509_CERT_INVALID_DATE:				printf("X509 Certificate Error: Invalid date\n");	break;
	case POLARSSL_ERR_X509_CERT_INVALID_PUBKEY:				printf("X509 Certificate Error: Invalid public key\n");	break;
	case POLARSSL_ERR_X509_CERT_INVALID_SIGNATURE:			printf("X509 Certificate Error: Invalid signature\n");	break;

	case POLARSSL_ERR_X509_CERT_INVALID_EXTENSIONS:			printf("X509 Certificate Error: Invalid extensions\n");	break;
	case POLARSSL_ERR_X509_CERT_UNKNOWN_VERSION:			printf("X509 Certificate Error: Unknown version\n");	break;
	case POLARSSL_ERR_X509_CERT_UNKNOWN_SIG_ALG:			printf("X509 Certificate Error: Unknown signature algorithm\n");	break;
	case POLARSSL_ERR_X509_UNKNOWN_PK_ALG:					printf("X509 Error: Unknown algorithm\n");	break;
	case POLARSSL_ERR_X509_CERT_SIG_MISMATCH:				printf("X509 Certificate Error: Signature mismatch\n");	break;

	case POLARSSL_ERR_X509_CERT_VERIFY_FAILED:				printf("X509 Certificate Error: Verify failed\n");	break;
	case POLARSSL_ERR_X509_KEY_INVALID_VERSION:				printf("X509 Key Error: Invalid version\n");	break;
	case POLARSSL_ERR_X509_KEY_INVALID_FORMAT:				printf("X509 Key Error: Invalid format\n");	break;
	case POLARSSL_ERR_SSL_FEATURE_UNAVAILABLE:				printf("The requested feature is not available.\n\n");	break;
	case POLARSSL_ERR_SSL_BAD_INPUT_DATA:                   printf("Bad input parameters to function.\n\n");	break;
	case POLARSSL_ERR_SSL_INVALID_MAC:                      printf("Verification of the message MAC failed.\n");	break;
	case POLARSSL_ERR_SSL_INVALID_RECORD:                   printf("An invalid SSL record was received.\n");	break;
	case POLARSSL_ERR_SSL_CONN_EOF:                         printf("The connection indicated an EOF.\n");	break;
	case POLARSSL_ERR_SSL_UNKNOWN_CIPHER:                   printf("An unknown cipher was received.\n");	break;
	case POLARSSL_ERR_SSL_NO_CIPHER_CHOSEN:                 printf("The server has no ciphersuites in common with the client.\n");	break;
	case POLARSSL_ERR_SSL_NO_SESSION_FOUND:                 printf("No session to recover was found.\n");	break;
	case POLARSSL_ERR_SSL_NO_CLIENT_CERTIFICATE:            printf("No client certification received from the client, but required by the authentication mode.\n");	break;
	case POLARSSL_ERR_SSL_CERTIFICATE_TOO_LARGE:            printf("Our own certificate(s) is/are too large to send in an SSL message.\n");	break;
	case POLARSSL_ERR_SSL_CERTIFICATE_REQUIRED:             printf("The own certificate is not set, but needed by the server.\n");	break;
	case POLARSSL_ERR_SSL_PRIVATE_KEY_REQUIRED:             printf("The own private key is not set, but needed.\n");	break;
	case POLARSSL_ERR_SSL_CA_CHAIN_REQUIRED:                printf("No CA Chain is set, but required to operate.\n");	break;
	case POLARSSL_ERR_SSL_UNEXPECTED_MESSAGE:               printf("An unexpected message was received from our peer.\n");	break;
	case POLARSSL_ERR_SSL_FATAL_ALERT_MESSAGE:              printf("A fatal alert message was received from our peer.\n");	break;
	case POLARSSL_ERR_SSL_PEER_VERIFY_FAILED:               printf("Verification of our peer failed.\n");	break;
	case POLARSSL_ERR_SSL_PEER_CLOSE_NOTIFY:                printf("The peer notified us that the connection is going to be closed.\n");	break;
	case POLARSSL_ERR_SSL_BAD_HS_CLIENT_HELLO:              printf("Processing of the ClientHello handshake message failed.\n");	break;
	case POLARSSL_ERR_SSL_BAD_HS_SERVER_HELLO:              printf("Processing of the ServerHello handshake message failed.\n");	break;
	case POLARSSL_ERR_SSL_BAD_HS_CERTIFICATE:               printf("Processing of the Certificate handshake message failed.\n");	break;
	case POLARSSL_ERR_SSL_BAD_HS_CERTIFICATE_REQUEST:       printf("Processing of the CertificateRequest handshake message failed.\n");	break;
	case POLARSSL_ERR_SSL_BAD_HS_SERVER_KEY_EXCHANGE:       printf("Processing of the ServerKeyExchange handshake message failed.\n");	break;
	case POLARSSL_ERR_SSL_BAD_HS_SERVER_HELLO_DONE:         printf("Processing of the ServerHelloDone handshake message failed.\n");	break;
	case POLARSSL_ERR_SSL_BAD_HS_CLIENT_KEY_EXCHANGE:       printf("Processing of the ClientKeyExchange handshake message failed.\n");	break;
	case POLARSSL_ERR_SSL_BAD_HS_CLIENT_KEY_EXCHANGE_DHM_RP:printf("Processing of the ClientKeyExchange handshake message failed in DHM Read Public.\n");	break;
	case POLARSSL_ERR_SSL_BAD_HS_CLIENT_KEY_EXCHANGE_DHM_CS:printf("Processing of the ClientKeyExchange handshake message failed in DHM Calculate Secret.\n");	break;
	case POLARSSL_ERR_SSL_BAD_HS_CERTIFICATE_VERIFY:        printf("Processing of the CertificateVerify handshake message failed.\n");	break;
	case POLARSSL_ERR_SSL_BAD_HS_CHANGE_CIPHER_SPEC:        printf("Processing of the ChangeCipherSpec handshake message failed.\n");	break;
	case POLARSSL_ERR_SSL_BAD_HS_FINISHED:                  printf("Processing of the Finished handshake message failed.\n");	break;
	case POLARSSL_ERR_SSL_MALLOC_FAILED:                    printf("Memory allocation failed.\n");	break;

	default:												printf("SSL Error -0x%04x\n", -error);	break;
	}
}
)

