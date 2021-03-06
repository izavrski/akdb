/**
@file memoman.c Defines functions for the memory manager of
 Kalashnikov DB
 */
/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * dbman.c
 * Copyright (C) Markus Schatten 2009 <markus.schatten@foi.hr>
 *
 * main.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * main.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "memoman.h"

/**
  * @author Nikola Bakoš, Matija Šestak(revised)
  * @brief Function caches block into memory.
  * @param num block number (address)
  * @param mem_block address of memmory block
  * @return EXIT_SUCCESS if the block has been successfully read into memory, EXIT_ERROR otherwise
 */

int AK_cache_block(int num, AK_mem_block *mem_block)
{
    unsigned long timestamp;
    AK_block *block_cache;
    AK_block *block_cache_old;
    AK_PRO;
    /// read the block from the given address
    block_cache = AK_read_block(num);
    block_cache_old = mem_block->block;
    mem_block->block = block_cache;
    mem_block->dirty = BLOCK_CLEAN; /// set dirty bit in mem_block struct

    timestamp = clock(); /// get the timestamp
    mem_block->timestamp_read = timestamp; /// set timestamp_read
    mem_block->timestamp_last_change = timestamp; /// set timestamp_last_change

    AK_free(block_cache_old);
    AK_EPI;
    return EXIT_SUCCESS;
}

/**
  * @author Markus Schatten, Matija Šestak(revised)
  * @brief Function initializes the global cache memory (variable db_cache)
  * @return EXIT_SUCCESS if the cache memory has been initialized, EXIT_ERROR otherwise
 */
int AK_cache_AK_malloc()
{
    int i;
    AK_PRO;
    if ((db_cache = (AK_db_cache *) AK_malloc(sizeof(AK_db_cache))) == NULL)
    {
        AK_EPI;
        return EXIT_ERROR;
    }

    db_cache->next_replace = 0;
    for (i = 0; i < MAX_CACHE_MEMORY; i++)
    {
        db_cache->cache[ i ] = (AK_mem_block *) AK_malloc(sizeof(AK_mem_block));
        db_cache->cache[ i ]->block = (AK_block *) AK_malloc(sizeof(AK_block));

        if ((AK_cache_block(i, db_cache->cache[ i ])) == EXIT_ERROR)
        {
            AK_EPI;
            return EXIT_ERROR;
        }
        //printf( "Cached block %d with address %d\n", i,  &db_cache->cache[ i ]->block->address );
    }
    AK_EPI;
    return EXIT_SUCCESS;
}

/**
  * @author Dejan Sambolić updated by Dražen Bandić
  * @brief Function initializes the global redo log memory (variable redo_log)
  * @return EXIT_SUCCESS if the redo log memory has been initialized, EXIT_ERROR otherwise
 */
int AK_redo_log_AK_malloc()
{
    AK_PRO;
    if ((redo_log = (AK_redo_log *) AK_malloc(sizeof ( AK_redo_log))) == NULL)
    {
        AK_EPI;
        return EXIT_ERROR;
    }

    redo_log->number = 0;
    redo_log->table_name = AK_malloc(MAX_REDO_LOG_ENTRIES * sizeof(char*));
    redo_log->command = AK_malloc(MAX_REDO_LOG_ENTRIES * sizeof(char*));
    redo_log->attributes = AK_malloc(MAX_REDO_LOG_ENTRIES * sizeof(char*));

    int i = 0;
    for (i = 0; i < MAX_REDO_LOG_ENTRIES; i++)
    {
        redo_log->table_name[i] = AK_calloc(MAX_VARCHAR_LENGTH, sizeof(char));
        redo_log->command[i] = AK_calloc(MAX_VARCHAR_LENGTH, sizeof(char));
        redo_log->attributes[i] = AK_calloc(MAX_VARCHAR_LENGTH, sizeof(char));
    }

    AK_EPI;
    return EXIT_SUCCESS;
}
/**
  * @author Mario Novoselec
  * @brief Function find available block for result caching in circular array
  * @return available_index
 */
