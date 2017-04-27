#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t buffer;
pthread_cond_t condc,  condp;
int limit = 0;
int entry = 0;

struct item {
	int value;
	int ctimer;
} citem[32];

#define N 624
#define M 397
#define MATRIX_A 0x9908b0df   
#define UPPER_MASK 0x80000000 
#define LOWER_MASK 0x7fffffff 

#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000
#define TEMPERING_SHIFT_U(y)  (y >> 11)
#define TEMPERING_SHIFT_S(y)  (y << 7)
#define TEMPERING_SHIFT_T(y)  (y << 15)
#define TEMPERING_SHIFT_L(y)  (y >> 18)

static unsigned long mt[N]; 
static int mti=N+1; 

void sgenrand(seed) unsigned long seed;	{
    int i;

    for (i=0;i<N;i++) {
         mt[i] = seed & 0xffff0000;
         seed = 69069 * seed + 1;
         mt[i] |= (seed & 0xffff0000) >> 16;
         seed = 69069 * seed + 1;
    }
    mti = N;
}

double genrand() {
    unsigned long y;
    static unsigned long mag01[2]={0x0, MATRIX_A};

    if (mti >= N) { 
        int kk;

        if (mti == N+1)   
            sgenrand(4357); 

        for (kk=0;kk<N-M;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1];
        }
        for (;kk<N-1;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1];
        }
        y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
        mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1];

        mti = 0;
    }
  
    y = mt[mti++];
    y ^= TEMPERING_SHIFT_U(y);
    y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
    y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
    y ^= TEMPERING_SHIFT_L(y);

    return y;
}

void* producer(void *ptr) {
	int ptimer = 0;
	while(1) {
		pthread_mutex_lock(&buffer);		
		while(limit >= 32)
			pthread_cond_wait(&condp, &buffer);
		citem[limit].value = abs((unsigned int)genrand());
		citem[limit].ctimer = ((abs((unsigned int)genrand()) % 7) + 2);
		ptimer = ((abs((unsigned int)genrand()) % 4) + 3);
		limit++;
		entry++;
		pthread_cond_signal(&condc);
		pthread_mutex_unlock(&buffer);
		sleep(ptimer * 1000);
	}
}

void* consumer(void *ptr) {
	int consumetime;
	while(1) {
		pthread_mutex_lock(&buffer);
		while(limit == 0)
			pthread_cond_wait(&condc, &buffer);
		printf("Consumer consumed: %d\n", citem[limit-1].value);
		consumetime = citem[limit].ctimer * 1000;
		limit--;
		entry++;
		pthread_cond_signal(&condp);
		pthread_mutex_unlock(&buffer);
		sleep(consumetime);
	}
}

int main () {

	pthread_t prod, cons;
	 
	pthread_mutex_init(&buffer, NULL);
	pthread_cond_init(&condc, NULL);
	pthread_cond_init(&condp, NULL);

	int i = 0;
	int seed = 4567;
	sgenrand(seed);
	genrand();
	while(entry <= 20) {
			pthread_create(&cons, NULL, consumer, NULL);
			pthread_create(&prod, NULL, producer, NULL);
	}
	
	pthread_mutex_destroy(&buffer);
	pthread_cond_destroy(&condc);
	pthread_cond_destroy(&condp);
}




