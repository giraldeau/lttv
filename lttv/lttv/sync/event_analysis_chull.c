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
#define _ISOC99_SOURCE

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "sync_chain.h"

#include "event_analysis_chull.h"


typedef enum
{
	LOWER,
	UPPER
} HullType;


typedef enum
{
	MINIMUM,
	MAXIMUM
} LineType;


// Functions common to all analysis modules
static void initAnalysisCHull(SyncState* const syncState);
static void destroyAnalysisCHull(SyncState* const syncState);

static void analyzeMessageCHull(SyncState* const syncState, Message* const
	message);
static GArray* finalizeAnalysisCHull(SyncState* const syncState);
static void printAnalysisStatsCHull(SyncState* const syncState);
static void writeAnalysisGraphsPlotsCHull(SyncState* const syncState, const
	unsigned int i, const unsigned int j);

// Functions specific to this module
static void registerAnalysisCHull() __attribute__((constructor (101)));

static void openGraphFiles(SyncState* const syncState);
static void closeGraphFiles(SyncState* const syncState);
static void writeGraphFiles(SyncState* const syncState);
static void gfDumpHullToFile(gpointer data, gpointer userData);

static void grahamScan(GQueue* const hull, Point* const newPoint, const
	HullType type);
static int jointCmp(const Point* const p1, const Point* const p2, const Point*
	const p3) __attribute__((pure));
static double crossProductK(const Point const* p1, const Point const* p2,
	const Point const* p3, const Point const* p4) __attribute__((pure));
static Factors* calculateFactorsExact(GQueue* const cu, GQueue* const cl, const
	LineType lineType) __attribute__((pure));
static void calculateFactorsFallback(GQueue* const cr, GQueue* const cs,
	FactorsCHull* const result);
static double slope(const Point* const p1, const Point* const p2)
	__attribute__((pure));
static double intercept(const Point* const p1, const Point* const p2)
	__attribute__((pure));
static GArray* reduceFactors(SyncState* const syncState, FactorsCHull**
	allFactors);
static double verticalDistance(Point* p1, Point* p2, Point* const point)
	__attribute__((pure));
static void floydWarshall(SyncState* const syncState, FactorsCHull** const
	allFactors, double*** const distances, unsigned int*** const
	predecessors);
static void getFactors(FactorsCHull** const allFactors, unsigned int** const
	predecessors, unsigned int* const references, const unsigned int traceNum,
	Factors* const factors);

static void gfPointDestroy(gpointer data, gpointer userData);


static AnalysisModule analysisModuleCHull= {
	.name= "chull",
	.initAnalysis= &initAnalysisCHull,
	.destroyAnalysis= &destroyAnalysisCHull,
	.analyzeMessage= &analyzeMessageCHull,
	.finalizeAnalysis= &finalizeAnalysisCHull,
	.printAnalysisStats= &printAnalysisStatsCHull,
	.graphFunctions= {
		.writeTraceTraceForePlots= &writeAnalysisGraphsPlotsCHull,
	}
};

const char* const approxNames[]= {
	[EXACT]= "Exact",
	[MIDDLE]= "Middle",
	[FALLBACK]= "Fallback",
	[INCOMPLETE]= "Incomplete",
	[ABSENT]= "Absent",
	[SCREWED]= "Screwed",
};


/*
 * Analysis module registering function
 */
static void registerAnalysisCHull()
{
	g_queue_push_tail(&analysisModules, &analysisModuleCHull);
}


/*
 * Analysis init function
 *
 * This function is called at the beginning of a synchronization run for a set
 * of traces.
 *
 * Allocate some of the analysis specific data structures
 *
 * Args:
 *   syncState     container for synchronization data.
 *                 This function allocates or initializes these analysisData
 *                 members:
 *                 hullArray
 *                 dropped
 */
static void initAnalysisCHull(SyncState* const syncState)
{
	unsigned int i, j;
	AnalysisDataCHull* analysisData;

	analysisData= malloc(sizeof(AnalysisDataCHull));
	syncState->analysisData= analysisData;

	analysisData->hullArray= malloc(syncState->traceNb * sizeof(GQueue**));
	for (i= 0; i < syncState->traceNb; i++)
	{
		analysisData->hullArray[i]= malloc(syncState->traceNb * sizeof(GQueue*));

		for (j= 0; j < syncState->traceNb; j++)
		{
			analysisData->hullArray[i][j]= g_queue_new();
		}
	}

	if (syncState->stats)
	{
		analysisData->stats= malloc(sizeof(AnalysisStatsCHull));
		analysisData->stats->dropped= 0;
		analysisData->stats->allFactors= NULL;
	}

	if (syncState->graphsStream)
	{
		analysisData->graphsData= malloc(sizeof(AnalysisGraphsDataCHull));
		openGraphFiles(syncState);
		analysisData->graphsData->allFactors= NULL;
	}
}


/*
 * Create and open files used to store convex hull points to genereate
 * graphs. Allocate and populate array to store file pointers.
 *
 * Args:
 *   syncState:    container for synchronization data
 */
static void openGraphFiles(SyncState* const syncState)
{
	unsigned int i, j;
	int retval;
	char* cwd;
	char name[31];
	AnalysisDataCHull* analysisData;

	analysisData= (AnalysisDataCHull*) syncState->analysisData;

	cwd= changeToGraphsDir(syncState->graphsDir);

	analysisData->graphsData->hullPoints= malloc(syncState->traceNb *
		sizeof(FILE**));
	for (i= 0; i < syncState->traceNb; i++)
	{
		analysisData->graphsData->hullPoints[i]= malloc(syncState->traceNb *
			sizeof(FILE*));
		for (j= 0; j < syncState->traceNb; j++)
		{
			if (i != j)
			{
				retval= snprintf(name, sizeof(name),
					"analysis_chull-%03u_to_%03u.data", j, i);
				if (retval > sizeof(name) - 1)
				{
					name[sizeof(name) - 1]= '\0';
				}
				if ((analysisData->graphsData->hullPoints[i][j]= fopen(name, "w")) ==
					NULL)
				{
					g_error(strerror(errno));
				}
			}
		}
	}

	retval= chdir(cwd);
	if (retval == -1)
	{
		g_error(strerror(errno));
	}
	free(cwd);
}


