#include "crypto.h"

#define SKEY	"server.key"	// Server's RSA key file

#define POLARSSL_DHM_RFC3526_MODP_2048_P               \
    "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1" \
    "29024E088A67CC74020BBEA63B139B22514A08798E3404DD" \
    "EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245" \
    "E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED" \
    "EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3D" \
    "C2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F" \
    "83655D23DCA3AD961C62F356208552BB9ED529077096966D" \
    "670C354E4ABC9804F1746C08CA18217C32905E462E36CE3B" \
    "E39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9" \
    "DE2BCBF6955817183995497CEA956AE515D2261898FA0510" \
    "15728E5A8AACAA68FFFFFFFFFFFFFFFF\0"

#define POLARSSL_DHM_RFC3526_MODP_2048_G          "02\0"


int find_DH_SecretKey( ssl_context *ssl);

//SSL has already performed a Diffie-Hellman Key exchange and has it's own
// internal dhm_context witha secret key.  We need to create another
// dhm_context and have it perform another Diffie_Hellman Key Exchange to
// generate another new secret Key which will be used as the key for aes
// encryption.
dhm_context *dhHandshake( ssl_context *ssl );
dhm_context *dhClientExchange( ssl_context *ssl );
dhm_context *dhServerExchange( ssl_context *ssl );
