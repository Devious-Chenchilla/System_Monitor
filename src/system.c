#include "../includes/system.h"
#include "../includes/functions.h"
#include "../includes/struct_cpu.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>

#define CPU_DEVICES_DIR "/sys/bus/cpu/devices/"
static const int cpu_devices_dir_len = sizeof(CPU_DEVICES_DIR) - 1;
#define HWMON_DIR "/sys/class/hwmon/"
static const int hwmon_dir_len = sizeof(HWMON_DIR) - 1;
#define BLOCK_DEVICES_DIR "/sys/block/"
static const int block_devices_dir_len = sizeof(BLOCK_DEVICES_DIR) - 1;
#define INTERFACES_DIR "/sys/class/net/"
static const int interfaces_dir_len = sizeof(INTERFACES_DIR) - 1;
#define POWER_DIR "/sys/class/power_supply/"
static const int power_dir_len = sizeof(POWER_DIR) - 1;
#define PROC_STAT_DIR "/proc/stat"
#define MEMINFO_PATH "/proc/meminfo"


static void system_cpu_init(struct system_t *);


struct system_t system_init(void)
{
	struct system_t system;

	system.proc_stat_fd = open_file_readonly(PROC_STAT_DIR);
	system.meminfo_fd = open_file_readonly(MEMINFO_PATH);

	system_cpu_init(&system);


	system.buffer = NULL;
	system.buffer_size = 0;

	system_refresh_info(&system);

	return system;
}

void system_delete(struct system_t system)
{
	// Close files
	close(system.proc_stat_fd);
	close(system.meminfo_fd);
	for (int i = 0; i < system.cpu_count; ++i)
		close(system.cpus[i].cur_freq_fd);


	// Free memory
	free(system.buffer);
	free(system.cpus);

}


static void system_refresh_cpus(struct system_t *system);



void system_refresh_info(struct system_t *system)
{
	system_refresh_cpus(system);

}


// Comparison function for sorting CPUs
static int cpu_cmp(const void *a, const void *b)
{
	const struct cpu_t *c1 = (const struct cpu_t *)a;
	const struct cpu_t *c2 = (const struct cpu_t *)b;
	if (c1->package_id == c2->package_id)
		if (c1->core_id == c2->core_id)
			return c1->id - c2->id;
		else
			return c1->core_id - c2->core_id;
	else
		return c1->package_id - c2->package_id;
}

// Initialize the CPU portion of system
static void system_cpu_init(struct system_t *system)
{
	system->cpu_count = 0;
	system->cpus = NULL;
	int cpus_container_size = 0;

	// List /sys/bus/cpu/devices/
	char fname[128];
	strcpy(fname, CPU_DEVICES_DIR);
	DIR *cpu_devices_dir = opendir(CPU_DEVICES_DIR);
	if (cpu_devices_dir == NULL)
		return;
	struct dirent *cpu_ent;
	while ((cpu_ent = readdir(cpu_devices_dir))) {
		// Skip entities not starting with "cpu"
		if (strncmp(cpu_ent->d_name, "cpu", 3))
			continue;

		struct cpu_t cpu;
		for (int i = 0; i < CPU_STATS_COUNT; ++i)
			cpu.stats[i] = 0;
		cpu.id = atoi(cpu_ent->d_name + 3);

		// Set fname to /sys/bus/cpu/devices/cpuN
		strcpy(fname + cpu_devices_dir_len, cpu_ent->d_name);
		int cpu_dir_len = cpu_devices_dir_len +
			strlen(cpu_ent->d_name);
		
		// Load static CPU parameters
		strcpy(fname + cpu_dir_len, "/topology/core_id");
		cpu.core_id = read_int_from_file(fname);

		strcpy(fname + cpu_dir_len, "/topology/physical_package_id");
		cpu.package_id = read_int_from_file(fname);

		strcpy(fname + cpu_dir_len, "/cpufreq/cpuinfo_max_freq");
		cpu.max_freq = read_int_from_file(fname);

		strcpy(fname + cpu_dir_len, "/cpufreq/cpuinfo_min_freq");
		cpu.min_freq = read_int_from_file(fname);

		// Open the files for the dynamic CPU parameters
		strcpy(fname + cpu_dir_len, "/cpufreq/scaling_cur_freq");
		cpu.cur_freq_fd = open_file_readonly(fname);

		// Set the current cpu temperature to the default
		cpu.cur_temp = 0;

		// Add cpu to system.cpus
		if (cpus_container_size <= system->cpu_count) {
			cpus_container_size += 64;
			system->cpus = (struct cpu_t *)realloc(system->cpus,
					sizeof(struct cpu_t) * cpus_container_size);
		}
		system->cpus[system->cpu_count++] = cpu;
	}
	closedir(cpu_devices_dir);

	// Sort the cpus array by package and core IDs
	qsort(system->cpus, system->cpu_count,
			sizeof(struct cpu_t), cpu_cmp);
}