/*
 * Write hull points to files to generate graphs.
 *
 * Args:
 *   syncState:    container for synchronization data
 */
static void writeGraphFiles(SyncState* const syncState)
{
	unsigned int i, j;
	AnalysisDataCHull* analysisData;

	analysisData= (AnalysisDataCHull*) syncState->analysisData;

	for (i= 0; i < syncState->traceNb; i++)
	{
		for (j= 0; j < syncState->traceNb; j++)
		{
			if (i != j)
			{
				g_queue_foreach(analysisData->hullArray[i][j],
					&gfDumpHullToFile,
					analysisData->graphsData->hullPoints[i][j]);
			}
		}
	}
}


/*
 * A GFunc for g_queue_foreach. Write a hull point to a file used to generate
 * graphs
 *
 * Args:
 *   data:         Point*, point to write to the file
 *   userData:     FILE*, file pointer where to write the point
 */
static void gfDumpHullToFile(gpointer data, gpointer userData)
{
	Point* point;

	point= (Point*) data;
	fprintf((FILE*) userData, "%20llu %20llu\n", point->x, point->y);
}


/*
 * Close files used to store convex hull points to generate graphs.
 * Deallocate array to store file pointers.
 *
 * Args:
 *   syncState:    container for synchronization data
 */
static void closeGraphFiles(SyncState* const syncState)
{
	unsigned int i, j;
	AnalysisDataCHull* analysisData;
	int retval;

	analysisData= (AnalysisDataCHull*) syncState->analysisData;

	if (analysisData->graphsData->hullPoints == NULL)
	{
		return;
	}

	for (i= 0; i < syncState->traceNb; i++)
	{
		for (j= 0; j < syncState->traceNb; j++)
		{
			if (i != j)
			{
				retval= fclose(analysisData->graphsData->hullPoints[i][j]);
				if (retval != 0)
				{
					g_error(strerror(errno));
				}
			}
		}
		free(analysisData->graphsData->hullPoints[i]);
	}
	free(analysisData->graphsData->hullPoints);
	analysisData->graphsData->hullPoints= NULL;
}


/*
 * Analysis destroy function
 *
 * Free the analysis specific data structures
 *
 * Args:
 *   syncState     container for synchronization data.
 *                 This function deallocates these analysisData members:
 *                 hullArray
 *                 stDev
 */
static void destroyAnalysisCHull(SyncState* const syncState)
{
	unsigned int i, j;
	AnalysisDataCHull* analysisData;

	analysisData= (AnalysisDataCHull*) syncState->analysisData;

	if (analysisData == NULL)
	{
		return;
	}

	for (i= 0; i < syncState->traceNb; i++)
	{
		for (j= 0; j < syncState->traceNb; j++)
		{
			g_queue_foreach(analysisData->hullArray[i][j], gfPointDestroy, NULL);
		}
		free(analysisData->hullArray[i]);
	}
	free(analysisData->hullArray);

	if (syncState->stats)
	{
		if (analysisData->stats->allFactors != NULL)
		{
			freeAllFactors(syncState->traceNb, analysisData->stats->allFactors);
		}

		free(analysisData->stats);
	}

	if (syncState->graphsStream)
	{
		if (analysisData->graphsData->hullPoints != NULL)
		{
			closeGraphFiles(syncState);
		}

		if (!syncState->stats && analysisData->graphsData->allFactors != NULL)
		{
			freeAllFactors(syncState->traceNb, analysisData->graphsData->allFactors);
		}

		free(analysisData->graphsData);
	}

	free(syncState->analysisData);
	syncState->analysisData= NULL;
}


/*
 * Perform analysis on an event pair.
 *
 * Args:
 *   syncState     container for synchronization data
 *   message       structure containing the events
 */
static void analyzeMessageCHull(SyncState* const syncState, Message* const message)
{
	AnalysisDataCHull* analysisData;
	Point* newPoint;
	HullType hullType;
	GQueue* hull;

	analysisData= (AnalysisDataCHull*) syncState->analysisData;

	newPoint= malloc(sizeof(Point));
	if (message->inE->traceNum < message->outE->traceNum)
	{
		// CA is inE->traceNum
		newPoint->x= message->inE->cpuTime;
		newPoint->y= message->outE->cpuTime;
		hullType= UPPER;
		g_debug("Reception point hullArray[%lu][%lu] x= inE->time= %llu y= outE->time= %llu",
			message->inE->traceNum, message->outE->traceNum, newPoint->x,
			newPoint->y);
	}
	else
	{
		// CA is outE->traceNum
		newPoint->x= message->outE->cpuTime;
		newPoint->y= message->inE->cpuTime;
		hullType= LOWER;
		g_debug("Send point hullArray[%lu][%lu] x= inE->time= %llu y= outE->time= %llu",
			message->inE->traceNum, message->outE->traceNum, newPoint->x,
			newPoint->y);
	}

	hull=
		analysisData->hullArray[message->inE->traceNum][message->outE->traceNum];

	if (hull->length >= 1 && newPoint->x < ((Point*)
			g_queue_peek_tail(hull))->x)
	{
		if (syncState->stats)
		{
			analysisData->stats->dropped++;
		}

		free(newPoint);
	}
	else
	{
		grahamScan(hull, newPoint, hullType);
	}
}


/*
 * Construct one half of a convex hull from abscissa-sorted points
 *
 * Args:
 *   hull:         the points already in the hull
 *   newPoint:     a new point to consider
 *   type:         which half of the hull to construct
 */
