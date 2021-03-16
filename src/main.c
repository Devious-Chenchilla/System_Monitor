#include "../includes/system.h"
#include "../includes/functions.h"
#include "../includes/struct_cpu.h"
#include "../includes/struct_disk.h"

#include "../includes/rdtsc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>
#include <signal.h>
#include <proc/readproc.h>
#include <proc/readproc.h>
#include <arpa/inet.h>

#define TERM_CLEAR_SCREEN "\e[2J"
#define TERM_POSITION_HOME "\e[H"
#define TERM_ERASE_REST_OF_LINE "\e[K"
#define TERM_ERASE_DOWN "\e[J"

#define SIZE 5000


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




void send_file(FILE *fp, int sockfd){
  //int n;
  char data[SIZE] = {0};

  while(fgets(data, SIZE, fp) != NULL) {
    if (send(sockfd, data, sizeof(data), 0) == -1) {
      perror("[-]Error in sending file.");
      exit(1);
    }
    bzero(data, SIZE);
  }
}

//------------------------------------------------------------------------------------**
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

//-----------------------------------------------------HELP ARGV ++ -------------------
for (int i = 1; i < argc; ++i) {
		const char *arg = argv[i];
		if (!strcmp(arg, "-h") || !strcmp(arg, "--help")) {
			printf(
					"-h --help    		Print this help message\n"
					"-local	      	    execute programme in my terminal\n"
					"-send : 127.0.0.1	execute programme in my terminal\n"

					"implement : \n"
					"    | cpuX{usage,temp,freq}     |\n"	  
					"    | ram_{used,buffers,cached} |\n"
					"    | disk_NAME_{read,write}    |\n"
					);
			return 0;
		} else if (!strcmp(arg, "-send") || !strcmp(arg, "127.0.0.1")) {
			char *ip = "127.0.0.1";
			int port = 8080;
			int e;

			int sockfd;
			struct sockaddr_in server_addr;
			FILE *fp, *fp1;
			char *filename = "../build/send.txt";

			sockfd = socket(AF_INET, SOCK_STREAM, 0);
			if(sockfd < 0) {
				perror("[-]Error in socket");
				exit(1);
			}
			printf("[+]Server socket created successfully.\n");
			printf("\n** waiting for the green light to send the data \n");
			server_addr.sin_family = AF_INET;
			server_addr.sin_port = port;
			server_addr.sin_addr.s_addr = inet_addr(ip);

			

				// Loop forever, show CPU usage and frequency and disk usage
			//-------------------------------------------------------------------------------------
			for (;;) {
				int before = rdtsc();
			fp1 = fopen(filename, "w");
			if (fp1== NULL) {
				perror("[-]Error in writing file.");
				exit(1);
			}
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
							fprintf(fp1,"CPU %d : %4d MHz %3d%% usage %3dC\n",
									c + 1,
									cpu->cur_freq / 1000,
									(int)(cpu->total_usage * 100),
									cpu->cur_temp / 1000);
						} else {
							// CPU info without temperature
							fprintf(fp1,"CPU %d : %4d MHz %3d%% usage\n",
									c + 1,
									cpu->cur_freq / 1000,
									(int)(cpu->total_usage * 100));
						}
					}
			// RAM usage
					{
						    char used[10], buffers[10], cached[10], total[10], free[10];
							bytes_to_human_readable(system.ram_used, used);
							bytes_to_human_readable(system.ram_buffers, buffers);
							bytes_to_human_readable(system.ram_cached, cached);
							bytes_to_human_readable(system.ram_total, total);
							bytes_to_human_readable(system.ram_free, free);

						fprintf(fp1, "Total:  %8s\n" TERM_ERASE_REST_OF_LINE
 									"Used:    %8s\n" TERM_ERASE_REST_OF_LINE
									"Free:  %8s\n" TERM_ERASE_REST_OF_LINE
									"Buffers: %8s\n" TERM_ERASE_REST_OF_LINE
									"Cached:  %8s\n" TERM_ERASE_REST_OF_LINE,

									total, used, free, buffers, cached);
					}


					// Disk usage
					fprintf(fp1,"%-*s        Read       Write\n", max_name_length, "Disk");
						int d = 0;
						const struct disk_t *disk = &system.disks[d];
						char read[10], write[10];
						// TODO: find a way to check actual sector size
						bytes_to_human_readable(
								disk->stats_delta[DISK_READ_SECTORS] * 512, read);
						bytes_to_human_readable(
								disk->stats_delta[DISK_WRITE_SECTORS] * 512, write);

						fprintf(fp1,"%-*s %9s/s %9s/s\n",
								max_name_length, disk->name, read, write);
				
					
					
						fprintf(fp1,"\ntid\tppid\tmem\tcpu\tstat\tcommand\n\n");											// fillarg used for cmdline
						// fillstat used for cmd
						PROCTAB* proc = openproc(PROC_FILLARG | PROC_FILLSTAT);

						proc_t proc_info;

						// zero out the allocated proc_info memory
						memset(&proc_info, 0, sizeof(proc_info));

						while (readproc(proc, &proc_info) != NULL) {
						fprintf(fp1,"%d\t%d\t%lu\t%d\t%c\t", proc_info.tid, proc_info.ppid, proc_info.vm_size, proc_info.pcpu,proc_info.state);
						if (proc_info.cmdline != NULL) {
						// print full cmd line if available
						fprintf(fp1,"[%s]\n", proc_info.cmd);
						} else {
						// if no cmd line use executable filename 
						fprintf(fp1,"[%s]\n", proc_info.cmd);
						}
						}

						closeproc(proc);
						
					// fillarg used for cmdline


					//fflush(stdout);
					
			//-----------------------------*$$$$

					int c = wait_for_keypress();
					if (c == 'q' || c == 'Q' || c == 3 || must_exit){
						fclose(fp1);
							e = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
							if(e == -1) {
								perror("[-]Error in socket");
								exit(1);
							}
								printf("[+]Connected to Server.\n");

							fp = fopen(filename, "r");
							if (fp == NULL) {
								perror("[-]Error in reading file.");
								exit(1);
							}

							send_file(fp, sockfd);
							printf("[+]File data sent successfully.\n");

								printf("[+]Closing the connection.\n");

							break;

									
					}
					int after = rdtsc();
							printf("temps ecoulé %d", after - before);
						
					}
								close(sockfd);

										system_delete(system);
						
						return 0;
		} else if (!strcmp(arg, "-local")) {

					// Catch SIGTERM
					{
						struct sigaction action;
						memset(&action, 0, sizeof(struct sigaction));
						action.sa_handler = signal_handler;
						sigaction(SIGTERM, &action, NULL);
					}
								// Loop forever, show CPU usage and frequency and disk usage
					int before = rdtsc();
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
								printf("CPU %d : %4d MHz %3d%% usage %3dC"
										TERM_ERASE_REST_OF_LINE "\n",
										c + 1,
										cpu->cur_freq / 1000,
										(int)(cpu->total_usage * 100),
										cpu->cur_temp / 1000);
							} else {
								// CPU info without temperature
								printf("CPU %d : %4d MHz %3d%% usage"
										TERM_ERASE_REST_OF_LINE "\n",
										c + 1,
										cpu->cur_freq / 1000,
										(int)(cpu->total_usage * 100));
							}
						}
						printf(TERM_ERASE_REST_OF_LINE "\n");

						// RAM usage
						{
							char used[10], buffers[10], cached[10], total[10], free[10];
							bytes_to_human_readable(system.ram_used, used);
							bytes_to_human_readable(system.ram_buffers, buffers);
							bytes_to_human_readable(system.ram_cached, cached);
							bytes_to_human_readable(system.ram_total, total);
							bytes_to_human_readable(system.ram_free, free);

							printf( "Total:   %8s\n" TERM_ERASE_REST_OF_LINE
 									"Used:    %8s\n" TERM_ERASE_REST_OF_LINE
									"Free:    %8s\n" TERM_ERASE_REST_OF_LINE
									"Buffers: %8s\n" TERM_ERASE_REST_OF_LINE
									"Cached:  %8s\n" TERM_ERASE_REST_OF_LINE,

									total, used, free, buffers, cached);
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
									printf("\n\n\n\n");
						}

						printf(TERM_ERASE_REST_OF_LINE "\n");
						
						

						printf(TERM_ERASE_REST_OF_LINE
								TERM_ERASE_DOWN
								TERM_POSITION_HOME"\n");
						fflush(stdout);

						int c = wait_for_keypress();
						if (c == 'q' || c == 'Q' || c == 3 || must_exit){
							printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
							printf("\ntid\tppid\tmem\tcpu\tprocID\tstat\tcommand\n\n");
												// fillarg used for cmdline
							// fillstat used for cmd
							PROCTAB* proc = openproc(PROC_FILLARG | PROC_FILLSTAT);

							proc_t proc_info;

							// zero out the allocated proc_info memory
							memset(&proc_info, 0, sizeof(proc_info));
							char pourc = '%';
							while (readproc(proc, &proc_info) != NULL) {
							printf("%d\t%d\t%lu\t%d%c\t%d\t%c\t", proc_info.tid, proc_info.ppid, proc_info.vm_size, proc_info.pcpu,pourc,proc_info.processor,proc_info.state);
							if (proc_info.cmdline != NULL) {
							// print full cmd line if available
							printf("[%s]\n", proc_info.cmd);
							} else {
							// if no cmd line use executable filename 
							printf("[%s]\n", proc_info.cmd);
							}
							}

							closeproc(proc);
								
								break;
					}
					}


					system_delete(system);
															int after = rdtsc();
							printf("temps ecoulé %d cycles", after - before);

							return 0;

		}
}





//------------------------------------------------------socket-------------------------

  

	

}
