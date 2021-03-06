/**
@file constraint_names.h Header file that provides functions and data structures for checking if constraint name is unique in database
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

#ifndef CONSTRAINT_NAMES
#define CONSTRAINT_NAMES

#include "../../file/table.h"
#include "../../file/fileio.h"
#include "../../auxi/mempro.h"

int Ak_check_constraint_name(char* constraintName);
void AK_constraint_names_test();

#endif