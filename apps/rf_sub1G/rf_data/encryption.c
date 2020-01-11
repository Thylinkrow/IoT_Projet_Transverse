#include "encryption.h"

#define CBC 1

#include "aes.h"

void encrypt(uint8_t *data, uint8_t *key, uint8_t *iv, uint8_t len){

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, iv);
    AES_CBC_encrypt_buffer(&ctx, data, len * ENCRYPT_BLOCK_LEN);
}

void decrypt(uint8_t *data, uint8_t *key, uint8_t *iv, uint8_t len){

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, iv);
    AES_CBC_decrypt_buffer(&ctx, data, len * ENCRYPT_BLOCK_LEN);
}