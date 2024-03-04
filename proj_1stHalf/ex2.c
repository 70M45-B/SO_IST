#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include "constants.h"
#include "operations.h"
#include "parser.h"

void* child_code(char *dirpath,struct dirent *entry);

int main(int argc, char *argv[]) {
  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;

  if (argc > 3) {
    char *endptr;
    unsigned long int delay = strtoul(argv[3], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_ms = (unsigned int)delay;
  }

  if (ems_init(state_access_delay_ms)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }

  char *dirpath = argv[1];
  int const MAX_PROC = atoi(argv[2]);
  DIR *dir = opendir(dirpath);
  printf("MAX_PROC: %d\n", MAX_PROC); // FIXME remove
  if(dir == NULL) {
    fprintf(stderr, "Failed to open directory\n");
    return 1;
  }
  
  int pid;
  struct dirent *entry;
  int num_proc = 0;
  while ((entry = readdir(dir)) != NULL) {
    if(strstr(entry->d_name, ".jobs") == NULL){
      continue;
    }
    
    if(num_proc < MAX_PROC){
      pid = fork();
      if (pid == -1) {
        fprintf(stderr, "Error creating child process\n");
      }
      if(pid == 0){printf("child process %dcreated1: %d\n", getpid(),pid ); // FIXME 
      }
      num_proc++;
    }
    else{
      int status;
      pid = wait(&status);
      if(pid == -1){
        fprintf(stderr, "Error waiting for child process\n");
        continue;
      }
      if(WIFEXITED(status)){
        printf("Child process %d exited with status %d\n", pid, WEXITSTATUS(status));
      } 
      pid = fork();
      if (pid == -1) {
        fprintf(stderr, "Error creating child process\n");
      }
    }

    if(pid == 0){
      child_code(dirpath, entry);
      printf("child process %d exiting\n", getpid()); // FIXME remove
      break;
    }
  }
    
  if(pid>0){
    for(int i = 0; i < num_proc; i++){
      int status;
      pid = wait(&status);
      if(pid == -1){
        fprintf(stderr, "Error waiting for child process\n");
        continue;
      }
      if(WIFEXITED(status)){
        printf("Child process %d exited with status %d\n", pid, WEXITSTATUS(status));
      } 
    }
  }
  ems_terminate();
  if(closedir(dir) == -1){
    fprintf(stderr, "Failed to close directory\n");
    return 1;
  }
  exit(0);
}

void* child_code(char *dirpath,struct dirent *entry) {
  unsigned int event_id, delay;
  size_t num_rows, num_columns, num_coords;
  size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];
  int fd;
  char filename[strlen(entry->d_name)+1];
  strcpy(filename, entry->d_name);

  char *filepath = (char *)malloc(strlen(dirpath) + strlen("/") + strlen(filename)+1);
  strcpy(filepath, dirpath);
  strcat(filepath, "/");
  strcat(filepath, filename);

  
  //printf("> ");
  //fflush(stdout);

  fd = open(filepath, O_RDONLY);
  
  if(fd == -1) {
    free(filepath);
    fprintf(stderr, "Failed to open file\n");
    return NULL;
  }

  int var = 1;
  while (var){
    switch (get_next(fd)) {
      case CMD_CREATE:
        if (parse_create(fd, &event_id, &num_rows, &num_columns) != 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (ems_create(event_id, num_rows, num_columns)) {
          fprintf(stderr, "Failed to create event\n");
        }

        break;

      case CMD_RESERVE:
        num_coords = parse_reserve(fd, MAX_RESERVATION_SIZE, &event_id, xs, ys);

        if (num_coords == 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (ems_reserve(event_id, num_coords, xs, ys)) {
          fprintf(stderr, "Failed to reserve seats\n");
        }

        break;

      case CMD_SHOW:
        if (parse_show(fd, &event_id) != 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (ems_show(event_id, filepath)) {
          fprintf(stderr, "Failed to show event\n");
        }
        
        break;

      case CMD_LIST_EVENTS:
        if (ems_list_events()) {
          fprintf(stderr, "Failed to list events\n");
        }

        break;

      case CMD_WAIT:
        if (parse_wait(fd, &delay, NULL) == -1) {  // thread_id is not implemented
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (delay > 0) {
          printf("Waiting...\n");
          ems_wait(delay);
        }

        break;

      case CMD_INVALID:
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        break;

      case CMD_HELP:
        printf(
            "Available commands:\n"
            "  CREATE <event_id> <num_rows> <num_columns>\n"
            "  RESERVE <event_id> [(<x1>,<y1>) (<x2>,<y2>) ...]\n"
            "  SHOW <event_id>\n"
            "  LIST\n"
            "  WAIT <delay_ms> [thread_id]\n"  // thread_id is not implemented
            "  BARRIER\n"                      // Not implemented
            "  HELP\n");

        break;

      case CMD_BARRIER:  // Not implemented
      case CMD_EMPTY:
        break;

      case EOC:
        var = 0;
        break;
    }
  }
  free(filepath);//FIXME
  if(close(fd) == -1){
    fprintf(stderr, "Failed to close file\n");
    return NULL;
  }
  return NULL;
}