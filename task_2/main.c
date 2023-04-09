#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>
#include <signal.h>


#define buffer_size 1024
#define lock_file_error "Ошибка при попытке блокировки файла\n"
#define unlock_file_error "Ошибка при попытке разблокировать файл\n"
#define params_error "Ошибка параметра\n"
#define read_file_error "Ошибка чтения файла\n"
#define write_file_error "Ошибка записи в файл\n"
#define sigint_error "Ошибка настройки обработки SIGINT\n"

char *finput_name = NULL;
char *flock_name = NULL;
bool flag_lock_file = false;
int lock_count = 0;
int unlock_count = 0;

bool generate_exception(char *message, bool err_exit){
    fprintf(stderr, message);
    if (err_exit)
        exit(EXIT_FAILURE);
    return false;
}

bool lock_file(bool err_exit) {
    char pid[buffer_size];
    int fd_lock_file = -1;
    while (fd_lock_file == -1) {
        fd_lock_file = open(flock_name, O_CREAT | O_EXCL | O_WRONLY, 0600);
    }
    flag_lock_file = true;
    sprintf(pid, "%d\n", getpid());
    if (write(fd_lock_file, pid, strlen(pid)) != strlen(pid) || close(fd_lock_file) == -1) 
        return generate_exception(lock_file_error, err_exit);
    return true;
}

bool unlock_file(bool err_exit) {
    char *pid_ = (char *) malloc(buffer_size);
    int fd = open(flock_name, O_RDONLY);
    if (
        fd == -1 || 
        read(fd, pid_, buffer_size) == -1 ||
        atoi(pid_) != getpid() || 
        close(fd) == -1 ||
        remove(flock_name) == -1
        )
        return generate_exception(unlock_file_error, err_exit);
    flag_lock_file = false;
    free(pid_);
    return true;
}

void remove_empty_lock_file() {
    char *buffer = (char *) malloc(buffer_size);
    int fd = open(flock_name, O_RDONLY);
    if (fd != -1) {
        ssize_t read_bytes = read(fd, buffer, buffer_size); 
        if (close(fd) != -1 && read_bytes == 0)
            remove(flock_name);
    }
    free(buffer);
}

void sigint_handler(int sig) {
    if (flag_lock_file && unlock_file(false)) 
        unlock_count++;
    remove_empty_lock_file();
    char buffer[buffer_size];
    sprintf(buffer, "%d\t%d\t%d\n", getpid(), lock_count, unlock_count);
    int fd = open("result.txt", O_WRONLY | O_APPEND | O_CREAT);
    if (fd == -1 || write(fd, buffer, strlen(buffer)) != strlen(buffer))
        generate_exception(write_file_error, true);
    close(fd);
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    
    int index;
    int sleep_time;
    bool err_exit = false;

    if (signal(SIGINT, sigint_handler) == SIG_ERR)
        generate_exception(sigint_error, true);

    while ((index = getopt(argc, argv, "s:e")) != -1)
    {
        switch (index)
        {
            case 's':
                sleep_time = atoi(optarg);
                break;
            case 'e':
                err_exit = true;
                break;
            default:
                generate_exception(params_error, true);
        }
    }

    while (optind < argc) 
    {
        char *file = (char *) malloc(strlen(argv[optind]) + 1);
        strcpy(file, argv[optind]);
        optind++;
        if (finput_name == NULL) {
            finput_name = file;
            continue;
        }
        generate_exception(params_error, true);
    }

    if (finput_name == NULL)
        generate_exception(params_error, true);
    
    flock_name = malloc(strlen(finput_name) + strlen(".lck") + 1);
    strcpy(flock_name, finput_name);
    strcat(flock_name, ".lck");

    char *buffer = (char *) malloc(buffer_size);
    while (true) {
        if (lock_file(err_exit)) {
            lock_count++;
            int input_fd = open(finput_name, O_RDONLY);
            if (input_fd == -1 || read(input_fd, buffer, buffer_size) == -1) {
                unlock_file(err_exit);
                generate_exception(read_file_error, true);
            }
            sleep(sleep_time);
            if (unlock_file(err_exit))
                unlock_count++;
        }
    }
}