int AK_find_available_result_block(){
	int available_index=0;
	int i=0;
	for(i; i<MAX_QUERY_RESULT_MEMORY;i++){
		if(query_mem->result->results[i].free==1){
			available_index=i;
			break;
		}
	}
	return available_index;
}
/**
  * @author Mario Novoselec
  * @brief Generate unique hash identifier for each cached result by using djb2 algorithm
  * @return hash
 */
unsigned long AK_generate_result_id(unsigned char *str) {
    unsigned long hash = 5381;
    int c;
	srand(time(NULL));
	int hash_randomizer = rand()%1000;

    while (c = *str++) {
        hash = ((hash << 5) + hash) + c; 
    }

    return abs(hash+hash_randomizer);
}
/**
  * @author Mario Novoselec
  * @brief Cache fetched result block in memory
 */
void AK_cache_result(char *srcTable,AK_block *temp_block,AK_header header[]){
	//find available block in memory for query caching
	int available_index=AK_find_available_result_block();
	unsigned long tmp;
	
	//generate query date and time
	time_t rawtime;
	struct tm * timeinfo;
	time (&rawtime);
	timeinfo = localtime (&rawtime);
	
	strftime (query_mem->result->results[available_index].date_created,80,"%F %T",timeinfo);
	query_mem->result->results[available_index].result_id=AK_generate_result_id(query_mem->result->results[available_index].date_created);
	memcpy(&query_mem->result->results[available_index].source_table,&srcTable,sizeof(srcTable));
	memcpy(&query_mem->result->results[available_index].result_block,&temp_block,sizeof(temp_block));
	memset(query_mem->result->results[available_index].header, 0, sizeof( AK_header ) * MAX_ATTRIBUTES);
	
	int head=0;
	while(strcmp(header[head].att_name, "") != 0) {
		
		memcpy(&query_mem->result->results[available_index].header[head],&header[head], sizeof (header[head]));
		head++;
	}
	query_mem->result->results[available_index].result_size=sizeof(temp_block);
	
	printf("\n ****LAST CACHED QUERY***** \n \n");
	printf("---Table attributes--- \n");
	printf("%s \t",query_mem->result->results[available_index].header[0].constr_name);
	int i=0;
	for(i=0; i<MAX_ATTRIBUTES; i++){
		printf("%s \t",query_mem->result->results[available_index].header[i].att_name);
	};
	printf("\n ---Unique result id--- \n");
	printf("%d \n",query_mem->result->results[available_index].result_id);
	printf("---Date created--- \n");
	printf("%s \n",query_mem->result->results[available_index].date_created);
	printf("---Source table--- \n");
	printf("%s \n",query_mem->result->results[available_index].source_table);
	printf("---Result size--- \n");
	printf("%d \n",query_mem->result->results[available_index].result_size);
}
/**
  *  @author Matija Novak
  *  @brief Function initializes the global query memory (variable query_mem)
  *  @return EXIT_SUCCESS if the query memory has been initialized, EXIT_ERROR otherwise
 */
