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

#define _GNU_SOURCE
#define NANOSECONDS_PER_SECOND 1000000000
#define CPU_FREQ 1e9

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sync_chain.h"

#include "event_processing_text.h"


// Functions common to all processing modules
static void initProcessingText(SyncState* const syncState, ...);
static void destroyProcessingText(SyncState* const syncState);
static AllFactors* finalizeProcessingText(SyncState* const syncState);
static void writeProcessingTraceTimeOptionsText(SyncState* const syncState,
	const unsigned int i, const unsigned int j);
static void writeProcessingTraceTraceOptionsText(SyncState* const syncState,
	const unsigned int i, const unsigned int j);
static void writeProcessingGraphVariablesText(SyncState* const syncState,
	const unsigned int i);

// Functions specific to this module
static unsigned int readTraceNb(FILE* testCase);
static void skipCommentLines(FILE* testCase);


static ProcessingModule processingModuleText = {
	.name= "text",
	.initProcessing= &initProcessingText,
	.destroyProcessing= &destroyProcessingText,
	.finalizeProcessing= &finalizeProcessingText,
	.graphFunctions= {
		.writeVariables= &writeProcessingGraphVariablesText,
		.writeTraceTraceOptions= &writeProcessingTraceTraceOptionsText,
		.writeTraceTimeOptions= &writeProcessingTraceTimeOptionsText,
	},
};


/*
 * Processing Module registering function
 */
void registerProcessingText()
{
	g_queue_push_tail(&processingModules, &processingModuleText);
}


/*
 * Allocate and initialize data structures for synchronizing a traceset.
 * Open test case file.
 *
 * Args:
 *   syncState:    container for synchronization data.
 *   testCaseName: const char*, test case file name
 */
static void initProcessingText(SyncState* const syncState, ...)
{
	ProcessingDataText* processingData;
	const char* testCaseName;
	va_list ap;

	processingData= malloc(sizeof(ProcessingDataText));
	syncState->processingData= processingData;
	va_start(ap, syncState);
	testCaseName= va_arg(ap, const char*);
	va_end(ap);

	processingData->testCase= fopen(testCaseName, "r");
	if (processingData->testCase == NULL)
	{
		g_error(strerror(errno));
	}
	syncState->traceNb= readTraceNb(processingData->testCase);

	if (syncState->stats)
	{
		processingData->factors= NULL;
	}
}


static void destroyProcessingText(SyncState* const syncState)
{
	ProcessingDataText* processingData= (ProcessingDataText*)
		syncState->processingData;

	if (processingData == NULL)
	{
		return;
	}

	fclose(processingData->testCase);

	if (syncState->stats && processingData->factors)
	{
		freeAllFactors(processingData->factors, syncState->traceNb);
	}

	free(syncState->processingData);
	syncState->processingData= NULL;
}


/*
 * Read the test case file and make up events. Dispatch those events to the
 * matching module.
 *
 * Args:
 *   syncState:    container for synchronization data.
 *
 * Returns:
 *   AllFactors    synchronization factors for each trace pair
 */
