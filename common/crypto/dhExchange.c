//SERVER FILES
#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

#include <string.h>
#include <stdio.h>

#if defined LINUX
#include <stdint.h>
#else
#endif

#include <stdlib.h>

#include "polarssl/config.h"

#include "polarssl/net.h"
#include "polarssl/aes.h"
#include "polarssl/dhm.h"
#include "polarssl/rsa.h"
#include "polarssl/ssl.h"
#include "polarssl/sha1.h"
#include "polarssl/entropy.h"
#include "polarssl/ctr_drbg.h"
#include "polarssl/debug.h"
#include "dhExchange.h"
#include "debug.h"

#if 0
int find_DH_SecretKey( ssl_context *ssl)
{
	uint8_t *		kKey;
	size_t			kKeySize;
	int				mpiRet;

	DL(8);
	if ((kKeySize = mpi_size(&(ssl->dhm_ctx.K))) > 0) {
		DLX(6, printf("kKeySize = %d\n", kKeySize));
		kKey= (uint8_t *) calloc(kKeySize, sizeof(uint8_t));
	}
	//TODO: Add error code here
	DL(8);
	memcpy(kKey, &(ssl->dhm_ctx.K), kKeySize);
#ifdef SSL_DEBUG_MPI
	SSL_DEBUG_MPI( 3, "kKey:", (mpi *)kKey);
#endif
	DLX(4, printf( "kKey now has Length of %d.\n", kKeySize));

	DLX(4, printf("Freeing kKey for now...\n") );
	if (kKey != NULL)
		free( kKey);

	return mpiRet;
}
#endif

/*!
 * @brief dhHandshake performs a Diffie Hellman key exchange
 * @param ssl SSL context
 * @return
 * @retval 0 Error, shared secret not created
 */
dhm_context *dhHandshake(ssl_context *ssl )
{
	if ( ssl->endpoint == SSL_IS_CLIENT )
	{
		DLX(6, printf( "Performing dhClientExchange\n"));
		return (dhClientExchange( ssl ));
	}
	else
	{
		DLX(6, printf( "Performing dhServerExchange\n"));
		return (dhServerExchange( ssl ));
	}
}


//Will use ssl_write and ssl_read since connection already exists.
////#define SERVER_PORT 11999
//#define PLAINTEXT "==Hello there!=="


//Will use ssl_write and ssl_read since connection already exists.
//#define SERVER_NAME "localhost"
//#define SERVER_PORT 11999



/////////////////////////////////////////////////////////////////////
//  CLIENT PORTION
//
/*
 *  Diffie-Hellman-Merkle key exchange (client side)
 *
 *  Copyright (C) 2006-2011, Brainspark B.V.
 *
 *  This file is part of PolarSSL (http://www.polarssl.org)
 *
 *  Lead Maintainer: Paul Bakker <polarssl_maintainer at polarssl.org>
 *
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#if !defined(POLARSSL_AES_C) || !defined(POLARSSL_DHM_C) ||     \
    !defined(POLARSSL_ENTROPY_C) || !defined(POLARSSL_NET_C) ||  \
    !defined(POLARSSL_RSA_C) || !defined(POLARSSL_SHA1_C) ||    \
    !defined(POLARSSL_FS_IO) || !defined(POLARSSL_CTR_DRBG_C)
int dhClientExchange( ssl_context *ssl )
{
    printf("POLARSSL_AES_C and/or POLARSSL_DHM_C and/or POLARSSL_ENTROPY_C "
           "and/or POLARSSL_NET_C and/or POLARSSL_RSA_C and/or "
           "POLARSSL_SHA1_C and/or POLARSSL_FS_IO and/or "
           "POLARSSL_CTR_DRBG_C not defined.\n");
    return( 0 );
}
#else

/*!	@brief Diffie-Hellman-Merkle Client Exchange
 *
 * @param ssl ssl context
 * @returns generated shared secret
 * @retval < 0 is an error
 */