int AK_query_mem_AK_malloc()
{
    AK_PRO;
    Ak_dbg_messg(HIGH, MEMO_MAN, "AK_query_mem_AK_malloc: Start query_mem_AK_malloc\n");

    /// allocate memory for global variable query_mem
    if ((query_mem = (AK_query_mem *) AK_malloc(sizeof ( AK_query_mem))) == NULL)
    {
        printf("AK_query_mem_AK_malloc: ERROR. Cannot allocate query memory \n");
        AK_EPI;
        exit(EXIT_ERROR);
    }

    /// allocate memory for variable query_mem_lib which is used in query_mem->parsed
    AK_query_mem_lib * query_mem_lib;
    if ((query_mem_lib = (AK_query_mem_lib *) AK_malloc(sizeof (AK_query_mem_lib))) == NULL)
    {
        printf("AK_query_mem_AK_malloc: ERROR. Cannot allocate query library memory \n");
        AK_EPI;
        exit(EXIT_ERROR);
    }

    /// allocate memory for variable query_mem_dict which is used in query_mem->dictionary
    AK_query_mem_dict * query_mem_dict;
    if ((query_mem_dict = (AK_query_mem_dict *) AK_malloc(sizeof (AK_query_mem_dict))) == NULL)
    {
        printf("AK_query_mem_AK_malloc: ERROR. Cannot allocate query dictionary memory \n");
        AK_EPI;
        exit(EXIT_ERROR);
    }

    /// allocate memory for variable query_mem_result which is used in query_mem->result
    AK_query_mem_result * query_mem_result;
    if ((query_mem_result = (AK_query_mem_result *) AK_malloc(sizeof (AK_query_mem_result))) == NULL)
    {
        printf("  AK_query_mem_AK_malloc: ERROR. Cannot allocate query result memory \n");
        AK_EPI;
        exit(EXIT_ERROR);
    }
	query_mem_result->results=AK_malloc(MAX_QUERY_RESULT_MEMORY*sizeof(*query_mem_result->results));
	
    /// allocate memory for variable tuple_dict which is used in query_mem->dictionary->dictionary[]
    AK_tuple_dict * tuple_dict = (AK_tuple_dict *) AK_malloc(sizeof (AK_tuple_dict));
    if ((tuple_dict = (AK_tuple_dict *) AK_malloc(sizeof (AK_tuple_dict))) == NULL)
    {
        printf("  AK_query_mem_AK_malloc: ERROR. Cannot allocate tuple dictionary memory \n");
        AK_EPI;
        exit(EXIT_ERROR);
    }

    memcpy(query_mem_dict->dictionary, tuple_dict, sizeof (* tuple_dict));

    query_mem->parsed = query_mem_lib;
    query_mem->dictionary = query_mem_dict;
    query_mem->result = query_mem_result;
	//initializing values for result block status
	//by default all blocks are free
	int i=0;
	for(i=0; i<MAX_QUERY_RESULT_MEMORY; i++){
		query_mem->result->results[i].free=1;
	}
    /*	wrong way because we don't have data only adress which must be written in query_mem variables
            memcpy(query_mem->parsed, query_mem_lib, sizeof(* query_mem_lib));
            memcpy(query_mem->dictionary,query_mem_dict,sizeof(* query_mem_dict));
            memcpy(query_mem->result,query_mem_result,sizeof(* query_mem_result));*/

    Ak_dbg_messg(HIGH, MEMO_MAN, "AK_query_mem_AK_malloc: Success!\n");
    AK_EPI;
    return EXIT_SUCCESS;
}

/**
 * @author Miroslav Policki
 * @brief  Function initializes memory manager (cache, redo log and query memory)
 * @return EXIT_SUCCESS if the query memory manager has been initialized, EXIT_ERROR otherwise
 */
int AK_memoman_init()
{
    AK_PRO;
    printf("AK_memoman_init: Initializing memory manager...\n");

    if (AK_cache_AK_malloc() == EXIT_ERROR)
    {
        printf("AK_memoman_init: ERROR. AK_cache_AK_malloc() failed.\n");
        AK_EPI;
        return EXIT_ERROR;
    }

    if (AK_redo_log_AK_malloc() == EXIT_ERROR)
    {
        printf("AK_memoman_init: ERROR. AK_redo_log_AK_malloc() failed.\n");
        AK_EPI;
        return EXIT_ERROR;
    }

    if (AK_query_mem_AK_malloc() == EXIT_ERROR)
    {
        AK_EPI;
        printf("AK_memoman_init: ERROR. AK_query_mem_AK_malloc() failed.\n");
        return EXIT_ERROR;
    }


//AK_memoman_test();
//AK_print_block(NULL, 0, "Memoman_75_cached");
//AK_print_block(NULL, 1, "Memoman_78_cached");
//AK_print_block(db_cache->cache[ 200 ]->block, 200, "Memoman_79_cached");
//AK_print_block(db_cache->cache[ 1]->block, 1, "Memoman_80_cached");
//AK_print_block(db_cache->cache[ 61]->block, 61, "Memoman_80_cached");
//AK_print_block(db_cache->cache[2]->block, 2, "Memoman_80_cached");
//AK_print_block(db_cache->cache[3]->block, 3, "Memoman_80_cached");
//AK_print_block(db_cache->cache[4]->block, 4, "Memoman_80_cached");
//AK_print_block(db_cache->cache[5]->block, 5, "Memoman_80_cached");
//AK_print_block(db_cache->cache[6]->block, 6, "Memoman_80_cached",stdout);
//AK_print_block(NULL, 5, "Memoman_77_cached");
//AK_print_block(NULL, 61, "Memoman_78_cached");

    printf("AK_memoman_init: Memory manager initialized...\n");
    AK_EPI;
    return EXIT_SUCCESS;
}

