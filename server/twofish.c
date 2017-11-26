#include "proj_strings.h"
#include <string.h>
#include <unistd.h>

#if defined LINUX
#include <stdint.h>
#endif

#if defined SOLARIS
#include <sys/isa_defs.h>
#endif

#include <sys/param.h>
#include "twofish.h"
#include "port.h"

typedef unsigned char BYTE;
typedef	unsigned long DWORD;		/* 32-bit unsigned quantity */
/* $Id: twofish2.cc,v 1.3 2004/12/18 04:34:27 jleplast Exp $
 *
 * Copyright (C) 1997-2000 The Cryptix Foundation Limited.
 * All rights reserved.
 *
 * Use, modification, copying and distribution of this software is subject
 * the terms and conditions of the Cryptix General Licence. You should have
 * received a copy of the Cryptix General Licence along with this library;
 * if not, you can download a copy from http://www.cryptix.org/ .
 */

 /** Fixed 8x8 permutation S-boxes */
static unsigned char P[2][256] = 
    {
        {  // p0
            0xA9, 0x67, 0xB3, 0xE8,
            0x04, 0xFD, 0xA3, 0x76,
            0x9A, 0x92, 0x80, 0x78,
            0xE4, 0xDD, 0xD1, 0x38,
            0x0D, 0xC6, 0x35, 0x98,
            0x18, 0xF7, 0xEC, 0x6C,
            0x43, 0x75, 0x37, 0x26,
            0xFA, 0x13, 0x94, 0x48,
            0xF2, 0xD0, 0x8B, 0x30,
            0x84, 0x54, 0xDF, 0x23,
            0x19, 0x5B, 0x3D, 0x59,
            0xF3, 0xAE, 0xA2, 0x82,
            0x63, 0x01, 0x83, 0x2E,
            0xD9, 0x51, 0x9B, 0x7C,
            0xA6, 0xEB, 0xA5, 0xBE,
            0x16, 0x0C, 0xE3, 0x61,
            0xC0, 0x8C, 0x3A, 0xF5,
            0x73, 0x2C, 0x25, 0x0B,
            0xBB, 0x4E, 0x89, 0x6B,
            0x53, 0x6A, 0xB4, 0xF1,
            0xE1, 0xE6, 0xBD, 0x45,
            0xE2, 0xF4, 0xB6, 0x66,
            0xCC, 0x95, 0x03, 0x56,
            0xD4, 0x1C, 0x1E, 0xD7,
            0xFB, 0xC3, 0x8E, 0xB5,
            0xE9, 0xCF, 0xBF, 0xBA,
            0xEA, 0x77, 0x39, 0xAF,
            0x33, 0xC9, 0x62, 0x71,
            0x81, 0x79, 0x09, 0xAD,
            0x24, 0xCD, 0xF9, 0xD8,
            0xE5, 0xC5, 0xB9, 0x4D,
            0x44, 0x08, 0x86, 0xE7,
            0xA1, 0x1D, 0xAA, 0xED,
            0x06, 0x70, 0xB2, 0xD2,
            0x41, 0x7B, 0xA0, 0x11,
            0x31, 0xC2, 0x27, 0x90,
            0x20, 0xF6, 0x60, 0xFF,
            0x96, 0x5C, 0xB1, 0xAB,
            0x9E, 0x9C, 0x52, 0x1B,
            0x5F, 0x93, 0x0A, 0xEF,
            0x91, 0x85, 0x49, 0xEE,
            0x2D, 0x4F, 0x8F, 0x3B,
            0x47, 0x87, 0x6D, 0x46,
            0xD6, 0x3E, 0x69, 0x64,
            0x2A, 0xCE, 0xCB, 0x2F,
            0xFC, 0x97, 0x05, 0x7A,
            0xAC, 0x7F, 0xD5, 0x1A,
            0x4B, 0x0E, 0xA7, 0x5A,
            0x28, 0x14, 0x3F, 0x29,
            0x88, 0x3C, 0x4C, 0x02,
            0xB8, 0xDA, 0xB0, 0x17,
            0x55, 0x1F, 0x8A, 0x7D,
            0x57, 0xC7, 0x8D, 0x74,
            0xB7, 0xC4, 0x9F, 0x72,
            0x7E, 0x15, 0x22, 0x12,
            0x58, 0x07, 0x99, 0x34,
            0x6E, 0x50, 0xDE, 0x68,
            0x65, 0xBC, 0xDB, 0xF8,
            0xC8, 0xA8, 0x2B, 0x40,
            0xDC, 0xFE, 0x32, 0xA4,
            0xCA, 0x10, 0x21, 0xF0,
            0xD3, 0x5D, 0x0F, 0x00,
            0x6F, 0x9D, 0x36, 0x42,
            0x4A, 0x5E, 0xC1, 0xE0
        },
        {  // p1
            0x75, 0xF3, 0xC6, 0xF4,
            0xDB, 0x7B, 0xFB, 0xC8,
            0x4A, 0xD3, 0xE6, 0x6B,
            0x45, 0x7D, 0xE8, 0x4B,
            0xD6, 0x32, 0xD8, 0xFD,
            0x37, 0x71, 0xF1, 0xE1,
            0x30, 0x0F, 0xF8, 0x1B,
            0x87, 0xFA, 0x06, 0x3F,
            0x5E, 0xBA, 0xAE, 0x5B,
            0x8A, 0x00, 0xBC, 0x9D,
            0x6D, 0xC1, 0xB1, 0x0E,
            0x80, 0x5D, 0xD2, 0xD5,
            0xA0, 0x84, 0x07, 0x14,
            0xB5, 0x90, 0x2C, 0xA3,
            0xB2, 0x73, 0x4C, 0x54,
            0x92, 0x74, 0x36, 0x51,
            0x38, 0xB0, 0xBD, 0x5A,
            0xFC, 0x60, 0x62, 0x96,
            0x6C, 0x42, 0xF7, 0x10,
            0x7C, 0x28, 0x27, 0x8C,
            0x13, 0x95, 0x9C, 0xC7,
            0x24, 0x46, 0x3B, 0x70,
            0xCA, 0xE3, 0x85, 0xCB,
            0x11, 0xD0, 0x93, 0xB8,
            0xA6, 0x83, 0x20, 0xFF,
            0x9F, 0x77, 0xC3, 0xCC,
            0x03, 0x6F, 0x08, 0xBF,
            0x40, 0xE7, 0x2B, 0xE2,
            0x79, 0x0C, 0xAA, 0x82,
            0x41, 0x3A, 0xEA, 0xB9,
            0xE4, 0x9A, 0xA4, 0x97,
            0x7E, 0xDA, 0x7A, 0x17,
            0x66, 0x94, 0xA1, 0x1D,
            0x3D, 0xF0, 0xDE, 0xB3,
            0x0B, 0x72, 0xA7, 0x1C,
            0xEF, 0xD1, 0x53, 0x3E,
            0x8F, 0x33, 0x26, 0x5F,
            0xEC, 0x76, 0x2A, 0x49,
            0x81, 0x88, 0xEE, 0x21,
            0xC4, 0x1A, 0xEB, 0xD9,
            0xC5, 0x39, 0x99, 0xCD,
            0xAD, 0x31, 0x8B, 0x01,
            0x18, 0x23, 0xDD, 0x1F,
            0x4E, 0x2D, 0xF9, 0x48,
            0x4F, 0xF2, 0x65, 0x8E,
            0x78, 0x5C, 0x58, 0x19,
            0x8D, 0xE5, 0x98, 0x57,
            0x67, 0x7F, 0x05, 0x64,
            0xAF, 0x63, 0xB6, 0xFE,
            0xF5, 0xB7, 0x3C, 0xA5,
            0xCE, 0xE9, 0x68, 0x44,
            0xE0, 0x4D, 0x43, 0x69,
            0x29, 0x2E, 0xAC, 0x15,
            0x59, 0xA8, 0x0A, 0x9E,
            0x6E, 0x47, 0xDF, 0x34,
            0x35, 0x6A, 0xCF, 0xDC,
            0x22, 0xC9, 0xC0, 0x9B,
            0x89, 0xD4, 0xED, 0xAB,
            0x12, 0xA2, 0x0D, 0x52,
            0xBB, 0x02, 0x2F, 0xA9,
            0xD7, 0x61, 0x1E, 0xB4,
            0x50, 0x04, 0xF6, 0xC2,
            0x16, 0x25, 0x86, 0x56,
            0x55, 0x09, 0xBE, 0x91
        }
};

