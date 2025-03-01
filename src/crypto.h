
#pragma once

#include <stdint.h>
#include <string.h>

#include "os.h"
#include "cx.h"
#include "constants.h"

#include "./common/bip32.h"
#include "./common/varint.h"
#include "./common/write.h"

/**
 * A serialized extended pubkey according to BIP32 specifications.
 * All the fields are represented as fixed-length arrays serialized in big-endian.
 */
typedef struct serialized_extended_pubkey_s {
    uint8_t version[4];
    uint8_t depth;
    uint8_t parent_fingerprint[4];
    uint8_t child_number[4];
    uint8_t chain_code[32];
    uint8_t compressed_pubkey[33];
} serialized_extended_pubkey_t;

typedef struct {
    serialized_extended_pubkey_t serialized_extended_pubkey;
    uint8_t checksum[4];
} serialized_extended_pubkey_check_t;

/**
 * Derive private key given BIP32 path.
 * It must be wrapped in a TRY block that wipes the output private key in the FINALLY block.
 *
 * @param[out] private_key
 *   Pointer to private key.
 * @param[out] chain_code
 *   Pointer to 32 bytes array for chain code.
 * @param[in]  bip32_path
 *   Pointer to buffer with BIP32 path.
 * @param[in]  bip32_path_len
 *   Number of path in BIP32 path.
 *
 * @return 0 if success, -1 otherwise.
 */
int crypto_derive_private_key(cx_ecfp_private_key_t *private_key,
                              uint8_t chain_code[static 32],
                              const uint32_t *bip32_path,
                              uint8_t bip32_path_len);

/**
 * Initialize public key given private key.
 *
 * @param[in]  parent
 *   Pointer to the extended serialized pubkey of the parent.
 * @param[out] index
 *   Index of the child to derive. It MUST be not hardened, that is, strictly less than 0x80000000.
 * @param[out] child
 *   Pointer to the output struct for the child's serialized pubkey. It can equal parent, which in
 * that case is overwritten.
 *
 * @return 0 if success, a negative number on failure.
 *
 */
int bip32_CKDpub(const serialized_extended_pubkey_t *parent,
                 uint32_t index,
                 serialized_extended_pubkey_t *child);

/**
 * Convenience wrapper for cx_hash to add some data to an intialized hash context.
 *
 * @param[in] hash_context
 *   The context of the hash, which must already be initialized.
 * @param[in] in
 *   Pointer to the data to be added to the hash computation.
 * @param[in] in_len
 *   Size of the passed data.
 *
 * @return the return value of cx_hash.
 */
static inline int crypto_hash_update(cx_hash_t *hash_context, const void *in, size_t in_len) {
    return cx_hash(hash_context, 0, in, in_len, NULL, 0);
}

/**
 * Convenience wrapper for cx_hash to compute the final hash, without adding any extra data
 * to the hash context.
 *
 * @param[in] hash_context
 *   The context of the hash, which must already be initialized.
 * @param[in] out
 *   Pointer to the output buffer for the result.
 * @param[in] out_len
 *   Size of output buffer, which must be large enough to contain the result.
 *
 * @return the return value of cx_hash.
 */
static inline int crypto_hash_digest(cx_hash_t *hash_context, uint8_t *out, size_t out_len) {
    return cx_hash(hash_context, CX_LAST, NULL, 0, out, out_len);
}

/**
 * Convenience wrapper for crypto_hash_update, updating a hash with an uint8_t.
 *
 * @param[in] hash_context
 *  The context of the hash, which must already be initialized.
 * @param[in] data
 *  The uint8_t to be added to the hash.
 *
 * @return the return value of cx_hash.
 */
static inline int crypto_hash_update_u8(cx_hash_t *hash_context, uint8_t data) {
    return crypto_hash_update(hash_context, &data, 1);
}

/**
 * Convenience wrapper for crypto_hash_update, updating a hash with an uint16_t,
 * encoded in big-endian.
 *
 * @param[in] hash_context
 *  The context of the hash, which must already be initialized.
 * @param[in] data
 *  The uint16_t to be added to the hash.
 *
 * @return the return value of cx_hash.
 */
static inline int crypto_hash_update_u16(cx_hash_t *hash_context, uint16_t data) {
    uint8_t buf[2];
    write_u16_be(buf, 0, data);
    return crypto_hash_update(hash_context, &buf, sizeof(buf));
}