/**
  * @author Tomislav Fotak, updated by Matija Šestak
  * @brief Function reads a block from memory. If the block is cached returns the cached block. Else uses AK_cache_block to read the block
		to cache and then returns it.
  * @param num block number (address)
  * @return segment start address
 */
AK_mem_block *AK_get_block(int num)
{
    int i = 0;
    int min = 0;
    int pos;
    int oldest_block = db_cache->next_replace;
    int second_oldest = 0;
    int found_in_cache = 0;
    int first_AK_free_mem_block = -1;
    int block_written = 0;

    AK_mem_block *cached_block;
    AK_block *data_block;
    AK_PRO;

    /* search cache for already-cached block */
    for (i = 0; i < MAX_CACHE_MEMORY; i++)
    {
        if (db_cache->cache[i]->block->address == num)
        {
            found_in_cache = 1;
            pos = i;
            cached_block = db_cache->cache[i];
            break;
        }
    }
    if (!found_in_cache)
    {
        //printf("Blok NOT FOUND IN CACHE!\n");
        /* find first empty slot in cache for block to be read into */
        for (i = 0; i < MAX_CACHE_MEMORY; i++)
        {
            if (db_cache->cache[i]->timestamp_read == -1)
            {
                first_AK_free_mem_block = i;
                break;
            }
        }

        if (first_AK_free_mem_block != -1)
        {
            if (AK_cache_block(num, db_cache->cache[ first_AK_free_mem_block ]) == EXIT_SUCCESS)
            {
                cached_block = db_cache->cache[first_AK_free_mem_block];
                printf("USPJEH!\n");
            }
        }
        else
        {
            if (db_cache->cache[oldest_block]->dirty == BLOCK_DIRTY)
            {
                data_block = db_cache->cache[oldest_block]->block;
                block_written = AK_write_block(data_block);
                /// if block form cache can not be writed to DB file -> EXIT_ERROR
                if (block_written != EXIT_SUCCESS)
                {
                    AK_EPI;
                    exit(EXIT_ERROR);
                }
                else
                {
                    //printf("BLOK NIJE ZAPISAN!\n");
                }
            }
            if (AK_cache_block(num, db_cache->cache[ oldest_block ]) == EXIT_SUCCESS)
            {
                cached_block = db_cache->cache[ oldest_block ];
            }
        }
    }
    else
    {
        if (db_cache->cache[pos]->dirty == BLOCK_DIRTY)
        {
            data_block = db_cache->cache[pos]->block;
            block_written = AK_write_block(data_block);
            /// if block form cache can not be writed to DB file -> EXIT_ERROR
            if (block_written != EXIT_SUCCESS)
            {
                AK_EPI;
                exit(EXIT_ERROR);
            }
            else
            {
                //printf("BLOK NIJE ZAPISAN!\n");
            }
        }
    }
    /*i = 1;*/
    for (i = 0; i < MAX_CACHE_MEMORY; i++)
    {
        if (db_cache->cache[i]->timestamp_read != -1 &&
                db_cache->cache[i]->timestamp_read < db_cache->cache[ min ]->timestamp_read)
        {
            min = i;
        }
    }
    //printf("KESANI BLOK %i \n",cached_block);
    db_cache->next_replace = min;
    AK_EPI;
    return cached_block;
}

/**
 * @author Alen Novosel.
 * @brief  Modify the "dirty" bit of a block, and update timestamps accordingly.
 */
void AK_mem_block_modify(AK_mem_block* mem_block, int dirty)
{
    unsigned long timestamp;
    AK_PRO;
    mem_block->dirty = dirty;

    timestamp = clock();
    mem_block->timestamp_last_change = timestamp;
    AK_EPI;
}