/** MDS matrix */
static int MDS[4][256]; // blank final

static void precomputeMDSmatrix( void );
static void tf_flushOutput( struct tf_context *ctx, char* output, int size );
static void tf_qBlockPush( struct tf_context *ctx, char* p, char* c );
static void tf_qBlockPop( struct tf_context *ctx, char* p, char* c );
static void tf_qBlockFlush( struct tf_context *ctx );
static void tf_makeSubKeys( struct tf_context *ctx, char* k );

static int RS_MDS_Encode( int k0, int k1 );
static int F32( int k64Cnt, int x, int* k32 );
//static int Fe32( int* sBox, int x, int R );
static int Fe320( int* sBox, int x );
static int Fe323( int* sBox, int x );


void tf_setDecrypt( struct tf_context *ctx, bool d )
{ 
	ctx->decrypt = d;
	return;
}

void tf_setFp( struct tf_context *ctx, FILE* fp )
{
	ctx->fpout = fp;
	if ( fp != NULL )
		ctx->outputIsFile = true;
	else
		ctx->outputIsFile = false;

	return;
}

void tf_setOutputBuffer( struct tf_context *ctx, unsigned char* obuf )
{
	ctx->outputBuffer = obuf;

	if ( ctx->outputBuffer != NULL )
		ctx->outputIsBuffer = true;
	else
		ctx->outputIsBuffer = false;
	
	return;
}

