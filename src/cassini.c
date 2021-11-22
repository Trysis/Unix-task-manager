#include "../include/cassini.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "../include/timing.h"
#include "../src/timing-text-io.c"
#include <endian.h>
#include <inttypes.h>
const char usage_info[] = "\
   usage: cassini [OPTIONS] -l -> list all tasks\n\
      or: cassini [OPTIONS]    -> same\n\
      or: cassini [OPTIONS] -q -> terminate the daemon\n\
      or: cassini [OPTIONS] -c [-m MINUTES] [-H HOURS] [-d DAYSOFWEEK] COMMAND_NAME [ARG_1] ... [ARG_N]\n\
          -> add a new task and print its TASKID\n\
             format & semantics of the \"timing\" fields defined here:\n\
             https://pubs.opengroup.org/onlinepubs/9699919799/utilities/crontab.html\n\
             default value for each field is \"*\"\n\
      or: cassini [OPTIONS] -r TASKID -> remove a task\n\
      or: cassini [OPTIONS] -x TASKID -> get info (time + exit code) on all the past runs of a task\n\
      or: cassini [OPTIONS] -o TASKID -> get the standard output of the last run of a task\n\
      or: cassini [OPTIONS] -e TASKID -> get the standard error\n\
      or: cassini -h -> display this message\n\
\n\
   options:\n\
     -p PIPES_DIR -> look for the pipes in PIPES_DIR (default: /tmp/<USERNAME>/saturnd/pipes)\n\
";
 //STRUCTURES
  typedef struct string {
    uint32_t L;
    char * chaine;
  }string;
  typedef struct commandline {
    uint32_t ARGC;
    string *ARGV;
  }commandline;
//FIN STRUCTURES