/**
 * @author Matija Šestak.
 * @brief  Function re-read all the blocks from disk
 * @result EXIT_SUCCESS
 */
int AK_refresh_cache()
{
    int i;
    AK_block *new_block;
    AK_block *old_block;

    AK_PRO;
    for (i = 0; i < MAX_CACHE_MEMORY; i++)
    {
        new_block = AK_read_block(db_cache->cache[i]->block->address);
        old_block = db_cache->cache[i]->block;
        db_cache->cache[i]->block = new_block;
        AK_free(old_block);
    }
    AK_EPI;
    return EXIT_SUCCESS;
}

/**
* @author Matija Novak, updated by Matija Šestak(function now uses caching), modified and renamed by Mislav Čakarić,Lovro Predovan
* @brief Function for geting addresses of some table
* @param table table name that you search for
* @return structure table_addresses witch contains start and end adresses of table extents, when form and to are 0 you are on the end of addresses
*/
table_addresses *AK_get_index_segment_addresses(char * segmentName)
{
    int i = 0;
    int AK_freeVar = 0;
    int data_adr = 0;
    int data_size = 0;
    int data_type = 0;
    int address_sys;
    char name_sys[MAX_ATT_NAME];
    char *sys_table;
    sys_table = "AK_index";

    AK_PRO;


    Ak_dbg_messg(HIGH, MEMO_MAN,"get_segment_addresses: Serching for %s table and %s \n\n\n\n", sys_table,segmentName);
    AK_mem_block *mem_block = AK_get_block(0);

    //printf("get_segment_addresses: Serching for %s table and %s \n\n\n\n", sys_table,segmentName);
    for (i = 0; i < DATA_BLOCK_SIZE; i++)
    {

        memset(name_sys, 0, MAX_ATT_NAME);
        if (mem_block->block->tuple_dict[i].address == FREE_INT)
        {
            break;
        }

        data_adr = mem_block->block->tuple_dict[i].address;
        data_size = mem_block->block->tuple_dict[i].size;
        data_type = mem_block->block->tuple_dict[i].type;
        memcpy(name_sys, mem_block->block->data + data_adr, data_size);
        i++;
        data_adr = mem_block->block->tuple_dict[i].address;
        data_size = mem_block->block->tuple_dict[i].size;
        data_type = mem_block->block->tuple_dict[i].type;
        memcpy(&address_sys, mem_block->block->data + data_adr, sizeof(int));

        if (strcmp(name_sys, sys_table) == 0)
        {
            Ak_dbg_messg(HIGH, MEMO_MAN, "get_segment_addresses: Found the address of the %s table: %d \n", sys_table, address_sys);
            break;
        }
    }


    mem_block = AK_get_block(address_sys);
    table_addresses * addresses = (table_addresses *) AK_malloc(sizeof (table_addresses));

    for (AK_freeVar = 0; AK_freeVar < MAX_EXTENTS_IN_SEGMENT; AK_freeVar++)
    {
        addresses->address_from[AK_freeVar] = 0;
        addresses->address_to[AK_freeVar] = 0;
    }

    char name[MAX_VARCHAR_LENGTH];
    int address_from;
    int address_to;
    int j = 0;
    for (i = 0; i < DATA_BLOCK_SIZE; i++)
    {
        if (mem_block->block->tuple_dict[i].type == FREE_INT)
            break;
        if ( (mem_block->block->last_tuple_dict_id) <= i )
            break;
        i++;
        memcpy(name, &(mem_block->block->data[mem_block->block->tuple_dict[i].address]), mem_block->block->tuple_dict[i].size);
        name[ mem_block->block->tuple_dict[i].size] = '\0';
        i++;
        memcpy(&address_from, &(mem_block->block->data[mem_block->block->tuple_dict[i].address]), mem_block->block->tuple_dict[i].size);
        i++;
        memcpy(&address_to, &(mem_block->block->data[mem_block->block->tuple_dict[i].address]), mem_block->block->tuple_dict[i].size);

        //SEGMENT NAME AND OUR TABLE WE WANT TO PRINT ARE THE SAME
        if (strcmp(name, segmentName) == 0)
        {
            addresses->address_from[j] = address_from;
            addresses->address_to[j] = address_to;
            j++;
            Ak_dbg_messg(HIGH, MEMO_MAN, "get_segment_addresses(%s): Found addresses of searching segment: %d , %d \n", name, address_from, address_to);
        }

            i += 2;

    }
    //printf("RESULTS FOR TABLE (%s): Found addresses of searching segment: %d , %d \n", name, address_from, address_to);
    AK_EPI;
    return addresses;
}


