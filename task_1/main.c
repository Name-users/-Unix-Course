#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

#define params_error "Ошибка параметра\n"
#define open_file_error "Ошибка открытия файла\n"
#define read_file_error "Ошибка чтения файла\n"
#define write_file_error "Ошибка записи в файл\n"
#define lseek_func_error "Ошибка операции lseek\n"
#define close_file_error "Ошибка закрытия файла\n"

void generate_exception(char *message){
    fprintf(stderr, message);
    exit(1);
}

int main(int argc, char *argv[]) {
    
    char *input_file = NULL;
    char *output_file = NULL;
    int block_size = 4096;
    int index;

    while ((index = getopt(argc, argv, "b:")) != -1)
    {
        switch (index)
        {
            case 'b':
                block_size = atoi(optarg);
                break;
            default:
                generate_exception(params_error);
        }
    }
    
    if (optind < argc) 
        while (optind < argc) {
            char *file = (char *) malloc(strlen(argv[optind]) + 1);
            strcpy(file, argv[optind]);
            optind++;
            if (input_file == NULL) {
                input_file = file;
                continue;
            }
            if (output_file == NULL) {
                output_file = file;
                continue;
            }
            generate_exception(params_error);
        }
    
    if (input_file != NULL && output_file == NULL) {
        output_file = input_file;
        input_file = NULL;
    }
    if (output_file == NULL || input_file != NULL && strcmp(input_file, output_file) == 0 || block_size < 1)
        generate_exception(params_error);

    int input_fd = STDIN_FILENO;
    if (input_file != NULL)
        input_fd = open(input_file, O_RDONLY);
    int output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (input_fd == -1 || output_fd == -1)
        generate_exception(open_file_error);

    char *buffer = (char *) malloc(block_size);
    while (true)
    {
        ssize_t read_bytes = read(input_fd, buffer, block_size);
        if (read_bytes == -1)
            generate_exception(read_file_error);
        if (read_bytes == 0)
            break;
        char *buffer_ = buffer;
        bool zero_block = true;
        for (int index = 0; index < read_bytes; index++)
            if (*(buffer_++)) {
                zero_block = false;
                break;
            }
        if (zero_block) {
            if (lseek(output_fd, block_size, SEEK_CUR) == -1)
                generate_exception(lseek_func_error);
        }
        else
            if (write(output_fd, buffer, read_bytes) != read_bytes)
                generate_exception(write_file_error);
    }
    if (input_file != NULL && close(input_fd) == -1 || close(output_fd) == -1)
        generate_exception(close_file_error);
}