void tf_setSocket( struct tf_context *ctx, int sfd )
{
	ctx->sockfd = sfd;

	if ( sfd != -1 )
		ctx->outputIsSocket = true;
	else
		ctx->outputIsSocket = false;

	return;
}

void tf_resetCBC( struct tf_context *ctx ) 
{
	ctx->qBlockDefined = false;
	return;
}

////////////////////////////////////////////////////////////////////
////////////////////// DEFINES /////////////////////////////////////
////////////////////////////////////////////////////////////////////

#define	LFSR1(x) ( ((x) >> 1)  ^ (((x) & 0x01) ?   MDS_GF_FDBK/2 : 0))
#define	LFSR2(x) ( ((x) >> 2)  ^ (((x) & 0x02) ?   MDS_GF_FDBK/2 : 0)  ^ (((x) & 0x01) ?   MDS_GF_FDBK/4 : 0))

#define	Mx_1(x) ((DWORD)  (x))		/* force result to dword so << will work */
#define	Mx_X(x) ((DWORD) ((x) ^            LFSR2(x)))	/* 5B */
#define	Mx_Y(x) ((DWORD) ((x) ^ LFSR1(x) ^ LFSR2(x)))	/* EF */
#define	RS_rem(x) { BYTE  b  = (BYTE) (x >> 24); DWORD g2 = ((b << 1) ^ ((b & 0x80) ? RS_GF_FDBK : 0 )) & 0xFF;		DWORD g3 = ((b >> 1) & 0x7F) ^ ((b & 1) ? RS_GF_FDBK >> 1 : 0 ) ^ g2 ; x = (x << 8) ^ (g3 << 24) ^ (g2 << 16) ^ (g3 << 8) ^ b;				 }
//#define	_b(x,N)	(((BYTE *)&x)[((N) & 3) ^ ADDR_XOR]) /* pick bytes out of a dword */
//#define	__b(x,N)	(((BYTE *)&x)[((N) & 3)]) /* pick bytes out of a dword */

#define b0(x)	__b(x,0)
#define b1(x)	__b(x,1)
#define b2(x)	__b(x,2)
#define b3(x)	__b(x,3)

uint8_t __b( uint32_t x, int n )
{
	n &= 3;
	while ( n-- > 0 )
	{
		x >>= 8;
	}
	return( uint8_t )x;
}


////////////////////// METHODS /////////////////////////////////////
////////////////////////////////////////////////////////////////////

static void precomputeMDSmatrix( void ) {
    // precompute the MDS matrix
    int m1[2];
    int mX[2];
    int mY[2];
    int i, j;
    for (i = 0; i < 256; i++) {
        j = P[0][i]       & 0xFF; // compute all the matrix elements
        m1[0] = j;
        mX[0] = Mx_X( j ) & 0xFF;
        mY[0] = Mx_Y( j ) & 0xFF;

        j = P[1][i]       & 0xFF;
        m1[1] = j;
        mX[1] = Mx_X( j ) & 0xFF;
        mY[1] = Mx_Y( j ) & 0xFF;

        MDS[0][i] = m1[P_00] <<  0 | // fill matrix w/ above elements
                    mX[P_00] <<  8 |
                    mY[P_00] << 16 |
                    mY[P_00] << 24;
        MDS[1][i] = mY[P_10] <<  0 |
                    mY[P_10] <<  8 |
                    mX[P_10] << 16 |
                    m1[P_10] << 24;
        MDS[2][i] = mX[P_20] <<  0 |
                    mY[P_20] <<  8 |
                    m1[P_20] << 16 |
                    mY[P_20] << 24;
        MDS[3][i] = mX[P_30] <<  0 |
                    m1[P_30] <<  8 |
                    mY[P_30] << 16 |
                    mX[P_30] << 24;
    }
}


// Constructor
//...........................................................................
void tf_init(struct tf_context *ctx, char* userkey, bool _decrypt, FILE* _fpout, unsigned char* _outputBuffer ) {
    ctx->decrypt   = _decrypt;
    ctx->fpout = _fpout;
    if ( ctx->fpout == NULL ) {
        ctx->outputIsFile = false;
    } else {
        ctx->outputIsFile = true;
    }
    ctx->outputBuffer = _outputBuffer;
    if ( ctx->outputBuffer == NULL ) {
        ctx->outputIsBuffer = false;
    } else {
        ctx->outputIsBuffer = true;
    }
    precomputeMDSmatrix();
    tf_makeSubKeys( ctx, userkey );
    ctx->qBlockDefined = false;
}