/**
* @author Matija Novak, updated by Matija Šestak(function now uses caching), modified and renamed by Mislav Čakarić
* @brief Function for geting addresses of some table
* @param table table name that you search for
* @return structure table_addresses witch contains start and end adresses of table extents, when form and to are 0 you are on the end of addresses
*/
table_addresses *AK_get_segment_addresses(char * segmentName)
{
    int i = 0;
    int AK_freeVar = 0;
    int data_adr = 0;
    int data_size = 0;
    int data_type = 0;
    int address_sys;
    char name_sys[MAX_ATT_NAME];
    char *sys_table;
    sys_table = "AK_relation";

    AK_PRO;

    Ak_dbg_messg(HIGH, MEMO_MAN,"get_segment_addresses: Serching for %s table \n", sys_table);
    AK_mem_block *mem_block = AK_get_block(0);
    for (i = 0; i < DATA_BLOCK_SIZE; i++)
    {
        memset(name_sys, 0, MAX_ATT_NAME);
        if (mem_block->block->tuple_dict[i].address == FREE_INT)
        {
            break;
        }
        data_adr = mem_block->block->tuple_dict[i].address;
        data_size = mem_block->block->tuple_dict[i].size;
        data_type = mem_block->block->tuple_dict[i].type;
        memcpy(name_sys, mem_block->block->data + data_adr, data_size);
        i++;
        data_adr = mem_block->block->tuple_dict[i].address;
        data_size = mem_block->block->tuple_dict[i].size;
        data_type = mem_block->block->tuple_dict[i].type;
        memcpy(&address_sys, mem_block->block->data + data_adr, sizeof(int));
        if (strcmp(name_sys, sys_table) == 0)
        {
            Ak_dbg_messg(HIGH, MEMO_MAN, "get_segment_addresses: Found the address of the %s table: %d \n", sys_table, address_sys);
            break;
        }
    }
    mem_block = AK_get_block(address_sys);
    table_addresses * addresses = (table_addresses *) AK_malloc(sizeof (table_addresses));
//memset(addresses->address_from, 0, MAX_EXTENTS_IN_SEGMENT);
//memset(addresses->address_to, 0, MAX_EXTENTS_IN_SEGMENT);
    for (AK_freeVar = 0; AK_freeVar < MAX_EXTENTS_IN_SEGMENT; AK_freeVar++)
    {
        addresses->address_from[AK_freeVar] = 0;
        addresses->address_to[AK_freeVar] = 0;
    }
    char name[MAX_VARCHAR_LENGTH];
    int address_from;
    int address_to;
    int j = 0;
    for (i = 0; i < DATA_BLOCK_SIZE; i++)
    {
        if (mem_block->block->tuple_dict[i].type == FREE_INT)
            break;
        if ( (mem_block->block->last_tuple_dict_id) <= i )
            break;
        i++;
        memcpy(name, &(mem_block->block->data[mem_block->block->tuple_dict[i].address]), mem_block->block->tuple_dict[i].size);
        name[ mem_block->block->tuple_dict[i].size] = '\0';
        i++;
        memcpy(&address_from, &(mem_block->block->data[mem_block->block->tuple_dict[i].address]), mem_block->block->tuple_dict[i].size);
        i++;
        memcpy(&address_to, &(mem_block->block->data[mem_block->block->tuple_dict[i].address]), mem_block->block->tuple_dict[i].size);
//if found the table that addresses we need
        if (strcmp(name, segmentName) == 0)
        {
            addresses->address_from[j] = address_from;
            addresses->address_to[j] = address_to;
            j++;
            Ak_dbg_messg(HIGH, MEMO_MAN, "get_segment_addresses(%s): Found addresses of searching segment: %d , %d \n", name, address_from, address_to);
        }
        /*if (segmentType == SEGMENT_TYPE_INDEX)
        {
            i += 2;
        }*/
    }
    AK_EPI;
    return addresses;
}

