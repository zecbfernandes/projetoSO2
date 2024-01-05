#define _XOPEN_SOURCE 700

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h> 
#include <sys/stat.h> 
#include <string.h> 
#include <signal.h>
#include "common/constants.h"
#include "common/io.h"
#include "operations.h"
#include <pthread.h>

void sigusr1_handler(int signo) {
    sigusr1_received = 1;
}

volatile sig_atomic_t sigusr1_received = 0;

#define BUFFER_SIZE 10

int buffer[BUFFER_SIZE];
int in = 0;
int out = 0;
int count = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_producer = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_consumer = PTHREAD_COND_INITIALIZER;

// Function that the host thread will execute (producer)
void* host_thread(void* arg) {
  // Bloqueia tids != 0
  if(!thread_equal(thread_self(), 0)) 
  {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0) {
      perror("Error blocking SIGUSR1 in threads");
      exit(EXIT_FAILURE);
    }
  };

  // Recebe signal
  if (signal(SIGUSR1, sigusr1_handler) == SIG_ERR) {
      perror("Error setting up signal handler");
      exit(EXIT_FAILURE);
  }

  int* args = (int*)arg;
  char* request_msg='\0';
  char* session_request='\0';
  char* request_pipe='\0';
  char* response_pipe='\0';
  int OP_CODE;
  ssize_t bytes_read;
  int freq, fresp;
  int fserv;
  fserv=args[0];
  fresp=args[1];
  freq=args[2];

  while(1){
    bytes_read = read(fserv, session_request, sizeof(char)*82);
    if (bytes_read == -1) {
      perror("Error reading from server pipe");  
    } 

    strncpy(request_pipe,session_request+1,40);
    strncpy(response_pipe,session_request+41,40);

    if ((freq = open(request_pipe, O_RDONLY)) < 0)
	    exit(1);
    if ((fresp = open(response_pipe, O_WRONLY)) < 0)
	    exit(1);
   
    OP_CODE=request_msg[0]; 
        
    pthread_mutex_lock(&mutex);

    while (count == BUFFER_SIZE) {
      pthread_cond_wait(&cond_producer, &mutex);
    }

    buffer[in] = OP_CODE;
    in = (in + 1) % BUFFER_SIZE;
    count++;

    pthread_cond_signal(&cond_consumer);
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
  }
}

// Function that each working thread will execute (consumer)
void* worker_thread(void* arg) {
  // Bloqueia tids != 0
  if(!thread_equal(thread_self(), 0)) 
  {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0) {
      perror("Error blocking SIGUSR1 in threads");
      exit(EXIT_FAILURE);
    }
  };

  int* args = (int*)arg;
  int result;
  int OP_CODE;
  char* request_pipe='\0';
  char* request_msg='\0';
  unsigned int event_id;
  size_t num_rows, num_cols;
  size_t num_seats;
  size_t * xs=0;
  size_t * ys=0;
  ssize_t bytes_read;
  ssize_t bytes_written;
  int freq, fresp,fserv;
  char* str='\0';
  int session_id;
  fserv=args[0];
  fresp=args[1];
  freq=args[2];
  session_id=args[3];

  while (1) {
    if (sigusr1_received) {
        ems_list_events(fresp);
        ems_show_all(fresp);
        sigusr1_received = 0;
      }
    pthread_mutex_lock(&mutex);

    while (count == 0) {
      pthread_cond_wait(&cond_consumer, &mutex);
    }

    OP_CODE = buffer[out];
    out = (out + 1) % BUFFER_SIZE;
    count--;

    pthread_cond_signal(&cond_producer);
    pthread_mutex_unlock(&mutex);
    switch (OP_CODE){
      case 1:
        sprintf(str, "%d", session_id);
        bytes_written = write(fserv, str, sizeof(char)*82);
        if (bytes_written == -1) {
          perror("Error reading from server pipe");
          break;  
        }
  
    
        if(request_pipe!=NULL && *request_pipe != '\0'){
          bytes_read = read(freq, request_msg, sizeof(request_msg));
          if (bytes_read == -1) {
            perror("Error reading from server pipe");
            break;  
          }
        }
        break;
      case 2:
        break;  
      case 3:
        memcpy(&event_id, request_msg + 1, sizeof(unsigned int));
        memcpy(&num_rows, request_msg + 1 + sizeof(unsigned int), sizeof(size_t));
        memcpy(&num_cols, request_msg + 1 + sizeof(unsigned int) + sizeof(size_t), sizeof(size_t));
        result = ems_create(event_id,num_rows,num_cols);
        bytes_written = write(fresp, &result, sizeof(result));
        if (bytes_written == -1) {
          perror("Error writing on response pipe");
          break;
        }
        break;
      case 4:

        memcpy(&event_id, request_msg + 1, sizeof(unsigned int));
        memcpy(&num_seats, request_msg + 1 + sizeof(unsigned int), sizeof(size_t));
        memcpy(xs, request_msg + 1 + sizeof(unsigned int) + sizeof(size_t), sizeof(size_t) * 40);
        memcpy(ys, request_msg + 1 + sizeof(unsigned int) + sizeof(size_t) + sizeof(size_t) * 40, sizeof(size_t) * 40);

        result=ems_reserve(event_id,num_seats,xs,ys);

        bytes_written = write(fresp, &result, sizeof(result));
        if (bytes_written == -1) {
          perror("Error writing on response pipe");
          break;
        }
        break;
      case 5:
        memcpy(&event_id, request_msg + 1,sizeof(unsigned int));
        ems_show(fresp,event_id);
        break;
      case 6:
        ems_list_events(fresp);
        break;

      default:
        break;
    }
  }
}

int main(int argc, char* argv[]) {
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s\n <pipe_path> [delay]\n", argv[0]);
    return 1;
  }

  char* endptr;
  unsigned int state_access_delay_us = STATE_ACCESS_DELAY_US;
  if (argc == 3) {
    unsigned long int delay = strtoul(argv[2], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_us = (unsigned int)delay;
  }

  if (ems_init(state_access_delay_us)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }
  //TODO: Intialize server
  char* server_pipe=argv[1];
  int fserv=0,fresp=0,freq=0;

  if (mkfifo(server_pipe, 0777) < 0)
    exit (1);

  if ((fserv = open(server_pipe, O_RDONLY)) < 0)
	  exit(1);

    //TODO: Read from pipe
    int args[4];
    pthread_t host_tid;
    pthread_t worker_tids[MAX_SESSION_COUNT - 1];
    int thread_ids[MAX_SESSION_COUNT - 1];

    // Create host thread
    if (pthread_create(&host_tid, NULL, host_thread, NULL) != 0) {
        args[0]=fserv;
        args[1]=fresp;
        args[2]=freq;
        fprintf(stderr, "Error creating host thread\n");
        exit(1);
    }

    // Create working threads
    for (int i = 0; i < MAX_SESSION_COUNT - 1; ++i) {
        thread_ids[i] = i + 1;
        args[0]=fserv;
        args[1]=fresp;
        args[2]=freq;
        args[3]=thread_ids[i];
        if (pthread_create(&worker_tids[i], NULL, worker_thread, &args) != 0) {
            fprintf(stderr, "Error creating worker thread %d\n", i);
            exit(1);
        }
    }

    // Wait for the host thread to finish
    pthread_join(host_tid, NULL);


    printf("All threads have completed.\n");
      
  
  //TODO: Close Server
  close(fserv);
  unlink(server_pipe);

  ems_terminate();
}
