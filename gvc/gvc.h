/* $Id: gvc.h,v 1.32 2006/02/02 19:28:44 ellson Exp $ $Revision: 1.32 $ */
/* vim:set shiftwidth=4 ts=8: */

/**********************************************************
 * *      This software is part of the graphviz package      *
 * *                http://www.graphviz.org/                 *
 * *                                                         *
 * *            Copyright (c) 1994-2004 AT&T Corp.           *
 * *                and is licensed under the                *
 * *            Common Public License, Version 1.0           *
 * *                      by AT&T Corp.                      *
 * *                                                         *
 * *        Information and Software Systems Research        *
 * *              AT&T Research, Florham Park NJ             *
 * **********************************************************/

#ifndef			GVC_H
#define			GVC_H

#include "types.h"
#include "graph.h"

#ifdef __cplusplus
extern "C" {
#endif

#define dotneato_initialize dotneato_initialize_DEPRECATED_BY_gvParseArgs
#define parse_args parse_args_DEPRECATED_BY_gvParseArgs

#define dot_layout dot_layout_DEPRECATED_BY_gvLayout
#define neato_layout dot_layout_DEPRECATED_BY_gvLayout
#define fdp_layout dot_layout_DEPRECATED_BY_gvLayout
#define circo_layout dot_layout_DEPRECATED_BY_gvLayout
#define twopi_layout dot_layout_DEPRECATED_BY_gvLayout
#define gvBindContext gvBindContext_DEPRECATED_BY_gvLayout
#define gvlayout_layout gvlayout_layout_DEPRECATED_BY_gvLayoutJobs

#define emit_jobs emit_jobs_DEPRECATED_BY_gvRenderJobs
#define dotneato_write dotneato_write_DEPRECATED_BY_gvRenderJobs

#define dot_cleanup dot_cleanup_DEPRECATED_BY_gvFreeLayout
#define neato_cleanup dot_cleanup_DEPRECATED_BY_gvFreeLayout
#define fdp_cleanup dot_cleanup_DEPRECATED_BY_gvFreeLayout
#define circo_cleanup dot_cleanup_DEPRECATED_BY_gvFreeLayout
#define twopi_cleanup dot_cleanup_DEPRECATED_BY_gvFreeLayout
#define gvlayout_cleanup gvlayout_cleanup_DEPRECATED_BY_gvFreeLayout

#define gvCleanup gvCleanup_DEPRECATED_BY_gvFreeContext
#define dotneato_terminate dotneato_terminate_DEPRECATED_BY_gvFreeContext

/* misc */
/* FIXME - these need eliminating or renaming */
extern void gvToggle(int);
extern graph_t *next_input_graph(void);

/* set up a graphviz context */
extern GVC_t *gvNEWcontext(char **info, char *user);
extern char *gvUsername(void);

/*  set up a graphviz context - alternative */
/*     (wraps the above two functions using info built into libgvc) */
extern GVC_t *gvContext(void);

/* parse command line args - minimally argv[0] sets layout engine */
extern int gvParseArgs(GVC_t *gvc, int argc, char **argv);

/* Compute a layout using a specified engine */
extern int gvLayout(GVC_t *gvc, graph_t *g, char *engine);

/* Compute a layout using layout engine from command line args */
extern int gvLayoutJobs(GVC_t *gvc, graph_t *g);

/* Render layout into string attributes of the graph */
extern void attach_attrs(graph_t *g);

/* Parse an html string */
extern char *agstrdup_html(char *s);
extern int aghtmlstr(char *s);

/* Render layout in a specified format to an open FILE */
extern int gvRender(GVC_t *gvc, graph_t *g, char *format, FILE *out);

/* Render layout in a specified format to an open FILE */
extern int gvRenderFilename(GVC_t *gvc, graph_t *g, char *format, char *filename);

/* Render layout according to -T and -o options found by gvParseArgs */
extern int gvRenderJobs(GVC_t *gvc, graph_t *g);

/* Clean up layout data structures - layouts are not nestable (yet) */
extern int gvFreeLayout(GVC_t *gvc, graph_t *g);

/* Clean up graphviz context */
extern int gvFreeContext(GVC_t *gvc);

#ifdef __cplusplus
}
#endif
#endif			/* GVC_H */
