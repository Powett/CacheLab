#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "cachelab.h"

#define LINE_SIZE 500 // size of buffer containing the line : if a line is longer than that the program notifies and exits

// Implemented by Nathan PELUSO for INF559 Course
// LRU eviction is implemented using an "age" field
// When a line is being used, its age is set to 0 and all others in the set are incremented
// This could be optimised by using a specific structure (file or union-find) instead of cycling through the set every time


/* Globals set by command line args */
int verbosity = 0; /* print trace if set */
int s = 0;         /* set index bits */
int b = 0;         /* block offset bits */
int E = 0;         /* associativity */
char* trace_file = NULL;

typedef struct Line {
   int valid;
   int tag;
   int age; // Used to implement LRU eviction
   //int* data; // Useless here, could be used if we were to actually implement a cache system
} Line;

typedef struct Set {
  Line* lines;
} Set;


/*
 * printUsage - Print usage info
 */
void printUsage(char* argv[])
{
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\nExamples:\n");
    printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", argv[0]);
    printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", argv[0]);
    exit(0);
}

/*
 * main - Main routine 
 */
int main(int argc, char* argv[])
{
  char c;
  Set* cache;
  
  FILE* f;
  char lineRead[LINE_SIZE]; // fixed size buffer to hold a line
  char lineCode; // will hold the instruction code (L/M/S/...)
  unsigned int address=0x00; // will hold the address to fetch
  int setIndex=-1; // will hold the index of the set to look into
  int tag=-1; // will hold the tag, extracted from the address
  

  int hits=0;
  int misses=0;
  int evictions=0;

  
  while( (c=getopt(argc,argv,"s:E:b:t:vh")) != -1){
    switch(c){
    case 's':
      s = atoi(optarg);
      break;
    case 'E':
      E = atoi(optarg);
      break;
    case 'b':
      b = atoi(optarg);
      break;
    case 't':
      trace_file = optarg;
      break;
    case 'v':
      verbosity = 1;
      break;
    case 'h':
      printUsage(argv);
      exit(0);
    default:
      printUsage(argv);
      exit(1);
    }
  }

  /* Make sure that all required command line args were specified */
  if (s == 0 || E == 0 || b == 0 || trace_file == NULL) {
    printf("%s: Missing required command line argument\n", argv[0]);
    printUsage(argv);
    exit(1);
  }


  // créer cache: tableau de 2**s sets, chacune étant composée de E lignes, chacune étant composée de : 1 bit de validité, 1 tag
  // First we instantiate our cache : table of 2^s sets, each one composed of E lines, each composed of : 1 int (used as a bit) validity, 1 int tag, 1 int age
  int nb_set=pow(2,s);
  cache=malloc(sizeof(Set)*nb_set);
  for (int seti=0;seti<nb_set;seti++){
    cache[seti].lines=malloc(sizeof(Line)*E);
    for (int linei=0;linei<E;linei++){
      cache[seti].lines[linei].valid=0;
      cache[seti].lines[linei].tag=-1;
      cache[seti].lines[linei].age=-1;
    }
  } 
  f=fopen( trace_file, "r");
  char ch=getc(f);
  while(ch!=EOF){
    int chari=0;
    while(chari<LINE_SIZE && ch!='\n' && ch!=EOF){
      lineRead[chari]=ch;
      ch=getc(f);
      chari++;
    }
    if (chari==LINE_SIZE){
      printf("Line read is too long, check input file validity\n");
      exit(-1);
    }
    lineRead[chari-1]='\x00';
    if(ch!=EOF)
      ch=getc(f); //if we did not reach the end of file, we can go on to the next line
    if (lineRead[0]==' ' && sscanf(lineRead," %c %x,", &lineCode, &address)==2){ // we only handle " L...", " M..." and " S..." lines
      if(verbosity)
        printf(lineRead);
      address>>=b;
      setIndex=address%(nb_set);
      tag=address>>s;
      if (lineCode=='L' || lineCode=='S' || lineCode=='M'){
        int linei=0;
        int touched=0;
        int found_free=0;
        int oldest_age=-1;
        Line* oldest_line=NULL; //pointer to the currently oldest line found (or an invalid one)
        while(linei<E && !touched){
          if(cache[setIndex].lines[linei].valid==1 && cache[setIndex].lines[linei].tag==tag){
            hits++;
            if(verbosity)
              printf(" hit");
            touched=1; // if we touch, we do not have to handle eviction
            cache[setIndex].lines[linei].age=-1;
            for (int i=0;i<E;i++){
              cache[setIndex].lines[i].age++;
            }
          }else if (!found_free){ // if a free (invalid) line is found, we do not have to LRU
            if(cache[setIndex].lines[linei].valid==0){
              found_free=1;
              oldest_line=&(cache[setIndex].lines[linei]);
            }else if (cache[setIndex].lines[linei].age>oldest_age){
              oldest_age=cache[setIndex].lines[linei].age;
              oldest_line=&(cache[setIndex].lines[linei]);
            }
          }
          linei++;
        }
        if (!touched){
          misses++;
          if(verbosity)
            printf(" miss");
          oldest_line->age=-1;
          oldest_line->tag=tag;
          oldest_line->valid=1;
          for (int i=0;i<E;i++){
            cache[setIndex].lines[i].age++;
          }
          if (!found_free){
            evictions++;
            if(verbosity)
              printf(" eviction");
          }
        }
      }
      if(lineCode=='M'){ // M means an atomic L & S : wether the L hits or misses, the S will definitely hit
        if(verbosity)
          printf(" hit");
        hits++;
      }
      if(verbosity)
        printf("\n");
    }
  }
  // We have to free all allocated space properly
  for (int seti=0;seti<nb_set;seti++){
    free(cache[seti].lines);
  }
  free(cache);
  printSummary(hits, misses, evictions);
  return 0;
}