dhm_context *dhClientExchange( ssl_context *ssl )
{
    int 				ret;
    size_t 				n, buflen;
    int 				server_fd = -1;

    unsigned char		*p, *end;
    unsigned char		buf[1024];

    if ( (dhm = calloc(1, sizeof(dhm_context)) ) == NULL)
    		return NULL;

	if (!rng_initialized) {	// Verify that RNG has been initialized
		if (rng_init() < 0) {
			DLX(4, printf( "Failed to initialize random number generator\n"));
			return 0;
		}
	}

	// First get the buffer length

    DLX(6, printf("Receiving the server's DH parameters\n"));

    memset(buf, 0, sizeof(buf));
    if (( ret = ssl_read( ssl, buf, 2 )) != 2 ) {
    	DLX(4, printf("ssl_read() failed to receive buffer length: "); print_ssl_error(ret));
		goto exit;
	}

    buflen = ( buf[0] << 8 ) | buf[1];
    DLX(6, printf("Waiting to receive %u bytes\n", (unsigned int)buflen));
    if( buflen < 1 || buflen > sizeof( buf ) ) {
        DLX(4, printf("Received invalid buffer length: %u\n", (unsigned int)buflen));
        goto exit;
    }

	// Get the DHM parameters: P, G and Ys = G^Xs mod P

    memset(buf, 0, sizeof(buf));
    n = 0;
    do {
    	ret = ssl_read( ssl, buf+n, buflen-n );
    	if (ret == 0)
    		break;
    	if (ret < 0) {
    		DLX(4, printf("ssl_read() failed: "); print_ssl_error(ret));
    		continue;
    	} else
    		DLX(8, printf("Read %d bytes\n", ret));
    	n += ret;
    } while (n < buflen);

	DPB(8, "Received buffer follows:", buf, buflen);
    p = buf, end = buf + buflen;

    DLX(6, printf("Received DHM params: %u bytes\n", (unsigned int)n));
    if( ( ret = dhm_read_params( dhm, &p, end ) ) != 0 )  {
        DLX(4, printf("dhm_read_params() failed: "); print_ssl_error(ret));
        goto exit;
    }

    if( dhm->len < 64 || dhm->len > 256 ) {
        DLX(4, printf("Invalid DHM modulus size\n"));
        goto exit;
    }

#if 0
    // Verify the server's RSA signature matches the SHA-1 hash of (P, G, Ys)

    if ( (n = (size_t) (end - p)) != rsa.len )
    {
        DLX(4, printf("Invalid RSA length: %i, should be: %i\n", n, rsa.len));
        goto exit;
    }

    sha1(buf, (int)( p-2-buf ), hash );

    if ( (ret = rsa_pkcs1_verify( &rsa, RSA_PUBLIC, SIG_RSA_SHA1, 0, hash, p ) ) != 0 )
    {
        DLX(4, printf("rsa_pkcs1_verify() failed: "); print_ssl_error(ret));
        goto exit;
    }
#endif

	// Generate public value and send to server: Yc = G ^ Xc mod P
    DL(8);
    buflen = dhm->len;
    if (( ret = dhm_make_public( dhm, AES_KEY_SIZE, buf, buflen, ctr_drbg_random, &ctr_drbg)) != 0 ) {
        DLX(4, printf("dhm_make_public() failed: "); print_ssl_error(ret));
        goto exit;
    }
    DLX(6, printf("Sending public value to server, length = %u\n", (unsigned int)buflen));
    DPB(8, "DHM Parameters:", buf, buflen);
    n = 0;
    do {
    	ret = ssl_write(ssl, buf+n, buflen-n);
    	if (ret < 0) {
    		DLX(4, printf("ssl_write() failed: "); print_ssl_error(ret));
    		continue;
    	} else
    		DLX(8, printf("Wrote %d bytes\n", ret));
    	n += ret;
    } while (n < buflen);

	// Derive the shared secret: K = Ys ^ Xc mod P
    n = dhm->len;
    if( (ret = dhm_calc_secret(dhm, buf, &n ) ) != 0) {
        DLX(4, printf( "dhm_calc_secret() failed: "); print_ssl_error(ret));
        goto exit;
    }
    DPB(6, "Shared Secret:", buf, n);

    return(dhm);

exit:
    net_close( server_fd );
    dhm_free( dhm );
    return(NULL);
}