static AllFactors* finalizeProcessingText(SyncState* const syncState)
{
	int retval;
	unsigned int* seq;
	AllFactors* factors;
	ProcessingDataText* processingData= (ProcessingDataText*)
		syncState->processingData;
	FILE* testCase= processingData->testCase;
	char* line= NULL;
	size_t bufLen;

	seq= calloc(syncState->traceNb, sizeof(unsigned int));

	skipCommentLines(testCase);
	retval= getline(&line, &bufLen, testCase);
	while(!feof(testCase))
	{
		unsigned int sender, receiver;
		double sendTime, recvTime;
		char tmp;
		unsigned int i;

		if (retval == -1 && !feof(testCase))
		{
			g_error(strerror(errno));
		}

		if (line[retval - 1] == '\n')
		{
			line[retval - 1]= '\0';
		}

		retval= sscanf(line, " %u %u %lf %lf %c", &sender, &receiver,
			&sendTime, &recvTime, &tmp);
		if (retval == EOF)
		{
			g_error(strerror(errno));
		}
		else if (retval != 4)
		{
			g_error("Error parsing test file while looking for data point, line was '%s'", line);
		}

		if (sender + 1 > syncState->traceNb)
		{
			g_error("Error parsing test file, sender is out of range, line was '%s'", line);
		}

		if (receiver + 1 > syncState->traceNb)
		{
			g_error("Error parsing test file, receiver is out of range, line was '%s'", line);
		}

		if (sendTime < 0)
		{
			g_error("Error parsing test file, send time is negative, line was '%s'", line);
		}

		if (recvTime < 0)
		{
			g_error("Error parsing test file, receive time is negative, line was '%s'", line);
		}

		// Generate ouput and input events
		{
			unsigned int addressOffset;
			struct {
				unsigned int traceNum;
				double time;
				enum Direction direction;
			} loopValues[]= {
				{sender, sendTime, OUT},
				{receiver, recvTime, IN},
			};

			/* addressOffset is added to a traceNum to convert it to an address so
			 * that the address is not plainly the same as the traceNb. */
			if (syncState->traceNb > 1)
			{
				addressOffset= pow(10, floor(log(syncState->traceNb - 1) /
						log(10)) + 1);
			}
			else
			{
				addressOffset= 0;
			}

			for (i= 0; i < sizeof(loopValues) / sizeof(*loopValues); i++)
			{
				Event* event;

				event= malloc(sizeof(Event));
				event->traceNum= loopValues[i].traceNum;
				event->wallTime.seconds= floor(loopValues[i].time);
				event->wallTime.nanosec= floor((loopValues[i].time -
						floor(loopValues[i].time)) * NANOSECONDS_PER_SECOND);
				event->cpuTime= round(loopValues[i].time * CPU_FREQ);
				event->type= TCP;
				event->destroy= &destroyTCPEvent;
				event->event.tcpEvent= malloc(sizeof(TCPEvent));
				event->event.tcpEvent->direction= loopValues[i].direction;
				event->event.tcpEvent->segmentKey= malloc(sizeof(SegmentKey));
				event->event.tcpEvent->segmentKey->ihl= 5;
				event->event.tcpEvent->segmentKey->tot_len= 40;
				event->event.tcpEvent->segmentKey->connectionKey.saddr= sender +
					addressOffset;
				event->event.tcpEvent->segmentKey->connectionKey.daddr= receiver +
					addressOffset;
				event->event.tcpEvent->segmentKey->connectionKey.source= 57645;
				event->event.tcpEvent->segmentKey->connectionKey.dest= 80;
				event->event.tcpEvent->segmentKey->seq= seq[sender];
				event->event.tcpEvent->segmentKey->ack_seq= 0;
				event->event.tcpEvent->segmentKey->doff= 5;
				event->event.tcpEvent->segmentKey->ack= 0;
				event->event.tcpEvent->segmentKey->rst= 0;
				event->event.tcpEvent->segmentKey->syn= 1;
				event->event.tcpEvent->segmentKey->fin= 0;

				syncState->matchingModule->matchEvent(syncState, event);
			}
		}

		seq[sender]++;

		skipCommentLines(testCase);
		retval= getline(&line, &bufLen, testCase);
	}

	free(seq);

	if (line)
	{
		free(line);
	}

	factors= syncState->matchingModule->finalizeMatching(syncState);
	if (syncState->stats)
	{
		processingData->factors= factors;
	}

	return factors;
}


/*
 * Read trace number from the test case stream. The trace number should be the
 * first non-comment line and should be an unsigned int by itself on a line.
 *
 * Args:
 *   testCase:     test case stream
 *
 * Returns:
 *   The trace number
 */