int main(int argc, char * argv[]) {
  errno = 0;
  
  char * minutes_str = "*";
  char * hours_str = "*";
  char * daysofweek_str = "*";
  char * pipes_directory = "./run/pipes/";
  
  uint16_t operation;
  uint64_t taskid;
  uint16_t conv;
  
  int opt;
  char * strtoull_endp;
  
  //VARIABLES
  char * fichier_test = "/tmp/fichier.txt";
  char * fifo_request = "/saturnd-request-pipe";
  char * fifo_reply = "/saturnd-reply-pipe";
  
  char *str_request = malloc(strlen(pipes_directory)+strlen(fifo_request)+1);
  str_request[strlen(pipes_directory)+strlen(fifo_request)]='\0';
  strcat(str_request,pipes_directory);
  strcat(str_request,fifo_request);
  
  int fd=open(str_request, O_CREAT | O_WRONLY);
	if(fd<0){
		perror("open");
		exit(EXIT_FAILURE);
	}
  struct timing *time;//timing
  void *buffer;
  void *buf;
  void *buf3;
  uint16_t bigE;
  commandline command;
  string  * tab;
  int t;//timing from strings value
  int i;//indice
  char *pointeur;
  //FIN VARIABLES
  
  while ((opt = getopt(argc, argv, "hlcqm:H:d:p:r:x:o:e:")) != -1) {
    switch (opt) {
    case 'm':
      minutes_str = optarg;
      break;
    case 'H':
      hours_str = optarg;
      break;
    case 'd':
      daysofweek_str = optarg;
      break;
    case 'p':
      pipes_directory = strdup(optarg);
      if (pipes_directory == NULL) goto error;
      break;
    case 'l':
      operation = CLIENT_REQUEST_LIST_TASKS;
      break;
    case 'c':
      operation = CLIENT_REQUEST_CREATE_TASK;
      break;
    case 'q':
      operation = CLIENT_REQUEST_TERMINATE;
      break;
    case 'r':
      operation = CLIENT_REQUEST_REMOVE_TASK;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'x':
      operation = CLIENT_REQUEST_GET_TIMES_AND_EXITCODES;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'o':
      operation = CLIENT_REQUEST_GET_STDOUT;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'e':
      operation = CLIENT_REQUEST_GET_STDERR;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'h':
      printf("%s", usage_info);
      return 0;
    case '?':
      fprintf(stderr, "%s", usage_info);
      goto error;
    }
  }
  // --------
  // | TODO |
  // --------

  //SWITCH
  conv = htobe16(operation);

  switch(operation){
  case (CLIENT_REQUEST_LIST_TASKS)://'LS' *
    if(fd<0){
       exit(EXIT_FAILURE);
    }
    if(write(fd,&conv,sizeof(uint16_t))<0){
       exit(EXIT_FAILURE);
    }

    break;
    
  case (CLIENT_REQUEST_CREATE_TASK)://'CR'
	 if(write(fd,&conv,sizeof(uint16_t))<0){
       exit(EXIT_FAILURE);
    }
    time=malloc(sizeof(struct timing));//struct timing
    if(time==NULL)exit(EXIT_FAILURE);
    
    				t=timing_from_strings(time,minutes_str,hours_str,daysofweek_str);
    if(t<0)exit(EXIT_FAILURE);
    
    uint64_t conv1 = htobe64(time->minutes);
    uint32_t conv2 = htobe32(time->hours);

    if(write(fd,&conv1,sizeof(uint64_t))<0){
       exit(EXIT_FAILURE);
    } 
    if(write(fd,&conv2,sizeof(uint32_t))<0){
       exit(EXIT_FAILURE);
    } 
    if(write(fd,&(time->daysofweek),sizeof(uint8_t))<0){
       exit(EXIT_FAILURE);
    }    
    
    i=1;//In case -m -h -d option are defined
    if(strcmp(minutes_str,"*")!=0)i+=2;
    if(strcmp(hours_str,"*")!=0)i+=2;
    if(strcmp(daysofweek_str,"*")!=0)i+=2;
    
    if(argc-1<1) exit(EXIT_FAILURE);
    uint32_t arg_con = htobe32(argc-i-3);
    write(fd, &arg_con, sizeof(uint32_t));
   char c='c';
     for(int j=0;j<argc-i-3;j++){
		uint32_t taille = strlen(argv[j+i+3]);
		uint32_t taille2 = htobe32(taille);
		write(fd,&taille2, sizeof(uint32_t));
		for(int a=0;a<taille;a++){
			char k = argv[j+i+3][a];
			write(fd,&k ,1);
      }
    }
    
	free(time);
    break;
  case CLIENT_REQUEST_TERMINATE://'TM' *
    if(fd<0){
      exit(EXIT_FAILURE);
    }
    if(write(fd,&conv,sizeof(uint16_t))<0){
      exit(EXIT_FAILURE);
    }
    break;
  case CLIENT_REQUEST_REMOVE_TASK://'RM' *
    if(write(fd,&conv,sizeof(uint16_t))<0){//uint64_t est converti dans fd (utiliser des valeurs > 33)
       exit(EXIT_FAILURE);
    }
    if(write(fd,&taskid,sizeof(uint64_t))<0){//uint64_t est converti dans fd (utiliser des valeurs > 33)
       exit(EXIT_FAILURE);
    }
    break;
  case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES://'TX' *
     if(write(fd,&conv,sizeof(uint16_t))<0){//uint64_t est converti dans fd (utiliser des valeurs > 33)
       exit(EXIT_FAILURE);
    }
    if(write(fd,&taskid,sizeof(uint64_t))<0){//uint64_t est converti dans fd (utiliser des valeurs > 33)
       exit(EXIT_FAILURE);
    }
    break;
  case CLIENT_REQUEST_GET_STDOUT://'SO' *
      if(write(fd,&conv,sizeof(uint16_t))<0){//uint64_t est converti dans fd (utiliser des valeurs > 33)
       exit(EXIT_FAILURE);
    }
    if(write(fd,&taskid,sizeof(uint64_t))<0){//uint64_t est converti dans fd (utiliser des valeurs > 33)
       exit(EXIT_FAILURE);
    }
    break;
  case CLIENT_REQUEST_GET_STDERR://'SE' *
      if(write(fd,&conv,sizeof(uint16_t))<0){//uint64_t est converti dans fd (utiliser des valeurs > 33)
       exit(EXIT_FAILURE);
    }
    if(write(fd,&taskid,sizeof(uint64_t))<0){//uint64_t est converti dans fd (utiliser des valeurs > 33)
       exit(EXIT_FAILURE);
    }
    break;
  }

 
  //char *str = malloc(strlen(pipes_directory)+strlen(fifo_reply)+1);
 // str[strlen(pipes_directory)+strlen(fifo_reply)]='\0';
  
  //Ajout du chemin vers le fichier dans str
 // strcat(str,pipes_directory);
 // strcat(str,fifo_reply);

  int fd_reply=open("/home/trysis/Systeme/SY5-projet-2021-2022/run/pipes/saturnd-reply-pipe",O_RDONLY);
  struct timing tmps;
  uint16_t *buf16;
  uint64_t *buf64;
  uint32_t *buf32;
  
  uint64_t buf64tmp;//=malloc(sizeof(uint64_t));
  uint32_t buf32tmp;
  uint16_t buf16tmp;//=malloc(sizeof(uint16_t));
    
  int rd;
  int wr;
  switch(operation){	  
	case (CLIENT_REQUEST_LIST_TASKS):
	rd=read(fd_reply,&buf16tmp,sizeof(uint16_t));//REPTYPE
	if(rd<0)exit(1);
	
	rd=read(fd_reply,&buf32tmp,sizeof(uint32_t));//NBTASKS

	if(rd<0)exit(2);
	uint32_t nb=htobe32(buf32tmp);//prendre nb //printf("%" PRIu32,nb);

	int i=0;
	while(i<nb){
	rd=read(fd_reply,&buf64tmp,sizeof(uint64_t));//TASKID
	if(rd<0)exit(3);
	uint64_t buf64tmp_tob=htobe64(buf64tmp);
	//printf("%" PRIu64,buf64tmp_tob);//TASKID correct //TODO ICI Pb se fait printf a la fin dans le terminal (idk why)
	//printf("%lu",buf64tmp_tob);
	if(write(STDOUT_FILENO,&buf64tmp,sizeof(uint64_t))<0)exit(1);
		
	//rajouter un espace
	if(write(STDOUT_FILENO,": ",sizeof(char)*2)<0)exit(1);
	
	uint64_t min;
	uint32_t heure;
	uint8_t jours;
	rd=read(fd_reply,&min,sizeof(uint64_t));//MINUTES
	if(rd<0)exit(5);
	rd=read(fd_reply,&heure,sizeof(uint32_t));//HEURES
	if(rd<0)exit(6);
	rd=read(fd_reply,&jours,sizeof(uint8_t));//JOURS
	if(rd<0)exit(7);
	uint64_t tob_min=htobe64(min);
	uint32_t tob_heure=htobe32(heure);
	
	tmps.minutes=tob_min;
	tmps.hours=tob_heure;
	tmps.daysofweek=jours;
	
	char *str_buf3=malloc(TIMING_TEXT_MIN_BUFFERSIZE);
	if(str_buf3==NULL)exit(8);
	int sz=timing_string_from_timing(str_buf3,&tmps);
	
	if(write(STDOUT_FILENO,str_buf3,sz)<0)exit(1);
	
	if(write(STDOUT_FILENO," ",sizeof(char))<0)exit(1);
	free(str_buf3);
	
	commandline buf4;
        uint32_t size;
        uint32_t size_str;
        char *c;

        rd=read(fd_reply,&size,sizeof(uint32_t));//ARGC
        if(rd<0)exit(9);
	uint32_t size_tob=htobe32(size);
	buf4.ARGC=size_tob;

	//write(STDOUT_FILENO, &size, sizeof(uint32_t));
	
        buf4.ARGV=malloc(size_tob*sizeof(string));

        for(int k=0;k<size_tob;k++){
        
        rd=read(fd_reply,&size_str,sizeof(uint32_t));//string.L
        if(rd<0)exit(10);
	uint32_t size_str_tob=htobe32(size_str);
	
	buf4.ARGV[k].L=size_str_tob;
        buf4.ARGV[k].chaine=malloc(size_str_tob*sizeof(char));
        
	rd=read(fd_reply,&buf4.ARGV[k].chaine,sizeof(char)*size_str_tob);
	if(rd<0)exit(11);
	write(STDOUT_FILENO, &buf4.ARGV[k].chaine, sizeof(char)*size_str_tob);// chaine
	if(k<size_tob-1)write(STDOUT_FILENO," ", sizeof(char));//space chaine
        }
        i++;
	}
	break;
	  
	 
  case (CLIENT_REQUEST_CREATE_TASK) :
	
    rd=read(fd_reply,&buf16tmp,sizeof(uint16_t));//REPTYPE
    rd=read(fd_reply,&buf64tmp,sizeof(uint64_t));//TASKID
    

    uint64_t ok = htobe64(buf64tmp);
    
  //  char cc = '0';
   printf("%"PRIu64 "\n",ok);
    //wr=write(STDOUT_FILENO,&ok,sizeof(uint64_t));
    break;
 
  case CLIENT_REQUEST_REMOVE_TASK:

    buf16=malloc(sizeof(uint16_t));
    rd=read(fd_reply,buf16,sizeof(uint16_t));//REPTYPE
    //write(STDOUT_FILENO,buf2,sizeof(uint16_t));//
    uint16_t hto_buf=htobe16(buf16);
    if(SERVER_REPLY_ERROR==hto_buf){
      rd=read(fd_reply,&hto_buf,sizeof(uint16_t));//ERRCODE
      write(STDOUT_FILENO,&hto_buf,sizeof(uint16_t));
    }
    free(buf16);
    break;
    
  case CLIENT_REQUEST_GET_STDOUT:
    buf16=malloc(sizeof(uint16_t));
    rd=read(fd_reply,buf16,sizeof(uint16_t));//REPTYPE
    uint16_t hto_buf2=htobe16(buf16);
    string *str_buf=malloc(sizeof(struct string));;
    if(SERVER_REPLY_ERROR==hto_buf2){
      rd=read(fd_reply,&hto_buf2,sizeof(uint16_t));//ERRCODE
      write(STDOUT_FILENO,&hto_buf2,sizeof(uint16_t));
    } 
    else {
      rd=read(fd_reply,str_buf,sizeof(string));//<string>
      for(int i=0;i<str_buf->L;i++){
		write(STDOUT_FILENO,&str_buf->chaine[i],sizeof(char));
      }
      printf("\n");
    }
    free(buf16);
    free(str_buf);
    break;
    
  case CLIENT_REQUEST_GET_STDERR:
    buf16=malloc(sizeof(uint16_t));
    rd=read(fd_reply,buf16,sizeof(uint16_t));//REPTYPE 
    
    uint16_t hto_buf3=htobe16(buf16);
    string *str_buf2=malloc(sizeof(struct string));
    if(SERVER_REPLY_ERROR==hto_buf3){
      rd=read(fd_reply,&hto_buf3,sizeof(uint16_t));//ERRCODE
      write(STDOUT_FILENO,&hto_buf3,sizeof(uint16_t));
    } 
    else {
      rd=read(fd_reply,&str_buf2,sizeof(string));//<string>
      for(int i=0;i<str_buf2->L;i++){
		write(STDOUT_FILENO,&str_buf2->chaine[i],sizeof(char));
      }
      printf("\n");
    }
    free(str_buf2);
    free(buf16);
    break;
    
  case CLIENT_REQUEST_TERMINATE:
  	buf16=malloc(sizeof(uint16_t));
    	rd=read(fd_reply,buf16,sizeof(uint16_t));//REPTYPE
    free(buf16);	
    break;
  }
  //MANQUE EXIT CODE
  
  //faudrait aussi faire des goto error au lieu des exit direct
  return EXIT_SUCCESS;
  
 error:
  if (errno != 0) perror("main");
  free(pipes_directory);
  pipes_directory = NULL;
  return EXIT_FAILURE;
}