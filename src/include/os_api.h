#ifndef OS_API
#define OS_API
#include <stddef.h>
#include <stdio.h>


typedef struct os_shared_memory os_shared_memory;
typedef struct os_process os_process;

int os_shm_create(os_shared_memory **memory);
int os_shm_open(os_shared_memory **memory, const char *name);
int os_create_process(os_process **process, const char *executable, os_shared_memory *memory, const char *output_file);
int os_shm_write(os_shared_memory *memory, const char *data, size_t length);
int os_shm_read(os_shared_memory *memory, char **line, size_t *length);
int os_shm_close_writer(os_shared_memory *memory);
int os_process_wait(os_process *process, int *exit_code);
void os_shm_destroy(os_shared_memory *memory);
void os_destroy_process(os_process *process);

#endif