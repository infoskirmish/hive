#include <stdio.h>


class TwoFish 
{

// Constants
//...........................................................................

    enum {
        BLOCK_SIZE = 16, // bytes in a data-block
        ROUNDS     = 16,
        TOTAL_SUBKEYS = 4 + 4 + 2*ROUNDS,
        SK_BUMP = 0x01010101,
        SK_ROTL = 9,
        P_00 = 1,
        P_01 = 0,
        P_02 = 0,
        P_03 = P_01 ^ 1,
        P_04 = 1,
        P_10 = 0,
        P_11 = 0,
        P_12 = 1,
        P_13 = P_11 ^ 1,
        P_14 = 0,
        P_20 = 1,
        P_21 = 1,
        P_22 = 0,
        P_23 = P_21 ^ 1,
        P_24 = 0,
        P_30 = 0,
        P_31 = 1,
        P_32 = 1,
        P_33 = P_31 ^ 1,
        P_34 = 1,
        GF256_FDBK =   0x169,
        GF256_FDBK_2 = 0x169 / 2,
        GF256_FDBK_4 = 0x169 / 4,
        RS_GF_FDBK = 0x14D, // field generator
        MDS_GF_FDBK = 0x169	/* primitive polynomial for GF(256)*/
    };


// Static code - to intialise the MDS matrix
//...........................................................................
private:
    void precomputeMDSmatrix();


// Instance variables
//...........................................................................

    /** Encrypt (false) or decrypt mode (true) */
    bool decrypt;
    bool outputIsFile;
    bool outputIsBuffer;
    bool outputIsSocket;


    /** Key dependent S-box */
    int sBox[4 * 256];


    /** Subkeys */
    int subKeys[TOTAL_SUBKEYS];

    // output areas
    FILE* fpout;
    unsigned char* outputBuffer;
    int sockfd;


public:
// Constructor
//...........................................................................

    TwoFish( char* userkey, bool _decrypt, FILE* fpout, unsigned char* outbuf );
    void setDecrypt( bool d ) { decrypt = d; }
    void setFp( FILE* fp ) { fpout = fp; if ( fp != NULL ) outputIsFile = true; else outputIsFile = false; }
    void setOutputBuffer( unsigned char* obuf ) { outputBuffer = obuf; if ( outputBuffer != NULL ) outputIsBuffer = true; else outputIsBuffer = false; }
    void setSocket( int sfd ) { sockfd = sfd; if ( sfd != -1 ) outputIsSocket = true; else outputIsSocket = false; }
    void blockCrypt( char* in, char* out, int size );
    void blockCrypt16( char* in, char* out );
    void flush();
    void resetCBC() { qBlockDefined = false; }

private:
	void flushOutput( char* output, int size );
	void qBlockPush( char* p, char* c );
	void qBlockPop( char* p, char* c );
	void qBlockFlush();

	char qBlockPlain[16];
	char qBlockCrypt[16];
	char prevCipher[16];
	bool qBlockDefined;
// Private methods
//...........................................................................

    void makeSubKeys( char* k );


// own methods
//...........................................................................

    int RS_MDS_Encode( int k0, int k1 );
    int F32( int k64Cnt, int x, int* k32 );
    int Fe32( int* sBox, int x, int R );
    int Fe320( int* sBox, int x );
    int Fe323( int* sBox, int x );
};
char* generateKey( char* kstr );


class AsciiTwofish {
public:
    AsciiTwofish( TwoFish* engine );
    void encryptAscii( char* in, char* out, int outBufferSize );
    void decryptAscii( char* in, char* out );
private:
    TwoFish* engine;
};
