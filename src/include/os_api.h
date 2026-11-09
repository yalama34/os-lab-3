#ifndef OS_API
#define OS_API
#include <stddef.h>
#include <stdio.h>


typedef struct os_shared_memory os_shared_memory;
typedef struct os_process os_process;

int os_shm_create(os_shared_memory **memory);
int os_shm_open(os_shared_memory **memory, const char *name);
int os_create_process(os_process **process, const char *executable, os_shared_memory *memory, const char *output_file); //создаёт процесс, возвращает 0 если процесс создан, -1 если нет
int os_shm_write(os_shared_memory *memory, const void *data, size_t size); //возвращает 0 если наткнулся на EOF, -1 при ошибке, иначе количество записанных байтов
int os_shm_read(os_shared_memory *memory, char **line, size_t size);
int os_shm_close_writer(os_shared_memory *pipe); //закрывает файловый дескриптор для записи, возвращает 0 если успешно, иначе -1
int os_process_wait(os_process *process, int *exit_code); //в exit_code запишется код выхода процесса, функция вернёт 0 если ожидание успешно
void os_destroy_shm(os_shared_memory *memory); //удаляет канал, 0 если успешно, -1 если нет
void os_destroy_process(os_process *process); //удаляет процесс, 0 если успешно, -1 если нет

#endif