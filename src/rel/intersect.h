/**
@file intersect.h Provides data structures for relational intersect operation
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
#ifndef INTERSECT
#define INTERSECT

#include "../file/table.h"
#include "../file/fileio.h"
#include "../rec/archive_log.h"
#include "../auxi/mempro.h"

/**
 * @author Dino Laktašić
 * @struct intersect_attr
 * @brief Structure defines intersect attribute
 */
typedef struct {
    /// type of attribute
    int type;
    /// attribute name
    char att_name[ MAX_ATT_NAME ];
} intersect_attr;

int AK_intersect(char *srcTable1, char *srcTable2, char *dstTable);
void Ak_op_intersect_test();

#endif
