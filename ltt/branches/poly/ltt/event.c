
#include <ltt/event.h>

/*****************************************************************************
 *Function name
 *    ltt_event_position_get : get the event position data
 *Input params
 *    e                  : an instance of an event type   
 *    ep                 : a pointer to event's position structure
 *    tf                 : tracefile pointer
 *    block              : current block
 *    offset             : current offset
 *    tsc                : current tsc
 ****************************************************************************/
void ltt_event_position_get(LttEventPosition *ep, LttTracefile **tf,
        guint *block, guint *offset, guint64 *tsc)
{
  *tf = ep->tracefile;
  *block = ep->block;
  *offset = ep->offset;
  *tsc = ep->tsc;
}


void ltt_event_position_set(LttEventPosition *ep, LttTracefile *tf,
        guint block, guint offset, guint64 tsc)
{
  ep->tracefile = tf;
  ep->block = block;
  ep->offset = offset;
  ep->tsc = tsc;
}


/*****************************************************************************
 *Function name
 *    ltt_event_position : get the event's position
 *Input params
 *    e                  : an instance of an event type   
 *    ep                 : a pointer to event's position structure
 ****************************************************************************/

void ltt_event_position(LttEvent *e, LttEventPosition *ep)
{
  ep->tracefile = e->tracefile;
  ep->block = e->block;
  ep->offset = e->offset;
  ep->tsc = e->tsc;
}

LttEventPosition * ltt_event_position_new()
{
  return g_new(LttEventPosition, 1);
}


/*****************************************************************************
 * Function name
 *    ltt_event_position_compare : compare two positions
 *    A NULL value is infinite.
 * Input params
 *    ep1                    : a pointer to event's position structure
 *    ep2                    : a pointer to event's position structure
 * Return
 *    -1 is ep1 < ep2
 *    1 if ep1 > ep2
 *    0 if ep1 == ep2
 ****************************************************************************/


gint ltt_event_position_compare(const LttEventPosition *ep1,
                                const LttEventPosition *ep2)
{
  if(ep1 == NULL && ep2 == NULL)
      return 0;
  if(ep1 != NULL && ep2 == NULL)
      return -1;
  if(ep1 == NULL && ep2 != NULL)
      return 1;

   if(ep1->tracefile != ep2->tracefile)
    g_error("ltt_event_position_compare on different tracefiles makes no sense");
   
  if(ep1->block < ep2->block)
    return -1;
  if(ep1->block > ep2->block)
    return 1;
  if(ep1->offset < ep2->offset)
    return -1;
  if(ep1->offset > ep2->offset)
    return 1;
  return 0;
}

/*****************************************************************************
 * Function name
 *    ltt_event_position_copy : copy position
 * Input params
 *    src                    : a pointer to event's position structure source
 *    dest                   : a pointer to event's position structure dest
 * Return
 *    void
 ****************************************************************************/
void ltt_event_position_copy(LttEventPosition *dest,
                             const LttEventPosition *src)
{
  if(src == NULL)
    dest = NULL;
  else
    *dest = *src;
}



LttTracefile *ltt_event_position_tracefile(LttEventPosition *ep)
{
  return ep->tracefile;
}