/**
 * Convenience wrapper for crypto_hash_update, updating a hash with an uint64_t, serialized as a
 * variable length integer in bitcoin's format.
 *
 * @param[in] hash_context
 *  The context of the hash, which must already be initialized.
 * @param[in] data
 *  The uint64_t to be added to the hash as a bitcoin-style varint.
 *
 * @return the return value of cx_hash.
 */
static inline int crypto_hash_update_varint(cx_hash_t *hash_context, uint64_t data) {
    uint8_t buf[9];
    int len = varint_write(buf, 0, data);
    return crypto_hash_update(hash_context, &buf, len);
}

/**
 * Convenience wrapper for crypto_hash_update, updating a hash with an uint32_t,
 * encoded in big-endian.
 *
 * @param[in] hash_context
 *  The context of the hash, which must already be initialized.
 * @param[in] data
 *  The uint32_t to be added to the hash.
 *
 * @return the return value of cx_hash.
 */
static inline int crypto_hash_update_u32(cx_hash_t *hash_context, uint32_t data) {
    uint8_t buf[4];
    write_u32_be(buf, 0, data);
    return crypto_hash_update(hash_context, &buf, sizeof(buf));
}

/**
 * Computes RIPEMD160(in).
 *
 * @param[in] in
 *   Pointer to input data.
 * @param[in] in_len
 *   Length of input data.
 * @param[out] out
 *   Pointer to the 160-bit (20 bytes) output array.
 */
void crypto_ripemd160(const uint8_t *in, uint16_t inlen, uint8_t out[static 20]);

/**
 * Computes RIPEMD160(SHA256(in)).
 *
 * @param[in] in
 *   Pointer to input data.
 * @param[in] in_len
 *   Length of input data.
 * @param[out] out
 *   Pointer to the 160-bit (20 bytes) output array.
 */
void crypto_hash160(const uint8_t *in, uint16_t in_len, uint8_t *out);

/**
 * Computes the 33-bytes compressed public key from the uncompressed 65-bytes public key.
 *
 * @param[in] uncompressed_key
 *   Pointer to the uncompressed public key. The first byte must be 0x04, followed by 64 bytes
 * public key data.
 * @param[out] out
 *   Pointer to the output array, that must be 33 bytes long. The first byte of the output will be
 * 0x02 or 0x03. It is allowed to have out == uncompressed_key, and in that case the computation
 * will be done in-place. Otherwise, the input and output arrays MUST be non-overlapping.
 *
 * @return 0 on success, a negative number on failure.
 */
int crypto_get_compressed_pubkey(const uint8_t uncompressed_key[static 65], uint8_t out[static 33]);

/**
 * Computes the 65-bytes uncompressed public key from the compressed 33-bytes public key.
 *
 * @param[in] compressed_key
 *   Pointer to the compressed public key. The first byte must be 0x02 or 0x03, followed by 32
 * bytes.
 * @param[out] out
 *   Pointer to the output array, that must be 65 bytes long. The first byte of the output will be
 * 0x04. It is allowed to have out == compressed_key, and in that case the computation will be done
 * in-place. Otherwise, the input and output arrays MUST be non-overlapping.
 *
 * @return 0 on success, a negative number on failure.
 */
int crypto_get_uncompressed_pubkey(const uint8_t compressed_key[static 33], uint8_t out[static 65]);

/**
 * Computes the checksum as the first 4 bytes of the double sha256 hash of the input data.
 *
 * @param[in] in
 *   Pointer to the input data.
 * @param[in] in_len
 *   Length of the input data.
 * @param[out] out
 *   Pointer to the output buffer, which must contain at least 4 bytes.
 *
 */
void crypto_get_checksum(const uint8_t *in, uint16_t in_len, uint8_t out[static 4]);

/**
 * Gets the compressed pubkey and (optionally) the chain code at the given derivation path.
 *
 * @param[in]  bip32_path
 *   Pointer to 32-bit integer input buffer.
 * @param[in]  bip32_path_len
 *   Number of derivation steps.
 * @param[out]  pubkey
 *   A pointer to a 33-bytes buffer that will receive the compressed public key.
 * @param[out]  chaincode
 *   Either NULL, or a pointer to a 32-bytes buffer that will receive the chain code.
 */
