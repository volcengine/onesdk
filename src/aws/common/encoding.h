#ifndef VOLC_ENCODING_H
#define VOLC_ENCODING_H

#include "common.h"
#include "byte_buf.h"

AWS_COMMON_API
int aws_base64_compute_encoded_len(size_t to_encode_len, size_t *encoded_len);

/*
 * Base 64 encodes the contents of to_encode and stores the result in output.
 */
AWS_COMMON_API
int aws_base64_encode(const struct aws_byte_cursor *AWS_RESTRICT to_encode, struct aws_byte_buf *AWS_RESTRICT output);

/*
 * Computes the length necessary to store the output of aws_base64_decode call.
 * returns -1 on failure, and 0 on success. decoded_len will be set on success.
 */
AWS_COMMON_API
int aws_base64_compute_decoded_len(const struct aws_byte_cursor *AWS_RESTRICT to_decode, size_t *decoded_len);

/*
 * Base 64 decodes the contents of to_decode and stores the result in output.
 */
AWS_COMMON_API
int aws_base64_decode(const struct aws_byte_cursor *AWS_RESTRICT to_decode, struct aws_byte_buf *AWS_RESTRICT output);

#endif