#endif /* POLARSSL_AES_C && POLARSSL_DHM_C && POLARSSL_ENTROPY_C &&
          POLARSSL_NET_C && POLARSSL_RSA_C && POLARSSL_SHA1_C && 
          POLARSSL_FS_IO && POLARSSL_CTR_DRBG_C */

//
//  END OF CLIENT PORTION
/////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////
//  SERVER PORTION
//

/*
 *  Diffie-Hellman-Merkle key exchange (server side)
 *
 *  Copyright (C) 2006-2011, Brainspark B.V.
 *
 *  This file is part of PolarSSL (http://www.polarssl.org)
 *  Lead Maintainer: Paul Bakker <polarssl_maintainer at polarssl.org>
 *
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#if !defined(POLARSSL_AES_C) || !defined(POLARSSL_DHM_C) ||     \
    !defined(POLARSSL_ENTROPY_C) || !defined(POLARSSL_NET_C) ||  \
    !defined(POLARSSL_RSA_C) || !defined(POLARSSL_SHA1_C) ||    \
    !defined(POLARSSL_FS_IO) || !defined(POLARSSL_CTR_DRBG_C)
int dhServerExchange( ssl_context *ssl )
{
    printf("POLARSSL_AES_C and/or POLARSSL_DHM_C and/or POLARSSL_ENTROPY_C "
           "and/or POLARSSL_NET_C and/or POLARSSL_RSA_C and/or "
           "POLARSSL_SHA1_C and/or POLARSSL_FS_IO and/or "
           "POLARSSL_CTR_DBRG_C not defined.\n");
    return( 0 );
}
#else
/*!	@brief Diffie-Hellman-Merkle Server Exchange
 *
 * @param ssl ssl context
 * @returns generated shared secret
 * @retval < 0 is an error
 */
