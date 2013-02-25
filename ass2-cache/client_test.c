#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "data.h" 
#include "pthread.h"

#define SHMSZ     1024
#define DEBUG

pthread_mutex_t mutex1;
pthread_mutex_t mutex2;
pthread_mutex_t mutex3;
static int thread_count = 0;
static int requests_per_thread = 0;
static int remainder_threads = 0;
static int num_of_threads = 0;
static int num_of_requests = 0;
static int global_thread_requests = 0;


typedef struct stats_holder {
	long num_of_requests;
	unsigned long long global_run_time;
	unsigned long long running_time[100000];
} stats_t;

stats_t stats;
	
unsigned long long max_calc()
{
	int i;
	long max_value = 0;
	for (i = 1; i<=100000; i++)
	{
		if(stats.running_time[i] > max_value)
		max_value = stats.running_time[i];
	}
	return max_value;
}

unsigned long long average_calc()
{
	int i;
	unsigned long long average = 0;
	unsigned long long sum = 0;
	for (i=1; i<=stats.num_of_requests; i++)
	sum = sum + stats.running_time[i];
	
	average = (unsigned long long) sum/ stats.num_of_requests;

	return average;
}

unsigned long long min_calc()
{
	int i;
	unsigned long long min_value = 10000000000ULL;
	for (i=1; i<=100000; i++)
	{
		if(stats.running_time[i] < min_value)
		{
			if(stats.running_time[i] != 0)
			min_value = stats.running_time[i];
		}
	}
	
	return min_value;
}

unsigned long long std_dev_calc(unsigned long long mean)
{
	int i;
	unsigned long long std_dev = 0;
	unsigned long long sum = 0;

	for (i=1; i<=stats.num_of_requests; i++)
	{
		sum = sum + (stats.running_time[i] - mean)*(stats.running_time[i]-mean);
	}
	sum = (double) sum/stats.num_of_requests;
	std_dev = (double) sqrt(sum);
	
	return std_dev;
}


void print_final_stats()
{
	unsigned long long max_value = max_calc();
	unsigned long long average = average_calc();
	unsigned long long min_value = min_calc();
	unsigned long long std_dev = std_dev_calc(average);
	unsigned long long req_per_sec = stats.global_run_time/stats.num_of_requests;
	
	printf("%lld|%lld|%lld|%lld|%lld\n", max_value, average, min_value, std_dev, req_per_sec);
}

/*First shared memory function to indicate to the server that a new client has arrived*/

