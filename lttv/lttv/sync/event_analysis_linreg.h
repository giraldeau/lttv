/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2009, 2010 Benjamin Poirier <benjamin.poirier@polymtl.ca>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
	Fit** fitArray;

	// for statistics
	double* stDev;
} AnalysisDataLinReg;

void registerAnalysisLinReg();

#endif
