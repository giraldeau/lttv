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
#define _ISOC99_SOURCE

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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

#ifdef HAVE_LIBGLPK
struct LPAddRowInfo
{
	glp_prob* lp;
	int boundType;
	GArray* iArray, * jArray, * aArray;
};
#endif


// Functions common to all analysis modules
static void initAnalysisCHull(SyncState* const syncState);
static void destroyAnalysisCHull(SyncState* const syncState);

static void analyzeMessageCHull(SyncState* const syncState, Message* const
	message);
static AllFactors* finalizeAnalysisCHull(SyncState* const syncState);
static void printAnalysisStatsCHull(SyncState* const syncState);
static void writeAnalysisTraceTraceForePlotsCHull(SyncState* const syncState,
	const unsigned int i, const unsigned int j);

// Functions specific to this module
static void openGraphFiles(SyncState* const syncState);
static void closeGraphFiles(SyncState* const syncState);
static void writeGraphFiles(SyncState* const syncState);
static void gfDumpHullToFile(gpointer data, gpointer userData);

AllFactors* calculateAllFactors(struct _SyncState* const syncState);
void calculateFactorsMiddle(PairFactors* const factors);
static Factors* calculateFactorsExact(GQueue* const cu, GQueue* const cl, const
	LineType lineType) __attribute__((pure));
static void calculateFactorsFallback(GQueue* const cr, GQueue* const cs,
	PairFactors* const result);
static void grahamScan(GQueue* const hull, Point* const newPoint, const
	HullType type);
static int jointCmp(const Point* const p1, const Point* const p2, const Point*
	const p3) __attribute__((pure));
static double crossProductK(const Point const* p1, const Point const* p2,
	const Point const* p3, const Point const* p4) __attribute__((pure));
static double slope(const Point* const p1, const Point* const p2)
	__attribute__((pure));
static double intercept(const Point* const p1, const Point* const p2)
	__attribute__((pure));
static double verticalDistance(Point* p1, Point* p2, Point* const point)
	__attribute__((pure));

static void gfPointDestroy(gpointer data, gpointer userData);

// The next group of functions is only needed when computing synchronization
// accuracy.
#ifdef HAVE_LIBGLPK
static AllFactors* finalizeAnalysisCHullLP(SyncState* const syncState);
static void writeAnalysisTraceTimeBackPlotsCHull(SyncState* const syncState,
	const unsigned int i, const unsigned int j);
static void writeAnalysisTraceTimeForePlotsCHull(SyncState* const syncState,
	const unsigned int i, const unsigned int j);
static void writeAnalysisTraceTraceBackPlotsCHull(SyncState* const syncState,
	const unsigned int i, const unsigned int j);

static glp_prob* lpCreateProblem(GQueue* const lowerHull, GQueue* const
	upperHull);
static void gfLPAddRow(gpointer data, gpointer user_data);
static Factors* calculateFactorsLP(glp_prob* const lp, const int direction);
static void calculateCompleteFactorsLP(glp_prob* const lp, PairFactors*
	factors);
void timeCorrectionLP(glp_prob* const lp, const PairFactors* const lpFactors,
	const uint64_t time, CorrectedTime* const correctedTime);

static void gfAddAbsiscaToArray(gpointer data, gpointer user_data);
static gint gcfCompareUint64(gconstpointer a, gconstpointer b);
#else
static inline AllFactors* finalizeAnalysisCHullLP(SyncState* const syncState)
{
	return NULL;
}
#endif



static AnalysisModule analysisModuleCHull= {
	.name= "chull",
	.initAnalysis= &initAnalysisCHull,
	.destroyAnalysis= &destroyAnalysisCHull,
	.analyzeMessage= &analyzeMessageCHull,
	.finalizeAnalysis= &finalizeAnalysisCHull,
	.printAnalysisStats= &printAnalysisStatsCHull,
	.graphFunctions= {
#ifdef HAVE_LIBGLPK
		.writeTraceTimeBackPlots= &writeAnalysisTraceTimeBackPlotsCHull,
		.writeTraceTimeForePlots= &writeAnalysisTraceTimeForePlotsCHull,
		.writeTraceTraceBackPlots= &writeAnalysisTraceTraceBackPlotsCHull,
#endif
		.writeTraceTraceForePlots= &writeAnalysisTraceTraceForePlotsCHull,
	}
};