// Private methods
//...........................................................................

/**
 * Expand a user-supplied key material into a session key.
 *
 * @param key  The 64/128/192/256-bit user-key to use.
 * @return  This cipher's round keys.
 * @exception  InvalidKeyException  If the key is invalid.
 */
static void tf_makeSubKeys( struct tf_context *ctx, char* k ) {
    int length    = 32;
    int k64Cnt    = length / 8;
    int k32e[4]; // even 32-bit entities
    int k32o[4]; // odd 32-bit entities
    int sBoxKey[4];

    // split user key material into even and odd 32-bit entities and
    // compute S-box keys using (12, 8) Reed-Solomon code over GF(256)
    int i, j, offset = 0;
    for (i = 0, j = k64Cnt-1; i < 4 && offset < length; i++, j--) {
        k32e[i] = (k[offset] & 0xFF)       |
                  (k[offset+1] & 0xFF) <<  8 |
                  (k[offset+2] & 0xFF) << 16 |
                  (k[offset+3] & 0xFF) << 24;
		offset += 4;
        k32o[i] = (k[offset] & 0xFF)       |
                  (k[offset+1] & 0xFF) <<  8 |
                  (k[offset+2] & 0xFF) << 16 |
                  (k[offset+3] & 0xFF) << 24;
        offset += 4;
        sBoxKey[j] = RS_MDS_Encode( k32e[i], k32o[i] ); // reverse order
    }

    // compute the round decryption subkeys for PHT. these same subkeys
    // will be used in encryption but will be applied in reverse order.
    unsigned int A, B, q=0;
    i=0;
    while(i < TOTAL_SUBKEYS) {
        A = F32( k64Cnt, q, k32e ); // A uses even key entities
        q += SK_BUMP;

        B = F32( k64Cnt, q, k32o ); // B uses odd  key entities
        q += SK_BUMP;

        B = B << 8 | B >> 24;

        A += B;
        ctx->subKeys[i++] = A;           // combine with a PHT

        A += B;
        ctx->subKeys[i++] = A << SK_ROTL | A >> (32-SK_ROTL);
   }

    // fully expand the table for speed
    int k0 = sBoxKey[0];
    int k1 = sBoxKey[1];
    int k2 = sBoxKey[2];
    int k3 = sBoxKey[3];
    int b0, b1, b2, b3;
    for (i = 0; i < 256; i++) {
        b0 = b1 = b2 = b3 = i;
        switch (k64Cnt & 3) {
        case 1:
            ctx->sBox[      2*i  ] = MDS[0][(P[P_01][b0] & 0xFF) ^ b0(k0)];
            ctx->sBox[      2*i+1] = MDS[1][(P[P_11][b1] & 0xFF) ^ b1(k0)];
            ctx->sBox[0x200+2*i  ] = MDS[2][(P[P_21][b2] & 0xFF) ^ b2(k0)];
            ctx->sBox[0x200+2*i+1] = MDS[3][(P[P_31][b3] & 0xFF) ^ b3(k0)];
            break;
        case 0: // same as 4
            b0 = (P[P_04][b0] & 0xFF) ^ b0(k3);
            b1 = (P[P_14][b1] & 0xFF) ^ b1(k3);
            b2 = (P[P_24][b2] & 0xFF) ^ b2(k3);
            b3 = (P[P_34][b3] & 0xFF) ^ b3(k3);
        case 3:
            b0 = (P[P_03][b0] & 0xFF) ^ b0(k2);
            b1 = (P[P_13][b1] & 0xFF) ^ b1(k2);
            b2 = (P[P_23][b2] & 0xFF) ^ b2(k2);
            b3 = (P[P_33][b3] & 0xFF) ^ b3(k2);
        case 2: // 128-bit keys
            ctx->sBox[      2*i  ] = MDS[0][
                (P[P_01][(P[P_02][b0] & 0xFF) ^ b0(k1)] & 0xFF) ^ b0(k0)];

            ctx->sBox[      2*i+1] = MDS[1][
                (P[P_11][(P[P_12][b1] & 0xFF) ^ b1(k1)] & 0xFF) ^ b1(k0)];

            ctx->sBox[0x200+2*i  ] = MDS[2][
                (P[P_21][(P[P_22][b2] & 0xFF) ^ b2(k1)] & 0xFF) ^ b2(k0)];

            ctx->sBox[0x200+2*i+1] = MDS[3][
                (P[P_31][(P[P_32][b3] & 0xFF) ^ b3(k1)] & 0xFF) ^ b3(k0)];
        }
    }

    // swap input and output whitening keys when decrypting
    if( ctx->decrypt ) {
        for( i=0; i<4; i++ ) {
            int t        = ctx->subKeys[i];
            ctx->subKeys[i]   = ctx->subKeys[i+4];
            ctx->subKeys[i+4] = t;
        }
    }
}

