#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "data.h"
#define SHMSZ 1024

#define MAX_INDEX 19
#define CACHE_ENABLED


//unsigned char* cache;
int cache_sectors[MAX_INDEX];
static cache_insert_pointer = 0;
static int miss_count = 0;
static int hit_count = 0;

unsigned char cache[512*(2^20)];
void init_cache()
{
	posix_memalign((void*) &cache, 512, 512*(2^20));
} 

void insert_in_cache(unsigned char* line_value_temp, int address)
{
	int i;

	if(cache_insert_pointer == MAX_INDEX)
	cache_insert_pointer = 0;

	cache_sectors[cache_insert_pointer] = address;
	memcpy(&cache[cache_insert_pointer*512], line_value_temp, 512);	


	/*FILE* fp;
	fp = fopen ("cache_data.txt","w");
	for(i=cache_insert_pointer*512; i<512; i++)
	fprintf(fp, "%x\n", cache[i]);
	fclose(fp);*/
	cache_insert_pointer++;

}

unsigned char* lookup_in_cache(int address)
{

	static unsigned char line_value[512];
	int i;

	
	for(i = 0; i< MAX_INDEX; i++)
	{
		if (cache_sectors[i] == address)
		{
			memcpy(line_value, &cache[i*512], 512);	
			//for(i=0; i<512; i++)
			//printf("%x\n", line_value[i]);
			//printf("Hit\n");
			hit_count++;
			//printf("Hit count = %d\n", hit_count);
			return line_value;
		}
		
	}
	//printf("Miss\n");
	miss_count++;

	return NULL;

}

static int client_count = 0;
int getclient()
{
	char c;
	unsigned int tmp;
	int shmid;
	key_t key;
	unsigned int *shm, *s;	
 
    /*
     * Shared memory segment at 1234
     * "1234".
     */
	key = 2334;
 
    /*
     * Create the segment and set permissions.
     */
	if ((shmid = shmget(key, SHMSZ, IPC_CREAT | 0666)) < 0) {
		perror("shmget");
		return 1;
	}
 
    /*
     * Now we attach the segment to our data space.
     */
	if ((shm = shmat(shmid, NULL, 0)) == (unsigned int *) -1) {
		perror("shmat");
		return 1;
	}
 
	/*
	 * Zero out memory segment
	 */
	memset(shm,0,SHMSZ);
	s = shm;
 

 	while(*shm == 0)
	{
		sleep(1);//loop until a new client comes in
		//printf("Waiting for client\n");
	}
		tmp = *shm;
		*shm = 1;
	client_count++;
 	//printf("Got client %d", client_count++);
	if(shmdt(shm) != 0)
		fprintf(stderr, "Could not close memory segment.\n");

	/*delete shared memory*/
	shmctl(shmid, IPC_RMID, NULL);
 
	return tmp;
}

int file_exists(const char* filename)
{
	FILE* file;
	if (file = fopen(filename, "r"))
    	{
        	fclose(file);
	        return 1;
    	}
    	return 0;
}

int main(int argc, char **argv)
{

	key_t key;
	pid_t sid;	
	FILE* fptemp;
	int null = 0;	
	


	while(1)
  	{
		/*The parent thread continuously monitors for clients. The following function tells this parent whenever a new client arrives and gets the new client's key*/

		key = getclient();	
  		if (key != 1)
		{
			switch(fork())
			{
				case -1:
					printf("Failed to create child thread\n");
					exit(EXIT_FAILURE);
				
				case 0:
					/*Child thread starts here. Each child thread is a server in itself*/
					umask(0);
					sid = setsid();
					char c, tmp;
  					int shmid, i;


					//init_cache();
  					device_ring *shm, *_shm, *_data =NULL ; 

                     			/*Allocate and map shared memory*/
					if ((shmid = shmget(key, sizeof(device_ring), IPC_CREAT | 0666)) < 0)
					{
						perror("Server: shmget error");
    						return 1;
					}


  
					if ((shm = shmat(shmid, 0, 0)) == (void *) -1) 
					{
						perror("shmat error");
						return 1;
  					}
					memset(shm,0,sizeof(device_ring));
  					_shm = (device_ring*) shm;

					/* Get total number of sectors and inform client about this. This happens once for each child thread of the server*/
					FILE* fp;
					fp = fopen("disk1.img", "rb");
					fseek (fp, 0, SEEK_END);
					int size = ftell(fp);
					fclose(fp);
	
					_shm->num_of_sectors = size/512 - 1;
					_shm->num_of_sectors_ready = 1;
					

					/*Loop into all the slots in the ring buffer and service the threads*/
  					while(1)
    					{
      						//sleep(1); //Imp: to not overburden reading of ring buffer
	    					for (i=0; i < NUM_SLOTS; i++)
	    					{
	 						if( _shm->slot[i].full == 1)
							{
						      		int sector_num = _shm->slot[i].sector_number;	

	      							//printf("Started slot number = %d, tid = %d, sector number = %d\n", i, _shm->slot[i].tid, sector_num);	  
	      							unsigned char* buf;
	      							posix_memalign((void*) &buf, 512, 4096);   	   
#ifdef CACHE_ENABLED
								unsigned char* buffer_cache;

								posix_memalign((void*) &buffer_cache, 512, 512);
								buffer_cache = lookup_in_cache(sector_num);
								int j;
								if(buffer_cache != NULL)
								{
	      								memcpy(_shm->slot[i].data, buffer_cache, 512);									      //for(j=0; j<512; j++)	
									//printf("%x\n", buffer_cache[j]);

								}	
								else
								{
#endif
	      								int fd = open("disk1.img", O_DIRECT | O_SYNC);
		      							int offset = lseek(fd, sector_num*512, SEEK_SET);	 
	      								read(fd, /*_shm->slot[i].data*/ buf, 512);
		      							memcpy(_shm->slot[i].data, buf, 512);	
									insert_in_cache(buf, sector_num);
							
	      								close(fd); 	
#ifdef CACHE_ENABLED
								}
#endif

	      							//printf("Done slot number = %d, tid = %d, sector number = %d\n", i, _shm->slot[i].tid, sector_num);	  
	      							_shm->slot[i].data_ready = 1;	 
	    
	    						}


						}	
    					}
					/*detach shared memory*/	  
  					if(shmdt(shm) != 0)
					printf("Could not close memory segment.\n");
					
					/*delete shared memory*/
					shmctl(shmid, IPC_RMID, NULL);


					exit(EXIT_SUCCESS); 
  					/*end of case 0*/
				default:
				/*Parent thread*/
					//sleep(1);
					break;	
  			}
		}
		//printf("End of Switch\n");
    	}

  	return 0;
}
