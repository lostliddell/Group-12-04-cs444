#include <stdio.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <immintrin.h>


/*START OF MERSAND TWISTER CODE BLOCK*/
/* Period parameters */  
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UPPER_MASK 0x80000000UL /* most significant w-r bits */
#define LOWER_MASK 0x7fffffffUL /* least significant r bits */

static unsigned long mt[N]; /* the array for the state vector  */
static int mti=N+1; /* mti==N+1 means mt[N] is not initialized */

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


/*MERSAND TWISTER CODE END*/

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

}


pthread_mutex_t buffer;
pthread_cond_t condc,  condp;

int limit = 0;

struct item{

   int value;
   int time;

} citem[32];



int genrand(){

   int ifRdrand = checkRdrand();
   int smallRand;

   if( ifRdrand == 0 ){

      unsigned long randNum = genrand_int32();
      smallRand = randNum & INT_MAX;

   }else if( ifRdrand == 1 ){

	 uint32_t rnd = 0;

	 int result = _rdrand32_step( &rnd );
	 smallRand = rnd & INT_MAX; 

	 }

   return smallRand;

}



void* producer( void *ptr ){

   int pTimer = 0;

   while( 1 ){

      pthread_mutex_lock(&buffer);		

      while(limit >= 32){

	 pthread_cond_wait(&condp, &buffer);

      }

      citem[limit].value = genrand();
      citem[limit].time = ( ( genrand() % 8 ) + 2 );
      pTimer = ( ( genrand() % 5 ) + 3 );
      limit++;
      printf( "\nTotal items: %d\n", limit );

      pthread_cond_signal(&condc);
      pthread_mutex_unlock(&buffer);

      sleep( pTimer );

   }

}

void* consumer(void *ptr){

   int consumeTime;

   while( 1 ){

      pthread_mutex_lock(&buffer);

      while( limit == 0 ){

	 pthread_cond_wait(&condc, &buffer);

      }

      limit--;
      printf( "\nConsumer consumed: %d\n", citem[limit].value );
      consumeTime = citem[limit].time;

      citem[limit].time = 0;
      citem[limit].value = 0;

      pthread_cond_signal(&condp);
      pthread_mutex_unlock(&buffer);

      sleep( consumeTime );

   }

}




int main( int argc, char *argv[] ){

   printf( "\nIS RDRAND: %d\n", checkRdrand() );

   if( argc == 2 ){ 

      int num = atoi(argv[1]);

      pthread_t prod[num], cons[num];

      pthread_mutex_init(&buffer, NULL);
      pthread_cond_init(&condc, NULL);
      pthread_cond_init(&condp, NULL);

      int i = 0;
      unsigned long seed = time( NULL );
      init_genrand( seed );

      for( i = 0; i < num; i++ ){

	 pthread_create( &prod[i], NULL, producer, NULL );
	 pthread_create( &cons[i], NULL, consumer, NULL );

      }

      for( i = 0; i < num; i++ ){

	 pthread_join( prod[i], NULL );
	 pthread_join( cons[i], NULL );

      }

      pthread_mutex_destroy( &buffer );
      pthread_cond_destroy( &condc );
      pthread_cond_destroy( &condp );

   }else{

      printf( "\nERROR: Invalid number of inputs\n" );

   }

}
