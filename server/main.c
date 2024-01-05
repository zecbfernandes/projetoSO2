#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h> 
#include <sys/stat.h> 
#include <string.h> 
#include "common/constants.h"
#include "common/io.h"
#include "operations.h"

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
  int fserv;

  if (mkfifo(server_pipe, 0777) < 0)
    exit (1);

  if ((fserv = open(server_pipe, O_RDONLY)) < 0)
	  exit(1);

  int session_count=0;

  while (1) {
    //TODO: Read from pipe
    int result;
    char* request_msg='\0';
    char* session_request='\0';
    char* request_pipe='\0';
    char* response_pipe='\0';
    int OP_CODE;
    unsigned int event_id;
    size_t num_rows, num_cols;
    size_t num_seats;
    size_t * xs=0;
    size_t * ys=0;
    ssize_t bytes_read;
    ssize_t bytes_written;
    int freq, fresp;
    int session_id;
    char* str='\0';
	
    if(session_count!=MAX_SESSION_COUNT){
      bytes_read = read(fserv, session_request, sizeof(char)*82);
      if (bytes_read == -1) {
        perror("Error reading from server pipe");
        break;  
      }
      strncpy(request_pipe,session_request+1,40);
      strncpy(response_pipe,session_request+41,40);

      if ((freq = open(request_pipe, O_RDONLY)) < 0)
	      exit(1);
      if ((fresp = open(response_pipe, O_WRONLY)) < 0)
	      exit(1);

      session_count+=1;
      session_id=session_count;
      sprintf(str, "%d", session_id);
      bytes_written = write(fserv, str, sizeof(char)*82);
      if (bytes_written == -1) {
        perror("Error reading from server pipe");
        break;  
      }
    }
    
    if(request_pipe!=NULL && *request_pipe != '\0'){
     bytes_read = read(freq, request_msg, sizeof(request_msg));
      if (bytes_read == -1) {
        perror("Error reading from server pipe");
        break;  
      }
      OP_CODE=request_msg[0];
    }
    switch (OP_CODE)
    {
    case 2:
      session_count-=1;
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
      //RESULT IS TO BE CONCATENATED WITH NUM_ROWS, NUM_COLS AND SEATS
      break;
    case 6:
      ems_list_events(fresp);
      //RESULT IS TO BE CONCATENATED WITH NUM_EVENTS AND IDS
      break;

    default:
      break;
    }
    free(xs);
    free(ys);
    free(session_request);
    free(request_pipe);
    free(response_pipe);
  }
  //TODO: Close Server
  close(fserv);
  unlink(server_pipe);

  ems_terminate();
}
