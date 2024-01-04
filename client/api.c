#include "api.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "common/constants.h"

struct client{
  int session_id;
  int req_pipe;
  int resp_pipe;
  char[40] req_path;
  char[40] resp_path
};

struct client user;

int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path) {
  //TODO: create pipes and connect to the server
  int freq, fresp;
  int OP_CODE=1;

  unlink(req_pipe_path);
  unlink(resp_pipe_path);

  if (mkfifo(req_pipe_path, 0777) < 0)
    exit (1);
  if (mkfifo(resp_pipe_path, 0777) < 0)
    exit (1);

  if ((freq = open(req_pipe_path, O_RDONLY)) < 0)
	  exit(1);
  if ((fresp = open(resp_pipe_path, O_WRONLY)) < 0)
	  exit(1);

  user->req_pipe=freq;
  user->resp_pipe=fresp;
  user->req_path=strdup(req_pipe_path);
  user->resp_path=strdup(resp_pipe_path);

  char request_msg[1 + sizeof(char*40) + sizeof(char*40)];
  request_msg[0] = OP_CODE;
  memcpy(request_msg + 1, &req_pipe_path, sizeof(char*40));
  memcpy(request_msg + 1 + sizeof(unsigned int), &resp_pipe_path, sizeof(char*40)); 

  ssize_t bytes_written = write(user->req_pipe, request_msg, sizeof(request_msg));
    if (bytes_written == -1) {
        perror("Erro ao escrever no pipe de solicitações");
        return 1; 
    }

    // Ler a resposta do servidor do pipe de respostas
    int server_response;
    ssize_t bytes_read = read(user->resp_pipe, &server_response, sizeof(server_response));
    if (bytes_read == -1) {
        perror("Erro ao ler a resposta do servidor");
        return 1;
    }

  user->session_id=server_response;
  
  return 0;
}

int ems_quit(void) { 
  //TODO: close pipes
  int OP_CODE=2;
  char request_msg[1];
  request_msg[0] = OP_CODE;
  

  ssize_t bytes_written = write(user->req_pipe, request_msg, sizeof(request_msg));
  if (bytes_written == -1) {
      perror("Erro ao escrever no pipe de solicitações");
      return 1; 
  }

  close(user->req_pipe);
  close(user->resp_pipe);
  unlink(user->resp_path);
  unlink(user->req_path);
  free(user->req_path);
  free(user->resp_path)
  return 0;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
  //TODO: send create request to the server (through the request pipe) and wait for the response (through the response pipe)
  int OP_CODE=3;
  char request_msg[1 + sizeof(unsigned int) + sizeof(size_t) + sizeof(size_t)];
  request_msg[0] = OP_CODE;
  memcpy(request_msg + 1, &event_id, sizeof(unsigned int));
  memcpy(request_msg + 1 + sizeof(unsigned int), &num_rows, sizeof(size_t));
  memcpy(request_msg + 1 + sizeof(unsigned int) + sizeof(size_t), &num_cols, sizeof(size_t));

  ssize_t bytes_written = write(user->req_pipe, request_msg, sizeof(request_msg));
    if (bytes_written == -1) {
        perror("Erro ao escrever no pipe de solicitações");
        return 1; 
    }

    // Ler a resposta do servidor do pipe de respostas
    int server_response;
    ssize_t bytes_read = read(user->resp_pipe, &server_response, sizeof(server_response));
    if (bytes_read == -1) {
        perror("Erro ao ler a resposta do servidor");
        return 1;
    } 
  return server_response;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  //TODO: send reserve request to the server (through the request pipe) and wait for the response (through the response pipe)
  int OP_CODE=4;
  char request_msg[1 + sizeof(unsigned int) + sizeof(size_t) + sizeof(size_t*40) + sizeof(size_t*40)];
  request_msg[0] = OP_CODE;
  memcpy(request_msg + 1, &event_id, sizeof(unsigned int));
  memcpy(request_msg + 1 + sizeof(unsigned int), &num_seats, sizeof(size_t));
  memcpy(request_msg + 1 + sizeof(unsigned int) + sizeof(size_t), &xs, sizeof(size_t*num_seats));
  memcpy(request_msg + 1 + sizeof(unsigned int) + sizeof(size_t) + sizeof(size_t*num_seats), &ys, sizeof(size_t*num_seats));

  ssize_t bytes_written = write(user->req_pipe, request_msg, sizeof(request_msg));
    if (bytes_written == -1) {
        perror("Erro ao escrever no pipe de solicitações");
        return 1; 
    }

    // Ler a resposta do servidor do pipe de respostas
    int server_response;
    ssize_t bytes_read = read(user->resp_pipe, &server_response, sizeof(server_response));
    if (bytes_read == -1) {
        perror("Erro ao ler a resposta do servidor");
        return 1;
    }
  return server_response;
}

int ems_show(int out_fd, unsigned int event_id) {
  //TODO: send show request to the server (through the request pipe) and wait for the response (through the response pipe)
  int OP_CODE=5;
  char request_msg[1 + sizeof(unsigned int)];
  request_msg[0] = OP_CODE;
  memcpy(request_msg + 1, &event_id, sizeof(unsigned int));

  ssize_t bytes_written = write(user->req_pipe, request_msg, sizeof(request_msg));
    if (bytes_written == -1) {
        perror("Erro ao escrever no pipe de solicitações");
        return 1; 
    }

    // Ler a resposta do servidor do pipe de respostas
    int server_response;
    ssize_t bytes_read = read(user->resp_pipe, &server_response, sizeof(server_response));
    if (bytes_read == -1) {
        perror("Erro ao ler a resposta do servidor");
        return 1;
    }
  return server_response;

}

int ems_list_events(int out_fd) {
  //TODO: send list request to the server (through the request pipe) and wait for the response (through the response pipe)
  int OP_CODE=6;
  char request_msg[1];
  request_msg[0] = OP_CODE;
  

  ssize_t bytes_written = write(user->req_pipe, request_msg, sizeof(request_msg));
    if (bytes_written == -1) {
        perror("Erro ao escrever no pipe de solicitações");
        return 1; 
    }

    // Ler a resposta do servidor do pipe de respostas
    int server_response;
    ssize_t bytes_read = read(user->resp_pipe, &server_response, sizeof(server_response));
    if (bytes_read == -1) {
        perror("Erro ao ler a resposta do servidor");
        return 1;
    }
  return server_response;
  
}
