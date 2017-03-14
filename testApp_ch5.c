
//
// testApp_ch4.c is an pthread application used to test Char Driver
//

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <pthread.h>
#include <sys/resource.h> // for get/setpriority()
#include <semaphore.h>
#include <sys/wait.h>
#include <sched.h> // for sched_*()

#define CMD1 1
#define CMD2 2
#define MYNUM 0x88888888
#define MYSTR "Eureka!"
#define MAX_LENGTH		256
#define READ_LENGTH		256


// Since I can't get open/read to work, try this:
// Taken from: http://stackoverflow.com/questions/646241/c-run-a-system-command-and-get-output
// Should probably redirect stderr to stdout on the command with "2>&1"
int run_op(const char* command) {

  FILE *fp;
  char path[1035];

  /* Open the command for reading. */
  fp = popen(command, "r");
  if (fp == NULL) {
    fprintf(stderr, "Failed to run command %s\n", command );
    return(1);
  }

  /* Read the output a line at a time - output it. */
  while (fgets(path, sizeof(path), fp) != NULL) {
    fprintf(stdout, "%s", path);
  }

  /* close */
  pclose(fp);

  return 0;
}

typedef unsigned long param_t;
typedef unsigned char uint8_t;

// NEEDED: mecahnism to wait until all threads are running
// This came from here: http://stackoverflow.com/questions/6154539/how-can-i-wait-for-any-all-pthreads-to-complete
volatile uint8_t running_threads = 0;
pthread_mutex_t running_mutex = PTHREAD_MUTEX_INITIALIZER;

//Use to read data from driver
void *thread_function(void *arg)
{
  // first, check in with the master that we've started up
  //pthread_mutex_lock(&running_mutex);
  running_threads--;
  //pthread_mutex_unlock(&running_mutex);

    param_t id = (param_t)arg;
    fprintf(stdout, "Thread %lu (pid=%d, ppid=%d) alive.\n", id, getpid(), getppid());
		sleep(3);
    fprintf(stdout, "Thread %lu dying.\n", id);

	return arg;
  //pthread_exit(arg);
  //exit(id);
}

int main(int argc, char *argv[])
{
	int			i, j, len, total, status, num_threads=0, fd, pid, wid;
  char testbuf[129];
	pthread_t	threads[100];
  pid_t child_pid;

	//Check options
	if(argc >= 2)
	{
		num_threads = atoi(argv[1]);
  }
	else
	{
		fprintf(stdout, " help menu..\n");
		fprintf(stdout, " ./testApp_ch4 arg1\n");
		fprintf(stdout, "   arg1: number of children to spawn (<100)\n");

		return 0;
	}

  pid = getpid();
  //pid = 2;
  printf("My TESTPID is %d (mine=%d,mypar=%d)\n", pid, getpid(), getppid());
	fprintf(stdout, "Spawning %d child threads\n", num_threads);
  //pthread_mutex_lock(&running_mutex);

  for (i=0; i<num_threads; ++i) {
    param_t tid = i;
    // make up a distribution of test priorities from 1-99 (Linux min/max prio)
    int new_prio = (3 * i) % 100, old_prio = getpriority(PRIO_PROCESS, child_pid);
    int psign = ((i % 2)? +1: -1); // regular >0, RT <0
    int ptype = (new_prio < 40 ? SCHED_FIFO : SCHED_RR); // only if RT used
    struct sched_param spm; // only if RT used
    new_prio = (new_prio==0 ? sched_get_priority_max(ptype): new_prio);
    spm.sched_priority = new_prio;
    // once the files are all opened, we can fork() which will duplicate them
    child_pid = fork();
    //
    if (child_pid == 0) {
      thread_function((void*)tid);
      // close open file handles for child
      exit (EXIT_SUCCESS);
    } else if (child_pid < 0) {
        fprintf(stderr,"ERR:on fork(%d):%s\n",i,strerror(errno));
    		break; // stop creating when we error out
    } else {
      running_threads++;
      if (psign > 0 && setpriority(PRIO_PROCESS, child_pid, new_prio) < 0) {
        fprintf(stderr, "Unable to change priority of pid=%d from %d to %d\n", child_pid, old_prio, new_prio);
        new_prio = old_prio;
      }
      if (psign < 0 && sched_setscheduler(child_pid, ptype, &spm) < 0) {
        fprintf(stderr, "Unable to change RT priority of pid=%d from %d to %d\n", child_pid, old_prio, new_prio);
        new_prio = old_prio;
      }
      fprintf(stdout,"Parent spawned process(%d) pid=%d successfully, prio set from %d to %d%s.\n"
        ,i,child_pid, old_prio, new_prio, (psign<0? ((ptype==SCHED_RR)? " (RR)": " (FIFO)"): "")
      );
    }
  }
  num_threads = i; // actual number of threads created
  //pthread_mutex_unlock(&running_mutex);
  fprintf(stdout, "Successfully created %d/%d child threads\n", num_threads, running_threads);

  // Now wait to give the threads a chance to start
  // while (running_threads > 0) {
  //   usleep(500000);
  // }
  //sleep(3);

  // reinvent cat /proc/myps
  fprintf(stdout, "Output from reading /proc/myps:\n");
  sprintf(testbuf, "/bin/echo %d > /proc/myps", pid);
  run_op(testbuf);
  run_op("/bin/cat /proc/myps");

  // shut down the threads
  //pthread_exit(NULL);
  while ((wid = wait(NULL)) > 0) {
    fprintf(stdout, "Shut down thread pid=%d\n", wid);
  }

	return EXIT_SUCCESS;

}