int send_client()
{
	int shmid;
	key_t key;
	unsigned int *shm, *s;
 
	/*
	* We need to get the segment named
	* "1234", created by the server.
	*/
	key = 2334;
 
	/*
	* Locate the segment.
	*/
	if ((shmid = shmget(key, SHMSZ, 0666)) < 0) {
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
 
	/*
	* Client writes user input character to memory
	* for server to read.
	*/
	int tmp;
	for(;;)
	{
		srand(time(NULL));
		tmp = rand()%1000;
		*shm = tmp;
		sleep(1);
		break;
	}
	if(shmdt(shm) != 0)
		fprintf(stderr, "Could not close memory segment.\n");


	/*Delete shared memory*/	
	shmctl(shmid, IPC_RMID, NULL);
 
	return tmp;
}



void *spawnthread(void* t)
{
	device_ring* temp;
	temp = (device_ring*) t;
	unsigned char* buf;
	
	
  	int i, curr_tid, curr_thread_requests = 0;



	/*Part 1: Thread id and num_of_sector_per_thread calculation*/

	pthread_mutex_lock(&mutex1);
	thread_count++; 
	curr_tid = thread_count;

	/*For all threads (except thread tid = 1), num_of_sector_reqs = num_requests/num_of_threads. For tid = 1, num_of_sector_reqs has an additional % value to handle all requests*/
 
	if(curr_tid == 0)
	curr_thread_requests = requests_per_thread + remainder_threads;

	else
	curr_thread_requests = requests_per_thread;

	/*Unlock this lock*/
	pthread_mutex_unlock(&mutex1);





	/*Part 2: When a thread is spawned, check if any slot in the ring buffer is free. If a slot is free, assign the thread some values and put it in the slot. Do this inside a lock i.e. only one thread can access the ring buffer at a time*/

	while (curr_thread_requests != 0)
	{
		pthread_mutex_lock(&mutex2);
		for(i = 0; i<NUM_SLOTS; i++)
		{
			if (temp->slot[i].full == 0)
			{
				global_thread_requests++;
				temp->slot[i].slot_id = i;
				temp->slot[i].data_ready = 0;
				temp->slot[i].tid = curr_tid;
				temp->slot[i].global_req_id = global_thread_requests;
				//printf("Global thread requests = %d\n", global_thread_requests);
				temp->slot[i].sector_number = rand() % temp->num_of_sectors;
				curr_thread_requests--;
				clock_gettime(CLOCK_THREAD_CPUTIME_ID, &(temp->slot[i].start_time));
				temp->slot[i].full = 1;
				break;
			}	

			/*Reset the pointer to the ring buffer*/
			if (i == NUM_SLOTS - 1)
			i = 0;

		}	
		pthread_mutex_unlock(&mutex2);	


		/*At this point, the request for sector number has been inserted into the ring buffer. So now, this thread should wait until it's requested data has returned back from the server*/
		while(temp->slot[i].data_ready == 0)
		{
			//sleep(1);
		}

	
		/*Part3: Have received all data. So now, put the data and sector number into relevant files and do time calculations*/

		pthread_mutex_lock(&mutex3);
		clock_gettime(CLOCK_THREAD_CPUTIME_ID, &(temp->slot[i].end_time));
		FILE* fp;
		char filename[12];

		/*Writing sector numbers into text file*/
#ifdef DEBUG
		sprintf(filename, "sectors.%d", getpid());

       		if(!(fp = fopen(filename,"a")))
		{
			printf("Unable to open file \n");
			exit(EXIT_FAILURE);
       		}
		fprintf(fp, "%d\n", temp->slot[i].sector_number); 
		fclose(fp);
#endif


		/*Writing sector numbers in binary format*/
		/*sprintf(filename, "sectors.%d", getpid());

       		if(!(fp = fopen(filename,"a")))
		{
			printf("Unable to open file \n");
			exit(EXIT_FAILURE);
       		}
		fwrite(&temp->slot[i].sector_number, 8, 1, fp);
		fclose(fp);*/




		/*Writing actual data into read file*/
		sprintf(filename, "read.%d", getpid());

       		if(!(fp = fopen(filename,"a")))
		{
			printf("Unable to open file \n");
			exit(EXIT_FAILURE);
       		}
		fwrite(temp->slot[i].data, 1, 512, fp);



		/*Time calculations*/
		struct timespec temp1;	
		if ((temp->slot[i].end_time.tv_nsec-temp->slot[i].start_time.tv_nsec)<0) 
		{
			temp1.tv_sec = temp->slot[i].end_time.tv_sec-temp->slot[i].start_time.tv_sec-1;
			temp1.tv_nsec = 1000000000+temp->slot[i].end_time.tv_nsec-temp->slot[i].start_time.tv_nsec;
		} 
		else 
		{
			temp1.tv_sec = temp->slot[i].end_time.tv_sec-temp->slot[i].start_time.tv_sec;
			temp1.tv_nsec = temp->slot[i].end_time.tv_nsec-temp->slot[i].start_time.tv_nsec;
		}

		stats.running_time[temp->slot[i].global_req_id] = temp1.tv_sec*1000000000 + temp1.tv_nsec;

		temp->slot[i].full = 0;


		fclose(fp);	
		pthread_mutex_unlock(&mutex3);
	}


	pthread_exit(NULL);
}



int main(int argc, char* argv[])
{
	/*Shm related variables*/
	int shmid;
	key_t key;

	device_ring *t, *shm;

	/*pthread related variables*/
	pthread_t threads[10000];
	pthread_attr_t attr;
	void* status;

	int rc, i, t_num;
	struct timespec global_start, global_end;

	/*Assign Attributes to threads*/
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	/*Get this client's key for shared memory*/
	key = send_client();
	

	/*Allocate and map shared memory*/
	if ((shmid = shmget(key, sizeof(device_ring), IPC_CREAT | 0666)) < 0) 
	{
    		perror("shmget");
    		return 1;
  	}  

  	if ((shm = shmat(shmid, NULL, 0)) == (device_ring *) -1) 
	{
    		perror("shmat");
    		return 1;
  	}

	/*Imp: This assigns shared memory ptr to t* */
	t = (device_ring*) shm;	

	/*Read arguments from command line*/
	num_of_threads=atoi(argv[1]);
	num_of_requests=atoi(argv[2]);

	/*Update global stats*/
	requests_per_thread = (int) num_of_requests/num_of_threads;
	remainder_threads = (int) num_of_requests%num_of_threads;
	stats.num_of_requests = (long) num_of_requests;
	

	while(t->num_of_sectors_ready != 1)
	{ //Loop here until the number of sectors is available
	}

	/*Get start time of the whole client*/
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &global_start);

	/*Spawn threads*/
	for (t_num = 0; t_num < num_of_threads; t_num++)
	{
	 	//printf("Spawning thread tid = %d\n", t_num);
         	rc = pthread_create(&threads[t_num], &attr, spawnthread, (void*) t);
         	if (rc)
		{
             		printf("ERROR; return code from pthread_create() is %d\n", rc);
             		exit(-1);
         	}

        }     

   /* Free attribute and wait for the other threads */
	pthread_attr_destroy(&attr);
   	for(t_num=0; t_num<num_of_threads; t_num++) 
	{
      		rc = pthread_join(threads[t_num], &status);
      		if (rc)
		{
        		printf("ERROR; return code from pthread_join() is %d\n", rc);
         		exit(-1);
        	}
      	}
	/*End global time calculation*/
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &global_end);

	/*Calculate time for completion of all threads*/
	struct timespec temp2;	
	if ((global_end.tv_nsec-global_start.tv_nsec)<0) {
		temp2.tv_sec = global_end.tv_sec-global_start.tv_sec-1;
		temp2.tv_nsec = 1000000000+global_end.tv_nsec-global_start.tv_nsec;
	} else {
		temp2.tv_sec = global_end.tv_sec-global_start.tv_sec;
		temp2.tv_nsec = global_end.tv_nsec-global_start.tv_nsec;
	}

		stats.global_run_time = temp2.tv_sec*1000000000 + temp2.tv_nsec;


	/*Detach memory segment*/
	if(shmdt(shm) != 0)
    	fprintf(stderr, "Could not close memory segment.\n");

	/*Delete shared memory*/	
	shmctl(shmid, IPC_RMID, NULL);
	
  	print_final_stats();
  	pthread_exit(NULL);
	return 0;
}
