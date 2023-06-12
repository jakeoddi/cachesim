/*
 * This file simulates a Least Recently Used (LRU) cache.
 *
 * Name: Jacob Oddi
 * 
 * 
*/

#include "lab3.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>

typedef unsigned long address;

typedef struct {
    int s; // 2^s sets, where s is in bits
    int E; // E lines
    int b; // B^b bytes, where b is in bits
} cache_params;

typedef struct {
    address tag;
    int valid;
    int lru_count; // tracks least recently used, lower = more recently used
} cache_line;

typedef struct {
    cache_line *lines;
} cache_set;

typedef struct {
    cache_set *sets;
} cache;

typedef struct {
    int hits;
    int misses;
    int evicts;
} results;

void initialize_cache(cache *Cache, cache_params cp, int num_sets);
void query_cache(cache *Cache, results *Results, cache_params cp, 
                                                            address address_i);

int main(int argc, char *argv[]) {
    // int v = 0; // verbose flag
    char *t = NULL; // filepath to traces
    results Results; // struct to keep track of results

    cache_params cp;
    int opt;
    // parsing command line args
    while ((opt = getopt(argc, argv, "vs:E:b:t:")) != -1) {
        switch (opt) {
            case 'v':
                // v = 1; // caused error because it wasn't used
                break;
            case 's':
                cp.s = atoi(optarg);
                break;
            case 'E':
                cp.E = atoi(optarg);
                break;
            case 'b':
                cp.b = atoi(optarg);
                break;
            case 't':
                t = optarg;
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }

    // calculate # sets in the cache (2^s)
    int S = pow(2, cp.s);

    // initialize cache
    cache Cache;
    initialize_cache(&Cache, cp, S);

    // run the simulation
    char buffer[25];
    char operation;
    address address_i;
    int size;
    FILE* fp = fopen(t, "r");

    // get instructions from trace file
    // this i/o was influenced by
    // https://hackmd.io/@TsundereChen/HkdZcYk0w#Part-A
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (buffer[0] != 'I') {
            sscanf(buffer, " %c %lx,%d", &operation, &address_i, &size);

            switch (operation) {
                case 'M':
                    query_cache(&Cache, &Results, cp, address_i);
                    Results.hits++;
                    break;
                case 'L':
                    query_cache(&Cache, &Results, cp, address_i);
                    break;
                case 'S':
                    query_cache(&Cache, &Results, cp, address_i);
                    break;
            }
        }
    }
    fclose(fp);

    // clear memory - was throwing errors


    printSummary(Results.hits, Results.misses, Results.evicts);
    return 0;
}

/*
 * Function to initialize our cache. 
 * Args:
 *  Cache: pointer to our Cache object
 *  cp: cache parameters (s, E, b) from our input
 *  num_sets: the number of sets in our cache
 *
 * 
*/
void initialize_cache(cache *Cache, cache_params cp, int num_sets) {
    Cache->sets = malloc(sizeof(cache_set)*num_sets);

    for (int i = 0; i < num_sets; i++) {
        Cache->sets[i].lines = malloc(sizeof(cache_line)*cp.E);
    }
}


/*
 *
 * Function that queries the cache and records hit, miss, or eviction
 * Args:
 *  Cache: pointer to our Cache object
 *  Results: pointer to our Results tracker object
 *  cp: cache parameters (s, E, b) from our input
 *  address_i: address given in the trace
 *
 * 
*/
void query_cache(cache *Cache, results *Results, cache_params cp, 
                 address address_i) {
    // extract tag and set index from the given address
    address cur_tag = address_i >> (cp.s + cp.b);
    unsigned int set_idx = address_i >> cp.b & ((1 << cp.s) - 1);

    int top_idx = 0; // keeps track of index of next line to be evicted
                    // 'top' is reference to heap that is sometimes used to 
                    // construct LRU
    int next_empty = -1; // keeps track of index of next empty line
    cache_set *cur_set = &Cache->sets[set_idx]; 



    for (int i = 0; i < cp.E; ++i) {
        // case: valid and matching tag
        if (cur_set->lines[i].valid && cur_set->lines[i].tag == cur_tag) {
            // increments our hits counter
            (Results->hits)++;
            // resets lru count to smallest count, indicating we just used it
            cur_set->lines[i].lru_count = 1;
            return;
        // case: not valid
        } else if (!cur_set->lines[i].valid) {
            // not valid space, indicating it's empty and we can allocate here
            next_empty = i;
        }
        // case: valid and also least recently used, so we set idx denoting lru
        // to the current one
        if (cur_set->lines[i].valid && 
            cur_set->lines[top_idx].lru_count <= cur_set->lines[i].lru_count) {
            top_idx = i;
        }
        // case: only valid, but not used, so we increment its lru counter
        if (cur_set->lines[i].valid) {
            cur_set->lines[i].lru_count++;
        }
    }
    // no hit, so we increment misses count
    (Results->misses)++;

    // cell that was empty at the beginning of our search has been filled,
    // so we update its params
    if (next_empty != -1) {
        cur_set->lines[next_empty].valid = 1;
        cur_set->lines[next_empty].tag = cur_tag;
        cur_set->lines[next_empty].lru_count = 1;
        return;
    }

    // cache had no empty blocks, meaning we must have had to evict
    // so we increment the counter and update the top of the heap
    cur_set->lines[top_idx].tag = cur_tag;
    cur_set->lines[top_idx].lru_count = 1;

    (Results->evicts)++;
    return;
}