// Refresh system CPU stats
static void system_refresh_cpus(struct system_t *system)
{
	// Update current CPU frequency
	for (int i = 0; i < system->cpu_count; ++i) {
		struct cpu_t *cpu = &system->cpus[i];
		lseek(cpu->cur_freq_fd, 0, SEEK_SET);
		cpu->cur_freq = read_int_from_fd(cpu->cur_freq_fd);
	}

	// Read the whole /proc/stat file
	lseek(system->proc_stat_fd, 0, SEEK_SET);
	int len = 0;
	for (;;) {
		if (system->buffer_size == len) {
			system->buffer_size += 2048;
			system->buffer = (char *)realloc(system->buffer,
					system->buffer_size);
		}

		int bytes_to_read = system->buffer_size - len;

		int bytes_read = read_fd_to_string(system->proc_stat_fd,
				system->buffer + len, bytes_to_read);
		if (bytes_read <= 0)
			break;
		len += bytes_read;
	}
	system->buffer[len] = '\0';

	// Skip the first line
	int i = 0;
	while (i < len && system->buffer[i] != '\n')
		++i;

	// For each cpuN line in /proc/stat
	for (;;) {
		// Make sure this line starts with cpuN
		char cpuname[16];
		int bytes;
		int fields_read = sscanf(system->buffer + i,
				"%s%n", cpuname, &bytes);
		i += bytes;
		if (fields_read <= 0 || strncmp(cpuname, "cpu", 3) != 0)
			break;

		// Get the cpu id (the N in cpuN)
		int cpu_id = atoi(cpuname + 3);
		struct cpu_t *cpu = NULL;
		for (int c = 0; c < system->cpu_count; ++c) {
			if (system->cpus[c].id == cpu_id) {
				cpu = &system->cpus[c];
				break;
			}
		}

		// Calculate the cpu usage

		int stats[CPU_STATS_COUNT];
		for (int t = 0; t < CPU_STATS_COUNT; ++t) {
			sscanf(system->buffer + i, "%d%n", &stats[t], &bytes);
			i += bytes;
		}

		int delta_stats[CPU_STATS_COUNT];
		for (int t = 0; t < CPU_STATS_COUNT; ++t)
			delta_stats[t] = stats[t] - cpu->stats[t];


		int total_cpu_time = 0;
		for (int t = CPU_USER_TIME; t <= CPU_STEAL_TIME; ++t)
			total_cpu_time += delta_stats[t];

		int idle_cpu_time = delta_stats[CPU_IDLE_TIME] +
							delta_stats[CPU_IOWAIT_TIME];

		cpu->total_usage = (double)(total_cpu_time - idle_cpu_time) /
							total_cpu_time;

		// Make sure the value is in [0.0, 1.0]
		// It will also change nan values to 0.0
		if (!(cpu->total_usage >= 0.0))
			cpu->total_usage = 0.0;
		else if (cpu->total_usage > 1.0)
			cpu->total_usage = 1.0;

		for (int f = 0; f < CPU_STATS_COUNT; ++f)
			cpu->stats[f] = stats[f];
	}

	// Get the cpu core temperatures

	// Set all cpu temps to the default
	for (int i = 0; i < system->cpu_count; ++i)
		system->cpus[i].cur_temp = 0;

	// Open /sys/class/hwmon/
	DIR *hwmon_dir = opendir(HWMON_DIR);
	if (hwmon_dir == NULL)
		return;
	char filename[hwmon_dir_len + 100];
	strcpy(filename, HWMON_DIR);
	// List /sys/class/hwmon/
	struct dirent *hwmon_ent;
	while ((hwmon_ent = readdir(hwmon_dir))) {
		const char *hwmon_subdir_name = hwmon_ent->d_name;
		const int hwmon_subdir_name_len = strlen(hwmon_subdir_name);
		// Ignore dotfiles
		if (hwmon_subdir_name[0] == '.')
			continue;

		// Open /sys/class/hwmon/$hwmon_subdir_name
		strcpy(filename + hwmon_dir_len, hwmon_subdir_name);
		strcpy(filename + hwmon_dir_len + hwmon_subdir_name_len, "/");
		DIR *hwmon_subdir = opendir(filename);
		if (hwmon_subdir == NULL)
			continue;
		// List /sys/class/hwmon/$hwmon_subdir_name
		struct dirent *hwmon_subent;
		while ((hwmon_subent = readdir(hwmon_subdir))) {
			// We're looking for files that match /^temp[0-9]+_label$/
			const char *fnm = hwmon_subent->d_name;
			// Name starts with temp
			if (strncmp(fnm, "temp", 4))
				continue;
			// Followed by at least one digit
			int digits = 0;
			for (; fnm[4 + digits] >= '0' && fnm[4 + digits] <= '9'; ++digits);
			if (digits == 0)
				continue;
			// Followed by "_label"
			if (strncmp(fnm + 4 + digits, "_label", 6))
				continue;
			// And that's it
			if (fnm[4 + digits + 6] != '\0')
				continue;

			// We found our file, now open it and read it's contents
			strcpy(filename + hwmon_dir_len + hwmon_subdir_name_len + 1, fnm);
			char contents[32];
			contents[read_file_to_string(filename, contents, sizeof(contents) - 1)] = '\0';
			// We hope we just read something like /^Core [0-9]+$/
			// Contents start with "Core "
			if (strncmp(contents, "Core ", 5))
				continue;
			// Followed by at least one digit (that's out core id)
			int core_id_len = 0;
			int core_id = 0;
			for (; contents[5 + core_id_len] >= '0' && contents[5 + core_id_len] <= '9'; ++core_id_len)
				core_id = core_id * 10 + contents[5 + core_id_len] - '0';
			if (core_id_len == 0)
				continue;
			// And that's it
			if (contents[5 + core_id_len] != '\n')
				continue;

			// Now let's try to read the file that has the actual core temperature
			char temp_filename[4 + digits + 6 + 1];;
			strncpy(temp_filename, fnm, 4 + digits);
			strcpy(temp_filename + 4 + digits, "_input");
			strcpy(filename + hwmon_dir_len + hwmon_subdir_name_len + 1, temp_filename);
			contents[read_file_to_string(filename, contents, sizeof(contents) - 1)] = '\0';
			if (contents[0] == '\0')
				continue;
			int temperature = atoi(contents);

			// Set it to all CPUs with this core id
			for (int i = 0; i < system->cpu_count; ++i)
				if (system->cpus[i].core_id == core_id)
					system->cpus[i].cur_temp = temperature;
		}
		closedir(hwmon_subdir);
	}
	closedir(hwmon_dir);
}

