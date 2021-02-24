#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>
void hundler(int num){
  write(STDOUT_FILENO, "I won't die!\n", 13);
}
void seghundler(int num){
  write(STDOUT_FILENO, "Seg Fault!\n", 10); // reason
}
int main(int argc, char ** argv){

  struct sigaction sa;
  sa.sa_handler = hundler;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  //signal(SIGINT, hundler);
  //signal(SIGTERM, hundler);

  while(1){
    printf("Wasting your cycles. %d\n", getpid());
    sleep(1);
  }
}
