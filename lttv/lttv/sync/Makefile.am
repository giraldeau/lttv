AM_CFLAGS= $(PACKAGE_CFLAGS)
LDADD = $(M_LIBS) $(GLPK_LIBS)

#Mathieu: does not build on x86 64, disable.
#check_PROGRAMS = unittest
#
#unittest_SOURCES = \
#	data_structures.c\
#	graph_functions.c\
#	sync_chain.c\
#	sync_chain_unittest.c\
#	event_processing_text.c\
#	event_matching_broadcast.c\
#	event_matching_distributor.c\
#	event_matching_tcp.c\
#	event_analysis_chull.c\
#	event_analysis_eval.c\
#	event_analysis_linreg.c

noinst_HEADERS = \
	data_structures.h \
	event_analysis_chull.h \
	event_analysis_eval.h \
	event_analysis.h \
	event_analysis_linreg.h \
	event_matching_broadcast.h \
	event_matching_distributor.h \
	event_matching.h \
	event_matching_tcp.h \
	event_processing.h \
	event_processing_lttng_common.h \
	event_processing_lttng_null.h \
	event_processing_lttng_standard.h \
	event_processing_text.h \
	graph_functions.h \
	lookup3.h \
	sync_chain.h \
	sync_chain_lttv.h