static void grahamScan(GQueue* const hull, Point* const newPoint, const
	HullType type)
{
	int inversionFactor;

	g_debug("grahamScan(hull (length: %u), newPoint, %s)", hull->length, type
		== LOWER ? "LOWER" : "UPPER");

	if (type == LOWER)
	{
		inversionFactor= 1;
	}
	else
	{
		inversionFactor= -1;
	}

	if (hull->length >= 2)
	{
		g_debug("jointCmp(hull[%u], hull[%u], newPoint) * inversionFactor = %d * %d = %d",
			hull->length - 2,
			hull->length - 1,
			jointCmp(g_queue_peek_nth(hull, hull->length - 2),
				g_queue_peek_tail(hull), newPoint),
			inversionFactor,
			jointCmp(g_queue_peek_nth(hull, hull->length - 2),
				g_queue_peek_tail(hull), newPoint) * inversionFactor);
	}
	while (hull->length >= 2 && jointCmp(g_queue_peek_nth(hull, hull->length -
				2), g_queue_peek_tail(hull), newPoint) * inversionFactor <= 0)
	{
		g_debug("Removing hull[%u]", hull->length);
		free((Point*) g_queue_pop_tail(hull));

		if (hull->length >= 2)
		{
			g_debug("jointCmp(hull[%u], hull[%u], newPoint) * inversionFactor = %d * %d = %d",
				hull->length - 2,
				hull->length - 1,
				jointCmp(g_queue_peek_nth(hull, hull->length - 2),
					g_queue_peek_tail(hull), newPoint),
				inversionFactor,
				jointCmp(g_queue_peek_nth(hull, hull->length - 2),
					g_queue_peek_tail(hull), newPoint) * inversionFactor);
		}
	}
	g_queue_push_tail(hull, newPoint);
}


/*
 * Finalize the factor calculations
 *
 * Args:
 *   syncState     container for synchronization data.
 *
 * Returns:
 *   Factors[traceNb] synchronization factors for each trace
 */
static GArray* finalizeAnalysisCHull(SyncState* const syncState)
{
	AnalysisDataCHull* analysisData;
	GArray* factors;
	FactorsCHull** allFactors;

	analysisData= (AnalysisDataCHull*) syncState->analysisData;

	if (syncState->graphsStream && analysisData->graphsData->hullPoints != NULL)
	{
		writeGraphFiles(syncState);
		closeGraphFiles(syncState);
	}

	allFactors= calculateAllFactors(syncState);

	factors= reduceFactors(syncState, allFactors);

	if (syncState->stats || syncState->graphsStream)
	{
		if (syncState->stats)
		{
			analysisData->stats->allFactors= allFactors;
		}

		if (syncState->graphsStream)
		{
			analysisData->graphsData->allFactors= allFactors;
		}
	}
	else
	{
		freeAllFactors(syncState->traceNb, allFactors);
	}

	return factors;
}


/*
 * Print statistics related to analysis. Must be called after
 * finalizeAnalysis.
 *
 * Args:
 *   syncState     container for synchronization data.
 */
static void printAnalysisStatsCHull(SyncState* const syncState)
{
	AnalysisDataCHull* analysisData;
	unsigned int i, j;

	if (!syncState->stats)
	{
		return;
	}

	analysisData= (AnalysisDataCHull*) syncState->analysisData;

	printf("Convex hull analysis stats:\n");
	printf("\tout of order packets dropped from analysis: %u\n",
		analysisData->stats->dropped);

	printf("\tNumber of points in convex hulls:\n");

	for (i= 0; i < syncState->traceNb; i++)
	{
		for (j= i + 1; j < syncState->traceNb; j++)
		{
			printf("\t\t%3d - %-3d: lower half-hull %-5u upper half-hull %-5u\n",
				i, j, analysisData->hullArray[j][i]->length,
				analysisData->hullArray[i][j]->length);
		}
	}

	printf("\tIndividual synchronization factors:\n");

	for (i= 0; i < syncState->traceNb; i++)
	{
		for (j= i + 1; j < syncState->traceNb; j++)
		{
			FactorsCHull* factorsCHull;

			factorsCHull= &analysisData->stats->allFactors[j][i];
			printf("\t\t%3d - %-3d: %s", i, j,
				approxNames[factorsCHull->type]);

			if (factorsCHull->type == EXACT)
			{
				printf("      a0= % 7g a1= 1 %c %7g\n",
					factorsCHull->approx->offset,
					factorsCHull->approx->drift < 0. ? '-' : '+',
					fabs(factorsCHull->approx->drift));
			}
			else if (factorsCHull->type == MIDDLE)
			{
				printf("     a0= % 7g a1= 1 %c %7g accuracy %7g\n",
					factorsCHull->approx->offset, factorsCHull->approx->drift
					- 1. < 0. ? '-' : '+', fabs(factorsCHull->approx->drift -
						1.), factorsCHull->accuracy);
				printf("\t\t                      a0: % 7g to % 7g (delta= %7g)\n",
					factorsCHull->max->offset, factorsCHull->min->offset,
					factorsCHull->min->offset - factorsCHull->max->offset);
				printf("\t\t                      a1: 1 %+7g to %+7g (delta= %7g)\n",
					factorsCHull->min->drift - 1., factorsCHull->max->drift -
					1., factorsCHull->max->drift - factorsCHull->min->drift);
			}
			else if (factorsCHull->type == FALLBACK)
			{
				printf("   a0= % 7g a1= 1 %c %7g error= %7g\n",
					factorsCHull->approx->offset, factorsCHull->approx->drift
					- 1. < 0. ? '-' : '+', fabs(factorsCHull->approx->drift -
						1.), factorsCHull->accuracy);
			}
			else if (factorsCHull->type == INCOMPLETE)
			{
				printf("\n");

				if (factorsCHull->min->drift != -INFINITY)
				{
					printf("\t\t                      min: a0: % 7g a1: 1 %c %7g\n",
						factorsCHull->min->offset, factorsCHull->min->drift -
						1. < 0 ? '-' : '+', fabs(factorsCHull->min->drift -
							1.));
				}
				if (factorsCHull->max->drift != INFINITY)
				{
					printf("\t\t                      max: a0: % 7g a1: 1 %c %7g\n",
						factorsCHull->max->offset, factorsCHull->max->drift -
						1. < 0 ? '-' : '+', fabs(factorsCHull->max->drift -
							1.));
				}
			}
			else if (factorsCHull->type == SCREWED)
			{
				printf("\n");

				if (factorsCHull->min != NULL && factorsCHull->min->drift != -INFINITY)
				{
					printf("\t\t                      min: a0: % 7g a1: 1 %c %7g\n",
						factorsCHull->min->offset, factorsCHull->min->drift -
						1. < 0 ? '-' : '+', fabs(factorsCHull->min->drift -
							1.));
				}
				if (factorsCHull->max != NULL && factorsCHull->max->drift != INFINITY)
				{
					printf("\t\t                      max: a0: % 7g a1: 1 %c %7g\n",
						factorsCHull->max->offset, factorsCHull->max->drift -
						1. < 0 ? '-' : '+', fabs(factorsCHull->max->drift -
							1.));
				}
			}
			else if (factorsCHull->type == ABSENT)
			{
				printf("\n");
			}
			else
			{
				g_assert_not_reached();
			}
		}
	}
}


