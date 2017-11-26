#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "b64.h"

static void b64_decodeblock(unsigned char in[4], unsigned char out[3]);

static void b64_encodeblock(unsigned char in[3], unsigned char out[4], int len);
/*
 * Translation Table as described in RFC1113
 */
static const char cb64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
 * Translation Table to decode (created by author)
 */
static const char cd64[] = "|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

/*
** encodeblock
**
** encode 3 8-bit binary bytes as 4 '6-bit' characters
*/

void b64_encodeblock(unsigned char in[3], unsigned char out[4], int len)
{
	out[0] = cb64[in[0] >> 2];
	out[1] = cb64[((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4)];
	out[2] = (unsigned char) (len > 1 ? cb64[((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6)] : '=');
	out[3] = (unsigned char) (len > 2 ? cb64[in[2] & 0x3f] : '=');
}

/*
** decodeblock
**
** decode 4 '6-bit' characters into 3 8-bit binary bytes
*/

void b64_decodeblock(unsigned char in[4], unsigned char out[3])
{
	out[0] = (unsigned char) (in[0] << 2 | in[1] >> 4);
	out[1] = (unsigned char) (in[1] << 4 | in[2] >> 2);
	out[2] = (unsigned char) (((in[2] << 6) & 0xc0) | in[3]);
}

int b64_encode_message(const uint8_t * message, uint8_t * output, int message_length, int *output_length)
{
	unsigned char in[3], out[4];
	int i, block_length;
	int message_index = 0;
	int output_index = 0;

	if (message == NULL || output == NULL || message_length < 0) {
		return FAILURE;
	}

	while (message_index < message_length) {
		// one block
		for (i = 0, block_length = 0; i < 3; i++) {

			in[i] = message_index < message_length ? (unsigned char) message[message_index] : EOF;
			if (message_index < message_length) {
				message_index++;
				block_length++;
			} else {
				in[i] = 0;
			}
		}

		if (block_length) {
			b64_encodeblock(in, out, block_length);
			for (i = 0; i < 4; i++) {
				output[output_index++] = out[i];
			}
		}
	}

	*output_length = output_index;

	return SUCCESS;
}

int b64_decode_message(const uint8_t * message, uint8_t * output, int message_length, int *output_length)
{
	unsigned char in[4], out[3], v;
	int i, block_length;
	int message_index = 0;
	int output_index = 0;

	DLX(2, printf("IN DECODE MESSAGE, %s", message));
	DLX(2, printf("LENGTH, %d", message_length));

	if (message == NULL || output == NULL || message_length <= 0) {
		return FAILURE;
	}

	while (message_index != (message_length + 1)) {

		for (i = 0, block_length = 0; i < 4 && message_index != (message_length + 1); i++) {

			v = 0;
			while (message_index != (message_length + 1) && v == 0) {
				v = message_index < message_length ? (unsigned char) message[message_index] : EOF;
				message_index++;
				v = (unsigned char) ((v < 43 || v > 122) ? 0 : cd64[v - 43]);
				if (v) {
					v = (unsigned char) ((v == '$') ? 0 : v - 61);
				}
			}

			// one block
			if (message_index != (message_length + 1)) {

				block_length++;
				if (v) {
					in[i] = (unsigned char) (v - 1);
				}
			} else {
				in[i] = 0;
			}
		}

		if (block_length) {
			b64_decodeblock(in, out);
			for (i = 0; i < block_length - 1; i++) {
				output[output_index++] = out[i];
			}
		}
	}

	*output_length = output_index;
	return SUCCESS;

}
