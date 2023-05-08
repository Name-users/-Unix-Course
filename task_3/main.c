#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>

#define work_dir "/"

struct Task {
    pid_t pid;
    char * finput;
    char * foutput;
    int args_len;
    char ** args;
};

struct TaskArray {
    int length;
    struct Task ** array;
};

int SIGNALRESTART = 0;
int SIGNALFINISH = 0;
char *config_file = NULL;
char *log_file = NULL;
struct TaskArray * task_array = NULL;

bool is_absolute_path(char * path) {
    return strlen(path) > 0 && path[0] == '/';
}

void add_task(struct TaskArray *tasks, struct Task *task) {
    struct Task ** array = (struct Task **) realloc(
        tasks -> array, 
        sizeof(struct Task *) * ((tasks -> length) + 1)
        );
    if (array == NULL) {
        fprintf(stderr, "ERROR: Ошибка при выделении памяти\n");
        exit(1);
    }
    tasks -> array = array;
    tasks -> array[tasks -> length++] = task;
}

struct Task * create_task() {
    struct Task * task = (struct Task * ) malloc(sizeof(struct Task));
    if (task == NULL) {
        fprintf(stderr, "ERROR: Ошибка при выделении памяти\n");
        exit(1);
    }
    task -> pid = 0;
    task -> finput = NULL;
    task -> foutput = NULL;
    task -> args_len = 0;
    task -> args = NULL;
    return task;
};

void free_task(struct Task * task) {
    if (task -> finput != NULL)
        free(task -> finput);
    if (task -> foutput != NULL)
        free(task -> foutput);
    for (int i = 0; i < task -> args_len; i++) {
        if (task -> args[i] != NULL)
            free(task -> args[i]);
    }
    if (task -> args != NULL)
        free(task -> args);
    free(task);
}

struct TaskArray * creat_task_array() {
    struct TaskArray * new_task_array = (struct TaskArray * ) malloc(sizeof(struct TaskArray));
    if (new_task_array == NULL) {
        fprintf(stderr, "ERROR: Ошибка при выделении памяти\n");
        exit(1);
    }
    new_task_array -> length = 0;
    new_task_array -> array = NULL;
    return new_task_array;
}

void free_task_array(struct TaskArray * task_array) {
    for (int i = 0; i < task_array -> length; i++)
        if (task_array -> array[i] != NULL)
            free_task(task_array -> array[i]);
    if (task_array -> array != NULL)
        free(task_array -> array);
    task_array -> length = 0;
    free(task_array);
}

char * read_line(int fd) {
    int len = 0;
    char * array = NULL;
    char * new_char;
    
    while (1) {
        new_char = malloc(1);
        int r_bytes = read(fd, new_char, 1);
        if (r_bytes == 0)
            break;
        if (array == NULL)
            array = malloc(1);
        else
            array = (char *) realloc(array, len + 1);
        if (new_char[0] == '\n') {
            array[len++] = '\0';
            break;
        }
        else
            array[len++] = new_char[0];
        free(new_char);
    }
    if (new_char != NULL)
        free(new_char);
    return array;
}

void add_string(char * string, char ** * array, int * length) {
    char ** new_arr = (char ** ) realloc( * array, sizeof(char * ) * (( * length) + 1));
    if (new_arr == NULL) {
        fprintf(stderr, "ERROR: Ошибка при выделении памяти\n");
        exit(1);
    }
    * array = new_arr;
    if (string == NULL)
        ( * array)[( * length) ++] = NULL;
    else
        ( * array)[( * length) ++] = strdup(string);
}

void split_string(char * string, char ** * array, int * length) {
    * array = NULL;
    * length = 0;
    char * arg = string;

    while (true) {
        if ( * arg == '\0')
            break;
        char * next_arg = strchr(arg, ' ');
        if (next_arg == NULL) {
            add_string(arg, array, length);
            break;
        }
        * next_arg = '\0';
        add_string(arg, array, length);
        if (next_arg == NULL)
            break;
        arg = next_arg + 1;
    }
    add_string(NULL, array, length);
}

