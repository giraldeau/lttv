#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>  
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include "LTTTypes.h"
#include "LinuxEvents.h"

#define TRACE_HEARTBEAT_ID  19
#define PROCESS_FORK_ID     20
#define PROCESS_EXIT_ID     21

#define INFO_ENTRY          9
#define OVERFLOW_FIGURE     0x100000000ULL

typedef struct _new_process
{
  uint32_t  event_data1;     /* Data associated with event */
  uint32_t  event_data2;    
} LTT_PACKED_STRUCT new_process;

#define write_to_buffer(DEST, SRC, SIZE) \
do\
{\
   memcpy(DEST, SRC, SIZE);\
   DEST += SIZE;\
} while(0);

int readFile(int fd, void * buf, size_t size, char * mesg)
{
   ssize_t nbBytes = read(fd, buf, size);
   
   if((size_t)nbBytes != size) {
     if(nbBytes < 0) {
       perror("Error in readFile : ");
     } else {
       printf("%s\n",mesg);
     }
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

typedef struct _buffer_end{
  uint32_t seconds;
  uint32_t nanoseconds;
  uint64_t cycle_count;
  uint32_t block_id;  
} __attribute__ ((packed)) buffer_end;


typedef struct _heartbeat{
  uint32_t seconds;
  uint32_t nanoseconds;
  uint64_t cycle_count;
} __attribute__ ((packed)) heartbeat;


int main(int argc, char ** argv){

  int fd, fdCpu;
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
  int  ltt_block_size=0;
  int  ltt_major_version=0;
  int  ltt_minor_version=0;
  int  ltt_log_cpu;
  guint ltt_trace_start_size = 0;
  char buf[BUFFER_SIZE];
  int i, k;

  uint8_t cpu_id;

  char foo[4*BUFFER_SIZE];
  char foo_eventdefs[4*BUFFER_SIZE];
  char foo_control[4*BUFFER_SIZE];
  char foo_cpu[4*BUFFER_SIZE];
  char foo_info[4*BUFFER_SIZE];

  char foo_control_facilities[4*BUFFER_SIZE];
  char foo_control_processes[4*BUFFER_SIZE];
  char foo_control_interrupts[4*BUFFER_SIZE];
  char foo_info_system[4*BUFFER_SIZE];

  struct stat      lTDFStat;   
  off_t file_size;                  
  int block_number, block_size;                 
  char * buffer, *buf_out, cpuStr[4*BUFFER_SIZE];
  char * buf_fac, * buf_intr, * buf_proc;
  void * write_pos, *write_pos_fac, * write_pos_intr, *write_pos_proc;
  trace_start_any *tStart;
  trace_buffer_start *tBufStart;
  trace_buffer_end *tBufEnd;
  trace_file_system * tFileSys;
  uint16_t newId, startId, tmpId;
  uint8_t  evId;
  uint32_t time_delta, startTimeDelta;
  void * cur_pos, *end_pos;
  buffer_start start, start_proc, start_intr;
  buffer_end end, end_proc, end_intr;
  heartbeat beat;
  uint64_t adaptation_tsc;    // (Mathieu)
  uint32_t size_lost;
  int reserve_size = sizeof(buffer_start) + 
                     sizeof(buffer_end) + //buffer_end event
                     sizeof(uint32_t);  //lost size
  int nb_para;

  new_process process;

  if(argc < 4){
    printf("Usage : ./convert processfile_name number_of_cpu tracefile1 tracefile2 ... trace_creation_directory\n");
		printf("For more details, see README.\n");
    exit(1);
  }

  cpu = atoi(argv[2]);
  printf("cpu number = %d\n", cpu);
  nb_para = 3 + cpu;

  if(argc != nb_para && argc != nb_para+1){
    printf("need trace files and cpu number or root directory for the new tracefile\n");
    exit(1);
  }

  if(argc == nb_para){
    strcpy(foo, "foo");
    strcpy(foo_eventdefs, "foo/eventdefs");
    strcpy(foo_control, "foo/control");
    strcpy(foo_cpu, "foo/cpu");
    strcpy(foo_info, "foo/info");
  }else{
    strcpy(foo, argv[nb_para]);
    strcpy(foo_eventdefs, argv[nb_para]);
    strcat(foo_eventdefs,"/eventdefs");
    strcpy(foo_control, argv[nb_para]);
    strcat(foo_control,"/control");
    strcpy(foo_cpu, argv[nb_para]);
    strcat(foo_cpu,"/cpu");
    strcpy(foo_info, argv[nb_para]);
    strcat(foo_info,"/info");
  }
  strcpy(foo_control_facilities, foo_control);
  strcat(foo_control_facilities,"/facilities");
  strcpy(foo_control_processes, foo_control);
  strcat(foo_control_processes, "/processes");
  strcpy(foo_control_interrupts, foo_control);
  strcat(foo_control_interrupts, "/interrupts");
  strcpy(foo_info_system, foo_info);
  strcat(foo_info_system, "/system.xml");


  getDataEndianType(arch_size, endian);
  printf("Arch_size: %s,  Endian: %s\n", arch_size, endian);

  fp = fopen("sysInfo.out","r");
  if(!fp){
    g_error("Unable to open file sysInfo.out\n");
  }

  for(i=0;i<INFO_ENTRY;i++){
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

  if(mkdir(foo, S_IFDIR | S_IRWXU | S_IRGRP |  S_IROTH)) 
    g_error("can not make %s directory", foo);
  if(mkdir(foo_info, S_IFDIR | S_IRWXU | S_IRGRP |  S_IROTH)) 
    g_error("can not make %s directory", foo_info);
  if(mkdir(foo_cpu, S_IFDIR | S_IRWXU | S_IRGRP |  S_IROTH)) 
    g_error("can not make %s directory", foo_cpu);
  if(mkdir(foo_control, S_IFDIR | S_IRWXU | S_IRGRP |  S_IROTH)) 
    g_error("can not make %s directory", foo_control);
  if(mkdir(foo_eventdefs, S_IFDIR | S_IRWXU | S_IRGRP |  S_IROTH)) 
    g_error("can not make %s directory", foo_eventdefs);
  
  fp = fopen(foo_info_system,"w");
  if(!fp){
    g_error("Unable to open file system.xml\n");
  }

  fdFac = open(foo_control_facilities,O_CREAT | O_RDWR | O_TRUNC,S_IRUSR |S_IWUSR | S_IRGRP | S_IROTH);
  if(fdFac < 0){
    g_error("Unable to open file facilities\n");
  }
  fdIntr = open(foo_control_interrupts,O_CREAT | O_RDWR | O_TRUNC,S_IRUSR |S_IWUSR | S_IRGRP | S_IROTH);
  if(fdIntr<0){
    g_error("Unable to open file interrupts\n");
  }
  fdProc = open(foo_control_processes,O_CREAT | O_RDWR | O_TRUNC,S_IRUSR |S_IWUSR | S_IRGRP | S_IROTH);
  if(fdProc<0){
    g_error("Unable to open file process\n");
  }


  for(k=0;k<cpu;k++){
    fd = open(argv[nb_para-cpu+k], O_RDONLY, 0);
    if(fd < 0){
      g_error("Unable to open input data file %s\n", argv[nb_para-cpu+k]);
    }
    
    if(fstat(fd, &lTDFStat) < 0){
      g_error("Unable to get the status of the input data file\n");
    }
    file_size = lTDFStat.st_size;

    buffer = g_new(char, 4000);
    readFile(fd,(void*)buffer, 3500, "Unable to read block header");
    
    cur_pos= buffer;
    evId = *(uint8_t *)cur_pos;
    cur_pos += sizeof(uint8_t);
    newId = evId;
    time_delta = *(uint32_t*)cur_pos;
    cur_pos += sizeof(uint32_t); 
    tBufStart = (trace_buffer_start*)cur_pos;
    cur_pos += sizeof(trace_buffer_start);
    cur_pos += sizeof(uint16_t); //Skip event size

    evId = *(uint8_t *)cur_pos;
    g_assert(evId == TRACE_START);
    cur_pos += sizeof(uint8_t); //skip EvId
    cur_pos += sizeof(uint32_t); //skip time delta
    tStart = (trace_start_any*)cur_pos;
    if(tStart->MagicNumber != TRACER_MAGIC_NUMBER)
      g_error("Trace magic number does not match : %lx, should be %lx",
               tStart->MagicNumber, TRACER_MAGIC_NUMBER);
    if(tStart->MajorVersion != TRACER_SUP_VERSION_MAJOR)
      g_error("Trace Major number does match : %hu, should be %u",
               tStart->MajorVersion, TRACER_SUP_VERSION_MAJOR);

    startId = newId;
    startTimeDelta = time_delta;
    start.seconds = tBufStart->Time.tv_sec;
    /* Fix (Mathieu) */
    start.nanoseconds = tBufStart->Time.tv_usec * 1000;
    start.cycle_count = tBufStart->TSC;
    start.block_id = tBufStart->ID;
    end.block_id = start.block_id;


    g_printf("Trace version %hu.%hu detected\n",
             tStart->MajorVersion,
             tStart->MinorVersion);
    if(tStart->MinorVersion == 2) {
      trace_start_2_2* tStart_2_2 = (trace_start_2_2*)tStart;
      ltt_major_version = tStart_2_2->MajorVersion;
      ltt_minor_version = tStart_2_2->MinorVersion;
      ltt_block_size    = tStart_2_2->BufferSize;
      ltt_log_cpu       = tStart_2_2->LogCPUID;
      ltt_trace_start_size = sizeof(trace_start_2_2);
      /* Verify if it's a broken 2.2 format */
      if(*(uint8_t*)(cur_pos + sizeof(trace_start_2_2)) == 0) {
        /* Cannot have two trace start events. We cannot detect the problem
         * if the flight recording flag is set to 1, as it conflicts
         * with TRACE_SYSCALL_ENTRY.
         */
        g_warning("This is a 2.3 trace format that has a 2.2 tag. Please upgrade your kernel");
        g_printf("Processing the trace as a 2.3 format\n");

        tStart->MinorVersion = 3;
      }
    }
    
    if(tStart->MinorVersion == 3) {
      trace_start_2_3* tStart_2_3 = (trace_start_2_3*)tStart;
      ltt_major_version = tStart_2_3->MajorVersion;
      ltt_minor_version = tStart_2_3->MinorVersion;
      ltt_block_size    = tStart_2_3->BufferSize;
      ltt_log_cpu       = tStart_2_3->LogCPUID;
      ltt_trace_start_size = sizeof(trace_start_2_3);
    /* We do not use the flight recorder information for now, because we
     * never use the .proc file anyway */
    }
    
    if(ltt_trace_start_size == 0) 
      g_error("Minor version unknown : %hu. Supported minors : 2, 3",
               tStart->MinorVersion);

    block_size = ltt_block_size;//FIXME
    block_number = file_size/ltt_block_size;

    g_free(buffer);
    buffer         = g_new(char, ltt_block_size);
    buf_fac        = g_new(char, block_size);
    write_pos_fac  = buf_fac;
    buf_intr       = g_new(char, block_size);
    write_pos_intr = buf_intr;
    buf_proc       = g_new(char, block_size);
    write_pos_proc = buf_proc;

    buf_out = g_new(char, block_size);
    write_pos = buf_out;
    sprintf(cpuStr,"%s/%d",foo_cpu,k);
    fdCpu = open(cpuStr, O_CREAT | O_RDWR | O_TRUNC,S_IRUSR |S_IWUSR | S_IRGRP | S_IROTH); //for cpu k
    if(fdCpu < 0)  g_error("Unable to open  cpu file %d\n", k);    
    lseek(fd,0,SEEK_SET);
    
    for(i=0;i<block_number;i++){
      int event_count = 0;

      memset((void*)buf_out, 0, block_size);
      write_pos = buf_out;
      memset((void*)buf_intr, 0, block_size);    
      memset((void*)buf_fac, 0, block_size);    
      memset((void*)buf_proc, 0, block_size);    
      write_pos_intr = buf_intr;
      write_pos_fac = buf_fac;
      write_pos_proc = buf_proc;
      
      memset((void*)buffer,0,ltt_block_size); 
      readFile(fd,(void*)buffer, ltt_block_size, "Unable to read block header");
      
      cur_pos= buffer;
      evId = *(uint8_t *)cur_pos;
      cur_pos += sizeof(uint8_t);
      newId = evId;
      time_delta = *(uint32_t*)cur_pos;
      cur_pos += sizeof(uint32_t); 
      tBufStart = (trace_buffer_start*)cur_pos;
      cur_pos += sizeof(trace_buffer_start);
      cur_pos += sizeof(uint16_t); //Skip event size

      startId = newId;
      startTimeDelta = time_delta;
      start.seconds = tBufStart->Time.tv_sec;
      /* usec -> nsec (Mathieu) */
      start.nanoseconds = tBufStart->Time.tv_usec * 1000;
      start.block_id = tBufStart->ID;
      end.block_id = start.block_id;
      
      end_pos = buffer + ltt_block_size; //end of the buffer
      size_lost = *(uint32_t*)(end_pos - sizeof(uint32_t));
      
      end_pos = buffer + ltt_block_size - size_lost ; //buffer_end event
      if(ltt_log_cpu){
	tBufEnd = (trace_buffer_end*)(end_pos + 2 * sizeof(uint8_t)+sizeof(uint32_t));
      }else{
	tBufEnd = (trace_buffer_end*)(end_pos+sizeof(uint8_t)+sizeof(uint32_t));
      }
      end.seconds = tBufEnd->Time.tv_sec;
      /* usec -> nsec (Mathieu) */
      end.nanoseconds = tBufEnd->Time.tv_usec * 1000;
      // only 32 bits :( 
      //end.cycle_count = tBufEnd->TSC;
    
      //skip buffer start and trace start events
      if(i==0) {
        //the first block
        adaptation_tsc = (uint64_t)tBufStart->TSC;
	      cur_pos = buffer + sizeof(trace_buffer_start) 
                         + ltt_trace_start_size 
                         + 2*(sizeof(uint8_t)
                         + sizeof(uint16_t)+sizeof(uint32_t));
      } else {
        //other blocks
	      cur_pos = buffer + sizeof(trace_buffer_start) 
                         + sizeof(uint8_t)
                         + sizeof(uint16_t)+sizeof(uint32_t);

        /* Fix (Mathieu) */
        if(time_delta < (0xFFFFFFFFULL&adaptation_tsc)) {
          /* Overflow */
          adaptation_tsc = (adaptation_tsc&0xFFFFFFFF00000000ULL) 
                                       + 0x100000000ULL 
                                       + (uint64_t)time_delta;
        } else {
          /* No overflow */
          adaptation_tsc = (adaptation_tsc&0xFFFFFFFF00000000ULL) + time_delta;
        }

      }
      start.cycle_count = adaptation_tsc;

      //write start block event
      write_to_buffer(write_pos,(void*)&startId, sizeof(uint16_t));    
      write_to_buffer(write_pos,(void*)&startTimeDelta, sizeof(uint32_t));
      write_to_buffer(write_pos,(void*)&start, sizeof(buffer_start));
      
      //write start block event into processes and interrupts files
      write_to_buffer(write_pos_intr,(void*)&startId, sizeof(uint16_t));    
      write_to_buffer(write_pos_intr,(void*)&startTimeDelta, sizeof(uint32_t));
      start_intr = start;
      start_intr.nanoseconds -= 20;
      write_to_buffer(write_pos_intr,(void*)&start_intr, sizeof(buffer_start));

      write_to_buffer(write_pos_proc,(void*)&startId, sizeof(uint16_t));    
      write_to_buffer(write_pos_proc,(void*)&startTimeDelta, sizeof(uint32_t));
      start_proc = start;
      start_proc.nanoseconds -= 40;
      write_to_buffer(write_pos_proc,(void*)&start_proc, sizeof(buffer_start));
      
      //parse *.proc file to get process and irq info
      if(i == 0){
	int        lIntID;                 /* Interrupt ID */
	int        lPID, lPPID;            /* Process PID and Parent PID */
	char       lName[256];             /* Process name */
	FILE *          fProc;
	uint16_t        defaultId;
	trace_irq_entry irq;
	
	fProc = fopen(argv[1],"r");
	if(!fProc){
	  g_error("Unable to open file %s\n", argv[1]);
	}
	
	while(fscanf(fProc, "PID: %d; PPID: %d; NAME: %s\n", &lPID, &lPPID, lName) > 0){
	  defaultId = PROCESS_FORK_ID;
	  process.event_data1 = lPID;
	  process.event_data2 = lPPID;
	  write_to_buffer(write_pos_proc,(void*)&defaultId, sizeof(uint16_t));    
	  write_to_buffer(write_pos_proc,(void*)&startTimeDelta, sizeof(uint32_t));
	  write_to_buffer(write_pos_proc,(void*)&process, sizeof(new_process));	
	}
	
	while(fscanf(fProc, "IRQ: %d; NAME: ", &lIntID) > 0){
	  /* Read 'til the end of the line */
	  fgets(lName, 200, fProc);

	  defaultId =  TRACE_IRQ_ENTRY;
	  irq.irq_id = lIntID;
	  irq.kernel = 1;
	  write_to_buffer(write_pos_intr,(void*)&defaultId, sizeof(uint16_t));    
	  write_to_buffer(write_pos_intr,(void*)&startTimeDelta, sizeof(uint32_t));
	  write_to_buffer(write_pos_intr,(void*)&irq, sizeof(trace_irq_entry));	
	}
	fclose(fProc);
      }

      while(1){
	int event_size;
	uint64_t timeDelta;
	uint8_t  subId;
	
	if(ltt_log_cpu){
	  cpu_id = *(uint8_t*)cur_pos;
	  cur_pos += sizeof(uint8_t);
	}
	evId = *(uint8_t *)cur_pos;
	newId = evId;
	if(evId == TRACE_HEARTBEAT) {
	  newId = TRACE_HEARTBEAT_ID;
	}
	cur_pos += sizeof(uint8_t);
	time_delta = *(uint32_t*)cur_pos;
	cur_pos += sizeof(uint32_t); 


	//write event_id and time_delta
	write_to_buffer(write_pos,(void*)&newId,sizeof(uint16_t));
	write_to_buffer(write_pos,(void*)&time_delta, sizeof(uint32_t));     

  /* Fix (Mathieu) */
  if(time_delta < (0xFFFFFFFFULL&adaptation_tsc)) {
    /* Overflow */
    adaptation_tsc = (adaptation_tsc&0xFFFFFFFF00000000ULL) + 0x100000000ULL 
                                 + (uint64_t)time_delta;
  } else {
    /* No overflow */
    adaptation_tsc = (adaptation_tsc&0xFFFFFFFF00000000ULL) + time_delta;
  }


	if(evId == TRACE_BUFFER_END){
#if 0
    /* Fix (Mathieu) */
    if(time_delta < (0xFFFFFFFFULL&adaptation_tsc)) {
      /* Overflow */
     adaptation_tsc = (adaptation_tsc&0xFFFFFFFF00000000ULL) + 0x100000000ULL 
                                   + (uint64_t)time_delta;
    } else {
      /* No overflow */
      adaptation_tsc = (adaptation_tsc&0xFFFFFFFF00000000ULL) + time_delta;
    }
#endif //0
    end.cycle_count = adaptation_tsc;
	  int size = (void*)buf_out + block_size - write_pos 
                   - sizeof(buffer_end) - sizeof(uint32_t);

    /* size _lost_ ? */
    //int size = (void*)buf_out + block_size - write_pos 
    //            + sizeof(uint16_t) + sizeof(uint32_t);
    g_assert((void*)write_pos < (void*)buf_out + block_size);
	  write_to_buffer(write_pos,(void*)&end,sizeof(buffer_end));
	  write_pos = buf_out + block_size - sizeof(uint32_t);
	  write_to_buffer(write_pos,(void*)&size, sizeof(uint32_t));
	  write(fdCpu,(void*)buf_out, block_size);
	  
	  //write out processes and intrrupts files
	  {
	    int size_intr = block_size + (void*)buf_intr - write_pos_intr
                         - sizeof(buffer_end) - sizeof(uint32_t);
	    int size_proc = block_size + (void*)buf_proc - write_pos_proc
                         - sizeof(buffer_end) - sizeof(uint32_t);
      //int size_intr = block_size - (write_pos_intr - (void*)buf_intr);
      //int size_proc = block_size - (write_pos_proc - (void*)buf_proc);
	    write_to_buffer(write_pos_intr,(void*)&newId,sizeof(uint16_t));
	    write_to_buffer(write_pos_intr,(void*)&time_delta, sizeof(uint32_t));     
	    end_intr = end;
	    end_intr.nanoseconds -= 20;
	    write_to_buffer(write_pos_intr,(void*)&end_intr,sizeof(buffer_start));   
	    
	    write_to_buffer(write_pos_proc,(void*)&newId,sizeof(uint16_t));
	    write_to_buffer(write_pos_proc,(void*)&time_delta, sizeof(uint32_t));     
	    end_proc = end;
	    end_proc.nanoseconds -= 40;
	    write_to_buffer(write_pos_proc,(void*)&end_proc,sizeof(buffer_start));   

	    write_pos_intr = buf_intr + block_size - sizeof(uint32_t);
	    write_pos_proc = buf_proc + block_size - sizeof(uint32_t);
	    write_to_buffer(write_pos_intr,(void*)&size_intr, sizeof(uint32_t));
	    write_to_buffer(write_pos_proc,(void*)&size_proc, sizeof(uint32_t));
	    //for now don't output processes and interrupt information
	    //	  write(fdIntr,(void*)buf_intr,block_size);	  
	    //	  write(fdProc,(void*)buf_proc,block_size);	  
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
	    write_to_buffer(write_pos_intr,(void*)&timeDelta, sizeof(uint32_t));
	    write_to_buffer(write_pos_intr,cur_pos, event_size);
	    break;
	  case TRACE_IRQ_EXIT:
	    event_size = 0;
	    timeDelta = time_delta;
	    write_to_buffer(write_pos_intr,(void*)&newId, sizeof(uint16_t));
	    write_to_buffer(write_pos_intr,(void*)&timeDelta, sizeof(uint32_t));
	    break;
	  case TRACE_SCHEDCHANGE:
	    event_size = sizeof(trace_schedchange);
	    break;
	  case TRACE_KERNEL_TIMER:
	    event_size = 0;
	    break;
	  case TRACE_SOFT_IRQ:
	    event_size = sizeof(trace_soft_irq);
	    //	  timeDelta = time_delta;
	    //	  write_to_buffer(write_pos_intr,(void*)&newId, sizeof(uint16_t));
	    //	  write_to_buffer(write_pos_intr,(void*)&timeDelta, sizeof(uint32_t));
	    //	  write_to_buffer(write_pos_intr,cur_pos, event_size);
	    break;
	  case TRACE_PROCESS:
	    event_size = sizeof(trace_process);
	    timeDelta = time_delta;
	    subId = *(uint8_t*)cur_pos;
	    if(subId == TRACE_PROCESS_FORK || subId ==TRACE_PROCESS_EXIT){
	      if( subId == TRACE_PROCESS_FORK)tmpId = PROCESS_FORK_ID;
	      else tmpId = PROCESS_EXIT_ID;
	      write_to_buffer(write_pos_proc,(void*)&tmpId, sizeof(uint16_t));
	      write_to_buffer(write_pos_proc,(void*)&timeDelta, sizeof(uint32_t));
	      
	      process = *(new_process*)(cur_pos + sizeof(uint8_t));
	      write_to_buffer(write_pos_proc,(void*)&process, sizeof(new_process));
	    } 
	    break;
	  case TRACE_FILE_SYSTEM:
	    event_size = sizeof(trace_file_system)- sizeof(char*);
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
	    beat.seconds = 0;
	    beat.nanoseconds = 0;
	    beat.cycle_count = adaptation_tsc;
	    event_size = 0;
	    
	    write_to_buffer(write_pos_intr,(void*)&newId, sizeof(uint16_t));
	    write_to_buffer(write_pos_intr,(void*)&time_delta, sizeof(uint32_t));
	    write_to_buffer(write_pos_intr,(void*)&beat, sizeof(heartbeat)); 	    	  
	    write_to_buffer(write_pos_proc,(void*)&newId, sizeof(uint16_t));
	    write_to_buffer(write_pos_proc,(void*)&time_delta, sizeof(uint32_t));
	    write_to_buffer(write_pos_proc,(void*)&beat, sizeof(heartbeat)); 	    	  
	    break;
          default:
	    event_size = -1;
	    break;
	}
	if(evId != TRACE_FILE_SYSTEM && event_size >=0){
	  write_to_buffer(write_pos, cur_pos, event_size); 
	
	  if(evId == TRACE_HEARTBEAT){
	    write_to_buffer(write_pos, (void*)&beat, sizeof(heartbeat)); 	    	  
	  }
	  
	  cur_pos += event_size + sizeof(uint16_t); //skip data_size
	}else if(evId == TRACE_FILE_SYSTEM){
	  size_t nbBytes;
	  char  c = '\0';
	  tFileSys = (trace_file_system*)cur_pos;
	  subId = tFileSys->event_sub_id;	
	  if(subId == TRACE_FILE_SYSTEM_OPEN || subId == TRACE_FILE_SYSTEM_EXEC){
	    nbBytes = tFileSys->event_data2 +1;
	  }else nbBytes = 0;
	  
	  write_to_buffer(write_pos, cur_pos, event_size);
	  cur_pos += event_size + sizeof(char*);
	  if(nbBytes){
	    write_to_buffer(write_pos, cur_pos, nbBytes); 	
	  }else{
	    write_to_buffer(write_pos, (void*)&c, 1);
	  }
	  cur_pos += nbBytes + sizeof(uint16_t); //skip data_size
	}else if(event_size == -1){
	  printf("Unknown event: evId=%d, i=%d, event_count=%d\n", newId, i, event_count);
	  exit(1);
	}
      } //end while(1)      
    }
    close(fd);
    close(fdCpu);
    g_free(buffer);
    buffer = NULL;
    g_free(buf_fac);
    g_free(buf_intr);
    g_free(buf_proc);
    g_free(buf_out);
  }

  



  //write to system.xml
  fprintf(fp,"<system \n");
  fprintf(fp,"node_name=\"%s\" \n", node_name);
  fprintf(fp,"domainname=\"%s\" \n", domainname);
  fprintf(fp,"cpu=\"%d\" \n", cpu);
  fprintf(fp,"arch_size=\"%s\" \n", arch_size);
  fprintf(fp,"endian=\"%s\" \n",endian);
  fprintf(fp,"kernel_name=\"%s\" \n",kernel_name);
  fprintf(fp,"kernel_release=\"%s\" \n",kernel_release);
  fprintf(fp,"kernel_version=\"%s\" \n",kernel_version);
  fprintf(fp,"machine=\"%s\" \n",machine);
  fprintf(fp,"processor=\"%s\" \n",processor);
  fprintf(fp,"hardware_platform=\"%s\" \n",hardware_platform);
  fprintf(fp,"operating_system=\"%s\" \n",operating_system);
  fprintf(fp,"ltt_major_version=\"%d\" \n",ltt_major_version);
  fprintf(fp,"ltt_minor_version=\"%d\" \n",ltt_minor_version);
  fprintf(fp,"ltt_block_size=\"%d\" \n",ltt_block_size);
  fprintf(fp,">\n");
  fprintf(fp,"This is just a test\n");
  fprintf(fp,"</system>\n");
  fflush(fp);

  close(fdFac);
  close(fdIntr);
  close(fdProc); 
  fclose(fp);

  g_printf("Conversion completed. Don't forget to copy core.xml to eventdefs directory\n");
  
  return 0;
}

