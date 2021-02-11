#ifndef SYSTEM_H_INCLUDE
#define SYSTEM_H_INCLUDE

struct cpu_t;
struct disk_t;
struct interface_t;
struct battery_t;


/*Toutes les données à propos du systeme sont stocké dedans */
struct system_t{

  //
  int cpu_count; // nombre de cpu dans le system
  struct cpu_t *cpus; // tous les cpu dans le systeme ordonné par core_id et package_id

  //
  int disk_count; //nombre de disks(block devices)
  struct disk_t *disks; // ledisk actuel sur le systeme
  int max_disk_count;

  //
  int interface_count; // le nombre d'interface reseau
  struct interface_t *interfaces; // l'interface reseau *p
  int max_interface_count;

  //
  int battery_count; // le nombre de batterie
  struct battery_t *batteries; // la batterie *p
  int max_battery_count;

  // tous en (bytes)
  long long ram_used; //la quantité de ram utilisée par les applications
  long long ram_buffers; // la quantité de ram utilisée comme buffer
  long long ram_cached; // la quantité de ram utilisé pour les caches

  //les descripteurs de fichiers pour les fichiers qui restent ouvert
  int proc_stat_fd;
  int meminfo_fd;

  //buffer generic, utilisé quand on lit de /proc/stat
  char *buffer;
  int buffer_size;
};

struct system_t system_init(void);
void system_delete(struct system_t system);

//rafrachir les stat system qui changent
void system_refresh_info(struct system_t *system);


#endif