dhm_context *dhServerExchange( ssl_context *ssl )
{
    int			ret;
    size_t		n, buflen;

	unsigned char	buf[1024];
	unsigned char	buf2[2];
	unsigned char	hash[20];
    rsa_context		rsa;

    if (dhm == NULL) {
		if ( (dhm = calloc(1, sizeof(dhm_context)) ) == NULL)
				return NULL;
    }

	if (!rng_initialized) {	// Verify that RNG has been initialized
		if (rng_init() < 0) {
			DLX(4, printf( "Failed to initialize random number generator\n"));
			return 0;
		}
	}

	rsa_init( &rsa, RSA_PKCS_V15, 0 );
	if ((ret = x509parse_keyfile(&rsa, "server.key", NULL)) != 0) {
		printf("Could not read server key\n");
		goto exit;
	}
	if (rsa.len == 0) {
		DLX(4, printf("ERROR: RSA Length = 0\n"));
		goto exit;
	}

	// Get the DHM modulus and generator
    DLX(6, printf("Reading P\n"));
    if ((ret = mpi_read_string(&dhm->P, 16, POLARSSL_DHM_RFC3526_MODP_2048_P)) != 0 ) {
    	DLX(4, printf("mpi_read_string() failed: "); print_ssl_error(ret));
    	goto exit;
    }
    DLX(6, printf("Reading G\n"));
    if ((ret = mpi_read_string(&dhm->G, 16, POLARSSL_DHM_RFC3526_MODP_2048_G)) != 0 ) {
    	DLX(4, printf("mpi_read_string() failed: "); print_ssl_error(ret));
    	goto exit;
    }

	// Setup the DH parameters (P,G,Ys)

    DLX(6, printf("Creating the server's DH parameters\n"));
    memset( buf, 0, sizeof(buf));	// Clear buffer
    if (( ret = dhm_make_params( dhm, AES_KEY_SIZE, buf, &n, ctr_drbg_random, &ctr_drbg)) != 0 ) {
        DLX(4, printf("dhm_make_params() failed: "); print_ssl_error(ret));
        goto exit;
    }
    DLX(6, printf("buf: %p, n: %u\n", buf, (unsigned int)n));

	// Sign the parameters and send them
    // NOTE: The client side does not currently verify the hash, but the dhm_read_params() function
    //       used on the client side expects the hash to be present and fails otherwise.
    sha1( buf, n, hash );
    DL(8);
    buf[n++] = (unsigned char)( rsa.len >> 8 );
    buf[n++] = (unsigned char)( rsa.len      );
    DLX(6, printf("rsa.len = %u\n", (unsigned int)rsa.len));
    DPB(6, "hash:", hash, sizeof(hash));
    DPB(6, "buf:", buf, n);
    DLX(6, printf("rsa.padding = %x\n", rsa.padding));

    if ( (ret = rsa_pkcs1_sign(&rsa, NULL, NULL, RSA_PRIVATE, SIG_RSA_SHA1, 0, hash, buf+n) ) != 0) {
        DLX(4, printf("rsa_pcks1_sign() failed: "); print_ssl_error(ret));
    	goto exit;
    }
	buflen = n + rsa.len;

    DPB(6, "buf:", buf, buflen);
    buf2[0] = (unsigned char)( buflen >> 8 );
    buf2[1] = (unsigned char)( buflen      );

    // Send the buffer length to the client
    DLX(6, printf("Sending buffer of length: %u\n", (unsigned int)buflen));
	if ( (ret = ssl_write( ssl, buf2, 2)) != 2) {
		DLX(4, printf("ssl_write() failed to send buffer length to client: "); print_ssl_error(ret));
		goto exit;
	}

	// Send the buffer to the client
    n = 0;
    do {
    	ret = ssl_write( ssl, buf+n, buflen-n );
    	if (ret < 0) {
    		DLX(4, printf("ssl_write() error: -%x\n", -ret));
    		continue;
    	} else
    		DLX(8, printf("Wrote %d bytes\n", ret));
    	n += ret;
    } while (n < buflen);

	DPB(6, "Buffer sent follows:", buf, buflen);

	// Get the client's public value: Yc = G ^ Xc mod P

    DLX (5, printf("Receiving the client's public value\n"));

    memset(buf, 0, sizeof(buf));	// Clear buffer
    buflen = dhm->len;
    n = 0;
    do {
    	ret = ssl_read( ssl, buf+n, buflen-n );
    	if (ret == 0)	// EOF
    		break;
    	if (ret < 0) {
    		DLX(4, printf("ssl_read() failed: "); print_ssl_error(ret));
    		continue;
    	}else
    		DLX(6, printf("Read %d bytes\n", ret));
    	n += ret;
    } while (n < buflen);

    DL(8);
    if ((ret = dhm_read_public( dhm, buf, dhm->len )) != 0 ) {
		DLX(4, printf("dhm_read_public() failed: "); print_ssl_error(ret));
        goto exit;
    }

	// Derive the shared secret: K = Ys ^ Xc mod P
    DL(8);
    if( ( ret = dhm_calc_secret( dhm, buf, &n ) ) != 0 ) {
        DLX(4, printf("dhm_calc_secret() failed: "); print_ssl_error(ret));
        goto exit;
    }
    DPB(6, "Shared Secret:", buf, n);
    return(dhm);

exit:
    dhm_free( dhm );
    free(dhm);
    return(NULL);

}
#endif /* POLARSSL_AES_C && POLARSSL_DHM_C && POLARSSL_ENTROPY_C &&
          POLARSSL_NET_C && POLARSSL_RSA_C && POLARSSL_SHA1_C &&
          POLARSSL_FS_IO && POLARSSL_CTR_DRBG_C */

//
//  END OF SERVER PORTION
/////////////////////////////////////////////////////////////////////
