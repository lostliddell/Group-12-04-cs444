#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <cpuid.h>
#include <x86intrin.h>

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

_Bool supportsRDRND() {
	const unsigned int flag_RDRAND = (1 << 30);
	unsigned int level = 0;
    unsigned int eax = 0;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;
	
	//Determines if RDRND is supported or not, get_cpuid returns 0, RDRND is supported
	!__get_cpuid(level, &eax, &ebx, &ecx, &edx);
	return((ecx & flag_RDRAND) == flag_RDRAND);
}

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
	
/* 	if(supportsRDRND()) {
		unsigned int value;
		int result;
		result = _rdrand32_step(&value);
		return value;
	} */
		
	
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
		ptimer = ((abs((unsigned int)genrand()) % 4) + 3);
		wait(ptimer * 1000);
		pthread_mutex_lock(&buffer);		
		while(limit >= 32)
			pthread_cond_wait(&condp, &buffer);
		citem[limit].value = abs((unsigned int)genrand());
		citem[limit].ctimer = ((abs((unsigned int)genrand()) % 7) + 2);
		limit++;
		entry++;
		pthread_cond_signal(&condc);
		pthread_mutex_unlock(&buffer);
	}
}

void* consumer(void *ptr) {
	int consumetime;
	while(1) {
		pthread_mutex_lock(&buffer);
		while(limit == 0 || citem[limit-1].value == 0)
			pthread_cond_wait(&condc, &buffer);
		printf("Consumer consumed: %d\n", citem[limit-1].value);
		consumetime = citem[limit].ctimer * 1000;
		limit--;
		entry++;
		wait(consumetime);
		pthread_cond_signal(&condp);
		pthread_mutex_unlock(&buffer);
	}
}

int main (int argc, char *argv[]) {
	if(argc == 2) {	
		int num = atoi(argv[1]);
		
		pthread_t prod[num], cons[num];
		 
		pthread_mutex_init(&buffer, NULL);
		pthread_cond_init(&condc, NULL);
		pthread_cond_init(&condp, NULL);

		int i = 0;
		int seed = 4567;
		sgenrand(seed);
		genrand();

		for(;i < num; i++) {
			pthread_create(&prod[i], NULL, producer, NULL);
			pthread_create(&cons[i], NULL, consumer, NULL);
		}
		
		for(;i < num; i++) {
			pthread_join(prod[i], NULL);
			pthread_join(cons[i], NULL);
		}
		
		pthread_mutex_destroy(&buffer);
		pthread_cond_destroy(&condc);
		pthread_cond_destroy(&condp);
	}
	
	else
		printf("ERROR: Invalid number of inputs\n");
}




