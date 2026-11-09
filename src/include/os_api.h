#ifndef OS_API
#define OS_API
#include <stddef.h>
#include <stdio.h>


typedef struct os_pipe os_pipe;
typedef struct os_process os_process;

int os_create_pipe(os_pipe **pipe); //создаёт пайп, возвращает 0 если пайп создан, -1 если нет
int os_create_process(os_process **process, const char *executable, os_pipe *input, const char *output_file); //создаёт процесс, возвращает 0 если процесс создан, -1 если нет
int os_pipe_write(os_pipe *pipe, const void *data, size_t size); //возвращает 0 если наткнулся на EOF, -1 при ошибке, иначе количество записанных байтов
int os_pipe_close_writer(os_pipe *pipe); //закрывает файловый дескриптор для записи, возвращает 0 если успешно, иначе -1
int os_process_wait(os_process *process, int *exit_code); //в exit_code запишется код выхода процесса, функция вернёт 0 если ожидание успешно
void os_destroy_pipe(os_pipe *pipe); //удаляет канал, 0 если успешно, -1 если нет
void os_destroy_process(os_process *process); //удаляет процесс, 0 если успешно, -1 если нет

#endif