//
//  write output to all active output areas
//
static void tf_flushOutput( struct tf_context *ctx, char* b, int len ) {

    if ( ctx->outputIsSocket ) {
       // int wret = write( sockfd, b, len );
        (void) write( ctx->sockfd, b, len );
    }
	int i;
    for ( i = 0; i < len; i++, b++ ) {
        if ( ctx->outputIsFile ) {
            fputc( *b, ctx->fpout );
        }
        if ( ctx->outputIsBuffer ) {
            *(ctx->outputBuffer) = *b;
            ctx->outputBuffer++;
        }
    }
}


/**
 * Encrypt or decrypt exactly one block of plaintext in CBC mode.  
 * Use "ciphertext stealing" technique described on pg. 196
 * of "Applied Cryptography" to encrypt the final partial
 * (i.e. <16 byte) block if necessary.
 *
 * Note: the "ciphertext stealing" requires we read ahead and have
 * special handling for the last two blocks.  Because of this, the
 * output from the TwoFish algorithm is handled internally here. 
 * It would be better to have a higher level handle this as well as
 * CBC mode.  Unfortunately, I've mixed the two together, which is
 * pretty crappy... The Java version separates these out correctly.
 *
 * @param in   The plaintext.
 * @param out  The ciphertext
 * @param size how much to encrypt
 * @return none
 */
void tf_blockCrypt( struct tf_context *ctx, char* in, char* out, int size ) {
	int i;
    // here is where we implement CBC mode and cipher block stealing
    if ( size == 16 ) {
        // if we are encrypting, CBC means we XOR the plain text block with the
        // previous cipher text block before encrypting
        if ( !ctx->decrypt && ctx->qBlockDefined ) {
            char* scanner = in;
            for ( i = 0; i < 16; i++, scanner++ ) {
                *scanner = *scanner ^ ctx->qBlockCrypt[i];
            }
        }

        // TwoFish block level encryption or decryption
        tf_blockCrypt16( ctx, in, out );

        // if we are decrypting, CBC means we XOR the result of the decryption
        // with the previous ciper text block to get the resulting plain text
        if ( ctx->decrypt && ctx->qBlockDefined ) {
            char* scanner = out;
            for ( i = 0; i < 16; i++, scanner++ ) {
                *scanner = *scanner ^ ctx->qBlockPlain[i];
            }
        }

        // save the input and output blocks, since CBC needs these for XOR
        // operations
        tf_qBlockPush( ctx, in, out );
    } else {
        // cipher block stealing, we are at Pn,
        // but since Cn-1 must now be replaced with CnC'
        // we pop it off, and recalculate Cn-1
        //
        char PnMinusOne[16];
        char CnMinusOne[16];
        if ( ctx->decrypt ) {
            // We are on an odd block, and had to do cipher block stealing,
            // so the PnMinusOne has to be derived differently.
            tf_qBlockPop( ctx, &CnMinusOne[0], &PnMinusOne[0] );

            // First we decrypt it into CBC and C'
            char CBCplusCprime[16];
            tf_blockCrypt16( ctx, &CnMinusOne[0], &CBCplusCprime[0] );

            // we then xor the first few bytes with the "in" bytes (Cn)
            // to recover Pn, which we put in out
            char* scanner = in;
            char* outScanner = out;
            for ( i = 0; i < size; i++, scanner++, outScanner++ ) {
                *outScanner = *scanner ^ CBCplusCprime[i];
            }

            // We now recover the original CnMinusOne, which consists of
            // the first "size" bytes of "in" data, followed by the
            // "Cprime" portion of CBCplusCprime
            scanner = in;
            for ( i = 0; i < size; i++, scanner++ ) {
                CnMinusOne[i] = *scanner;
            }
            for ( i = size; i < 16; i++ ) {
                CnMinusOne[i] = CBCplusCprime[i];
            }
            // we now decrypt CnMinusOne to get PnMinusOne xored with Cn-2
            tf_blockCrypt16( ctx, &CnMinusOne[0], &PnMinusOne[0] );

            for ( i = 0; i < 16; i++ ) {
                PnMinusOne[i] = PnMinusOne[i] ^ ctx->prevCipher[i];
            }

            // So at this point, out has PnMinusOne
            tf_qBlockPush( ctx, &CnMinusOne[0], &PnMinusOne[0] );
            tf_qBlockFlush( ctx );
            tf_flushOutput( ctx, out, size );
        } else {
            tf_qBlockPop( ctx, &PnMinusOne[0], &CnMinusOne[0] );
			char Pn[16];
			memset( &Pn[0], 0, 16 );
			memcpy( &Pn[0], in, size );
			for ( i = 0; i < 16; i++ ) {
				Pn[i] = CnMinusOne[i] ^ Pn[i];
			}
			tf_blockCrypt16( ctx, &Pn[0], out );
			tf_qBlockPush( ctx, &Pn[0], out );  // now we officially have Cn-1
			tf_qBlockFlush( ctx );  // write them all out
			tf_flushOutput( ctx, &CnMinusOne[0], size );  // old Cn-1 becomes new partial Cn
		}
		ctx->qBlockDefined = false;
	}
}

