#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/errno.h>  

#include <glib.h>
#include "LTTTypes.h"
#include "LinuxEvents.h"

#define write_to_buffer(DEST, SRC, SIZE) \
do\
{\
   memcpy(DEST, SRC, SIZE);\
   DEST += SIZE;\
} while(0);

int readFile(int fd, void * buf, size_t size, char * mesg)
{
   ssize_t nbBytes;
   nbBytes = read(fd, buf, size);
   if(nbBytes != size){
     printf("%s\n",mesg);
     exit(1);
   }
   return 0;
}

void getDataEndianType(char * size, char * endian)
{
  int i = 1;
  char c = (char) i;
  int sizeInt=sizeof(int), sizeLong=sizeof(long), sizePointer=sizeof(void *);

  if(c == 1) strcpy(endian,"LITTLE_ENDIAN");
  else strcpy(endian, "BIG_ENDIAN");

  if(sizeInt == 2 && sizeLong == 4 && sizePointer == 4) 
    strcpy(size,"LP32");
  else if(sizeInt == 4 && sizeLong == 4 && sizePointer == 4) 
    strcpy(size,"ILP32");
  else if(sizeInt == 4 && sizeLong == 8 && sizePointer == 8) 
    strcpy(size,"LP64");
  else if(sizeInt == 8 && sizeLong == 8 && sizePointer == 8) 
    strcpy(size,"ILP64");
  else strcpy(size,"UNKNOWN");
}

#define BUFFER_SIZE 80

typedef struct _buffer_start{
  uint32_t seconds;
  uint32_t nanoseconds;
  uint64_t cycle_count;
  uint32_t block_id;  
} __attribute__ ((packed)) buffer_start;

typedef struct _heartbeat{
  uint32_t seconds;
  uint32_t nanoseconds;
  uint64_t cycle_count;
} __attribute__ ((packed)) heartbeat;


