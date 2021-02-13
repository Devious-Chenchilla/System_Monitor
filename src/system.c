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

//system init
struct system_t system_init(void){

  struct system_t system;

  //open file stat_proc & meme infos
  system.proc_stat_fd = open_file_readonly(PROC_STAT_DIR);
  sytsem.meminfo_fd = open_file_readonly(MEMINFO_PATH);

  system_cpu_init(&system);
  system_disk_init(&system);
  system_net_init(&system);
  system_bat_init(&system);

  system.buffer = NULL;
  system.buffer_size = 0;

  system_refresh_info(&system);

  return system;
}


void system_delete(struct system_t system){

  //fermer les descripteurs de fichiers proc/stat et memeinfo
  close(system.proc_stat_fd);
  close(system.meminfo_fd);

  // fermer cur_freq_fd
  for(int i =0; i <system.cpu_count; ++i)
    close(system.cpus[i].cur_freq_fd);
  // fermer stat_fd
  for(int i =0; i <system.disk_count; ++i)
    close(system.disks[i].stat_fd);
  // fermer rx & tx b_fd
  for(int i =0; i <system.cpu_count; ++i){
    close(system.interfaces[i].rx_bytes_fd);
    close(system.interfaces[i].tx_bytes_fd);
  }
  // fermer charge current voltage fd
  for (int i = 0; i < system.battery_count; ++i) {
		close(system.batteries[i].charge_fd);
		close(system.batteries[i].current_fd);
		close(system.batteries[i].voltage_fd);
	}

	// libérer la mémoire
	free(system.buffer);
	free(system.cpus);
	free(system.disks);
	free(system.interfaces);
	free(system.batteries);
  
}


static void system_cpu_init(struct system_t *system){
  system->cpu_count = 0; // nb cpu dans le systéme
  system->cpus = NULL; // tous les cpu ordonnés par core_id et package_id
  int cpus_container_size = 0;


  // lister le dossier /sys/bus/cpu/devices
  char fname[128];
  strcpy(fname, CPU_DEVICES_DIR); // copier /sys/bus/cpu/devices/ dans fname
  DIR *cpu_devices_dir = opendir(CPU_DEVICES_DIR);

  if(cpu_devices_dir == NULL)
    return;

  struct dirent *cpu_ent; // parcourir les dossiers cpuN
  while((cpu_ent = readdir(cpu_devices_dir))){
    //ignorer ceux qui ne commencent pas par "cpu"
    if(strncmp(cpu_ent->d_name, "cpu",3)) // compare le d_name lu avec cpu avec 3 lettres premieres
      continue;

    struct cpu_t cpu;

    for(int i = 0; i < CPU_STATS_COUNT; ++i)
      cpu.stats[i] = 0;
    cpu.id = atoi(cpu_ent->d_name +3);

    //rajouter cpuN au fname, devient : /sys/bus/cpu/devices/cpuN
    strcat(fname, cpu_ent->d_name); // rajout cpu'N'

    //charger les parametres du cpu (c'est à dire les paramétres fixes)
    
    strcat(fname, "/topology/core_id");
    cpu.core_id = read_int_from_file(fname); // stocker le core_id

    strcat(fname, "/topology/physical_package_id");
    cpu.package_id = read_int_from_file(fname); // stocker le package_id

    strcat(fname, "/cpu_freq/cpuinfo_max_freq");
    cpu.max_freq = read_int_from_file(fname); // stocker la freq maximal

    strcat(fname, "/cpu_freq/cpuinfo_min_freq");
    cpu_min_freq = read_int_from_file(fname); // stocker la freq min

    //charger les paramétres dynamique (c'est à dire ceux qui changent)
    //fichier des paramétres dynamique
    strcat(fname, "/cpufreq/scaling_cur_freq");
    cpu.cur_freq_fd = open_file_readonly(fname);

    // definir la température du cpu par defaut
    cpu.cur_temp = 0;

    //rajouter cpu à system.cpus : notre structure
    if(cpus_container_size <= system->cpu_count){
       cpus_container_size += 64;
       system->cpus = (struct cpu_t *)realloc(system->cpus, sizeof(struct cpu_t)* cpus_container_size);
    }
    system->cpus[system->cpu_count++] = cpu;
  }
  closedir(cpu_devices_dir);

  //trier le tableau cpu par package et core IDS

  qsort(system->cpus, system->cpu_count, sizeof(struct cpu_t), cpu_cmp);
}

//refresh system cpu stats
static void system_refresh_cpus(struct system_t *system){

  //mettre à jour la frequence cpu courante

  for(int i = 0; i < system->cpu_count; ++i){
    struct cpu_t *cpu = &system->cpus[i];
    lseek(cpu->cur_freq_fd, 0, SEEK_SET);
    cpu->cur_freq = read_int_from_fd(cpu->cpu_freq_fd);
  }
}