static void tf_qBlockPush( struct tf_context *ctx, char* p, char* c ) {
	if ( ctx->qBlockDefined ) {
		tf_qBlockFlush( ctx );
	}
	memcpy( &(ctx->prevCipher[0]), &(ctx->qBlockPlain[0]), 16 );
	memcpy( &(ctx->qBlockPlain[0]), p, 16 );
	memcpy( &(ctx->qBlockCrypt[0]), c, 16 );
	ctx->qBlockDefined = true;
}

static void tf_qBlockPop( struct tf_context *ctx, char* p, char* c ) {
	memcpy( p, &(ctx->qBlockPlain[0]), 16 );
	memcpy( c, &(ctx->qBlockCrypt[0]), 16 );
	ctx->qBlockDefined = false;
}

//
//  flush a complete block to all active output areas
//  this occurs when we know the block does not need to be
//  re-encrypted or re-decrypted.  The redoing of encryption
//  and decryption is necessary for cipher text stealing technique
//  and is done on the last complete block.
//
static void tf_qBlockFlush( struct tf_context *ctx ) {
    tf_flushOutput( ctx, &(ctx->qBlockCrypt[0]), 16 );
}

void tf_flush( struct tf_context *ctx ) {
    if ( ctx->qBlockDefined ) {
        tf_qBlockFlush( ctx );
    }
}

void tf_blockCrypt16( struct tf_context *ctx, char* in, char* out ) {

	int inOffset = 0;
	int outOffset = 0;
    unsigned int x0 = (in[inOffset] & 0xFF)       |
             (in[inOffset+1] & 0xFF) <<  8 |
             (in[inOffset+2] & 0xFF) << 16 |
             (in[inOffset+3] & 0xFF) << 24;
	inOffset += 4;
    unsigned int x1 = (in[inOffset] & 0xFF)       |
             (in[inOffset+1] & 0xFF) <<  8 |
             (in[inOffset+2] & 0xFF) << 16 |
             (in[inOffset+3] & 0xFF) << 24;
	inOffset += 4;
    unsigned int x2 = (in[inOffset] & 0xFF)       |
             (in[inOffset+1] & 0xFF) <<  8 |
             (in[inOffset+2] & 0xFF) << 16 |
             (in[inOffset+3] & 0xFF) << 24;
	inOffset += 4;
    unsigned int x3 = (in[inOffset] & 0xFF)       |
             (in[inOffset+1] & 0xFF) <<  8 |
             (in[inOffset+2] & 0xFF) << 16 |
             (in[inOffset+3] & 0xFF) << 24;

/*	unsigned int x0;
	unsigned int x1;
	unsigned int x2;
	unsigned int x3;
	in += inOffset;
	memcpy( &x0, in, 4 );
	in += 4;
	memcpy( &x1, in, 4 );
	in += 4;
	memcpy( &x2, in, 4 );
	in += 4;
	memcpy( &x3, in, 4 );*/
    x0 ^= ctx->subKeys[0];
    x1 ^= ctx->subKeys[1];
    x2 ^= ctx->subKeys[2];
    x3 ^= ctx->subKeys[3];

    int k, t0, t1;
	int R;
    if ( ctx->decrypt ) {
        k = 39;
        for ( R = 0; R < ROUNDS; R += 2) {
            t0 = Fe320( ctx->sBox, x0 );
            t1 = Fe323( ctx->sBox, x1 );
            x3 ^= t0 + (t1<<1) + ctx->subKeys[k--];
            x3  = x3 >> 1 | x3 << 31;
            x2  = x2 << 1 | x2 >> 31;
            x2 ^= t0 + t1 + ctx->subKeys[k--]; 

            t0 = Fe320( ctx->sBox, x2 );
            t1 = Fe323( ctx->sBox, x3 );
            x1 ^= t0 + (t1<<1) + ctx->subKeys[k--]; 
            x1  = x1 >> 1 | x1 << 31;
            x0  = x0 << 1 | x0 >> 31;
            x0 ^= t0 + t1 + ctx->subKeys[k--];
        }
    } else {
        k = 8;
        for ( R = 0; R < ROUNDS; R += 2) {
            t0 = Fe320( ctx->sBox, x0 );
            t1 = Fe323( ctx->sBox, x1 );
            x2 ^= t0 + t1 + ctx->subKeys[k++]; 
            x2  = x2 >> 1 | x2 << 31;
            x3  = x3 << 1 | x3 >> 31;
            x3 ^= t0 + (t1<<1) + ctx->subKeys[k++];

            t0 = Fe320( ctx->sBox, x2 );
            t1 = Fe323( ctx->sBox, x3 );
            x0 ^= t0 + t1 + ctx->subKeys[k++];
            x0  = x0 >> 1 | x0 << 31;
            x1  = x1 << 1 | x1 >> 31;
            x1 ^= t0 + (t1<<1) + ctx->subKeys[k++];
        }
    }

    x2 ^= ctx->subKeys[4];
    x3 ^= ctx->subKeys[5];
    x0 ^= ctx->subKeys[6];
    x1 ^= ctx->subKeys[7];

    out += outOffset;
	/*memcpy( out, &x2, 4 );
	out += 4;
	memcpy( out, &x3, 4 );
	out += 4;
	memcpy( out, &x0, 4 );
	out += 4;
	memcpy( out, &x1, 4 );*/
	
    *out++ = (x2       );
    *out++ = (x2 >>  8);
    *out++ = (x2 >> 16);
    *out++ = (x2 >> 24);

    *out++ = (x3       );
    *out++ = (x3 >>  8);
    *out++ = (x3 >> 16);
    *out++ = (x3 >> 24);

    *out++ = (x0       );
    *out++ = (x0 >>  8);
    *out++ = (x0 >> 16);
    *out++ = (x0 >> 24);

    *out++ = (x1       );
    *out++ = (x1 >>  8);
    *out++ = (x1 >> 16);
    *out++ = (x1 >> 24);
}


