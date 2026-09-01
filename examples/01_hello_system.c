/*
 * 01_hello_system.c — system info + logging
 *
 * Build:
 *   gcc -I../include 01_hello_system.c -o hello_system -lplayos
 */
#include <playos.h>

int main(void)
{
    PLAYOS_LOG_I("hello", "PlayOS API version %u", playos_system_api_version());
    PLAYOS_LOG_I("hello", "OS %s on %s", playos_system_os_version(),
                 playos_system_device_model());
    PLAYOS_LOG_I("hello", "CPU: %s", playos_system_cpu_description());
    PLAYOS_LOG_I("hello", "GPU: %s", playos_system_gpu_description());
    PLAYOS_LOG_I("hello", "RAM: %llu bytes, %llu available",
                 (unsigned long long)playos_system_total_memory_bytes(),
                 (unsigned long long)playos_system_available_memory_bytes());
    PLAYOS_LOG_I("hello", "Locale: %s", playos_system_locale());
    return 0;
}
