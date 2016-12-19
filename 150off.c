#include "cachelab.h"
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/*Tony Bumatay; tony.bumatay*/

typedef unsigned long long int mem_address_tag;//memory address

typedef struct {
   int s; //S = 2^s
   int b; // B = 2^b bytes
   int E; // number of lines in every set
   int S; //number of sets, S=2^s
   int B; //line block size (bytes), B= 2^b

   int hits;
   int misses;
   int evicts;
} cache_attributes;

typedef struct {
   int last_used;
   int valid;
   mem_address_tag tag;
   char *block;
}set_line;

typedef struct {
   set_line *lines;
} cache_set;

typedef struct {
   cache_set *sets;
}cache;

int verbosity;

//cache =  s * E * b
//for the given values of s(number of sets), E (number of lines per set), and b (block size)
cache create_cache(long long num_sets, int num_lines, long long block_size){
   cache newCache;
   cache_set set;
   set_line line;
   int setIndex;
   int lineIndex;

 newCache.sets = (cache_set *) malloc(sizeof(cache_set) * num_sets);

   for (setIndex = 0; setIndex < num_sets; setIndex ++){
      set.lines = (set_line *) malloc(sizeof(set_line) * num_lines);
      newCache.sets[setIndex] = set;

        for(lineIndex = 0; lineIndex < num_lines; lineIndex ++){
           line.last_used = 0;
           line.valid = 0;
           line.tag = 0;
           set.lines[lineIndex] = line;


        }

   }
   return newCache;

}

//check the valid tag of a set to see if the line is empty or not
//when the valid tag = 0, the line is empty
int find_empty_line(cache_set set, cache_attributes attributes){
   set_line line;
   int num_lines = attributes.E;

   for (int i = 0; i < num_lines; i++){
      line = set.lines[i];
      if (line.valid == 0) {
        return i;
      }

   }
   return 0;//this should never happen because the method should never be called unless there is at least one empty line 

}

//find and return the index of the least recently used line (LRU)
int get_LRU (cache_set this_set, cache_attributes attributes, int *used_lines){
   
   int num_lines = attributes.E;

   //initialize most and least used lines to the last used line
   int most_used = this_set.lines[0].last_used;
   int least_used = this_set.lines[0].last_used;

   int least_used_index = 0;
   set_line line;

   for (int i =1; i< num_lines; i++){
      line = this_set.lines[i];//iterate through all lines in the set
      if (line.last_used < least_used){//if the new last_used value is less than the current least_used
        least_used_index = i;
        least_used = line.last_used;
      }

      if ( line.last_used > most_used){//if the new last_used value is greater than the current most_used value
        most_used = line.last_used;
      }
   }

   used_lines[0] = least_used;//the LRU
   used_lines[1] = most_used;//the Most Recently Used 

   return least_used_index;//return the index of the least recently used line
}

//run the cache simulation
cache_attributes simulate_cache (cache my_cache, cache_attributes attributes, mem_address_tag address){
   int line_index;
   int cache_filled = 1; //boolean for if cache is completely full
   int numLines = attributes.E;
   int previous_hits = attributes.hits;
   int tag_size = (64 - (attributes.s + attributes.b));// tag_size = 64 - s - b

   unsigned long long temp = address << (tag_size);
   unsigned long long set_index = temp >> (tag_size + attributes.b);

   mem_address_tag input_tag = address >> (attributes.s + attributes.b);

   cache_set this_set = my_cache.sets[set_index];

   for (line_index = 0; line_index < numLines; line_index++){
        set_line this_line = this_set.lines[line_index];
        if(this_line.valid){
            if (this_line.tag == input_tag){//it's a hit
                this_line.last_used++;
                attributes.hits++;
                this_set.lines[line_index] = this_line;
            }
        }
        else if (!(this_line.valid) && (cache_filled)){
            cache_filled = 0; //set cache filled equal to false; there must be an empty line
        }
   }

   if (previous_hits == attributes.hits){//hits was not incremented, so it must have been a cache miss
        attributes.misses++; //Increment the misses
   }
   else {
    return attributes; //there was already a hit and the data is already in the cache
   }


   //there was not a hit->so it must have been a miss.
   // Evict by ge
   int *used_lines = (int*) malloc(sizeof (int) * 2);
   int indexOf_least_used = get_LRU(this_set, attributes, used_lines);

   if (cache_filled){//if the cache is full, we'll need to ovwe
        attributes.evicts++;

    //write and replace LRU
    this_set.lines[indexOf_least_used].tag = input_tag;
    this_set.lines[indexOf_least_used].last_used = used_lines[1] + 1;
   }
   else { //there is at least one empty line that we can use: write to it.
        int indexOf_empty_line = find_empty_line(this_set, attributes);
        // update valid/ tag bits with the input cache's at the empty line 
        this_set.lines[indexOf_empty_line].tag = input_tag; //tag bits
        this_set.lines[indexOf_empty_line].valid = 1; //valid bit
        this_set.lines[indexOf_empty_line].last_used = used_lines[1] + 1; 
   }
   free(used_lines);
   return attributes;
} //end of simulate_cache