/*
 * Analysis module registering function
 */
void registerAnalysisCHull()
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
#ifdef HAVE_LIBGLPK
	analysisData->lps= NULL;
#endif

	if (syncState->stats)
	{
		analysisData->stats= calloc(1, sizeof(AnalysisStatsCHull));
	}

	if (syncState->graphsStream)
	{
		analysisData->graphsData= calloc(1, sizeof(AnalysisGraphsDataCHull));
		openGraphFiles(syncState);
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
	fprintf((FILE*) userData, "%20" PRIu64 " %20" PRIu64 "\n", point->x, point->y);
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
			g_queue_foreach(analysisData->hullArray[i][j], gfPointDestroy,
				NULL);
			g_queue_free(analysisData->hullArray[i][j]);
		}
		free(analysisData->hullArray[i]);
	}
	free(analysisData->hullArray);

#ifdef HAVE_LIBGLPK
	if (analysisData->lps != NULL)
	{
		for (i= 0; i < syncState->traceNb; i++)
		{
			unsigned int j;

			for (j= 0; j < i; j++)
			{
				// There seems to be a memory leak in glpk, valgrind reports a
				// loss (reachable) even if the problem is deleted
				glp_delete_prob(analysisData->lps[i][j]);
			}
			free(analysisData->lps[i]);
		}
		free(analysisData->lps);
	}