/*
 * A GFunc for g_queue_foreach()
 *
 * Args:
 *   data          Point*, point to destroy
 *   user_data     NULL
 */
static void gfPointDestroy(gpointer data, gpointer userData)
{
	Point* point;

	point= (Point*) data;
	free(point);
}


/*
 * Find out if a sequence of three points constitutes a "left turn" or a
 * "right turn".
 *
 * Args:
 *   p1, p2, p3:   The three points.
 *
 * Returns:
 *   < 0           right turn
 *   0             colinear (unlikely result since this uses floating point
 *                 arithmetic)
 *   > 0           left turn
 */
static int jointCmp(const Point const* p1, const Point const* p2, const
	Point const* p3)
{
	double result;
	const double fuzzFactor= 0.;

	result= crossProductK(p1, p2, p1, p3);
	g_debug("crossProductK(p1= (%llu, %llu), p2= (%llu, %llu), p1= (%llu, %llu), p3= (%llu, %llu))= %g",
		p1->x, p1->y, p2->x, p2->y, p1->x, p1->y, p3->x, p3->y, result);
	if (result < fuzzFactor)
	{
		return -1;
	}
	else if (result > fuzzFactor)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


/*
 * Calculate the k component of the cross product of two vectors.
 *
 * Args:
 *   p1, p2:       start and end points of the first vector
 *   p3, p4:       start and end points of the second vector
 *
 * Returns:
 *   the k component of the cross product when considering the two vectors to
 *   be in the i-j plane. The direction (sign) of the result can be useful to
 *   determine the relative orientation of the two vectors.
 */
static double crossProductK(const Point const* p1, const Point const* p2,
	const Point const* p3, const Point const* p4)
{
	return ((double) p2->x - p1->x) * ((double) p4->y - p3->y) - ((double)
		p2->y - p1->y) * ((double) p4->x - p3->x);
}


/*
 * Free a container of FactorsCHull
 *
 * Args:
 *   traceNb:      number of traces
 *   allFactors:   container of FactorsCHull
 */
void freeAllFactors(const unsigned int traceNb, FactorsCHull** const
	allFactors)
{
	unsigned int i, j;

	for (i= 0; i < traceNb; i++)
	{
		for (j= 0; j <= i; j++)
		{
			destroyFactorsCHull(&allFactors[i][j]);
		}
		free(allFactors[i]);
	}
	free(allFactors);
}


/*
 * Free a FactorsCHull
 *
 * Args:
 *   factorsCHull: container of Factors
 */
void destroyFactorsCHull(FactorsCHull* factorsCHull)
{
	if (factorsCHull->type == MIDDLE || factorsCHull->type ==
		INCOMPLETE || factorsCHull->type == ABSENT)
	{
		free(factorsCHull->min);
		free(factorsCHull->max);
	}
	else if (factorsCHull->type == SCREWED)
	{
		if (factorsCHull->min != NULL)
		{
			free(factorsCHull->min);
		}
		if (factorsCHull->max != NULL)
		{
			free(factorsCHull->max);
		}
	}

	if (factorsCHull->type == EXACT || factorsCHull->type == MIDDLE ||
		factorsCHull->type == FALLBACK)
	{
		free(factorsCHull->approx);
	}
}


/*
 * Analyze the convex hulls to determine the synchronization factors between
 * each pair of trace.
 *
 * Args:
 *   syncState     container for synchronization data.
 *
 * Returns:
 *   FactorsCHull*[TraceNum][TraceNum] array. See the documentation for the
 *   member allFactors of AnalysisStatsCHull.
 */
FactorsCHull** calculateAllFactors(SyncState* const syncState)
{
	unsigned int traceNumA, traceNumB;
	FactorsCHull** allFactors;
	AnalysisDataCHull* analysisData;

	analysisData= (AnalysisDataCHull*) syncState->analysisData;

	// Allocate allFactors and calculate min and max
	allFactors= malloc(syncState->traceNb * sizeof(FactorsCHull*));
	for (traceNumA= 0; traceNumA < syncState->traceNb; traceNumA++)
	{
		allFactors[traceNumA]= malloc((traceNumA + 1) * sizeof(FactorsCHull));

		allFactors[traceNumA][traceNumA].type= EXACT;
		allFactors[traceNumA][traceNumA].approx= malloc(sizeof(Factors));
		allFactors[traceNumA][traceNumA].approx->drift= 1.;
		allFactors[traceNumA][traceNumA].approx->offset= 0.;

		for (traceNumB= 0; traceNumB < traceNumA; traceNumB++)
		{
			unsigned int i;
			GQueue* cs, * cr;
			const struct
			{
				LineType lineType;
				size_t factorsOffset;
			} loopValues[]= {
				{MINIMUM, offsetof(FactorsCHull, min)},
				{MAXIMUM, offsetof(FactorsCHull, max)}
			};

			cr= analysisData->hullArray[traceNumB][traceNumA];
			cs= analysisData->hullArray[traceNumA][traceNumB];

			for (i= 0; i < sizeof(loopValues) / sizeof(*loopValues); i++)
			{
				g_debug("allFactors[%u][%u].%s = calculateFactorsExact(cr= hullArray[%u][%u], cs= hullArray[%u][%u], %s)",
					traceNumA, traceNumB, loopValues[i].factorsOffset ==
					offsetof(FactorsCHull, min) ? "min" : "max", traceNumB,
					traceNumA, traceNumA, traceNumB, loopValues[i].lineType ==
					MINIMUM ? "MINIMUM" : "MAXIMUM");
				*((Factors**) ((void*) &allFactors[traceNumA][traceNumB] +
						loopValues[i].factorsOffset))=
					calculateFactorsExact(cr, cs, loopValues[i].lineType);
			}
		}
	}

	// Calculate approx when possible
	for (traceNumA= 0; traceNumA < syncState->traceNb; traceNumA++)
	{
		for (traceNumB= 0; traceNumB < traceNumA; traceNumB++)
		{
			FactorsCHull* factorsCHull;

			factorsCHull= &allFactors[traceNumA][traceNumB];
			if (factorsCHull->min == NULL && factorsCHull->max == NULL)
			{
				factorsCHull->type= FALLBACK;
				calculateFactorsFallback(analysisData->hullArray[traceNumB][traceNumA],
					analysisData->hullArray[traceNumA][traceNumB],
					&allFactors[traceNumA][traceNumB]);
			}
			else if (factorsCHull->min != NULL && factorsCHull->max != NULL)
			{
				if (factorsCHull->min->drift != -INFINITY &&
					factorsCHull->max->drift != INFINITY)
				{
					factorsCHull->type= MIDDLE;
					calculateFactorsMiddle(factorsCHull);
				}
				else if (factorsCHull->min->drift != -INFINITY ||
					factorsCHull->max->drift != INFINITY)
				{
					factorsCHull->type= INCOMPLETE;
				}
				else
				{
					factorsCHull->type= ABSENT;
				}
			}
			else
			{
				//g_assert_not_reached();
				factorsCHull->type= SCREWED;
			}
		}
	}

	return allFactors;
}


/* Calculate approximative factors based on minimum and maximum limits. The
 * best approximation to make is the interior bissector of the angle formed by
 * the minimum and maximum lines.
 *
 * The formulae used come from [Haddad, Yoram: Performance dans les systèmes
 * répartis: des outils pour les mesures, Université de Paris-Sud, Centre
 * d'Orsay, September 1988] Section 6.1 p.44
 *
 * The reasoning for choosing this estimator comes from [Duda, A., Harrus, G.,
 * Haddad, Y., and Bernard, G.: Estimating global time in distributed systems,
 * Proc. 7th Int. Conf. on Distributed Computing Systems, Berlin, volume 18,
 * 1987] p.303
 *
 * Args:
 *   factors:      contains the min and max limits, used to store the result
 */
void calculateFactorsMiddle(FactorsCHull* const factors)
{
	double amin, amax, bmin, bmax, bhat;

	amin= factors->max->offset;
	amax= factors->min->offset;
	bmin= factors->min->drift;
	bmax= factors->max->drift;

	g_assert_cmpfloat(bmax, >, bmin);

	factors->approx= malloc(sizeof(Factors));
	bhat= (bmax * bmin - 1. + sqrt(1. + pow(bmax, 2.) * pow(bmin, 2.) +
			pow(bmax, 2.) + pow(bmin, 2.))) / (bmax + bmin);
	factors->approx->offset= amax - (amax - amin) / 2. * (pow(bhat, 2.) + 1.)
		/ (1. + bhat * bmax);
	factors->approx->drift= bhat;
	factors->accuracy= bmax - bmin;
}


/*
 * Analyze the convex hulls to determine the minimum or maximum
 * synchronization factors between one pair of trace.
 *
 * This implements and improves upon the algorithm in [Haddad, Yoram:
 * Performance dans les systèmes répartis: des outils pour les mesures,
 * Université de Paris-Sud, Centre d'Orsay, September 1988] Section 6.2 p.47
 *
 * Some degenerate cases are possible:
 * 1) the result is unbounded. In that case, when searching for the maximum
 *    factors, result->drift= INFINITY; result->offset= -INFINITY. When
 *    searching for the minimum factors, it is the opposite. It is not
 *    possible to improve the situation with this data.
 * 2) no line can be above the upper hull and below the lower hull. This is
 *    because the hulls intersect each other or are reversed. This means that
 *    an assertion was false. Most probably, the clocks are not linear. It is
 *    possible to repeat the search with another algorithm that will find a
 *    "best effort" approximation. See calculateFactorsApprox().
 *
 * Args:
 *   cu:           the upper half-convex hull, the line must pass above this
 *                 and touch it in one point
 *   cl:           the lower half-convex hull, the line must pass below this
 *                 and touch it in one point
 *   lineType:     search for minimum or maximum factors
 *
 * Returns:
 *   If a result is found, a struct Factors is allocated, filed with the
 *   result and returned
 *   NULL otherwise, degenerate case 2 is in effect
 */
static Factors* calculateFactorsExact(GQueue* const cu, GQueue* const cl, const
	LineType lineType)
{
	GQueue* c1, * c2;
	unsigned int i1, i2;
	Point* p1, * p2;
	double inversionFactor;
	Factors* result;

	g_debug("calculateFactorsExact(cu= %p, cl= %p, %s)", cu, cl, lineType ==
		MINIMUM ? "MINIMUM" : "MAXIMUM");

	if (lineType == MINIMUM)
	{
		c1= cl;
		c2= cu;
		inversionFactor= -1.;
	}
	else
	{
		c1= cu;
		c2= cl;
		inversionFactor= 1.;
	}

	i1= 0;
	i2= c2->length - 1;

	// Check for degenerate case 1
	if (c1->length == 0 || c2->length == 0 || ((Point*) g_queue_peek_nth(c1,
				i1))->x >= ((Point*) g_queue_peek_nth(c2, i2))->x)
	{
		result= malloc(sizeof(Factors));
		if (lineType == MINIMUM)
		{
			result->drift= -INFINITY;
			result->offset= INFINITY;
		}
		else
		{
			result->drift= INFINITY;
			result->offset= -INFINITY;
		}

		return result;
	}

	do
	{
		while
		(
			(int) i2 - 1 > 0
			&& crossProductK(
				g_queue_peek_nth(c1, i1),
				g_queue_peek_nth(c2, i2),
				g_queue_peek_nth(c1, i1),
				g_queue_peek_nth(c2, i2 - 1)) * inversionFactor < 0.
		)
		{
			if (((Point*) g_queue_peek_nth(c1, i1))->x
				< ((Point*) g_queue_peek_nth(c2, i2 - 1))->x)
			{
				i2--;
			}
			else
			{
				// Degenerate case 2
				return NULL;
			}
		}
		while
		(
			i1 + 1 < c1->length - 1
			&& crossProductK(
				g_queue_peek_nth(c1, i1),
				g_queue_peek_nth(c2, i2),
				g_queue_peek_nth(c1, i1 + 1),
				g_queue_peek_nth(c2, i2)) * inversionFactor < 0.
		)
		{
			if (((Point*) g_queue_peek_nth(c1, i1 + 1))->x
					< ((Point*) g_queue_peek_nth(c2, i2))->x)
			{
				i1++;
			}
			else
			{
				// Degenerate case 2
				return NULL;
			}
		}
	} while
	(
		(int) i2 - 1 > 0
		&& crossProductK(
			g_queue_peek_nth(c1, i1),
			g_queue_peek_nth(c2, i2),
			g_queue_peek_nth(c1, i1),
			g_queue_peek_nth(c2, i2 - 1)) * inversionFactor < 0.
	);

	p1= g_queue_peek_nth(c1, i1);
	p2= g_queue_peek_nth(c2, i2);

	g_debug("Resulting points are: c1[i1]: x= %llu y= %llu c2[i2]: x= %llu y= %llu",
		p1->x, p1->y, p2->x, p2->y);

	result= malloc(sizeof(Factors));
	result->drift= slope(p1, p2);
	result->offset= intercept(p1, p2);

	g_debug("Resulting factors are: drift= %g offset= %g", result->drift, result->offset);

	return result;
}


/*
 * Analyze the convex hulls to determine approximate synchronization factors
 * between one pair of trace when there is no line that can fit in the
 * corridor separating them.
 *
 * This implements the algorithm in [Ashton, P.: Algorithms for Off-line Clock
 * Synchronisation, University of Canterbury, December 1995, 26] Section 4.2.2
 * p.7
 *
 * For each point p1 in cr
 *   For each point p2 in cs
 *     errorMin= 0
 *     Calculate the line paramaters
 *     For each point p3 in each convex hull
 *       If p3 is on the wrong side of the line
 *         error+= distance
 *     If error < errorMin
 *       Update results
 *
 * Args:
 *   cr:           the upper half-convex hull
 *   cs:           the lower half-convex hull
 *   result:       a pointer to the pre-allocated struct where the results
 *                 will be stored
 */
static void calculateFactorsFallback(GQueue* const cr, GQueue* const cs,
	FactorsCHull* const result)
{
	unsigned int i, j, k;
	double errorMin;
	Factors* approx;

	errorMin= INFINITY;
	approx= malloc(sizeof(Factors));

	for (i= 0; i < cs->length; i++)
	{
		for (j= 0; j < cr->length; j++)
		{
			double error;
			Point p1, p2;

			error= 0.;

			if (((Point*) g_queue_peek_nth(cs, i))->x < ((Point*)g_queue_peek_nth(cr, j))->x)
			{
				p1= *(Point*)g_queue_peek_nth(cs, i);
				p2= *(Point*)g_queue_peek_nth(cr, j);
			}
			else
			{
				p1= *(Point*)g_queue_peek_nth(cr, j);
				p2= *(Point*)g_queue_peek_nth(cs, i);
			}

			// The lower hull should be above the point
			for (k= 0; k < cs->length; k++)
			{
				if (jointCmp(&p1, &p2, g_queue_peek_nth(cs, k)) < 0.)
				{
					error+= verticalDistance(&p1, &p2, g_queue_peek_nth(cs, k));
				}
			}

			// The upper hull should be below the point
			for (k= 0; k < cr->length; k++)
			{
				if (jointCmp(&p1, &p2, g_queue_peek_nth(cr, k)) > 0.)
				{
					error+= verticalDistance(&p1, &p2, g_queue_peek_nth(cr, k));
				}
			}

			if (error < errorMin)
			{
				g_debug("Fallback: i= %u j= %u is a better match (error= %g)", i, j, error);
				approx->drift= slope(&p1, &p2);
				approx->offset= intercept(&p1, &p2);
				errorMin= error;
			}
		}
	}

	result->approx= approx;
	result->accuracy= errorMin;
}


/*
 * Calculate the vertical distance between a line and a point
 *
 * Args:
 *   p1, p2:       Two points defining the line
 *   point:        a point
 *
 * Return:
 *   the vertical distance
 */
static double verticalDistance(Point* p1, Point* p2, Point* const point)
{
	return fabs(slope(p1, p2) * point->x + intercept(p1, p2) - point->y);
}


/*
 * Calculate the slope between two points
 *
 * Args:
 *   p1, p2        the two points
 *
 * Returns:
 *   the slope
 */
static double slope(const Point* const p1, const Point* const p2)
{
	return ((double) p2->y - p1->y) / (p2->x - p1->x);
}


/* Calculate the y-intercept of a line that passes by two points
 *
 * Args:
 *   p1, p2        the two points
 *
 * Returns:
 *   the y-intercept
 */
static double intercept(const Point* const p1, const Point* const p2)
{
	return ((double) p2->y * p1->x - (double) p1->y * p2->x) / ((double) p1->x - p2->x);
}


/*
 * Calculate a resulting offset and drift for each trace.
 *
 * Traces are assembled in groups. A group is an "island" of nodes/traces that
 * exchanged messages. A reference is determined for each group by using a
 * shortest path search based on the accuracy of the approximation. This also
 * forms a tree of the best way to relate each node's clock to the reference's
 * based on the accuracy. Sometimes it may be necessary or advantageous to
 * propagate the factors through intermediary clocks. Resulting factors for
 * each trace are determined based on this tree.
 *
 * This part was not the focus of my research. The algorithm used here is
 * inexact in some ways:
 * 1) The reference used may not actually be the best one to use. This is
 *    because the accuracy is not corrected based on the drift during the
 *    shortest path search.
 * 2) The min and max factors are not propagated and are no longer valid.
 * 3) Approximations of different types (MIDDLE and FALLBACK) are compared
 *    together. The "accuracy" parameters of these have different meanings and
 *    are not readily comparable.
 *
 * Nevertheless, the result is satisfactory. You just can't tell "how much" it
 * is.
 *
 * Two alternative (and subtly different) ways of propagating factors to
 * preserve min and max bondaries have been proposed, see:
 * [Duda, A., Harrus, G., Haddad, Y., and Bernard, G.: Estimating global time
 * in distributed systems, Proc. 7th Int. Conf. on Distributed Computing
 * Systems, Berlin, volume 18, 1987] p.304
 *
 * [Jezequel, J.M., and Jard, C.: Building a global clock for observing
 * computations in distributed memory parallel computers, Concurrency:
 * Practice and Experience 8(1), volume 8, John Wiley & Sons, Ltd Chichester,
 * 1996, 32] Section 5; which is mostly the same as
 * [Jezequel, J.M.: Building a global time on parallel machines, Proceedings
 * of the 3rd International Workshop on Distributed Algorithms, LNCS, volume
 * 392, 136–147, 1989] Section 5
 *
 * Args:
 *   syncState:    container for synchronization data.
 *   allFactors:   offset and drift between each pair of traces
 *
 * Returns:
 *   Factors[traceNb] synchronization factors for each trace
 */
static GArray* reduceFactors(SyncState* const syncState, FactorsCHull** const
	allFactors)
{
	GArray* factors;
	double** distances;
	unsigned int** predecessors;
	double* distanceSums;
	unsigned int* references;
	unsigned int i, j;

	// Solve the all-pairs shortest path problem using the Floyd-Warshall
	// algorithm
	floydWarshall(syncState, allFactors, &distances, &predecessors);

	/* Find the reference for each node
	 *
	 * First calculate, for each node, the sum of the distances to each other
	 * node it can reach.
	 *
	 * Then, go through each "island" of traces to find the trace that has the
	 * lowest distance sum. Assign this trace as the reference to each trace
	 * of the island.
	 */
	distanceSums= malloc(syncState->traceNb * sizeof(double));
	for (i= 0; i < syncState->traceNb; i++)
	{
		distanceSums[i]= 0.;
		for (j= 0; j < syncState->traceNb; j++)
		{
			distanceSums[i]+= distances[i][j];
		}
	}

	references= malloc(syncState->traceNb * sizeof(unsigned int));
	for (i= 0; i < syncState->traceNb; i++)
	{
		references[i]= UINT_MAX;
	}
	for (i= 0; i < syncState->traceNb; i++)
	{
		if (references[i] == UINT_MAX)
		{
			unsigned int reference;
			double distanceSumMin;

			// A node is its own reference by default
			reference= i;
			distanceSumMin= INFINITY;
			for (j= 0; j < syncState->traceNb; j++)
			{
				if (distances[i][j] != INFINITY && distanceSums[j] <
					distanceSumMin)
				{
					reference= j;
					distanceSumMin= distanceSums[j];
				}
			}
			for (j= 0; j < syncState->traceNb; j++)
			{
				if (distances[i][j] != INFINITY)
				{
					references[j]= reference;
				}
			}
		}
	}

	for (i= 0; i < syncState->traceNb; i++)
	{
		free(distances[i]);
	}
	free(distances);
	free(distanceSums);

	/* For each trace, calculate the factors based on their corresponding
	 * tree. The tree is rooted at the reference and the shortest path to each
	 * other nodes are the branches.
	 */
	factors= g_array_sized_new(FALSE, FALSE, sizeof(Factors),
		syncState->traceNb);
	g_array_set_size(factors, syncState->traceNb);
	for (i= 0; i < syncState->traceNb; i++)
	{
		getFactors(allFactors, predecessors, references, i, &g_array_index(factors,
				Factors, i));
	}

	for (i= 0; i < syncState->traceNb; i++)
	{
		free(predecessors[i]);
	}
	free(predecessors);
	free(references);

	return factors;
}


/*
 * Perform an all-source shortest path search using the Floyd-Warshall
 * algorithm.
 *
 * The algorithm is implemented accoding to the description here:
 * http://web.mit.edu/urban_or_book/www/book/chapter6/6.2.2.html
 *
 * Args:
 *   syncState:    container for synchronization data.
 *   allFactors:   offset and drift between each pair of traces
 *   distances:    resulting matrix of the length of the shortest path between
 *                 two nodes. If there is no path between two nodes, the
 *                 length is INFINITY
 *   predecessors: resulting matrix of each node's predecessor on the shortest
 *                 path between two nodes
 */
static void floydWarshall(SyncState* const syncState, FactorsCHull** const
	allFactors, double*** const distances, unsigned int*** const
	predecessors)
{
	unsigned int i, j, k;

	// Setup initial conditions
	*distances= malloc(syncState->traceNb * sizeof(double*));
	*predecessors= malloc(syncState->traceNb * sizeof(unsigned int*));
	for (i= 0; i < syncState->traceNb; i++)
	{
		(*distances)[i]= malloc(syncState->traceNb * sizeof(double));
		for (j= 0; j < syncState->traceNb; j++)
		{
			if (i == j)
			{
				g_assert(allFactors[i][j].type == EXACT);

				(*distances)[i][j]= 0.;
			}
			else
			{
				unsigned int row, col;

				if (i > j)
				{
					row= i;
					col= j;
				}
				else if (i < j)
				{
					row= j;
					col= i;
				}

				if (allFactors[row][col].type == MIDDLE ||
					allFactors[row][col].type == FALLBACK)
				{
					(*distances)[i][j]= allFactors[row][col].accuracy;
				}
				else if (allFactors[row][col].type == INCOMPLETE ||
					allFactors[row][col].type == SCREWED ||
					allFactors[row][col].type == ABSENT)
				{
					(*distances)[i][j]= INFINITY;
				}
				else
				{
					g_assert_not_reached();
				}
			}
		}

		(*predecessors)[i]= malloc(syncState->traceNb * sizeof(unsigned int));
		for (j= 0; j < syncState->traceNb; j++)
		{
			if (i != j)
			{
				(*predecessors)[i][j]= i;
			}
			else
			{
				(*predecessors)[i][j]= UINT_MAX;
			}
		}
	}

	// Run the iterations
	for (k= 0; k < syncState->traceNb; k++)
	{
		for (i= 0; i < syncState->traceNb; i++)
		{
			for (j= 0; j < syncState->traceNb; j++)
			{
				double distanceMin;

				distanceMin= MIN((*distances)[i][j], (*distances)[i][k] +
					(*distances)[k][j]);

				if (distanceMin != (*distances)[i][j])
				{
					(*predecessors)[i][j]= (*predecessors)[k][j];
				}

				(*distances)[i][j]= distanceMin;
			}
		}
	}
}


/*
 * Cummulate the time correction factors to convert a node's time to its
 * reference's time.
 * This function recursively calls itself until it reaches the reference node.
 *
 * Args:
 *   allFactors:   offset and drift between each pair of traces
 *   predecessors: matrix of each node's predecessor on the shortest
 *                 path between two nodes
 *   references:   reference node for each node
 *   traceNum:     node for which to find the factors
 *   factors:      resulting factors
 */
static void getFactors(FactorsCHull** const allFactors, unsigned int** const
	predecessors, unsigned int* const references, const unsigned int traceNum,
	Factors* const factors)
{
	unsigned int reference;

	reference= references[traceNum];

	if (reference == traceNum)
	{
		factors->offset= 0.;
		factors->drift= 1.;
	}
	else
	{
		Factors previousVertexFactors;

		getFactors(allFactors, predecessors, references,
			predecessors[reference][traceNum], &previousVertexFactors);

		// convertir de traceNum à reference

		// allFactors convertit de col à row

		if (reference > traceNum)
		{
			factors->offset= previousVertexFactors.drift *
				allFactors[reference][traceNum].approx->offset +
				previousVertexFactors.offset;
			factors->drift= previousVertexFactors.drift *
				allFactors[reference][traceNum].approx->drift;
		}
		else
		{
			factors->offset= previousVertexFactors.drift * (-1. *
				allFactors[traceNum][reference].approx->offset /
				allFactors[traceNum][reference].approx->drift) +
				previousVertexFactors.offset;
			factors->drift= previousVertexFactors.drift * (1. /
				allFactors[traceNum][reference].approx->drift);
		}
	}
}


/*
 * Write the analysis-specific graph lines in the gnuplot script.
 *
 * Args:
 *   syncState:    container for synchronization data
 *   i:            first trace number
 *   j:            second trace number, garanteed to be larger than i
 */
void writeAnalysisGraphsPlotsCHull(SyncState* const syncState, const unsigned
	int i, const unsigned int j)
{
	AnalysisDataCHull* analysisData;
	FactorsCHull* factorsCHull;

	analysisData= (AnalysisDataCHull*) syncState->analysisData;

	fprintf(syncState->graphsStream,
		"\t\"analysis_chull-%1$03d_to_%2$03d.data\" "
			"title \"Lower half-hull\" with linespoints "
			"linecolor rgb \"#015a01\" linetype 4 pointtype 8 pointsize 0.8, \\\n"
		"\t\"analysis_chull-%2$03d_to_%1$03d.data\" "
			"title \"Upper half-hull\" with linespoints "
			"linecolor rgb \"#003366\" linetype 4 pointtype 10 pointsize 0.8, \\\n",
		i, j);

	factorsCHull= &analysisData->graphsData->allFactors[j][i];
	if (factorsCHull->type == EXACT)
	{
		fprintf(syncState->graphsStream,
			"\t%7g + %7g * x "
				"title \"Exact conversion\" with lines "
				"linecolor rgb \"black\" linetype 1, \\\n",
				factorsCHull->approx->offset, factorsCHull->approx->drift);
	}
	else if (factorsCHull->type == MIDDLE)
	{
		fprintf(syncState->graphsStream,
			"\t%.2f + %.10f * x "
				"title \"Min conversion\" with lines "
				"linecolor rgb \"black\" linetype 5, \\\n",
				factorsCHull->min->offset, factorsCHull->min->drift);
		fprintf(syncState->graphsStream,
			"\t%.2f + %.10f * x "
				"title \"Max conversion\" with lines "
				"linecolor rgb \"black\" linetype 8, \\\n",
				factorsCHull->max->offset, factorsCHull->max->drift);
		fprintf(syncState->graphsStream,
			"\t%.2f + %.10f * x "
				"title \"Middle conversion\" with lines "
				"linecolor rgb \"black\" linetype 1, \\\n",
				factorsCHull->approx->offset, factorsCHull->approx->drift);
	}
	else if (factorsCHull->type == FALLBACK)
	{
		fprintf(syncState->graphsStream,
			"\t%.2f + %.10f * x "
				"title \"Fallback conversion\" with lines "
				"linecolor rgb \"gray60\" linetype 1, \\\n",
				factorsCHull->approx->offset, factorsCHull->approx->drift);
	}
	else if (factorsCHull->type == INCOMPLETE)
	{
		if (factorsCHull->min->drift != -INFINITY)
		{
			fprintf(syncState->graphsStream,
				"\t%.2f + %.10f * x "
					"title \"Min conversion\" with lines "
					"linecolor rgb \"black\" linetype 5, \\\n",
					factorsCHull->min->offset, factorsCHull->min->drift);
		}

		if (factorsCHull->max->drift != INFINITY)
		{
			fprintf(syncState->graphsStream,
				"\t%.2f + %.10f * x "
					"title \"Max conversion\" with lines "
					"linecolor rgb \"black\" linetype 8, \\\n",
					factorsCHull->max->offset, factorsCHull->max->drift);
		}
	}
	else if (factorsCHull->type == SCREWED)
	{
		if (factorsCHull->min != NULL && factorsCHull->min->drift != -INFINITY)
		{
			fprintf(syncState->graphsStream,
				"\t%.2f + %.10f * x "
					"title \"Min conversion\" with lines "
					"linecolor rgb \"black\" linetype 5, \\\n",
					factorsCHull->min->offset, factorsCHull->min->drift);
		}

		if (factorsCHull->max != NULL && factorsCHull->max->drift != INFINITY)
		{
			fprintf(syncState->graphsStream,
				"\t%.2f + %.10f * x "
					"title \"Max conversion\" with lines "
					"linecolor rgb \"black\" linetype 8, \\\n",
					factorsCHull->max->offset, factorsCHull->max->drift);
		}
	}
	else if (factorsCHull->type == ABSENT)
	{
	}
	else
	{
		g_assert_not_reached();
	}
}