// /* call free function to clean up cache after main simulation is run */
// void clear_cache(cache this_cache, long long num_sets, int num_lines, long long block_size) 
// {
//     int setIndex;

//     for (setIndex = 0; setIndex < num_sets; setIndex ++) {
//         cache_set set = this_cache.sets[setIndex];
//         if (set.lines != NULL) {    
//             free(set.lines);
//         }
//     } 

//     if (this_cache.sets != NULL) {
//         free(this_cache.sets);
//     }
// } /* end clear_cache */




int count_lines(char *filename, int *numLines){

   FILE *file = fopen(filename, "r");

   char buff[25];
   char *line = fgets(buff, 25, file);

   int count;
   while (line){
      if (line[0] != 'I'){
        count++;
      }
      line = fgets(buff, 25, file);
   }
   *numLines = count;
   return 0;
}


/* instead of explicitly defining similar to pow(2.0, exp), just bit shift */
long long bit_pow(int power) {
    long long result = 1;
    result = result << power;
    return result;
} 

int read_file(char *filename, char operations[], long memAddresses[], int sizes[]){
   FILE *file = fopen(filename, "r");
   char buff[25];
   char operation;
   long mem_address;
   int size;

   char *line = fgets(buff, 25, file);

int counter = 0;
   while (line){
      if (line[0] != 'I'){
        sscanf(line, " %c %lx,%u", &operation, &mem_address, &size);
        operations[counter] = operation;
        memAddresses[counter] = mem_address;
        sizes[counter] = size;
        counter++;
      }
      line = fgets(buff, 25, file);
   }
   return 0;
}


/* main takes commands as input and prints the cache hits, misses, and evictions */
int main(int argc, char **argv)
{
   char *tracefile;
   tracefile = (argv[argc-1]);

   int numLines;
   count_lines(tracefile, &numLines);

   char operations[numLines];
   long memAddresses[numLines];
   int sizes[numLines];

   read_file(tracefile, operations, memAddresses, sizes);


    cache this_cache;
    cache_attributes attributes;
    //bzero(&attributes, sizeof(attributes));

    long long num_sets;
    long long block_size;

    FILE *read_trace;

    //char cmd; /* cmd will take in the operation address I, L, S, or M */
    //mem_address_tag address;
    //int size;

    char *trace_file;
    char c;
    while( (c=getopt(argc,argv,"s:E:b:t:vh")) != -1){
        switch(c){
        case 's':
            attributes.s = atoi(optarg);
            break;
        case 'E':
            attributes.E = atoi(optarg);
            break;
        case 'b':
            attributes.b = atoi(optarg);
            break;
        case 't':
            trace_file = optarg;
            break;
        case 'v':
            verbosity = 1;
            break;
        case 'h':
            //printUsage(argv);
            exit(0);
        default:
            //printUsage(argv);
            exit(1);
        }
    }
    if (attributes.s == 0 || attributes.E == 0 || attributes.b == 0 || trace_file == NULL) {
        printf("%s: Missing required command line argument\n", argv[0]);
        //printUsage(argv);
        exit(1);
    }

    /* compute S and B based on information passed in; S = 2^s and B = 2^b */
    num_sets = pow(2.0, attributes.s);
    block_size = bit_pow(attributes.b); 
    attributes.hits = 0;
    attributes.misses = 0;
    attributes.evicts = 0;

    this_cache = create_cache(num_sets, attributes.E, block_size); /* build_cache takes as input sets, lines, and blocks */

    read_trace = fopen(trace_file,"r");

    /* rest of simulator routine reads commands in */
   for (int i = 0; i< numLines; i++){
        if (operations[i]== 'I'){
            //do nothing
        } else if (operations[i] == 'L'){
            attributes = simulate_cache(this_cache, attributes, memAddresses[i]);
        } else if (operations[i] == 'S'){
            attributes = simulate_cache(this_cache, attributes, memAddresses[i]);
        } else if (operations[i] == 'M'){
            attributes = simulate_cache(this_cache, attributes, memAddresses[i]);
            attributes = simulate_cache(this_cache, attributes, memAddresses[i]); 
        }
    }

    /* print out real results */
    printSummary(attributes.hits, attributes.misses, attributes.evicts);

    /* clean up cache resources */
    //clear_cache(this_cache, num_sets, attributes.E, block_size);
    fclose(read_trace);

    return 0;
}