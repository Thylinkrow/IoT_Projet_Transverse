#include "lib/stdio.h"
#include <string.h>

#define ENCRYPT_BLOCK_LEN 16

void encrypt(uint8_t *data, uint8_t *key, uint8_t *iv, uint8_t len);
void decrypt(uint8_t *data, uint8_t *key, uint8_t *iv, uint8_t len);