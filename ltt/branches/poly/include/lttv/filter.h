#ifndef FILTER_H
#define FILTER_H

/* A filter expression consists in nested AND, OR and NOT expressions
   involving boolean relation (>, >=, =, !=, <, <=) between event fields and 
   specific values. It is compiled into an efficient data structure which
   is used in functions to check if a given event or tracefile satisfies the
   filter.

   The grammar for filters is:

   filter = expression

   expression = "(" expression ")" | "!" expression | 
                expression "&&" expression | expression "||" expression |
                simpleExpression

   simpleExpression = fieldPath op value

   fieldPath = fieldComponent [ "." fieldPath ]

   fieldComponent = name [ "[" integer "]" ]

   value = integer | double | string 

*/


typedef struct _lttv_filter lttv_filter;


/* Compile the filter expression into an efficient data structure */

lttv_filter *lttv_filter_new(char *expression, lttv_trace *t);


/* Check if the tracefile or event satisfies the filter. The arguments are
   declared as void * to allow these functions to be used as hooks. */

bool lttv_filter_tracefile(void *filter, void *tracefile);

bool lttv_filter_event(void *filter, void *event);

#endif // FILTER_H

