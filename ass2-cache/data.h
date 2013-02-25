#define NUM_SLOTS 100

typedef struct _slot{
  int preamble;
  int tid;	
  int global_req_id;	
  int sector_number;
  int slot_id;
  int data_ready;
  int full; 	 
  struct timespec start_time, end_time;
  unsigned char data[512];
} slot_t;

typedef struct _device_ring{
  int num_of_sectors;
  slot_t slot[NUM_SLOTS];  
  int num_of_sectors_ready;
} device_ring;

pthread_mutex_t mutex_disk;

