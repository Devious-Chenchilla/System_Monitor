#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>
#include<string.h>
/*
sigaction(SIGTERM, &action, NULL) définit l'action à effectuer pour le SIGTERM signal de la term() fonction. Cela signifie que term() sera appelée lorsque le programme reçoit un SIGTERM. Il permettra de définir la variable globale done à l' 1, et qui va provoquer la boucle d'arrêter après la fin de l'exécution en cours. Ça y est!
*/
volatile sig_atomic_t done = 0;
void term(int signum){
  done = 1;
}

int main(int argc, char ** argv){
  struct sigaction action;
  memset(&action, 0, sizeof(struct sigaction));
  action.sa_handler = term;
  sigaction(SIGTERM, &action, NULL);

  int loop = 0;
  while(!done){
    int t = sleep(1);

    while(t > 0){
      printf("Loop run wasinterrupted with %d "
	     "sec to go, finishing...\n", t);
    }
    printf("Finished loop run %d.\n", loop++);
  }
  printf("done.\n");

  return 0;
}