/**
  * @author Mislav Čakarić
  * @brief function for geting addresses of some table
  * @param table table name that you search for
  * @return structure table_addresses witch contains start and end adresses of table extents, when form and to are 0 you are on the end of addresses
 */
table_addresses *AK_get_table_addresses(char *table)
{
    AK_PRO;
    table_addresses* ret;
    ret = AK_get_segment_addresses(table);
    AK_EPI;
    return ret;
}

/**
  * @author Mislav Čakarić
  * @brief Function for geting addresses of some index
  * @param index index name that you search for
  * @return structure table_addresses witch contains start and end adresses of table extents, when form and to are 0 you are on the end of addresses
 */
table_addresses *AK_get_index_addresses(char * index)
{
    AK_PRO;
    table_addresses* ret;
    ret = AK_get_index_segment_addresses(index);
    AK_EPI;
    return ret;
}

/**
  * @author Matija Novak, updated by Matija Šestak( function now uses caching)
  * @brief Function to find AK_free space in some block betwen block addresses. It's made for insert_row()
  * @param address addresses of extents
  * @return address of the block to write in
 */
int AK_find_AK_free_space(table_addresses * addresses)
{
    AK_mem_block *mem_block;
    int from = 0, to = 0, j = 0, i = 0;
    AK_PRO;
    Ak_dbg_messg(HIGH, MEMO_MAN, "find_AK_free_space: Searching for block that has AK_free space < 500 \n");

    for (j = 0; j < MAX_EXTENTS_IN_SEGMENT; j++)
    {
        if (addresses->address_from != 0)
        {
            from = addresses->address_from[j];
            to = addresses->address_to[j];

            //searching block
            for (i = from; i <= to; i++)
            {
                mem_block = AK_get_block(i);
                int AK_free_space_on = mem_block->block->AK_free_space;

                Ak_dbg_messg(HIGH, MEMO_MAN, "find_AK_free_space: FREE SPACE %d\n", mem_block->block->AK_free_space);

                if ((AK_free_space_on < MAX_FREE_SPACE_SIZE) &&
                        (mem_block->block->last_tuple_dict_id < MAX_LAST_TUPLE_DICT_SIZE_TO_USE))  //found AK_free block to write
                {
                    AK_EPI;
                    return i;
                }
            }
        }
        else break;
    }

    //I cant call function from memoman must consider another solution to place these functions
    int adr = -1;

    //need to create new extent
    AK_EPI;
    return adr;
}

/**
 * @author Nikola Bakoš, updated by Matija Šestak (function now uses caching), updated by Mislav Čakarić, updated by Dino Laktašić
 * @brief Function that extends the segment
 * @param table_name name of segment to extent
 * @param extent_type type of extent (can be one of:
        SEGMENT_TYPE_SYSTEM_TABLE,
        SEGMENT_TYPE_TABLE,
        SEGMENT_TYPE_INDEX,
        SEGMENT_TYPE_TRANSACTION,
        SEGMENT_TYPE_TEMP
  * @return address of new extent, otherwise EXIT_ERROR

 */
