/**
@file intersect.c Provides functions for relational intersect operation
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */


#include "intersect.h"

/**
 * @author Dino Laktašić
 * @brief  Function to make intersect of the two tables. Intersect is implemented for working with multiple sets of data, i.e. duplicate 
          tuples can be written in same table (intersect)
 * @param srcTable1 name of the first table
 * @param srcTable2 name of the second table
 * @param dstTable name of the new table
 * @return if success returns EXIT_SUCCESS, else returns EXIT_ERROR
 */
int AK_intersect(char *srcTable1, char *srcTable2, char *dstTable) {
    table_addresses *src_addr1 = (table_addresses*) AK_get_table_addresses(srcTable1);
    table_addresses *src_addr2 = (table_addresses*) AK_get_table_addresses(srcTable2);

    int startAddress1 = src_addr1->address_from[0];
    int startAddress2 = src_addr2->address_from[0];

    if ((startAddress1 != 0) && (startAddress2 != 0)) {
        register int i, j, k, l;
        i = j = k = l = 0;

        AK_mem_block *tbl1_temp_block = (AK_mem_block *) AK_get_block(startAddress1);
        AK_mem_block *tbl2_temp_block = (AK_mem_block *) AK_get_block(startAddress2);
        
        int num_att = AK_check_tables_scheme(tbl1_temp_block, tbl2_temp_block, "Intersect");

        int something_to_copy = 0, m, n, o;
	int address, type, size;
		
        char data1[MAX_VARCHAR_LENGTH];
        char data2[MAX_VARCHAR_LENGTH];

        //initialize new segment
        AK_header *header = (AK_header *) malloc(num_att * sizeof (AK_header));
        memcpy(header, tbl1_temp_block->block->header, num_att * sizeof (AK_header));
        AK_initialize_new_segment(dstTable, SEGMENT_TYPE_TABLE, header);
        free(header);

        AK_list *row_root = (AK_list *) malloc(sizeof (AK_list));

        //TABLE1: for each extent in table1
        for (i = 0; src_addr1->address_from[i] != 0; i++) {
            startAddress1 = src_addr1->address_from[i];

                //BLOCK: for each block in table1 extent
                for (j = startAddress1; j < src_addr1->address_to[i]; j++) {
                    tbl1_temp_block = (AK_mem_block *) AK_get_block(j);

                    //if there is data in the block
                    if (tbl1_temp_block->block->free_space != 0) {

                        //TABLE2: for each extent in table2
                        for (k = 0; k < src_addr2->address_from[k] != 0; k++) {
                            startAddress2 = src_addr2->address_from[k];

                            if (startAddress2 != 0) {

                                //BLOCK: for each block in table2 extent
                                for (l = startAddress2; l < src_addr2->address_to[k]; l++) {
                                    tbl2_temp_block = (AK_mem_block *) AK_get_block(l);

                                    //if there is data in the block
                                    if (tbl2_temp_block->block->free_space != 0) {
										
                                        //TUPLE_DICTS: for each tuple_dict in the block
                                        for (m = 0; m < DATA_BLOCK_SIZE; m += num_att) {
                                            if (tbl1_temp_block->block->tuple_dict[m + 1].type == FREE_INT)
                                                break;

                                            for (o = 0; o < DATA_BLOCK_SIZE; o += num_att) {
                                                if (tbl2_temp_block->block->tuple_dict[o + 1].type == FREE_INT)
                                                    break;

                                                something_to_copy = 0;
                                                for (n = 0; n < num_att; n++) {
                                                    type = tbl1_temp_block->block->tuple_dict[m + n].type;
                                                    size = tbl1_temp_block->block->tuple_dict[m + n].size;
                                                    address = tbl1_temp_block->block->tuple_dict[m + n].address;
													
                                                    memcpy(data1, &(tbl1_temp_block->block->data[address]), size);
                                                    data1[size] = '\0';

                                                    type = tbl2_temp_block->block->tuple_dict[o + n].type;
                                                    size = tbl2_temp_block->block->tuple_dict[o + n].size;
                                                    address = tbl2_temp_block->block->tuple_dict[o + n].address;
													
                                                    memcpy(data2, &(tbl2_temp_block->block->data[address]), size);
                                                    data2[size] = '\0';

                                                    //printf("%s == %s\n", data1, data2);
                                                    if (strcmp(data1, data2) == 0) {
                                                        something_to_copy++;
                                                    }
                                                }

                                                if (something_to_copy == num_att) {
                                                    for (n = 0; n < num_att; n++) {
                                                        type = tbl1_temp_block->block->tuple_dict[m + n].type;
                                                        size = tbl1_temp_block->block->tuple_dict[m + n].size;
                                                        address = tbl1_temp_block->block->tuple_dict[m + n].address;
														
                                                        memcpy(data1, &(tbl1_temp_block->block->data[address]), size);
                                                        data1[size] = '\0';
														
                                                        Ak_Insert_New_Element_For_Update(type, data1, dstTable, tbl1_temp_block->block->header[n].att_name, row_root, 0);
                                                    }
                                                    Ak_insert_row(row_root);
                                                    Ak_DeleteAll_L(row_root);
                                                }
                                            }
                                        }
                                    }
                                }
                            } else break;
                        }
                    }
                }
        }

        free(src_addr1);
        free(src_addr2);
	Ak_dbg_messg(LOW, REL_OP, "INTERSECT_TEST_SUCCESS\n\n");
	
	AK_archive_log("AK_intersect", srcTable1, srcTable2, dstTable); //ARCHIVE_LOG
	
        return EXIT_SUCCESS;
    } else {
        Ak_dbg_messg(LOW, REL_OP, "\nAK_intersect: Table/s doesn't exist!");
        free(src_addr1);
        free(src_addr2);
	return EXIT_ERROR;
    }
}

/**
 * @author Dino Laktašić
 * @brief  Function for intersect operator testing
 * @return No return value
 */
void Ak_op_intersect_test() {
    printf("\n********** INTERSECT TEST **********\n\n");

    AK_intersect("professor", "assistant", "intersect_test");
    AK_print_table("intersect_test");
}