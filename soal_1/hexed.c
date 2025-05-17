#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

unsigned char hex_to_byte(const char *hex) {
    unsigned char byte;
    sscanf(hex, "%2hhx", &byte);
    return byte;
}

void get_timestamp(char *date_str, char *time_str) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    strftime(date_str, 20, "%Y-%m-%d", &tm);
    strftime(time_str, 20, "%H:%M:%S", &tm);
}

int main() {
    const char *input_folder = "anomali";
    const char *output_folder = "image";
    const char *log_file = "conversion.log";

    struct stat st = {0};
    if (stat(output_folder, &st) == -1) {
        mkdir(output_folder, 0700);
    }

    FILE *log = fopen(log_file, "a");
    if (!log) {
        perror("Gagal membuka log file");
        return 1;
    }

    char input_path[256], output_path[256], filename_only[64];

    for (int i = 1; i <= 7; i++) {
        snprintf(input_path, sizeof(input_path), "%s/%d.txt", input_folder, i);
        snprintf(filename_only, sizeof(filename_only), "%d.txt", i);

        FILE *input = fopen(input_path, "r");
        if (!input) {
            perror("Gagal membuka file input");
            continue;
        }

        fseek(input, 0, SEEK_END);
        long length = ftell(input);
        rewind(input);

        char *hex_string = malloc(length + 1);
        if (!hex_string) {
            perror("Gagal alloc memori");
            fclose(input);
            continue;
        }

        fread(hex_string, 1, length, input);
        hex_string[length] = '\0';
        fclose(input);

        char date_str[20], time_str[20];
        get_timestamp(date_str, time_str);

        snprintf(output_path, sizeof(output_path), "%s/%d_image_%s_%s.png", output_folder, i, date_str, time_str);

        FILE *output = fopen(output_path, "wb");
        if (!output) {
            perror("Gagal membuat file output");
            free(hex_string);
            continue;
        }

        for (size_t j = 0; j < strlen(hex_string); j += 2) {
            if (hex_string[j] == '\n' || hex_string[j+1] == '\n') continue;
            unsigned char byte = hex_to_byte(&hex_string[j]);
            fwrite(&byte, 1, 1, output);
        }

        fclose(output);
        free(hex_string);

        fprintf(log, "[%s][%s]: Successfully converted hexadecimal text %s to %d_image_%s_%s.png.\n",
                date_str, time_str, filename_only, i, date_str, time_str);
        fflush(log);

        printf("Berhasil konversi: %s → %s\n", input_path, output_path);
    }

    fclose(log);
    printf("Semua konversi selesai dan dicatat ke conversion.log.\n");
    return 0;
}