#endif

	if (syncState->stats)
	{
		freeAllFactors(analysisData->stats->allFactors, syncState->traceNb);
		freeAllFactors(analysisData->stats->geoFactors, syncState->traceNb);

#ifdef HAVE_LIBGLPK
		freeAllFactors(analysisData->stats->lpFactors, syncState->traceNb);
#endif

		free(analysisData->stats);
	}

	if (syncState->graphsStream)
	{
		AnalysisGraphsDataCHull* graphs= analysisData->graphsData;

		if (graphs->hullPoints != NULL)
		{
			closeGraphFiles(syncState);
		}

		freeAllFactors(graphs->allFactors, syncState->traceNb);

#ifdef HAVE_LIBGLPK
		freeAllFactors(graphs->lpFactors, syncState->traceNb);
#endif

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
		g_debug("Reception point hullArray[%lu][%lu] "
			"x= inE->time= %" PRIu64 " y= outE->time= %" PRIu64,
			message->inE->traceNum, message->outE->traceNum, newPoint->x,
			newPoint->y);
	}
	else
	{
		// CA is outE->traceNum
		newPoint->x= message->outE->cpuTime;
		newPoint->y= message->inE->cpuTime;
		hullType= LOWER;
		g_debug("Send point hullArray[%lu][%lu] "
			"x= inE->time= %" PRIu64 " y= outE->time= %" PRIu64,
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
 *   AllFactors*   synchronization factors for each trace pair, the caller is
 *   responsible for freeing the structure
 */
static AllFactors* finalizeAnalysisCHull(SyncState* const syncState)
{
	AnalysisDataCHull* analysisData;
	AllFactors* geoFactors, * lpFactors;

	analysisData= (AnalysisDataCHull*) syncState->analysisData;

	if (syncState->graphsStream && analysisData->graphsData->hullPoints != NULL)
	{
		writeGraphFiles(syncState);
		closeGraphFiles(syncState);
	}

	geoFactors= calculateAllFactors(syncState);
	lpFactors= finalizeAnalysisCHullLP(syncState);

	if (syncState->stats)
	{
		geoFactors->refCount++;
		analysisData->stats->geoFactors= geoFactors;

		if (lpFactors != NULL)
		{
			lpFactors->refCount++;
			analysisData->stats->allFactors= lpFactors;
		}
		else
		{
			geoFactors->refCount++;
			analysisData->stats->allFactors= geoFactors;
		}
	}

	if (syncState->graphsStream)
	{
		if (lpFactors != NULL)
		{
			lpFactors->refCount++;
			analysisData->graphsData->allFactors= lpFactors;
		}
		else
		{
			geoFactors->refCount++;
			analysisData->graphsData->allFactors= geoFactors;
		}
	}

	if (lpFactors != NULL)
	{
		freeAllFactors(geoFactors, syncState->traceNb);
		return lpFactors;
	}
	else
	{
		freeAllFactors(lpFactors, syncState->traceNb);
		return geoFactors;
	}
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
			PairFactors* factorsCHull;

			factorsCHull= &analysisData->stats->allFactors->pairFactors[j][i];
			printf("\t\t%3d - %-3d: %s", i, j,
				approxNames[factorsCHull->type]);

			if (factorsCHull->type == EXACT)
			{
				printf("      a0= % 7g a1= 1 %c %7g\n",
					factorsCHull->approx->offset,
					factorsCHull->approx->drift < 0. ? '-' : '+',
					fabs(factorsCHull->approx->drift));
			}
			else if (factorsCHull->type == ACCURATE)
			{
				printf("\n\t\t                      a0: % 7g to % 7g (delta= %7g)\n",
					factorsCHull->max->offset, factorsCHull->min->offset,
					factorsCHull->min->offset - factorsCHull->max->offset);
				printf("\t\t                      a1: 1 %+7g to %+7g (delta= %7g)\n",
					factorsCHull->min->drift - 1., factorsCHull->max->drift -
					1., factorsCHull->max->drift - factorsCHull->min->drift);
			}
			else if (factorsCHull->type == APPROXIMATE)
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
			else if (factorsCHull->type == FAIL)
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

#ifdef HAVE_LIBGLPK
	printf("\tFactor comparison:\n"
		"\t\tTrace pair  Factors type  Differences (lp - chull)\n"
		"\t\t                          a0                    a1\n"
		"\t\t                          Min        Max        Min        Max\n");

	for (i= 0; i < syncState->traceNb; i++)
	{
		for (j= 0; j < i; j++)
		{
			PairFactors* geoFactors=
				&analysisData->stats->geoFactors->pairFactors[i][j];
			PairFactors* lpFactors=
				&analysisData->stats->lpFactors->pairFactors[i][j];

			printf("\t\t%3d - %-3d   ", i, j);
			if (lpFactors->type == geoFactors->type)
			{
				if (lpFactors->type == ACCURATE)
				{
					printf("%-13s %-10.4g %-10.4g %-10.4g %.4g\n",
						approxNames[lpFactors->type],
						lpFactors->min->offset - geoFactors->min->offset,
						lpFactors->max->offset - geoFactors->max->offset,
						lpFactors->min->drift - geoFactors->min->drift,
						lpFactors->max->drift - geoFactors->max->drift);
				}
				else if (lpFactors->type == ABSENT)
				{
					printf("%s\n", approxNames[lpFactors->type]);
				}
			}
			else
			{
				printf("Different! %s and %s\n", approxNames[lpFactors->type],
					approxNames[geoFactors->type]);
			}
		}
	}
#endif
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
	g_debug("crossProductK(p1= (%" PRIu64 ", %" PRIu64 "), "
		"p2= (%" PRIu64 ", %" PRIu64 "), p1= (%" PRIu64 ", %" PRIu64 "), "
		"p3= (%" PRIu64 ", %" PRIu64 "))= %g",
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
 * Analyze the convex hulls to determine the synchronization factors between
 * each pair of trace.
 *
 * Args:
 *   syncState     container for synchronization data.
 *
 * Returns:
 *   AllFactors*, see the documentation for the member geoFactors of
 *   AnalysisStatsCHull.
 */
AllFactors* calculateAllFactors(SyncState* const syncState)
{
	unsigned int traceNumA, traceNumB;
	AllFactors* geoFactors;
	AnalysisDataCHull* analysisData;

	analysisData= (AnalysisDataCHull*) syncState->analysisData;

	// Allocate geoFactors and calculate min and max
	geoFactors= createAllFactors(syncState->traceNb);
	for (traceNumA= 0; traceNumA < syncState->traceNb; traceNumA++)
	{
		for (traceNumB= 0; traceNumB < traceNumA; traceNumB++)
		{
			unsigned int i;
			GQueue* cs, * cr;
			const struct
			{
				LineType lineType;
				size_t factorsOffset;
			} loopValues[]= {
				{MINIMUM, offsetof(PairFactors, min)},
				{MAXIMUM, offsetof(PairFactors, max)}
			};

			cr= analysisData->hullArray[traceNumB][traceNumA];
			cs= analysisData->hullArray[traceNumA][traceNumB];

			for (i= 0; i < sizeof(loopValues) / sizeof(*loopValues); i++)
			{
				g_debug("geoFactors[%u][%u].%s = calculateFactorsExact(cr= "
					"hullArray[%u][%u], cs= hullArray[%u][%u], %s)",
					traceNumA, traceNumB, loopValues[i].factorsOffset ==
					offsetof(PairFactors, min) ? "min" : "max", traceNumB,
					traceNumA, traceNumA, traceNumB, loopValues[i].lineType ==
					MINIMUM ? "MINIMUM" : "MAXIMUM");
				*((Factors**) ((void*)
						&geoFactors->pairFactors[traceNumA][traceNumB] +
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
			PairFactors* factorsCHull;

			factorsCHull= &geoFactors->pairFactors[traceNumA][traceNumB];
			if (factorsCHull->min == NULL && factorsCHull->max == NULL)
			{
				factorsCHull->type= APPROXIMATE;
				calculateFactorsFallback(analysisData->hullArray[traceNumB][traceNumA],
					analysisData->hullArray[traceNumA][traceNumB],
					&geoFactors->pairFactors[traceNumA][traceNumB]);
			}
			else if (factorsCHull->min != NULL && factorsCHull->max != NULL)
			{
				if (factorsCHull->min->drift != -INFINITY &&
					factorsCHull->max->drift != INFINITY)
				{
					factorsCHull->type= ACCURATE;
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
				factorsCHull->type= FAIL;
			}
		}
	}

	return geoFactors;
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
void calculateFactorsMiddle(PairFactors* const factors)
{
	double amin, amax, bmin, bmax, bhat;

	amin= factors->max->offset;
	amax= factors->min->offset;
	bmin= factors->min->drift;
	bmax= factors->max->drift;

	g_assert_cmpfloat(bmax, >=, bmin);

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

	g_debug("Resulting points are: c1[i1]: x= %" PRIu64 " y= %" PRIu64
		" c2[i2]: x= %" PRIu64 " y= %" PRIu64 "", p1->x, p1->y, p2->x, p2->y);

	result= malloc(sizeof(Factors));
	result->drift= slope(p1, p2);
	result->offset= intercept(p1, p2);

	g_debug("Resulting factors are: drift= %g offset= %g", result->drift,
		result->offset);

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
	PairFactors* const result)
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
 * Write the analysis-specific graph lines in the gnuplot script.
 *
 * Args:
 *   syncState:    container for synchronization data
 *   i:            first trace number
 *   j:            second trace number, garanteed to be larger than i
 */
void writeAnalysisTraceTraceForePlotsCHull(SyncState* const syncState, const unsigned
	int i, const unsigned int j)
{
	AnalysisDataCHull* analysisData;
	PairFactors* factorsCHull;

	analysisData= (AnalysisDataCHull*) syncState->analysisData;

	fprintf(syncState->graphsStream,
		"\t\"analysis_chull-%1$03d_to_%2$03d.data\" "
			"title \"Lower half-hull\" with linespoints "
			"linecolor rgb \"#015a01\" linetype 4 pointtype 8 pointsize 0.8, \\\n"
		"\t\"analysis_chull-%2$03d_to_%1$03d.data\" "
			"title \"Upper half-hull\" with linespoints "
			"linecolor rgb \"#003366\" linetype 4 pointtype 10 pointsize 0.8, \\\n",
		i, j);

	factorsCHull= &analysisData->graphsData->allFactors->pairFactors[j][i];
	if (factorsCHull->type == EXACT)
	{
		fprintf(syncState->graphsStream,
			"\t%7g + %7g * x "
				"title \"Exact conversion\" with lines "
				"linecolor rgb \"black\" linetype 1, \\\n",
				factorsCHull->approx->offset, factorsCHull->approx->drift);
	}
	else if (factorsCHull->type == ACCURATE)
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
	else if (factorsCHull->type == APPROXIMATE)
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
	else if (factorsCHull->type == FAIL)
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


#ifdef HAVE_LIBGLPK
/*
 * Create the linear programming problem containing the constraints defined by
 * two half-hulls. The objective function and optimization directions are not
 * written.
 *
 * Args:
 *   syncState:    container for synchronization data
 *   i:            first trace number
 *   j:            second trace number, garanteed to be larger than i
 * Returns:
 *   A new glp_prob*, this problem must be freed by the caller with
 *   glp_delete_prob()
 */
static glp_prob* lpCreateProblem(GQueue* const lowerHull, GQueue* const
	upperHull)
{
	unsigned int it;
	const int zero= 0;
	const double zeroD= 0.;
	glp_prob* lp= glp_create_prob();
	unsigned int hullPointNb= g_queue_get_length(lowerHull) +
		g_queue_get_length(upperHull);
	GArray* iArray= g_array_sized_new(FALSE, FALSE, sizeof(int), hullPointNb +
		1);
	GArray* jArray= g_array_sized_new(FALSE, FALSE, sizeof(int), hullPointNb +
		1);
	GArray* aArray= g_array_sized_new(FALSE, FALSE, sizeof(double),
		hullPointNb + 1);
	struct {
		GQueue* hull;
		struct LPAddRowInfo rowInfo;
	} loopValues[2]= {
		{lowerHull, {lp, GLP_UP, iArray, jArray, aArray}},
		{upperHull, {lp, GLP_LO, iArray, jArray, aArray}},
	};

	// Create the LP problem
	glp_term_out(GLP_OFF);
	if (hullPointNb > 0)
	{
		glp_add_rows(lp, hullPointNb);
	}
	glp_add_cols(lp, 2);

	glp_set_col_name(lp, 1, "a0");
	glp_set_col_bnds(lp, 1, GLP_FR, 0., 0.);
	glp_set_col_name(lp, 2, "a1");
	glp_set_col_bnds(lp, 2, GLP_LO, 0., 0.);

	// Add row constraints
	g_array_append_val(iArray, zero);
	g_array_append_val(jArray, zero);
	g_array_append_val(aArray, zeroD);

	for (it= 0; it < sizeof(loopValues) / sizeof(*loopValues); it++)
	{
		g_queue_foreach(loopValues[it].hull, &gfLPAddRow,
			&loopValues[it].rowInfo);
	}

	g_assert_cmpuint(iArray->len, ==, jArray->len);
	g_assert_cmpuint(jArray->len, ==, aArray->len);
	g_assert_cmpuint(aArray->len - 1, ==, hullPointNb * 2);

	glp_load_matrix(lp, aArray->len - 1, &g_array_index(iArray, int, 0),
		&g_array_index(jArray, int, 0), &g_array_index(aArray, double, 0));

	glp_scale_prob(lp, GLP_SF_AUTO);

	g_array_free(iArray, TRUE);
	g_array_free(jArray, TRUE);
	g_array_free(aArray, TRUE);

	return lp;
}


/*
 * A GFunc for g_queue_foreach(). Add constraints and bounds for one row.
 *
 * Args:
 *   data          Point*, synchronization point for which to add an LP row
 *                 (a constraint)
 *   user_data     LPAddRowInfo*
 */
static void gfLPAddRow(gpointer data, gpointer user_data)
{
	Point* p= data;
	struct LPAddRowInfo* rowInfo= user_data;
	int indexes[2];
	double constraints[2];

	indexes[0]= g_array_index(rowInfo->iArray, int, rowInfo->iArray->len - 1) + 1;
	indexes[1]= indexes[0];

	if (rowInfo->boundType == GLP_UP)
	{
		glp_set_row_bnds(rowInfo->lp, indexes[0], GLP_UP, 0., p->y);
	}
	else if (rowInfo->boundType == GLP_LO)
	{
		glp_set_row_bnds(rowInfo->lp, indexes[0], GLP_LO, p->y, 0.);
	}
	else
	{
		g_assert_not_reached();
	}

	g_array_append_vals(rowInfo->iArray, indexes, 2);
	indexes[0]= 1;
	indexes[1]= 2;
	g_array_append_vals(rowInfo->jArray, indexes, 2);
	constraints[0]= 1.;
	constraints[1]= p->x;
	g_array_append_vals(rowInfo->aArray, constraints, 2);
}


/*
 * Calculate min or max correction factors (as possible) using an LP problem.
 *
 * Args:
 *   lp:           A linear programming problem with constraints and bounds
 *                 initialized.
 *   direction:    The type of factors desired. Use GLP_MAX for max
 *                 approximation factors (a1, the drift or slope is the
 *                 largest) and GLP_MIN in the other case.
 *
 * Returns:
 *   If the calculation was successful, a new Factors struct. Otherwise, NULL.
 *   The calculation will fail if the hull assumptions are not respected.
 */
static Factors* calculateFactorsLP(glp_prob* const lp, const int direction)
{
	int retval, status;
	Factors* factors;

	glp_set_obj_coef(lp, 1, 0.);
	glp_set_obj_coef(lp, 2, 1.);

	glp_set_obj_dir(lp, direction);
	retval= glp_simplex(lp, NULL);
	status= glp_get_status(lp);

	if (retval == 0 && status == GLP_OPT)
	{
		factors= malloc(sizeof(Factors));
		factors->offset= glp_get_col_prim(lp, 1);
		factors->drift= glp_get_col_prim(lp, 2);
	}
	else
	{
		factors= NULL;
	}

	return factors;
}


/*
 * Calculate min, max and approx correction factors (as possible) using an LP
 * problem.
 *
 * Args:
 *   lp            A linear programming problem with constraints and bounds
 *                 initialized.
 *   factors       Resulting factors, must be preallocated
 */
static void calculateCompleteFactorsLP(glp_prob* const lp, PairFactors* factors)
{
	factors->min= calculateFactorsLP(lp, GLP_MIN);
	factors->max= calculateFactorsLP(lp, GLP_MAX);

	if (factors->min && factors->max)
	{
		factors->type= ACCURATE;
		calculateFactorsMiddle(factors);
	}
	else if (factors->min || factors->max)
	{
		factors->type= INCOMPLETE;
	}
	else
	{
		factors->type= ABSENT;
	}
}


/*
 * A GFunc for g_queue_foreach()
 *
 * Args:
 *   data          Point*, a convex hull point
 *   user_data     GArray*, an array of convex hull point absisca values, as
 *                 uint64_t
 */
static void gfAddAbsiscaToArray(gpointer data, gpointer user_data)
{
	Point* p= data;
	GArray* a= user_data;
	uint64_t v= p->x;

	g_array_append_val(a, v);
}


/*
 * A GCompareFunc for g_array_sort()
 *
 * Args:
 *   a, b          uint64_t*, absisca values
 *
 * Returns:
 *   "returns less than zero for first arg is less than second arg, zero for
 *   equal, greater zero if first arg is greater than second arg"
 *   - the great glib documentation
 */
static gint gcfCompareUint64(gconstpointer a, gconstpointer b)
{
	if (*(uint64_t*) a < *(uint64_t*) b)
	{
		return -1;
	}
	else if (*(uint64_t*) a > *(uint64_t*) b)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


/*
 * Compute synchronization factors using a linear programming approach.
 *
 * Args:
 *   syncState:    container for synchronization data
 */
static AllFactors* finalizeAnalysisCHullLP(SyncState* const syncState)
{
	AnalysisDataCHull* analysisData= syncState->analysisData;
	unsigned int i, j;
	AllFactors* lpFactorsArray;

	lpFactorsArray= createAllFactors(syncState->traceNb);
	
	analysisData->lps= malloc(syncState->traceNb * sizeof(glp_prob**));
	for (i= 0; i < syncState->traceNb; i++)
	{
		analysisData->lps[i]= malloc(i * sizeof(glp_prob*));
	}

	for (i= 0; i < syncState->traceNb; i++)
	{
		for (j= 0; j < i; j++)
		{
			glp_prob* lp;
			unsigned int it;
			GQueue*** hullArray= analysisData->hullArray;
			PairFactors* lpFactors= &lpFactorsArray->pairFactors[i][j];

			// Create the LP problem
			lp= lpCreateProblem(hullArray[i][j], hullArray[j][i]);
			analysisData->lps[i][j]= lp;

			// Use the LP problem to find the correction factors for this pair of
			// traces
			calculateCompleteFactorsLP(lp, lpFactors);

			// If possible, compute synchronization accuracy information for
			// graphs
			if (syncState->graphsStream && lpFactors->type == ACCURATE)
			{
				int retval;
				char* cwd;
				char fileName[43];
				FILE* fp;
				GArray* xValues;

				// Open the data file
				snprintf(fileName, sizeof(fileName),
					"analysis_chull_accuracy-%03u_and_%03u.data", j, i);
				fileName[sizeof(fileName) - 1]= '\0';

				cwd= changeToGraphsDir(syncState->graphsDir);

				if ((fp= fopen(fileName, "w")) == NULL)
				{
					g_error(strerror(errno));
				}
				fprintf(fp, "#%-24s %-25s %-25s %-25s\n", "x", "middle", "min", "max");

				retval= chdir(cwd);
				if (retval == -1)
				{
					g_error(strerror(errno));
				}
				free(cwd);

				// Build the list of absisca values for the points in the accuracy graph
				xValues= g_array_sized_new(FALSE, FALSE, sizeof(uint64_t),
					g_queue_get_length(hullArray[i][j]) +
					g_queue_get_length(hullArray[j][i]));

				g_queue_foreach(hullArray[i][j], &gfAddAbsiscaToArray, xValues);
				g_queue_foreach(hullArray[j][i], &gfAddAbsiscaToArray, xValues);

				g_array_sort(xValues, &gcfCompareUint64);

				/* For each absisca value and each optimisation direction, solve the LP
				 * and write a line in the data file */
				for (it= 0; it < xValues->len; it++)
				{
					uint64_t time;
					CorrectedTime correctedTime;

					time= g_array_index(xValues, uint64_t, it);
					timeCorrectionLP(lp, lpFactors, time, &correctedTime);
					fprintf(fp, "%24" PRIu64 " %24" PRIu64 " %24" PRIu64
						"%24" PRIu64 "\n", time, correctedTime.time,
						correctedTime.min, correctedTime.max);
				}

				g_array_free(xValues, TRUE);
				fclose(fp);
			}
		}
	}

	if (syncState->stats)
	{
		lpFactorsArray->refCount++;
		analysisData->stats->lpFactors= lpFactorsArray;
	}

	if (syncState->graphsStream)
	{
		lpFactorsArray->refCount++;
		analysisData->graphsData->lpFactors= lpFactorsArray;
	}

	return lpFactorsArray;
}


/*
 * Perform correction on one time value and calculate accuracy bounds.
 *
 * Args:
 *   lp:           Linear Programming problem containing the coefficients for
 *                 the trace pair between which to perform time correction.
 *   lpFactors:    Correction factors for this trace pair, the factors must be
 *                 of type ACCURATE.
 *   time:         Time value to correct.
 *   correctedTime: Result of the time correction, preallocated.
 */
void timeCorrectionLP(glp_prob* const lp, const PairFactors* const lpFactors,
	const uint64_t time, CorrectedTime* const correctedTime)
{
	unsigned int it;
	const struct
	{
		int direction;
		size_t offset;
	} loopValues[]= {
		{GLP_MIN, offsetof(CorrectedTime, min)},
		{GLP_MAX, offsetof(CorrectedTime, max)}
	};

	glp_set_obj_coef(lp, 1, 1.);
	glp_set_obj_coef(lp, 2, time);

	g_assert(lpFactors->type == ACCURATE);

	correctedTime->time= lpFactors->approx->offset + lpFactors->approx->drift
		* time;

	for (it= 0; it < ARRAY_SIZE(loopValues); it++)
	{
		int status;
		int retval;

		glp_set_obj_dir(lp, loopValues[it].direction);
		retval= glp_simplex(lp, NULL);
		status= glp_get_status(lp);

		g_assert(retval == 0 && status == GLP_OPT);
		*(uint64_t*) ((void*) correctedTime + loopValues[it].offset)=
			round(glp_get_obj_val(lp));
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
static void writeAnalysisTraceTimeBackPlotsCHull(SyncState* const syncState,
	const unsigned int i, const unsigned int j)
{
	if (((AnalysisDataCHull*)
			syncState->analysisData)->graphsData->lpFactors->pairFactors[j][i].type
		== ACCURATE)
	{
		fprintf(syncState->graphsStream,
			"\t\"analysis_chull_accuracy-%1$03u_and_%2$03u.data\" "
				"using 1:(($3 - $2) / clock_freq_%2$u):(($4 - $2) / clock_freq_%2$u) "
				"title \"Synchronization accuracy\" "
				"with filledcurves linewidth 2 linetype 1 "
				"linecolor rgb \"black\" fill solid 0.25 noborder, \\\n", i,
				j);
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
static void writeAnalysisTraceTimeForePlotsCHull(SyncState* const syncState,
	const unsigned int i, const unsigned int j)
{
	if (((AnalysisDataCHull*)
			syncState->analysisData)->graphsData->lpFactors->pairFactors[j][i].type
		== ACCURATE)
	{
		fprintf(syncState->graphsStream,
			"\t\"analysis_chull_accuracy-%1$03u_and_%2$03u.data\" "
				"using 1:(($3 - $2) / clock_freq_%2$u) notitle "
				"with lines linewidth 2 linetype 1 "
				"linecolor rgb \"gray60\", \\\n"
			"\t\"analysis_chull_accuracy-%1$03u_and_%2$03u.data\" "
				"using 1:(($4 - $2) / clock_freq_%2$u) notitle "
				"with lines linewidth 2 linetype 1 "
				"linecolor rgb \"gray60\", \\\n", i, j);
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
static void writeAnalysisTraceTraceBackPlotsCHull(SyncState* const syncState,
	const unsigned int i, const unsigned int j)
{
	if (((AnalysisDataCHull*)
			syncState->analysisData)->graphsData->lpFactors->pairFactors[j][i].type
		== ACCURATE)
	{
		fprintf(syncState->graphsStream,
			"\t\"analysis_chull_accuracy-%1$03u_and_%2$03u.data\" "
			"using 1:3:4 "
			"title \"Synchronization accuracy\" "
			"with filledcurves linewidth 2 linetype 1 "
			"linecolor rgb \"black\" fill solid 0.25 noborder, \\\n", i, j);
	}
}
#endif