int main(int argc, char ** argv){

  int fd, *fdCpu;
  FILE * fp;
  int  fdFac, fdIntr, fdProc;
  char arch_size[BUFFER_SIZE];
  char endian[BUFFER_SIZE];
  char node_name[BUFFER_SIZE];
  char domainname[BUFFER_SIZE];
  char kernel_name[BUFFER_SIZE];
  char kernel_release[BUFFER_SIZE];
  char kernel_version[BUFFER_SIZE];
  char machine[BUFFER_SIZE];
  char processor[BUFFER_SIZE];
  char hardware_platform[BUFFER_SIZE];
  char operating_system[BUFFER_SIZE];
  int  cpu;
  int  ltt_block_size;
  int  ltt_major_version;
  int  ltt_minor_version;
  int  ltt_log_cpu;
  char buf[BUFFER_SIZE];
  int i,j;

  uint8_t cpu_id;
  struct stat      lTDFStat;   
  off_t file_size;                  
  int block_number, block_size;                 
  char * buffer, **buf_out, cpuStr[BUFFER_SIZE];
  char * buf_fac, * buf_intr, * buf_proc;
  void ** write_pos, *write_pos_fac, * write_pos_intr, *write_pos_proc;
  trace_start *tStart;
  trace_buffer_start *tBufStart;
  trace_buffer_end *tBufEnd;
  trace_file_system * tFileSys;
  uint16_t newId;
  uint8_t  evId, startId;
  uint32_t time_delta, startTimeDelta;
  void * cur_pos, *end_pos;
  buffer_start start;
  buffer_start end;
  heartbeat beat;
  int beat_count = 0;
  int *size_count;
  gboolean * has_event;
  uint32_t size_lost;
  int reserve_size = sizeof(buffer_start) + sizeof(uint16_t) + 2*sizeof(uint32_t);//lost_size and buffer_end event

  if(argc != 3){
    printf("need a trace file and cpu number\n");
    exit(1);
  }

  cpu = atoi(argv[2]);
  printf("cpu number = %d\n", cpu);


  getDataEndianType(arch_size, endian);
  printf("Arch_size: %s,  Endian: %s\n", arch_size, endian);

  fp = fopen("sysInfo.out","r");
  if(!fp){
    g_error("Unable to open file sysInfo.out\n");
  }

  for(i=0;i<9;i++){
    if(!fgets(buf,BUFFER_SIZE-1,fp))
      g_error("The format of sysInfo.out is not right\n");
    if(strncmp(buf,"node_name=",10)==0){
      strcpy(node_name,&buf[10]);
      node_name[strlen(node_name)-1] = '\0';
    }else if(strncmp(buf,"domainname=",11)==0){
      strcpy(domainname,&buf[11]);
      domainname[strlen(domainname)-1] = '\0';
    }else if(strncmp(buf,"kernel_name=",12)==0){
      strcpy(kernel_name,&buf[12]);
      kernel_name[strlen(kernel_name)-1] = '\0';
    }else if(strncmp(buf,"kernel_release=",15)==0){
      strcpy(kernel_release,&buf[15]);
      kernel_release[strlen(kernel_release)-1] = '\0';
    }else if(strncmp(buf,"kernel_version=",15)==0){
      strcpy(kernel_version,&buf[15]);
      kernel_version[strlen(kernel_version)-1] = '\0';
    }else if(strncmp(buf,"machine=",8)==0){
      strcpy(machine,&buf[8]);
      machine[strlen(machine)-1] = '\0';
    }else if(strncmp(buf,"processor=",10)==0){
      strcpy(processor,&buf[10]);
      processor[strlen(processor)-1] = '\0';
    }else if(strncmp(buf,"hardware_platform=",18)==0){
      strcpy(hardware_platform,&buf[18]);
      hardware_platform[strlen(hardware_platform)-1] = '\0';
    }else if(strncmp(buf,"operating_system=",17)==0){
      strcpy(operating_system,&buf[17]);
      operating_system[strlen(operating_system)-1] = '\0';
    }
  }
  fclose(fp);

  if(mkdir("foo", S_IFDIR | S_IRWXU | S_IRGRP |  S_IROTH)) g_error("can not make foo directory");
  if(mkdir("foo/info", S_IFDIR | S_IRWXU | S_IRGRP |  S_IROTH)) g_error("can not make foo/info directory");
  if(mkdir("foo/cpu", S_IFDIR | S_IRWXU | S_IRGRP |  S_IROTH)) g_error("can not make foo/cpu directory");
  if(mkdir("foo/control", S_IFDIR | S_IRWXU | S_IRGRP |  S_IROTH)) g_error("can not make foo/control directory");
  if(mkdir("foo/eventdefs", S_IFDIR | S_IRWXU | S_IRGRP |  S_IROTH)) g_error("can not make foo/eventdefs directory");
  
  fp = fopen("foo/info/system.xml","w");
  if(!fp){
    g_error("Unable to open file system.xml\n");
  }

  fd = open(argv[1], O_RDONLY, 0);
  if(fd < 0){
    g_error("Unable to open input data file %s\n", argv[1]);
  }

  fdFac = open("foo/control/facilities",O_CREAT | O_RDWR | O_TRUNC,S_IRUSR |S_IWUSR | S_IRGRP | S_IROTH);
  if(fdFac < 0){
    g_error("Unable to open file facilities\n");
  }
  fdIntr = open("foo/control/interrupts",O_CREAT | O_RDWR | O_TRUNC,S_IRUSR |S_IWUSR | S_IRGRP | S_IROTH);
  if(fdIntr<0){
    g_error("Unable to open file interrupts\n");
  }
  fdProc = open("foo/control/processes",O_CREAT | O_RDWR | O_TRUNC,S_IRUSR |S_IWUSR | S_IRGRP | S_IROTH);
  if(fdIntr<0){
    g_error("Unable to open file process\n");
  }



  if(fstat(fd, &lTDFStat) < 0){
    g_error("Unable to get the status of the input data file\n");
  }
  file_size = lTDFStat.st_size;

  buffer = g_new(char, 4000);
  readFile(fd,(void*)buffer, 3500, "Unable to read block header");

  cur_pos = buffer;
  evId = *(uint8_t *)cur_pos;
  cur_pos += sizeof(uint8_t);
  newId = evId;
  time_delta = *(uint32_t*)cur_pos;
  cur_pos += sizeof(uint32_t); 
  tBufStart = (trace_buffer_start*)cur_pos;
  cur_pos += sizeof(trace_buffer_start);
  cur_pos += sizeof(uint16_t); //Skip event size

  evId = *(uint8_t *)cur_pos;
  cur_pos += sizeof(uint8_t);
  time_delta = *(uint32_t*)cur_pos;
  cur_pos += sizeof(uint32_t); 
  tStart = (trace_start*)cur_pos;

  startId = newId;
  startTimeDelta = time_delta;
  start.seconds = tBufStart->Time.tv_sec;
  start.nanoseconds = tBufStart->Time.tv_usec;
  start.cycle_count = tBufStart->TSC;
  start.block_id = tBufStart->ID;
  end.block_id = start.block_id;

  ltt_major_version = tStart->MajorVersion;
  ltt_minor_version = tStart->MinorVersion;
  ltt_block_size    = tStart->BufferSize;
  ltt_log_cpu       = tStart->LogCPUID;

  block_size = ltt_block_size;
  block_number = file_size/block_size;

  g_free(buffer);
  buffer         = g_new(char, block_size);
  buf_fac        = g_new(char, block_size);
  write_pos_fac  = buf_fac;
  buf_intr       = g_new(char, block_size);
  write_pos_intr = buf_intr;
  buf_proc       = g_new(char, block_size);
  write_pos_proc = buf_proc;
  
  buf_out    = g_new(char*,cpu);
  write_pos  = g_new(void*, cpu);
  fdCpu      = g_new(int, cpu); 
  size_count = g_new(int, cpu);
  has_event  = g_new(gboolean, cpu);
  for(i=0;i<cpu;i++){
    has_event[i] = FALSE;
    if(i==0)has_event[i] = TRUE;
    buf_out[i] = g_new(char, block_size);
    write_pos[i] = NULL;;
    sprintf(cpuStr,"foo/cpu/%d\0",i);
    fdCpu[i] = open(cpuStr, O_CREAT | O_RDWR | O_TRUNC,S_IRUSR |S_IWUSR | S_IRGRP | S_IROTH); //for cpu i
    if(fdCpu[i] < 0)  g_error("Unable to open  cpu file %d\n", i);    
  }
  lseek(fd,0,SEEK_SET);


  for(i=0;i<block_number;i++){
    int event_count = 0;
    beat_count = 0;

    for(j=0;j<cpu;j++) write_pos[j] = buf_out[j];
    write_pos_intr = buf_intr;
    write_pos_fac = buf_fac;
    write_pos_proc = buf_proc;

    readFile(fd,(void*)buffer, block_size, "Unable to read block header");

    end_pos = buffer + block_size; //end of the buffer
    size_lost = *(uint32_t*)(end_pos - sizeof(uint32_t));

    end_pos = buffer + block_size - size_lost ; //buffer_end event
    tBufEnd = (trace_buffer_end*)end_pos;
    end.seconds = tBufEnd->Time.tv_sec;
    end.nanoseconds = tBufEnd->Time.tv_usec;
    end.cycle_count = tBufEnd->TSC;
    
    //skip buffer start and trace start events
    if(i==0) //the first block
      cur_pos = buffer + sizeof(trace_buffer_start) + sizeof(trace_start) + 2*(sizeof(uint8_t)+sizeof(uint16_t)+sizeof(uint32_t));
    else //other blocks
      cur_pos = buffer + sizeof(trace_buffer_start) + sizeof(uint8_t)+sizeof(uint16_t)+sizeof(uint32_t);

    for(j=0;j<cpu;j++)
      size_count[j] = sizeof(buffer_start) + sizeof(uint16_t) + sizeof(uint32_t);

    //for cpu 0, always make records
    write_to_buffer(write_pos[0],(void*)&startId, sizeof(uint16_t));    
    write_to_buffer(write_pos[0],(void*)&startTimeDelta, sizeof(uint32_t));
    write_to_buffer(write_pos[0],(void*)&start, sizeof(buffer_start));
    
    //write start block event into processes and interrupts files
    write_to_buffer(write_pos_intr,(void*)&startId, sizeof(uint16_t));    
    write_to_buffer(write_pos_intr,(void*)&startTimeDelta, sizeof(uint32_t));
    write_to_buffer(write_pos_intr,(void*)&start, sizeof(buffer_start));
    write_to_buffer(write_pos_proc,(void*)&startId, sizeof(uint16_t));    
    write_to_buffer(write_pos_proc,(void*)&startTimeDelta, sizeof(uint32_t));
    write_to_buffer(write_pos_proc,(void*)&start, sizeof(buffer_start));

    while(1){
      int event_size;
      uint64_t timeDelta;
      uint8_t  subId;

      if(ltt_log_cpu){
	cpu_id = *(uint8_t*)cur_pos;
	cur_pos += sizeof(uint8_t);
	if(cpu_id != 0 && has_event[cpu_id] == FALSE){
	  has_event[cpu_id] = TRUE;
	  write_to_buffer(write_pos[cpu_id],(void*)&startId,sizeof(uint16_t));
	  write_to_buffer(write_pos[cpu_id],(void*)&startTimeDelta, sizeof(uint32_t));
	  write_to_buffer(write_pos[cpu_id],(void*)&start, sizeof(buffer_start));	
	}
      }
      evId = *(uint8_t *)cur_pos;
      newId = evId;
      cur_pos += sizeof(uint8_t);
      time_delta = *(uint32_t*)cur_pos;
      cur_pos += sizeof(uint32_t); 

      if(ltt_log_cpu){
	write_to_buffer(write_pos[cpu_id],(void*)&newId,sizeof(uint16_t));
	write_to_buffer(write_pos[cpu_id],(void*)&time_delta, sizeof(uint32_t));     	
      }else{	
	write_to_buffer(write_pos[0],(void*)&newId,sizeof(uint16_t));
	write_to_buffer(write_pos[0],(void*)&time_delta, sizeof(uint32_t));     
      }
      
      if(evId == TRACE_BUFFER_END){
	if(ltt_log_cpu){
	  int size, i;
	  if(has_event[i])
	    write_to_buffer(write_pos[cpu_id],(void*)&end,sizeof(buffer_start));
	  for(i=0;i<cpu;i++){
	    if(has_event[i]){
	      size = block_size - size_count[i];
	      write_pos[i] = buf_out[i] + block_size - sizeof(uint32_t);
	      write_to_buffer(write_pos[i],(void*)&size, sizeof(uint32_t));
	      write(fdCpu[i],(void*)buf_out[i], block_size);	    
	    }
	  }
	}else {
	  int size = block_size - size_count[0];
	  write_to_buffer(write_pos[0],(void*)&end,sizeof(buffer_start));   
	  write_pos[0] = buf_out[0] + block_size - sizeof(uint32_t);
  	  write_to_buffer(write_pos[0],(void*)&size, sizeof(uint32_t));
	  write(fdCpu[0],(void*)buf_out[0], block_size);
	}

	//write out processes and intrrupts files
	{
	  int size_intr =(int) (write_pos_intr - (void*)buf_intr);
	  int size_proc =(int) (write_pos_proc - (void*)buf_proc);
	  write_to_buffer(write_pos_intr,(void*)&end,sizeof(buffer_start));   
	  write_to_buffer(write_pos_proc,(void*)&end,sizeof(buffer_start));   
	  write_pos_intr = buf_intr + block_size - sizeof(uint32_t);
	  write_pos_proc = buf_intr + block_size - sizeof(uint32_t);
  	  write_to_buffer(write_pos_intr,(void*)&size_intr, sizeof(uint32_t));
  	  write_to_buffer(write_pos_proc,(void*)&size_proc, sizeof(uint32_t));
	  write(fdIntr,(void*)buf_intr,block_size);	  
	  write(fdProc,(void*)buf_proc,block_size);	  
	}
	break;   
      }

      event_count++;
      switch(evId){
	case TRACE_SYSCALL_ENTRY:
	  event_size = sizeof(trace_syscall_entry);
	  break;
	case TRACE_SYSCALL_EXIT:
	  event_size = 0;
	  break;
	case TRACE_TRAP_ENTRY:
	  event_size = sizeof(trace_trap_entry);
	  break;
	case TRACE_TRAP_EXIT:
	  event_size = 0;
	  break;
	case TRACE_IRQ_ENTRY:
	  event_size = sizeof(trace_irq_entry);
	  timeDelta = time_delta;
	  write_to_buffer(write_pos_intr,(void*)&newId, sizeof(uint16_t)); 
	  write_to_buffer(write_pos_intr,(void*)&timeDelta, sizeof(uint64_t));
	  write_to_buffer(write_pos_intr,cur_pos, event_size);
	  break;
	case TRACE_IRQ_EXIT:
	  event_size = 0;
	  timeDelta = time_delta;
	  write_to_buffer(write_pos_intr,(void*)&newId, sizeof(uint16_t));
	  write_to_buffer(write_pos_intr,(void*)&timeDelta, sizeof(uint64_t));
	  break;
	case TRACE_SCHEDCHANGE:
	  event_size = sizeof(trace_schedchange);
	  break;
	case TRACE_KERNEL_TIMER:
	  event_size = 0;
	  break;
	case TRACE_SOFT_IRQ:
	  event_size = sizeof(trace_soft_irq);
	  timeDelta = time_delta;
	  write_to_buffer(write_pos_intr,(void*)&newId, sizeof(uint16_t));
	  write_to_buffer(write_pos_intr,(void*)&timeDelta, sizeof(uint64_t));
	  write_to_buffer(write_pos_intr,cur_pos, event_size);
	  break;
	case TRACE_PROCESS:
	  event_size = sizeof(trace_process);
	  timeDelta = time_delta;
	  subId = *(uint8_t*)cur_pos;
	  if(subId == TRACE_PROCESS_FORK || subId ==TRACE_PROCESS_EXIT){
	    write_to_buffer(write_pos_proc,(void*)&newId, sizeof(uint16_t));
	    write_to_buffer(write_pos_proc,(void*)&timeDelta, sizeof(uint64_t));
	    write_to_buffer(write_pos_proc,cur_pos, event_size);
	  } 
	  break;
	case TRACE_FILE_SYSTEM:
	  event_size = sizeof(trace_file_system);
	  break;
	case TRACE_TIMER:
	  event_size = sizeof(trace_timer);
	  break;
	case TRACE_MEMORY:
	  event_size = sizeof(trace_memory);
	  break;
	case TRACE_SOCKET:
	  event_size = sizeof(trace_socket);
	  break;
	case TRACE_IPC:
	  event_size = sizeof(trace_ipc);
	  break;
	case TRACE_NETWORK:
	  event_size = sizeof(trace_network);
	  break;
	case TRACE_HEARTBEAT:
	  beat_count++;
	  beat.seconds = 0;
	  beat.nanoseconds = 0;
	  beat.cycle_count = start.cycle_count + beat_count * (0XFFFF+1);
	  event_size = 0;
	  break;
        default:
	  event_size = -1;
	  break;
      }
      if(evId != TRACE_FILE_SYSTEM && event_size >=0){
	if(ltt_log_cpu){
	  size_count[cpu_id] += sizeof(uint16_t) + sizeof(uint32_t) + event_size;
	  if(size_count[cpu_id] > block_size - reserve_size){
	    printf("size count exceeds the limit of the buffer\n");
	    exit(1);
	  }
	  write_to_buffer(write_pos[cpu_id], cur_pos, event_size); 
	}else{
	  size_count[0] += sizeof(uint16_t) + sizeof(uint32_t) + event_size;
	  if(size_count[0] > block_size - reserve_size){
	    printf("size count exceeds the limit of the buffer\n");
	    exit(1);
	  }
	  write_to_buffer(write_pos[0], cur_pos, event_size); 
	}
	
	if(evId == TRACE_HEARTBEAT){
	  if(ltt_log_cpu){
	    write_to_buffer(write_pos[cpu_id], cur_pos, sizeof(heartbeat)); 	 
	  }else{
	    write_to_buffer(write_pos[0], cur_pos, sizeof(heartbeat)); 	    	  
	  }
	}

	cur_pos += event_size + sizeof(uint16_t); //skip data_size
      }else if(evId == TRACE_FILE_SYSTEM){
	size_t nbBytes;
	tFileSys = (trace_file_system*)cur_pos;
	subId = tFileSys->event_sub_id;	
	if(subId == TRACE_FILE_SYSTEM_OPEN || subId == TRACE_FILE_SYSTEM_EXEC){
	  nbBytes = tFileSys->event_data2 +1;
	}else nbBytes = 0;
	nbBytes += event_size;

	//	printf("bytes : %d\n", nbBytes);

	if(ltt_log_cpu){
	  size_count[cpu_id] += nbBytes + sizeof(uint16_t) + sizeof(uint32_t);
	  if(size_count[cpu_id] > block_size - reserve_size){
	    printf("size count exceeds the limit of the buffer\n");
	    exit(1);
	  }
	  write_to_buffer(write_pos[cpu_id], cur_pos, nbBytes); 	
	}else{
	  size_count[0] += nbBytes + sizeof(uint16_t) + sizeof(uint32_t);
	  if(size_count[0] > block_size - reserve_size){
	    printf("size count exceeds the limit of the buffer\n");
	    exit(1);
	  }
	  write_to_buffer(write_pos[0], cur_pos, nbBytes); 	
	}
	cur_pos += nbBytes + sizeof(uint16_t); //skip data_size
      }else if(event_size == -1){
	printf("Unknown event: evId=%d, i=%d, event_count=%d\n", newId, i, event_count);
	exit(1);
      }
    } //end while(1)
 
  }

  



  //write to system.xml
  fprintf(fp,"<system\n");
  fprintf(fp,"node_name=\"%s\"\n", node_name);
  fprintf(fp,"domainname=\"%s\"\n", domainname);
  fprintf(fp,"cpu=%d\n", cpu);
  fprintf(fp,"arch_size=\"%s\"\n", arch_size);
  fprintf(fp,"endian=\"%s\"\n",endian);
  fprintf(fp,"kernel_name=\"%s\"\n",kernel_name);
  fprintf(fp,"kernel_release=\"%s\"\n",kernel_release);
  fprintf(fp,"kernel_version=\"%s\"\n",kernel_version);
  fprintf(fp,"machine=\"%s\"\n",machine);
  fprintf(fp,"processor=\"%s\"\n",processor);
  fprintf(fp,"hardware_platform=\"%s\"\n",hardware_platform);
  fprintf(fp,"operating_system=\"%s\"\n",operating_system);
  fprintf(fp,"ltt_major_version=%d\n",ltt_major_version);
  fprintf(fp,"ltt_minor_version=%d\n",ltt_minor_version);
  fprintf(fp,"ltt_block_size=%d\n",ltt_block_size);
  fprintf(fp,">\n");
  fprintf(fp,"This is just a test\n");
  fprintf(fp,"</system>\n");
  fflush(fp);

  fclose(fp);

  close(fdFac);
  close(fdIntr);
  close(fdProc); 
  close(fd);
  for(i=0;i<cpu;i++) close(fdCpu[i]);

}