/**
 * Use (12, 8) Reed-Solomon code over GF(256) to produce a key S-box
 * 32-bit entity from two key material 32-bit entities.
 *
 * @param  k0  1st 32-bit entity.
 * @param  k1  2nd 32-bit entity.
 * @return  Remainder polynomial generated using RS code
 */
int RS_MDS_Encode( int k0, int k1 ) {
    int r = k1;
	int i;
    for ( i = 0; i < 4; i++) // shift 1 byte at a time
        RS_rem( r );

    r ^= k0;
    for ( i = 0; i < 4; i++)
        RS_rem( r );

    return r;
}


int F32( int k64Cnt, int x, int* k32 ) {
    int b0 = b0(x);
    int b1 = b1(x);
    int b2 = b2(x);
    int b3 = b3(x);
    int k0 = k32[0];
    int k1 = k32[1];
    int k2 = k32[2];
    int k3 = k32[3];

    int result = 0;
    switch (k64Cnt & 3) {
    case 1:
        result =
              MDS[0][(P[P_01][b0] & 0xFF) ^ b0(k0)] ^
              MDS[1][(P[P_11][b1] & 0xFF) ^ b1(k0)] ^
              MDS[2][(P[P_21][b2] & 0xFF) ^ b2(k0)] ^
              MDS[3][(P[P_31][b3] & 0xFF) ^ b3(k0)];
        break;
    case 0:  // same as 4
        b0 = (P[P_04][b0] & 0xFF) ^ b0(k3);
        b1 = (P[P_14][b1] & 0xFF) ^ b1(k3);
        b2 = (P[P_24][b2] & 0xFF) ^ b2(k3);
        b3 = (P[P_34][b3] & 0xFF) ^ b3(k3);

    case 3:
        b0 = (P[P_03][b0] & 0xFF) ^ b0(k2);
        b1 = (P[P_13][b1] & 0xFF) ^ b1(k2);
        b2 = (P[P_23][b2] & 0xFF) ^ b2(k2);
        b3 = (P[P_33][b3] & 0xFF) ^ b3(k2);
    case 2:   
		// 128-bit keys (optimize for this case)
        result =
              MDS[0][(P[P_01][(P[P_02][b0] & 0xFF) ^ b0(k1)] & 0xFF) ^ b0(k0)] ^
              MDS[1][(P[P_11][(P[P_12][b1] & 0xFF) ^ b1(k1)] & 0xFF) ^ b1(k0)] ^
              MDS[2][(P[P_21][(P[P_22][b2] & 0xFF) ^ b2(k1)] & 0xFF) ^ b2(k0)] ^
              MDS[3][(P[P_31][(P[P_32][b3] & 0xFF) ^ b3(k1)] & 0xFF) ^ b3(k0)];
        break;
    }
    return result;
}