int AK_init_new_extent(char *table_name, int extent_type)
{
    char *sys_table;

    int old_size = 0;
    int new_size = 0;
    table_addresses *addresses = (table_addresses *) AK_get_segment_addresses(table_name);
    int block_address = addresses->address_from[0]; //before 1
    int block_written;

    AK_mem_block *mem_block = AK_get_block(block_address);
    int start_address = 0;
    float RESIZE_FACTOR = 0;
    int end_address;
    struct list_node *row_root;
    int obj_id = 0;
    //!!! to correct header BUG iterate through header from 0 to N-th block while there is
    //header attributes. Than create header and pass it to function for extent creation below.
    //Current implementation works only with tables with max MAX_ATTRIBUTES.
    int i = 0;
    AK_PRO;

    for (i = 0; i < MAX_EXTENTS_IN_SEGMENT; i++)
    {
        if (addresses->address_from[i] == 0)
            break;
        new_size = addresses->address_to[i] - addresses->address_from[i];
        if (new_size > old_size) //find largest extent
            old_size = new_size;
    }

    old_size++;

    if ((start_address = AK_new_extent(1, old_size, extent_type, mem_block->block->header)) == EXIT_ERROR)
    {
        printf("AK_init_new_extent: Could not allocate the new extent\n");
        AK_EPI;
        return EXIT_ERROR;
    }
    Ak_dbg_messg(HIGH, MEMO_MAN, "AK_init_new_extent: start_address=%i, old_size=%i, extent_type=%i\n", start_address, old_size, extent_type);

    switch (extent_type)
    {
    case SEGMENT_TYPE_TABLE:
        RESIZE_FACTOR = EXTENT_GROWTH_TABLE;
        sys_table = "AK_relation";
        break;
    case SEGMENT_TYPE_INDEX:
        RESIZE_FACTOR = EXTENT_GROWTH_INDEX;
        sys_table = "AK_index";
        break;
    case SEGMENT_TYPE_TRANSACTION:
        RESIZE_FACTOR = EXTENT_GROWTH_TRANSACTION;
        printf("Not implemented yet!\n");
        break;
    case SEGMENT_TYPE_TEMP:
        RESIZE_FACTOR = EXTENT_GROWTH_TEMP;
        printf("Not implemented yet!\n");
        break;
    }

    end_address = start_address + (old_size + old_size * RESIZE_FACTOR);
    //mem_block = (AK_mem_block *) AK_get_block(0);

    row_root = (struct list_node *) AK_malloc(sizeof (struct list_node));
    Ak_Init_L3(&row_root);
    //DeleteAllElements(row_root);
    Ak_Insert_New_Element(TYPE_INT, &obj_id, sys_table, "obj_id", row_root);
    Ak_Insert_New_Element(TYPE_VARCHAR, table_name, sys_table, "name", row_root);
    Ak_Insert_New_Element(TYPE_INT, &start_address, sys_table, "start_address", row_root);
    Ak_Insert_New_Element(TYPE_INT, &end_address, sys_table, "end_address", row_root);
    Ak_insert_row(row_root);
    AK_EPI;
    return start_address;
}

/**
 * @author Matija Šestak
 * @brief Function that flushes memory blocks to disk file
 * @return EXIT_SUCCESS
 */
int AK_flush_cache()
{
    int i = 0;
    int block_written;
    AK_block *data_block;
    AK_PRO;
    while (i < MAX_CACHE_MEMORY)
    {
        if (db_cache->cache[i]->dirty == BLOCK_DIRTY)
        {
            data_block = db_cache->cache[i]->block;
            block_written = AK_write_block(data_block);
            /// if block form cache can not be writed to DB file -> EXIT_ERROR
            if (block_written != EXIT_SUCCESS)
            {
                AK_EPI;
                exit(EXIT_ERROR);
            }
        }
        i++;
    }
    AK_EPI;
    return EXIT_SUCCESS;
}

void AK_memoman_test()
{
    int i;
    AK_PRO;

    for (i = 0; i < MAX_CACHE_MEMORY; i++)
        printf("Block: %d \t l_address: %d \t c_address: %x\n",i,db_cache->cache[i]->block->address, &db_cache->cache[i]->block );
    AK_EPI;
}

void AK_memoman_test2()
{
    int i;
    //int aa=0;
    int aa=406;
    AK_PRO;
    printf("\tPick up block from 0 to: %d \n",AK_allocationbit->last_allocated );

    scanf("%d", &aa);
    if(aa>=0 && aa<AK_allocationbit->last_allocated)
    {
        printf("\n\tFirst goes dump of block from HDD:\n");
        AK_print_block(NULL, aa, "Memoman_test2_HDD",stdout);
        printf("\n\n\n");

        //printf("\n\n\t Then dump of block from cache:\n");
        //AK_print_block(db_cache->cache[aa]->block, aa, "Memoman_test2_Cache",stdout);
    }
    AK_EPI;
}
