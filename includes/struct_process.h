#include <stdio.h>
#include <string.h>
#include <proc/readproc.h>

int main(int argc, char** argv)
{
 // fillarg used for cmdline
 // fillstat used for cmd
 PROCTAB* proc = openproc(PROC_FILLARG | PROC_FILLSTAT);

 proc_t proc_info;

 // zero out the allocated proc_info memory
 memset(&proc_info, 0, sizeof(proc_info));

 while (readproc(proc, &proc_info) != NULL) {
  printf("%-10d %-10d ", proc_info.tid, proc_info.ppid);
  if (proc_info.cmdline != NULL) {
   // print full cmd line if available
   printf("%s\n", *proc_info.cmdline);
  } else {
   // if no cmd line use executable filename 
   printf("[%s]\n", proc_info.cmd);
  }
 }

 closeproc(proc);
}