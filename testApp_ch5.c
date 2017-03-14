
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

// FILE DESCRIPTOR TESTING
// Create 20 FDs and allow subsets to be opened for the open FD test. Then remove them.
#define MAX_TESTFILES (20)

const char* getTestFilePath(int number) {
  static char tname[80];
  if (number < 0)
    sprintf(tname, "./TESTFILE\\*");
  else
    sprintf(tname, "./TESTFILE%02d", number);
  return tname;
}

void create_test_file_set(int total_files) {
  int i;
  static char command[80];
  for (i=0; i<total_files; ++i) {
    sprintf(command, "/usr/bin/touch %s", getTestFilePath(i));
    run_op(command);
  }
}

void delete_test_file_set(int total_files) {
  int i;
  static char command[80];
  for (i=0; i<total_files; ++i) {
    sprintf(command, "/usr/bin/unlink %s", getTestFilePath(i));
    run_op(command);
  }
}


int open_fds[MAX_TESTFILES * 100]; // room to make mistakes
int num_open_fds = 0;

void open_test_files(int num_files, int tpid) {
  int i, pid=getpid();
  const char* fname;
  for (i=0; i<num_files; ++i) {
    int fd = open((fname = getTestFilePath(i)), O_RDONLY);
    if (fd > 0) {
      open_fds[num_open_fds++] = fd;
    } else {
      fprintf(stderr, "ERR: Pid=%d Couldn't open test file %s - %s\n", pid, fname, strerror(errno));
    }
  }
  fprintf(stdout, "Opened %d/%d test files for id=%d(by PID=%d)\n", num_open_fds, num_files, tpid, pid);
}

void close_all_testfiles(int tpid) {
  int fc = num_open_fds, i=fc, err, pid=getpid();
  while(fc--) {
    err = close(open_fds[fc]);
    if (err >= 0) {
      --num_open_fds;
    }
  }
  if (num_open_fds != 0) {
    fprintf(stderr, "ERR: Couldn't close all test files for id=%d by pid=%d\n", tpid, pid);
  } else {
    fprintf(stdout, "Closed all %d test files for pid=%d\n", i, pid);
  }
  num_open_fds = 0; // reset for next time
}

int get_num_files_needed(int raw) {
  return raw % MAX_TESTFILES;
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
  create_test_file_set(MAX_TESTFILES);
	fprintf(stdout, "Spawning %d child threads\n", num_threads);
  //pthread_mutex_lock(&running_mutex);
  child_pid = pid;
  open_test_files(1, pid);

  for (i=0; i<num_threads; ++i) {
    param_t tid = i;
    // make up a distribution of test priorities from 1-99 (Linux min/max prio)
    int new_prio = (3 * i) % 100, old_prio = getpriority(PRIO_PROCESS, child_pid);
    int psign = ((i % 2)? +1: -1); // regular >0, RT <0
    int ptype = (new_prio < 40 ? SCHED_FIFO : SCHED_RR); // only if RT used
    struct sched_param spm; // only if RT used
    int num_files; // for 5.2c tests
    new_prio = (new_prio==0 ? sched_get_priority_max(ptype): new_prio);
    num_files = get_num_files_needed(new_prio+1);
    close_all_testfiles(child_pid); // close old fd set first
    open_test_files(num_files, i);
    spm.sched_priority = new_prio;
    // once the files are all opened, we can fork() which will duplicate them
    child_pid = fork();
    //
    if (child_pid == 0) {
      thread_function((void*)tid);
      // close open file handles for child
      close_all_testfiles(tid);
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
      fprintf(stdout,"Parent spawned process(%d) pid=%d successfully, prio set from %d to %d%s with %d extra files open.\n"
        ,i,child_pid, old_prio, new_prio, (psign<0? ((ptype==SCHED_RR)? " (RR)": " (FIFO)"): "")
        , num_files
      );
    }
  }
  num_threads = i; // actual number of threads created
  //close the final child's file descriptor set
  close_all_testfiles(child_pid);
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

  delete_test_file_set(MAX_TESTFILES);
	return EXIT_SUCCESS;

}
