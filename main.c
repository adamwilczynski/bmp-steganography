#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

const int BMP_HEADER_BYTE_COUNT = 14;

struct {
    char *char_array;
    int count;
} typedef str;

str extract_file_header(FILE *file_pointer) {
    str file_header;
    file_header.count = BMP_HEADER_BYTE_COUNT;
    file_header.char_array = malloc(sizeof (char) * file_header.count);
    for (int i = 0; i < BMP_HEADER_BYTE_COUNT; ++i) {
        file_header.char_array[i] = (char) fgetc(file_pointer);
//        printf("%c|", file_header.char_array[i]);
    }

    if (!(file_header.char_array[0] == 'B' & file_header.char_array[1] == 'M')) {
        printf("Incorrect byte type. Expected: BM Got: %c%c", file_header.char_array[0], file_header.char_array[1]);
    }

    int real_header_size = fgetc(file_pointer);
    file_header.count += real_header_size;
    file_header.char_array = realloc(file_header.char_array, sizeof (char) * file_header.count);
    file_header.char_array[BMP_HEADER_BYTE_COUNT] = (char) real_header_size;
    for (int i = BMP_HEADER_BYTE_COUNT + 1; i < file_header.count; ++i) {
        file_header.char_array[i] = (char) fgetc(file_pointer);
//        printf("%c|", file_header.char_array[i]);
    }
//    puts("");
    file_header.char_array[file_header.count - 1] = '\0';
    return file_header;
}

str char_to_binary(char c) {
    str binary_encoding_before_offset;
    binary_encoding_before_offset.count = 8 + 1;
    binary_encoding_before_offset.char_array = malloc(sizeof (char) * binary_encoding_before_offset.count);
    binary_encoding_before_offset.char_array[8] = '\0';
    itoa(c, binary_encoding_before_offset.char_array, 2);

    int offset = 8 - strlen(binary_encoding_before_offset.char_array);
    if (offset != 0) {
        str binary_encoding_after_offset;
        binary_encoding_after_offset.count = 8 + 1;
        binary_encoding_after_offset.char_array = malloc(sizeof (char) * binary_encoding_before_offset.count);
        binary_encoding_after_offset.char_array[8] = '\0';

        for (int i = strlen(binary_encoding_before_offset.char_array) - 1; i >= 0; --i) {
            binary_encoding_after_offset.char_array[i + offset] = binary_encoding_before_offset.char_array[i];
            binary_encoding_after_offset.char_array[i] = '0';
        }
        return binary_encoding_after_offset;
    }
    return binary_encoding_before_offset;
}

char binary_str_to_char(str binary_encoding) {
    return (char) strtol(binary_encoding.char_array, 0, 2);
}

str read_text_to_encode(char *text_to_encode) {
    str text_to_encode_str;

    text_to_encode_str.count = 1 + (int) strlen(text_to_encode) + 1;
    text_to_encode_str.char_array = malloc(sizeof (char) * text_to_encode_str.count);
    text_to_encode_str.char_array[text_to_encode_str.count - 1] = '\0';

    sprintf(text_to_encode_str.char_array, "%c", (char) (strlen(text_to_encode)) + 48);
    strncpy(text_to_encode_str.char_array + 1, text_to_encode, text_to_encode_str.count);
    return text_to_encode_str;
}

int get_file_bytes_size(FILE *file_pointer) {
    fseek(file_pointer, 0, SEEK_END);
    int bytes_size = ftell(file_pointer);
    fseek(file_pointer, 0, SEEK_SET);
    return bytes_size;
}

int main(int argc, char **argv) {
    const short encode_argument_number = 3;
    if (argc == encode_argument_number + 1) {
        FILE *input_file_pointer, *encoded_file;
        input_file_pointer = fopen(argv[1], "rb");

        int bytes_to_write = get_file_bytes_size(input_file_pointer);

        encoded_file = fopen(argv[2], "wb");

        str file_header = extract_file_header(input_file_pointer);
        printf("modulo: %i\n", ((unsigned char) file_header.char_array[0]) % 2);
        printf("Header contents:\n%s\nHeader size: %i\n", file_header.char_array, file_header.count);


        for (int i = 0; i < file_header.count; ++i) {
            fwrite(&file_header.char_array[i], sizeof (char), 1, encoded_file);
            --bytes_to_write;
        }

        str text_to_encode = read_text_to_encode(argv[3]);
        printf("Encoding string:\n\"%s\" of length: %i\n", text_to_encode.char_array, (unsigned int) strlen(text_to_encode.char_array));

        for (int message_to_encode_index = 0; message_to_encode_index < strlen(text_to_encode.char_array); ++message_to_encode_index) {
            str char_to_encode_binary = char_to_binary(text_to_encode.char_array[message_to_encode_index]);
            printf("Encoding char: %c as binary: %s\n", text_to_encode.char_array[message_to_encode_index], char_to_encode_binary.char_array);

            for (int byte_index = 0; byte_index < 8; ++byte_index) {
                unsigned char file_byte = fgetc(input_file_pointer);
                if ((char_to_encode_binary.char_array[byte_index] == '1') & (file_byte % 2 == 0)) {
                    printf("changed\n");
                    ++file_byte;
                    fwrite(&file_byte, sizeof (unsigned char), 1, encoded_file);
                    --bytes_to_write;
                } else if (((char_to_encode_binary.char_array[byte_index]) == '0') & (file_byte % 2 == 1)) {
                    printf("changed\n");
                    --file_byte;
                    fwrite(&file_byte, sizeof (unsigned char), 1, encoded_file);
                    --bytes_to_write;
                } else {
                    fwrite(&file_byte, sizeof (unsigned char), 1, encoded_file);
                    --bytes_to_write;
                }
            }
        }

        char c;
        while (bytes_to_write) {
            c = (char) fgetc(input_file_pointer);
            fwrite(&c, sizeof (char), 1, encoded_file);
            --bytes_to_write;
        }
        fclose(input_file_pointer);
        fclose(encoded_file);
    }
    const short decode_argument_number = 1;
    if (argc == decode_argument_number + 1) {
        FILE *input_file_pointer = fopen(argv[1], "rb");
        str file_header = extract_file_header(input_file_pointer);

        str message_size_binary;
        message_size_binary.count = 8 + 1;
        message_size_binary.char_array = malloc(sizeof (char) * message_size_binary.count);
        message_size_binary.char_array[message_size_binary.count - 1] = '\0';

        for (int i = 0; i < 8; ++i) {
            unsigned char file_byte = fgetc(input_file_pointer);;
            message_size_binary.char_array[i] = (char) (file_byte % 2 + 48);
        }
        puts("Encoded message size:");
        printf("binary %s = character %c\n", message_size_binary.char_array, binary_str_to_char(message_size_binary));

        int message_size = (int) binary_str_to_char(message_size_binary) - 48;
        printf("Message: size: %i\n", message_size);

        for (int message_index = 0; message_index < message_size; ++message_index) {
            for (int i = 0; i < 8; ++i) {
                unsigned char file_byte = fgetc(input_file_pointer);;
                message_size_binary.char_array[i] = (char) (file_byte % 2 + 48);
            }
            printf("binary %s = character %c\n", message_size_binary.char_array, binary_str_to_char(message_size_binary));
        }

        fclose(input_file_pointer);
    }
    return 0;
}