struct TaskArray * read_config() {
    struct TaskArray * task_array = creat_task_array();
    int fd = open(config_file, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "ERROR: Ошибка открытия файла '%s'\n", config_file);
        exit(1);
    }
    
    while (1) {
        char * line = read_line(fd);
        if (line == NULL)
            break;
        fprintf(stderr, "INFO: myinit обнаружил задачу: '%s'\n", line);
        struct Task * task = create_task();
        split_string(line, & task -> args, & task -> args_len);
        free(line);
        if (task -> args_len < 3) {
            fprintf(stderr, "ERROR: Ошибка определения задачи: количество аргументов меньше трёх\n");
            free_task(task);
            continue;
        }
        int indexes[] = { 0, task -> args_len - 3,  task -> args_len - 2 };
        for (int i = 0; i < 3; i++)
        if (!is_absolute_path(task -> args[indexes[i]])) {
            fprintf(
                stderr, 
                "ERROR: Ошибка определения пути '%s' - не является абсолютным\n", 
                task -> args[indexes[i]]
                );
            free_task(task);
            continue;
        }

        task -> finput = strdup(task -> args[task -> args_len - 3]);
        task -> foutput = strdup(task -> args[task -> args_len - 2]);
        free(task -> args[task -> args_len - 3]);
        task -> args[task -> args_len - 3] = NULL;
        free(task -> args[task -> args_len - 2]);
        task -> args[task -> args_len - 2] = NULL;
        add_task(task_array, task);
    }
    
    return task_array;
}

int dup_fd(char * file_name, int flags, mode_t mode, int new_fd) {
    int fd = open(file_name, flags, mode);
    if (fd == -1 || dup2(fd, new_fd) == -1) {
        fprintf(stderr, "ERROR: Ошибка при создании копии файлового дескриптора файла '%s'\n", file_name);
        return -1;
    }
    close(fd);
    return 0;
}

void start_task(int task_index, struct Task * task) {

    pid_t pid = fork();
    
    if (pid == -1) {
        fprintf(stderr, "ERROR: Ошибка запуска задачи под номером %d\n", task_index + 1);
        return;
    }
    
    if (pid != 0) {
        task -> pid = pid;
        fprintf(stderr, "INFO: Запущена задача под номером %d\n", task_index + 1);
        return;
    }

    if (
        dup_fd(task -> finput, O_RDONLY, 0600, 0) == -1 ||
        dup_fd(task -> foutput, O_CREAT | O_TRUNC | O_WRONLY, 0600, 1) == -1 ||
        dup_fd("/dev/null", O_CREAT | O_APPEND | O_WRONLY, 0600, 2) == -1
        ) 
        exit(1);

    char ** args = task -> args;
    execv(args[0], args);
    exit(0);
}

void stop_tasks(struct TaskArray * task_array) {

    for (int i = 0; i < task_array -> length; i++) {
        pid_t pid = task_array -> array[i] -> pid;
        if (pid != 0)
            kill(pid, SIGTERM);
    }
    
    int task_count = 0;
    for (int i = 0; i < task_array -> length; i++)
        if (task_array -> array[i] -> pid != 0)
            task_count++;
    
    while (task_count > 0) {
        int task_status = 0;
        pid_t task_pid = waitpid(-1, & task_status, 0);
        if (task_pid == -1) 
            continue;
        
        int task_index = 0;
        while (task_index < task_array -> length)
        {
            if (task_array -> array[task_index] -> pid == task_pid)
                break;
            task_index++;
        }
        if (WIFEXITED(task_status)) {
            fprintf(stderr, "INFO: Задача под номером %d была завершена с кодом %d\n",
                task_index + 1, WEXITSTATUS(task_status));
        } else if (WIFSIGNALED(task_status)) {
            fprintf(stderr, "INFO: Задача под номером %d была завершена сигналом с кодом %d\n",
                task_index + 1, WTERMSIG(task_status));
        }
        task_count--;
    }
}