static unsigned int readTraceNb(FILE* testCase)
{
	unsigned int result;
	int retval;
	char* line= NULL;
	size_t len;
	char tmp;

	skipCommentLines(testCase);
	retval= getline(&line, &len, testCase);
	if (retval == -1)
	{
		if (feof(testCase))
		{
			g_error("Unexpected end of file while looking for number of traces");
		}
		else
		{
			g_error(strerror(errno));
		}
	}
	if (line[retval - 1] == '\n')
	{
		line[retval - 1]= '\0';
	}

	retval= sscanf(line, " %u %c", &result, &tmp);
	if (retval == EOF || retval != 1)
	{
		g_error("Error parsing test file while looking for number of traces, line was '%s'", line);

		// Not really needed but avoids warning from gcc
		abort();
	}

	if (line)
	{
		free(line);
	}

	return result;
}


/*
 * Advance testCase stream over empty space, empty lines and lines that begin
 * with '#'
 *
 * Args:
 *   testCase:     test case stream
 */
static void skipCommentLines(FILE* testCase)
{
	int firstChar;
	ssize_t retval;
	char* line= NULL;
	size_t len;

	do
	{
		firstChar= fgetc(testCase);
		if (firstChar == (int) '#')
		{
			retval= getline(&line, &len, testCase);
			if (retval == -1)
			{
				if (feof(testCase))
				{
					goto outEof;
				}
				else
				{
					g_error(strerror(errno));
				}
			}
		}
		else if (firstChar == (int) '\n' || firstChar == (int) ' ')
		{}
		else if (firstChar == EOF)
		{
			goto outEof;
		}
		else
		{
			break;
		}
	} while (true);
	retval= ungetc(firstChar, testCase);
	if (retval == EOF)
	{
		g_error("Error: ungetc()");
	}

outEof:
	if (line)
	{
		free(line);
	}
}


/*
 * Write the processing-specific variables in the gnuplot script.
 *
 * Args:
 *   syncState:    container for synchronization data
 *   i:            trace number
 */
static void writeProcessingGraphVariablesText(SyncState* const syncState,
	const unsigned int i)
{
	fprintf(syncState->graphsStream, "clock_freq_%u= %.3f\n", i, CPU_FREQ);
}


/*
 * Write the processing-specific options in the gnuplot script.
 *
 * Args:
 *   syncState:    container for synchronization data
 *   i:            first trace number
 *   j:            second trace number, garanteed to be larger than i
 */
static void writeProcessingTraceTraceOptionsText(SyncState* const syncState,
	const unsigned int i, const unsigned int j)
{
	fprintf(syncState->graphsStream,
        "set key inside right bottom\n"
        "set xlabel \"Clock %1$u\"\n"
        "set xtics nomirror\n"
        "set ylabel \"Clock %2$u\"\n"
        "set ytics nomirror\n"
		"set x2label \"Clock %1$d (s)\"\n"
		"set x2range [GPVAL_X_MIN / clock_freq_%1$u : GPVAL_X_MAX / clock_freq_%1$u]\n"
		"set x2tics\n"
		"set y2label \"Clock %2$d (s)\"\n"
		"set y2range [GPVAL_Y_MIN / clock_freq_%2$u : GPVAL_Y_MAX / clock_freq_%2$u]\n"
		"set y2tics\n", i, j);
}


/*
 * Write the processing-specific options in the gnuplot script.
 *
 * Args:
 *   syncState:    container for synchronization data
 *   i:            first trace number
 *   j:            second trace number, garanteed to be larger than i
 */
static void writeProcessingTraceTimeOptionsText(SyncState* const syncState,
	const unsigned int i, const unsigned int j)
{
	fprintf(syncState->graphsStream,
        "set key inside right bottom\n"
        "set xlabel \"Clock %1$u\"\n"
        "set xtics nomirror\n"
        "set ylabel \"time (s)\"\n"
        "set ytics nomirror\n"
		"set x2label \"Clock %1$d (s)\"\n"
		"set x2range [GPVAL_X_MIN / clock_freq_%1$u : GPVAL_X_MAX / clock_freq_%1$u]\n"
		"set x2tics\n", i);
}
