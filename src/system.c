#include "../includes/system.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>

#define CPU_DEVICES_DIR "/sys/bus/cpu/devices/"

#define PROC_STAT_DIR "/proc/stat"
#define MEMINFO_PATH "/proc/meminfo"


struct system_t system_init(void){
  struct system_t system;

  system.proc_stat_fd = open_file_readonly(PROC_STAT_DIR);
  sytsem.meminfo_fd = open_file_readonly(MEMINFO_PATH);

  system_cpu_init(&system);
  system_disk_init(&system);
  system_net_init(&system);
  system_bat_init(&system);

  ystem.buffer = NULL;
  system.buffer_size = 0;

  system_refresh_info(&system);

  return system;
}


