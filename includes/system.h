﻿#ifndef SYSTEM_H_INCLUDED
#define SYSTEM_H_INCLUDED

/**< perspective : add battery, add Networks interfaces */

struct cpu_t;
struct disk_t;
struct process_t;

/* All the data about the system is stored here */
struct system_t
{
	int cpu_count; /**< The number of CPUs in the system */
	struct cpu_t *cpus; /**< All CPUs in the system ordered
						  by core_id and package_id */

	//int process_count; /**<Number of process in the system>*/
	//struct process_t *processes; /**< All process in the system ordred by name >*/


	int disk_count; /**< The number of disks (block devices) */
	struct disk_t *disks; /**< The actual disks in the system */
	int max_disk_count;


	long long ram_used; /**< The ammount of RAM used by applications (bytes) */
	long long ram_free;
	long long ram_shared;
	long long ram_buffers; /**< The ammount of RAM used as buffers (bytes) */
	long long ram_cached; /**< THe ammount of RAM used for caches (bytes) */
	long long ram_total;

	// File descriptors for files that are kept open
	int proc_stat_fd;
	int meminfo_fd;

	// Generic buffer. Used when reading from /proc/stat
	char *buffer;
	int buffer_size;
};

struct system_t system_init(void);

void system_delete(struct system_t system);

/** Refresh all dynamically changing system stats */
void system_refresh_info(struct system_t *system);

#endif
