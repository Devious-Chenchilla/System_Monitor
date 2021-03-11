#include "../includes/system.h"
#include "../includes/functions.h"
#include "../includes/struct_cpu.h"
#include "../includes/struct_disk.h"
#include "../includes/struct_process.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>
#include <signal.h>

#define TERM_CLEAR_SCREEN "\e[2J"
#define TERM_POSITION_HOME "\e[H"
#define TERM_ERASE_REST_OF_LINE "\e[K"
#define TERM_ERASE_DOWN "\e[J"


static struct termios orig_termios;

static void reset_terminal_mode(void)
{
	tcsetattr(0, TCSANOW, &orig_termios);
}

static void set_conio_terminal_mode(void)
{
	struct termios new_termios;

	// take two copies - one for now, one for later
	tcgetattr(0, &orig_termios);
	memcpy(&new_termios, &orig_termios, sizeof(new_termios));

	// register cleanup handler, and set the new terminal mode
	atexit(reset_terminal_mode);
	cfmakeraw(&new_termios);
	tcsetattr(0, TCSANOW, &new_termios);
}

static int getch(void)
{
	int r;
	unsigned char c;
	if ((r = read(STDIN_FILENO, &c, sizeof(c))) < 0)
		return r;
	return c;
}

static int wait_for_keypress(void)
{
	set_conio_terminal_mode();
	// Set timeout to 1.0 seconds
	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	// Initialize file descriptor sets
	fd_set read_fds, write_fds, except_fds;
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	FD_ZERO(&except_fds);
	FD_SET(STDIN_FILENO, &read_fds);

	int c = -1;
	if (select(STDIN_FILENO + 1, &read_fds, &write_fds, &except_fds, &timeout) == 1)
		c = getch();
	reset_terminal_mode();
	return c;
}


volatile sig_atomic_t must_exit = 0;

void signal_handler(int signum)
{
    must_exit = 1;
}




int main(int argc, char **argv)
{
	// Catch SIGTERM
	{
		struct sigaction action;
		memset(&action, 0, sizeof(struct sigaction));
		action.sa_handler = signal_handler;
		sigaction(SIGTERM, &action, NULL);
	}
	struct system_t system = system_init();

	// Loop forever, show CPU usage and frequency and disk usage

	printf(TERM_CLEAR_SCREEN TERM_POSITION_HOME);
	for (;;) {
		
		system_refresh_info(&system);
				
		int max_name_length = 9;
		for (int i = 0; i < system.disk_count; ++i) {
			int len = strlen(system.disks[i].name);
			if (len > max_name_length)
				max_name_length = len;
		}

		// CPU frequency and usage
		for (int c = 0; c < system.cpu_count; ++c) {
			const struct cpu_t *cpu = &system.cpus[c];
			if (c == 0 || cpu->core_id != system.cpus[c - 1].core_id) {
				// CPU info with temperature
				printf("CPU %d : %4d MHz %3d%% usage %3dC\n",
						c + 1,
						cpu->cur_freq / 1000,
						(int)(cpu->total_usage * 100),
						cpu->cur_temp / 1000);
			} else {
				// CPU info without temperature
				printf("CPU %d : %4d MHz %3d%% usage\n",
						c + 1,
						cpu->cur_freq / 1000,
						(int)(cpu->total_usage * 100));
			}
		}
				printf(TERM_ERASE_REST_OF_LINE "\n");
// RAM usage
		{
			char used[10], buffers[10], cached[10]/*, free[10], shared[10]*/;
			bytes_to_human_readable(system.ram_used, used);
			bytes_to_human_readable(system.ram_buffers, buffers);
			bytes_to_human_readable(system.ram_cached, cached);
			printf( "Used:    %8s\n" TERM_ERASE_REST_OF_LINE
					"Buffers: %8s\n" TERM_ERASE_REST_OF_LINE
					"Cached:  %8s\n" TERM_ERASE_REST_OF_LINE,
					used, buffers, cached);
		}

			printf(TERM_ERASE_REST_OF_LINE "\n");

		// Disk usage
		printf("%-*s        Read       Write\n", max_name_length, "Disk");
		for (int d = 0; d < system.disk_count; ++d) {
			const struct disk_t *disk = &system.disks[d];
			char read[10], write[10];
			// TODO: find a way to check actual sector size
			bytes_to_human_readable(
					disk->stats_delta[DISK_READ_SECTORS] * 512, read);
			bytes_to_human_readable(
					disk->stats_delta[DISK_WRITE_SECTORS] * 512, write);

			printf("%-*s %9s/s %9s/s" TERM_ERASE_REST_OF_LINE "\n",
					max_name_length, disk->name, read, write);
		}
		printf(TERM_ERASE_REST_OF_LINE "\n");
		
		
		/*printf("pid        ppid\n");
		for (int z = 0; z < system.process_count; ++z) {
			const struct process_t *process = &system.processes[z];
						
			printf("%d %9d" TERM_ERASE_REST_OF_LINE "\n",
					process->pid, process->ppid);
					printf(TERM_ERASE_REST_OF_LINE "\n");

		}
*/
		
		
		printf(TERM_ERASE_REST_OF_LINE
				TERM_ERASE_DOWN
				TERM_POSITION_HOME);
		
		
		
		
		fflush(stdout);

		
		



		int c = wait_for_keypress();
		if (c == 'q' || c == 'Q' || c == 3 || must_exit)
			break;
	
			
		}

			


	system_delete(system);
	return 0;

}
