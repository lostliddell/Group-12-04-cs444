#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <limits.h>
//#include <immintrin.h>
//#include <stdint.h>

// Philosopher States
#define THINKING  0
#define HUNGRY  1
#define EATING  2

/* Period parameters */  
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UPPER_MASK 0x80000000UL /* most significant w-r bits */
#define LOWER_MASK 0x7fffffffUL /* least significant r bits */

static unsigned long mt[N]; /* the array for the state vector  */
static int mti=N+1; /* mti==N+1 means mt[N] is not initialized */

/* Philosopher paramters */
pthread_mutex_t bowl; // Main mutex

struct philo_Struct {
	pthread_cond_t condp; // Condition flags for each philosopher
	int state;
	int blocktime; // Time philosopher is blocked
} people[5];

/*START OF MERSAND TWISTER CODE BLOCK*/
/* initializes mt[N] with a seed */
void init_genrand(unsigned long s)
{
   mt[0]= s & 0xffffffffUL;
   for (mti=1; mti<N; mti++) {
      mt[mti] = 
	 (1812433253UL * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti); 
      /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
      /* In the previous versions, MSBs of the seed affect   */
      /* only MSBs of the array mt[].                        */
      /* 2002/01/09 modified by Makoto Matsumoto             */
      mt[mti] &= 0xffffffffUL;
      /* for >32 bit machines */
   }
}

/* generates a random number on [0,0xffffffff]-interval */
unsigned long genrand_int32(void)
{
   unsigned long y;
   static unsigned long mag01[2]={0x0UL, MATRIX_A};
   /* mag01[x] = x * MATRIX_A  for x=0,1 */

   if (mti >= N) { /* generate N words at one time */
      int kk;

      if (mti == N+1)   /* if init_genrand() has not been called, */
	 init_genrand(5489UL); /* a default initial seed is used */

      for (kk=0;kk<N-M;kk++) {
	 y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
	 mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1UL];
      }
      for (;kk<N-1;kk++) {
	 y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
	 mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
      }
      y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
      mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];

      mti = 0;
   }

   y = mt[mti++];

   /* Tempering */
   y ^= (y >> 11);
   y ^= (y << 7) & 0x9d2c5680UL;
   y ^= (y << 15) & 0xefc60000UL;
   y ^= (y >> 18);


   return y;
}

/* ASM suppport check
_Bool asmCheck( unsigned int* eax, unsigned int* ebx, unsigned int* ecx, unsigned int* edx ){

   __asm__ __volatile__("cpuid"
	 : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
	 :  "a" (1), "c" (0)
	 );

}

_Bool checkRdrand(){

   unsigned int flag_Rdrand = (1 << 30);
   unsigned int eax, ebx, ecx, edx;

   asmCheck( &eax, &ebx, &ecx, &edx );

   _Bool answer = ( (ecx & flag_Rdrand ) == flag_Rdrand);

   return answer; 

} */

int genrand(){

   //int ifRdrand = checkRdrand();
   int ifRdrand = 0;
   int smallRand;

   if( ifRdrand == 0 ){

      unsigned int randNum = genrand_int32();
      smallRand = randNum & INT_MAX;

   } /*else if( ifRdrand == 1 ){

	 uint32_t rnd = 0;

	 int result = _rdrand32_step( &rnd );
	 smallRand = rnd & INT_MAX; 

	 }*/

   return smallRand;

}

// Algorithm to figure out which forks are being used or not
int abletoEat(int id) {
	int t;
	
	if(people[(id+4) % 5].state == EATING || people[(id+1) % 5].state == EATING) {
		return 0;
	}
	
	t = time(0);
	
	if(people[(id+4) % 5].state == HUNGRY && people[(id+4) % 5].blocktime < people[id].blocktime && (t - people[(id+4) % 5].blocktime) >= 5) {
		return 0;
	}
	
	if(people[(id+1) % 5].state == HUNGRY && people[(id+1) % 5].blocktime < people[id].blocktime && (t - people[(id+1) % 5].blocktime) >= 5) {
		return 0;
	}
	
	else {
		return 1;
	}
}

void getforks(int id) {
	pthread_mutex_lock(&bowl);
	people[id].state = HUNGRY;
	people[id].blocktime = time(0);
	while(!abletoEat(id)) {
		pthread_cond_wait(&people[id].condp, &bowl);
	}
	people[id].state = EATING;
	printf("Philosopher %d PICKS UP forks: %d and %d\n", id, id, ((id+4) % 5));
	pthread_mutex_unlock(&bowl);
}

void put_forks(int id) {
	pthread_mutex_lock(&bowl);
	printf("Philosopher %d PUTS DOWN forks: %d and %d\n", id, id, ((id+4) % 5));
	people[id].state = THINKING;
	if(people[(id+4) % 5].state == HUNGRY) {
		pthread_cond_signal(&people[(id+4) % 5].condp);
	}
	if(people[(id+1) % 5].state == HUNGRY) {
		pthread_cond_signal(&people[(id+1) % 5].condp);
	}
	pthread_mutex_unlock(&bowl);
}

void think(int id) {
	int thinkingtime = (genrand() % 19) + 1;
	printf("Philosopher %d is THINKING\n", id);
	sleep(thinkingtime);
}

void eat(int id) {
	int eatingtime = (genrand() % 7) + 2;
	printf("Philosopher %d is EATING\n", id );
	sleep(eatingtime);
}

void* philosopher(void *num){
	int id = *((int *) num);
	while( 1 ){
		
		think(id);
		getforks(id);
		eat(id);
		put_forks(id);

	}
} 

int main( int argc, char *argv[] ) {

	pthread_t philo[5];

	pthread_mutex_init(&bowl, NULL);

	int i = 0;
	// Need to pass (void *) into pthread_create, therefore, needed to make a pointer. Need this to pass the values correctly.
	int id[5] = {0,1,2,3,4};
	
	// Initializing some of struct values
	for(i = 0; i < 5; i++) {
		pthread_cond_init(&people[i].condp, NULL);
		people[i].state = THINKING;
		people[i].blocktime = 0;
	}
	
	for( i = 0; i < 5; i++ ){
		pthread_create( &philo[i], NULL, philosopher, (void *)(id + i));
	}

	for( i = 0; i < 5; i++ ){
		pthread_join( philo[i], NULL );
	}

	pthread_mutex_destroy( &bowl );
	for( i = 0; i < 5; i++ ){
		pthread_cond_destroy( &people[i].condp );
	}	
}