void crypto_get_compressed_pubkey_at_path(const uint32_t bip32_path[],
                                          uint8_t bip32_path_len,
                                          uint8_t pubkey[static 33],
                                          uint8_t chain_code[]);

/**
 * Computes the fingerprint of a compressed key as per BIP32; that is, the first 4 bytes of the
 * HASH160 of the given compressed pubkey, interpreted as a big-endian 32-bit unsigned integer.
 *
 * @param[in]  pub_key
 *   Pointer to 32-bit integer input buffer.
 *
 * @return the fingerprint of pub_key.
 */
uint32_t crypto_get_key_fingerprint(const uint8_t pub_key[static 33]);

/**
 * Computes the fingerprint of the master key as per BIP32.
 *
 * @return the fingerprint of the master key.
 */
uint32_t crypto_get_master_key_fingerprint();

/**
 * Computes the base58check-encoded extended pubkey at a given path.
 *
 * @param[in]  bip32_path
 *   Pointer to 32-bit integer input buffer.
 * @param[in]  bip32_path_len
 *   Number of BIP32 paths in the input buffer.
 * @param[in]  bip32_pubkey_version
 *   Version prefix to use for te pubkey.
 * @param[out] out
 *   Pointer to the output buffer, which must be long enough to contain the result (including the
 * terminating null).
 *
 * @return the length of the output pubkey (not including the null character), or -1 on error.
 */
int get_serialized_extended_pubkey_at_path(const uint32_t bip32_path[],
                                           uint8_t bip32_path_len,
                                           uint32_t bip32_pubkey_version,
                                           char out[static MAX_SERIALIZED_PUBKEY_LENGTH + 1]);

/**
 * Derives the level-1 symmetric key at the given label using SLIP-0021.
 * Must be wrapped in a TRY/FINALLY block to make sure that the output key is wiped after using it.
 *
 * @param[in]  label
 *   Pointer to the label. The first byte of the label must be 0x00 to comply with SLIP-0021.
 * @param[in]  label_len
 *   Length of te label.
 * @param[out] key
 *   Pointer to a 32-byte output buffer that will contain the generated key.
 */
void crypto_derive_symmetric_key(const char *label, size_t label_len, uint8_t key[static 32]);

/**
 * Encodes a 20-bytes hash in base58 with checksum, after prepending a version prefix.
 * If version < 256, it is prepended as 1 byte.
 * If 256 <= version < 65536, it is prepended in big-endian as 2 bytes.
 * Otherwise, it is prepended in big-endian as 4 bytes.
 *
 * @param[in]  in
 *   Pointer to the 20-bytes hash to encode.
 * @param[in]  version
 *   The 1-byte, 2-byte or 4-byte version prefix.
 * @param[out]  out
 *   The pointer to the output array.
 * @param[in]  out_len
 *   The 1-byte, 2-byte or 4-byte version prefix.
 *
 * @return the length of the encoded output on success, -1 on failure (that is, if the output
 *   would be longer than out_len).
 */
int base58_encode_address(const uint8_t in[20], uint32_t version, char *out, size_t out_len);

/**
 * TODO: docs
 */
void crypto_tr_tagged_hash_init(cx_sha256_t *hash_context, const uint8_t *tag, uint16_t tag_len);

/**
 * Builds a tweaked public key from a BIP340 public key array.
 * Implementation of taproot_tweak_pubkey of BIP341 with `h` set to the empty byte string.
 *
 * @param[in]  pubkey
 *   Pointer to the 32-byte to be used as public key.
 * @param[out]  y_parity
 *   Pointer to a variable that will be set to 0/1 according to the parity of th y-coordinate of the
 * final tweaked pubkey.
 * @param[out]  out
 *  Pointer to the a 32-byte array that will contain the x coordinate of the tweaked key.
 */
int crypto_tr_tweak_pubkey(uint8_t pubkey[static 32], uint8_t *y_parity, uint8_t out[static 32]);

/**
 * Builds a tweaked public key from a BIP340 public key array.
 * Implementation of taproot_tweak_seckey of BIP341 with `h` set to the empty byte string.
 *
 * @param[in|out] seckey
 *   Pointer to the 32-byte containing the secret key; it will contain the output tweaked secret
 * key.
 */
int crypto_tr_tweak_seckey(uint8_t seckey[static 32]);