void run_myinit(struct TaskArray * task_array) {
    while (1) {
        int task_status = 0;
        pid_t task_pid = waitpid(-1, & task_status, 0);
        
        if (task_pid == -1) {
            if (SIGNALRESTART || SIGNALFINISH)
                break;
            continue;
        }

        int task_index = 0;
        while (task_index < task_array -> length)
        {
            if (task_array -> array[task_index] -> pid == task_pid)
                break;
            task_index++;
        }
        if (WIFEXITED(task_status)) {
            fprintf(stderr, "INFO: Задача под номером %d была завершена с кодом %d\n",
                task_index + 1, WEXITSTATUS(task_status));
        } else if (WIFSIGNALED(task_status)) {
            fprintf(stderr, "INFO: Задача под номером %d была завершена сигналом с кодом %d\n",
                task_index + 1, WTERMSIG(task_status));
        }

        
        task_array -> array[task_index] -> pid = 0;
        
        if (SIGNALRESTART || SIGNALFINISH)
            break;
        
        start_task(task_index, task_array -> array[task_index]);
    }
}

void signal_handler(int s) {
    switch (s) {
        case 1:
            SIGNALRESTART = 1;
            fprintf(stderr, "INFO: myinit поймал сигнал SIGHUP\n");
            break;
        case 2:
            SIGNALFINISH = 1;
            fprintf(stderr, "INFO: myinit поймал сигнал SIGINT\n", s);
            break;
        case 15:
            exit(15);
        default:
            SIGNALFINISH = 1;
            fprintf(stderr, "WARN: myinit поймал неизвестный сигнал: %d\n", s);
            break;
    }
}

int main(int argc, char * argv[]) {
    if (argc != 3) {
        fprintf(stderr, "ERROR: Необходимо запускать следующим образом: %s <config_file> <log_file>\n", argv[0]);
        exit(1);
    }
    config_file = (char *) malloc(strlen(argv[1]) + 1);
    strcpy(config_file, argv[1]);
    log_file = (char *) malloc(strlen(argv[2]) + 1);
    strcpy(log_file, argv[2]);
        
    pid_t pid = fork();

    if (pid == -1) {
        fprintf(stderr, "ERROR: Ошибка вызова функции 'fork'\n");
        exit(1);
    }
    
    if (pid != 0)
        return 0;
    
    pid_t group_id = setsid();
    if (group_id == -1) {
        fprintf(stderr, "ERROR: Ошибка вызова функции 'setsid'\n");
        exit(1);
    }
  
    struct rlimit fd_limit;
    getrlimit(RLIMIT_NOFILE, &fd_limit);
    for (int fd = 0; fd < fd_limit.rlim_max; fd++)
        close(fd);

    int fd = open(log_file, O_CREAT | O_TRUNC | O_WRONLY, 0600);
    if (fd == -1 || dup2(fd, 1) == -1 || dup2(fd, 2) == -1) {
        fprintf(stderr, "ERROR: Ошибка при создании файла для логирования\n");
        exit(1);
    }
    close(fd);

    if (chdir(work_dir) == -1) {
        fprintf(stderr, "ERROR: Ошибка смены текущего каталога\n");
        exit(1);
    }

    if (signal(SIGINT, signal_handler) == SIG_ERR || 
        signal(SIGHUP, signal_handler) == SIG_ERR ||
        signal(SIGTERM, signal_handler) == SIG_ERR) {
        fprintf(stderr, "ERROR: Ошибка настройки обработки сигналов SIGINT/SIGHUP\n");
        exit(1);
    }
        
    fprintf(stderr, "INFO: myinit запущен\n");
    
    while (1) {
        task_array = read_config();
        for (int i = 0; i < task_array -> length; i++)
            start_task(i, task_array -> array[i]);
    
        run_myinit(task_array);
        
        if (SIGNALRESTART) {
            stop_tasks(task_array);
            SIGNALRESTART = 0;
            fprintf(stderr, "INFO: myinit перезапущен\n");
        }
        
        free_task_array(task_array);
        
        if (SIGNALFINISH) {
            SIGNALFINISH = 0;
            break;
        }
    }

    if (config_file != NULL)
        free(config_file);
    
    if (log_file != NULL)
        free(log_file);

    fprintf(stderr, "INFO: myinit завершен\n");    
    return 0;
}