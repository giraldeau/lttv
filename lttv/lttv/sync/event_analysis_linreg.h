/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2009 Benjamin Poirier <benjamin.poirier@polymtl.ca>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#ifndef EVENT_ANALYSIS_LINREG_H
#define EVENT_ANALYSIS_LINREG_H

#include <glib.h>

#include "data_structures.h"


typedef struct
{
	unsigned int n;
	// notation: s__: sum of __; __2: __ squared; example sd2: sum of d squared
	double st, st2, sd, sd2, std, x, d0, e;
} Fit;

typedef struct
{
	double errorSum;
	unsigned int* previousVertex;
	unsigned int reference;
} Graph;

typedef struct
{
	Fit** fitArray;

	// Graph[]
	GQueue* graphList;

	// for statistics
	double* stDev;
} AnalysisDataLinReg;

void registerAnalysisLinReg();

#endif