static int Fe320( int* sBox, int x ) {
    return sBox[            b0(x) << 1      ] ^
           sBox[ (         (b1(x) << 1) ) | ( 1 )] ^
           sBox[   0x200 + (b2(x) << 1)    ] ^
           sBox[ ( 0x200 + (b3(x) << 1) ) | ( 1 )]  ;
}

static int Fe323( int* sBox, int x ) {
    return sBox[           (b3(x) << 1)    ] ^
           sBox[ (         (b0(x) << 1) ) | ( 1 )] ^
           sBox[   0x200 + (b1(x) << 1)    ] ^
           sBox[ ( 0x200 + (b2(x) << 1) ) | ( 1 ) ];
}

static char key[32];
char* generateKey( char* s ) {
	int i;
    int sIdx = 0;
    for ( i = 0; i < 32; i++ ) {
        char sval = *( s + sIdx );
        if (( sval >= '0' ) && ( sval <= '9' )) {
            key[i] = sval;
        } else if (( sval >= 'a' ) && ( sval <= 'f' )) {
            key[i] = sval;
        } else {
            int q = sval%16;
            if ( q < 10 ) {
                key[i] = ('0' + q);
            } else {
                key[i] = ('a' + q - 10);
            }
        }
        sIdx++;
        if ( *( s + sIdx ) == 0 ) {
            sIdx = 0;
        }
    }
    return( &key[0] );
}


void tf_encryptAscii( struct tf_context *ctx, char* in, char* out, int outBufferSize ) {
	int i;
    tf_setDecrypt( ctx, false );
    tf_resetCBC( ctx );

    unsigned char byteBuf[200];
    //char* originalOut = out;
       
    // encrypt one block at a time with twofish
    char inList[16];
    unsigned char outList[16];
    tf_setOutputBuffer( ctx, byteBuf );    

    int remaining = strlen( in );
    int len = remaining;
    int bidx = 0;

    while ( remaining > 0 ) {
        if ( remaining > 16 ) {
            memcpy( inList, in + bidx, 16 );
            tf_blockCrypt( ctx, inList, (char*)outList, 16 );
        } else {
            memcpy( inList, in + bidx, remaining );
            tf_blockCrypt( ctx, inList, (char*)outList, remaining );
        }
        bidx += 16;
        remaining -= 16;
    }
    tf_flush( ctx );

    // now do totally stupid ascii encoding of bytes
    if ( outBufferSize < len*3 ) {
//        printf( "Hey, outBufferSize is %d, but len*3 is %d\n", outBufferSize, len*3 );
        printf( (const char *)ccat_err, outBufferSize, len*3 );

    } else {
       for ( i = 0; i < len; i++ ) {
          sprintf( out, "%03d", byteBuf[i] );
          out += 3;
       }
    }
    tf_setOutputBuffer( ctx, NULL );    
}

void tf_decryptAscii( struct tf_context *ctx, char* in, char* out ) {
	int i;
    tf_setDecrypt( ctx, true );
    tf_resetCBC( ctx );
    tf_setOutputBuffer( ctx, (unsigned char*)out );

    int inLen = strlen( in );
    if (( *( in + inLen - 1 ) == '\n' ) ||
        ( *( in + inLen - 1 ) == '\r' )) {
        *( in + inLen - 1 ) = 0;
        inLen = strlen( in );
    }
    unsigned char byteBuf[200];
    int byteBufIdx = 0;

    // first convert ascii to bytes, placing in another buffer
    for ( i = 0; i < inLen; i += 3 ) {
       unsigned int x = 0;
       x = x * 10 + ( *in++ - '0' );
       x = x * 10 + ( *in++ - '0' );
       x = x * 10 + ( *in++ - '0' );
       byteBuf[ byteBufIdx++ ] = x;
       byteBuf[ byteBufIdx ] = 0;
    }
       
    // then run it through twofish placing result into command buffer
    char inList[16];
    unsigned char outList[16];
    int remaining = byteBufIdx;
    *( out + byteBufIdx ) = 0;
    int bidx = 0;
    while ( remaining > 0 ) {
        if ( remaining > 16 ) {
            memcpy( inList, &byteBuf[bidx], 16 );
            tf_blockCrypt( ctx, inList, (char*)outList, 16 );
        } else {
            memcpy( inList, &byteBuf[bidx], remaining );
            tf_blockCrypt( ctx, inList, (char*)outList, remaining );
        }
        bidx += 16;
        remaining -= 16;
    }
    tf_flush( ctx );
    tf_setOutputBuffer( ctx, NULL );    
    *( out + byteBufIdx ) = 0;
}

