#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

int main(int argc, char* argv[]) {
    /* read in the length of the message and fix byte-ordering */
    uint16_t message_length;
    if (fread(&message_length, sizeof(uint16_t), 1, stdin) != 1) {
        exit(EXIT_FAILURE);
    }
    message_length = nthos(message_length);

    printf("scan in length %" PRId16 "\n", message_length);

    return 0;
}
