/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Michel Dagenais
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

#ifndef BATCH_ANALYSIS_H
#define BATCH_ANALYSIS_H

/* The batch analysis module defines a main traceset and command line options
   to specify the traces in the main traceset. It then processes that
   traceset calling hooks lists at various stages of the analysis.

   The hooks lists are defined in the global attributes for these various 
   stages of the analysis. Loaded modules may add hooks to these lists.
   Thus, by requesting that a certain module be loaded, the analysis may
   produce additional information as the module adds hooks and these hooks
   are called during the analysis.

   The hooks lists defined are as follows. These may be split in more
   specific lists eventually to select the hooks applicable to state update,
   incremental or batch processing...
 
   /hooks/traceset/before
       Before analyzing a traceset.

   /hooks/traceset/after
       After analyzing a traceset.

   /hooks/trace/before
       Before each trace.

   /hooks/trace/after
       After each trace.

   /hooks/tracefile/before
       Before each tracefile.

   /hooks/tracefile/after
       After each tracefile.

   /hooks/event/before
       Before each event.

   /hooks/event/after
       After each event.

*/

#endif // BATCH_ANALYSIS_H
