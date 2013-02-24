/* $Id: figgen.c,v 1.10 2005/10/18 21:05:22 ellson Exp $ $Revision: 1.10 $ */
/* vim:set shiftwidth=4 ts=8: */

/**********************************************************
*      This software is part of the graphviz package      *
*                http://www.graphviz.org/                 *
*                                                         *
*            Copyright (c) 1994-2004 AT&T Corp.           *
*                and is licensed under the                *
*            Common Public License, Version 1.0           *
*                      by AT&T Corp.                      *
*                                                         *
*        Information and Software Systems Research        *
*              AT&T Research, Florham Park NJ             *
**********************************************************/

#include <ctype.h>
#include "render.h"

/* FIG font modifiers */
#define REGULAR 0
#define BOLD	1
#define ITALIC	2

/* FIG patterns */
#define P_SOLID	0
#define P_NONE  15
#define P_DOTTED 2
#define P_DASHED 1

/* FIG bold line constant */
#define WIDTH_NORMAL 1
#define WIDTH_BOLD 3

/* Number of points to split splines into */
#define BEZIERSUBDIVISION 6

static int N_pages;
/* static 	point	Pages; */
static double Scale;
static int Rot;
static box PB;
static int onetime = TRUE;

typedef struct context_t {
    unsigned char pencolor_ix, fillcolor_ix;
    char *fontfam, fontopt, font_was_set;
    char line_style, fill, penwidth, style_was_set;
    double fontsz, style_val;
} context_t;

#define MAXNEST 4
static context_t cstk[MAXNEST];
static int SP;

static int figColorResolve(int *new, int r, int g, int b)
{
#define maxColors 256
    static int top = 0;
    static short red[maxColors], green[maxColors], blue[maxColors];
    int c;
    int ct = -1;
    long rd, gd, bd, dist;
    long mindist = 3 * 255 * 255;	/* init to max poss dist */

    *new = 0;			/* in case it is not a new color */
    for (c = 0; c < top; c++) {
	rd = (long) (red[c] - r);
	gd = (long) (green[c] - g);
	bd = (long) (blue[c] - b);
	dist = rd * rd + gd * gd + bd * bd;
	if (dist < mindist) {
	    if (dist == 0)
		return c;	/* Return exact match color */
	    mindist = dist;
	    ct = c;
	}
    }
    /* no exact match.  We now know closest, but first try to allocate exact */
    if (top++ == maxColors)
	return ct;		/* Return closest available color */
    red[c] = r;
    green[c] = g;
    blue[c] = b;
    *new = 1;			/* flag new color */
    return c;			/* Return newly allocated color */
}

static void fig_reset(void)
{
    onetime = TRUE;
}


static void init_fig(void)
{
    SP = 0;
    cstk[0].pencolor_ix = 0;	/* FIG pencolor index */
    cstk[0].fillcolor_ix = 0;	/* FIG fillcolor index */
    cstk[0].fontfam = DEFAULT_FONTNAME;	/* font family name */
    cstk[0].fontopt = REGULAR;	/* modifier: REGULAR, BOLD or ITALIC */
    cstk[0].line_style = P_SOLID;	/* pen pattern style, default is solid */
    cstk[0].style_val = 0.0;	/* style_val, used for dashed style */
    cstk[0].fill = P_NONE;
    cstk[0].penwidth = WIDTH_NORMAL;
}

static point figpt(point p)
{
    point rv;

    if (Rot == 0) {
	rv.x = Scale * p.x;
	rv.y = Scale * (2 * PB.UR.y - p.y);
    } else {
	rv.x = Scale * (2 * PB.UR.x - p.y);
	rv.y = Scale * p.x;
    }
    return rv;
}


static point figfpt(pointf p)
{
    point rv;

    if (Rot == 0) {
	rv.x = (int) (Scale * p.x);
	rv.y = (int) (Scale * (2 * PB.UR.y - p.y));
    } else {
	rv.x = (int) (Scale * (2 * PB.UR.x - p.y));
	rv.y = (int) (Scale * p.x);
    }
    return rv;
}

static double figfontsz(double size)
{
    return Scale * size * POINTS_PER_INCH / 1200.0;
}

static void figptarray(point * A, int n, int close)
{
    int i;
    point p;

    for (i = 0; i < n; i++) {
	p.x = A[i].x;
	p.y = A[i].y;
	p = figpt(p);
	fprintf(Output_file, " %d %d", p.x, p.y);
    }
    if (close) {
	p.x = A[0].x;
	p.y = A[0].y;
	p = figpt(p);
	fprintf(Output_file, " %d %d", p.x, p.y);
    }
    fprintf(Output_file, "\n");
}

static void fig_comment(char *str)
{
    fprintf(Output_file, "# %s\n", str);
}

static void
fig_begin_job(FILE * ofp, graph_t * g, char **lib, char *user,
	      char *info[], point pages)
{
    /* Pages = pages; */
    N_pages = pages.x * pages.y;
    fprintf(Output_file, "#FIG 3.2\n");
    fprintf(Output_file, "Portrait\n");	/* orientation */
    fprintf(Output_file, "Center\n");	/* justification */
    fprintf(Output_file, "Metric\n");	/* units */
    fprintf(Output_file, "A4\n");	/* papersize */
    fprintf(Output_file, "100.00\n");	/* magnification % */
    fprintf(Output_file, "Single\n");	/* multiple-page */
    fprintf(Output_file, "-2\n");	/* transparent color (none) */
    fprintf(Output_file, "# Generated by %s version %s (%s)\n", info[0],
	    info[1], info[2]);
    fprintf(Output_file, "# For: %s\n", user);
    fprintf(Output_file, "# Title: %s\n", g->name);
    fprintf(Output_file, "# Pages: %d\n", N_pages);
    fprintf(Output_file, "1200 ");	/* resolution */
    fprintf(Output_file, "2\n");	/* coordinate system (upper left) */
}

static void fig_end_job(void)
{
    fprintf(Output_file, "# end of FIG file\n");
}

static void fig_begin_graph(GVC_t * gvc, graph_t * g, box bb, point pb)
{
    PB = bb;
    if (onetime) {
#if 0
	fprintf(stderr, "LL %d %d UR %d %d\n", PB.LL.x, PB.LL.y, PB.UR.x,
		PB.UR.y);
#endif
	init_fig();
	onetime = FALSE;
    }
}

static void
fig_begin_page(graph_t * g, point page, double scale, int rot,
	       point offset)
{
    /* int          page_number; */
    /* point        sz; */

    Scale = scale * 1200.0 / POINTS_PER_INCH;
    Rot = rot;
/*
	page_number =  page.x + page.y * Pages.x + 1;
	sz = sub_points(PB.UR,PB.LL);
*/
}

static void fig_begin_context(void)
{
    assert(SP + 1 < MAXNEST);
    cstk[SP + 1] = cstk[SP];
    SP++;
}

static void fig_end_context(void)
{
    int psp = SP - 1;
    assert(SP > 0);
    /* if (cstk[SP].font_was_set) fig_font(&(cstk[psp])); */
    /* if (cstk[SP].style_was_set) fig_style(&(cstk[psp])); */
    /*free(cstk[psp].fontfam); */
    SP = psp;
}

static void fig_set_font(char *name, double size)
{
    char *p, *q;
    context_t *cp;

    cp = &(cstk[SP]);
    cp->font_was_set = TRUE;
    cp->fontsz = size;
    p = _strdup(name);
    if ((q = strchr(p, '-'))) {
	*q++ = 0;
	if (strcasecmp(q, "italic") == 0)
	    cp->fontopt = ITALIC;
	else if (strcasecmp(q, "bold") == 0)
	    cp->fontopt = BOLD;
    }
    cp->fontfam = p;
/*	fig_font(&cstk[SP]); */
}

static void fig_color(int i, int r, int g, int b)
{
    int object_code = 0;	/* always 0 for color */

    fprintf(Output_file, "%d %d #%02x%02x%02x\n", object_code, i, r, g, b);
}

static unsigned char fig_resolve_color(char *name)
{
    unsigned char i;
    int new;
    char *tok;
    gvcolor_t color;

    static char *figcolor[] = { "black", "blue", "green", "cyan",
	"red", "magenta", "yellow", "white", (char *) NULL
    };

    tok = canontoken(name);
    for (i = 0; figcolor[i]; i++) {
	if (streq(figcolor[i], tok))
	    return i;
    }
    colorxlate(name, &color, RGBA_BYTE);
    i = 32 + figColorResolve(&new, color.u.rgba[0], color.u.rgba[1],
			     color.u.rgba[2]);
    if (new)
	fig_color(i, color.u.rgba[0], color.u.rgba[1], color.u.rgba[2]);
    return i;
}

static void fig_set_pencolor(char *name)
{
    cstk[SP].pencolor_ix = fig_resolve_color(name);
}

static void fig_set_fillcolor(char *name)
{
    cstk[SP].fillcolor_ix = fig_resolve_color(name);
}

static void fig_set_style(char **s)
{
    char *line, *p;
    context_t *cp;

    cp = &(cstk[SP]);
    while ((p = line = *s++)) {
	if (streq(line, "solid")) {
	    cp->line_style = P_SOLID;
	    cp->style_val = 0.0;
	} else if (streq(line, "dashed")) {
	    cp->line_style = P_DASHED;
	    cp->style_val = 4.0;
	} else if (streq(line, "dotted")) {
	    cp->line_style = P_DOTTED;
	    cp->style_val = 3.0;
	} else if (streq(line, "invis"))
	    cp->line_style = P_NONE;
	else if (streq(line, "bold"))
	    cp->penwidth = WIDTH_BOLD;
	else if (streq(line, "setlinewidth")) {
	    while (*p)
		p++;
	    p++;
	    cp->penwidth = atol(p);
	} else if (streq(line, "filled"))
	    cp->fill = P_SOLID;
	else if (streq(line, "unfilled"))
	    cp->fill = P_NONE;
	else {
	    agerr(AGWARN,
		  "fig_set_style: unsupported style %s - ignoring\n",
		  line);
	}
	cp->style_was_set = TRUE;
    }
/*	if (cp->style_was_set) fig_style(cp); */
}

static char *fig_string(char *s)
{
    static char *buf = NULL;
    static int bufsize = 0;
    int pos = 0;
    char *p;
    unsigned char c;

    if (!buf) {
	bufsize = 64;
	buf = N_GNEW(bufsize, char);
    }

    p = buf;
    while ((c = *s++)) {
	if (pos > (bufsize - 8)) {
	    bufsize *= 2;
	    buf = grealloc(buf, bufsize);
	    p = buf + pos;
	}
	if (isascii(c)) {
	    if (c == '\\') {
		*p++ = '\\';
		pos++;
	    }
	    *p++ = c;
	    pos++;
	} else {
	    *p++ = '\\';
	    sprintf(p, "%03o", c);
	    p += 3;
	    pos += 4;
	}
    }
    *p = '\0';
    return buf;
}

static void fig_textline(point p, textline_t * line)
{
    int object_code = 4;	/* always 4 for text */
    int sub_type = 0;		/* text justification */
    int color = cstk[SP].pencolor_ix;
    int depth = 0;
    int pen_style = 0;		/* not used */
    int font = 0;
    double font_size = figfontsz(cstk[SP].fontsz);
    double angle = Rot ? (PI / 2.0) : 0.0;
    int font_flags = 2;
    double height = 0.0;
    double length = 0.0;
    point mp;

    switch (line->just) {
    case 'l':
	sub_type = 0;
	break;
    case 'r':
	sub_type = 2;
	break;
    default:
    case 'n':
	sub_type = 1;
	break;
    }
    mp.x = p.x;
    mp.y = p.y;
    mp = figpt(mp);

    fprintf(Output_file,
	    "%d %d %d %d %d %d %.1f %.4f %d %.1f %.1f %d %d %s\\001\n",
	    object_code, sub_type, color, depth, pen_style, font,
	    font_size, angle, font_flags, height, length, mp.x, mp.y,
	    fig_string(line->str));
}

static void fig_bezier(point * A, int n, int arrow_at_start,
		       int arrow_at_end, int filled)
{
    int object_code = 3;	/* always 3 for spline */
    int sub_type;
    int line_style = cstk[SP].line_style;	/* solid, dotted, dashed */
    int thickness = cstk[SP].penwidth;
    int pen_color = cstk[SP].pencolor_ix;
    int fill_color;
    int depth = 0;
    int pen_style = 0;		/* not used */
    int area_fill;
    double style_val = cstk[SP].style_val;
    int cap_style = 0;
    int forward_arrow = 0;
    int backward_arrow = 0;
    int npoints = n;
    int i;

    pointf p0, V[4];
    point p1;
    int j, step;
    int count = 0;
    int size;

    char *buffer;
    char *buf;
    buffer =
	malloc((npoints + 1) * (BEZIERSUBDIVISION +
				1) * 20 * sizeof(char));
    buf = buffer;

    if (filled) {
	sub_type = 5;     /* closed X-spline */
	area_fill = 20;   /* fully saturated color */
	fill_color = cstk[SP].fillcolor_ix;
    }
    else {
	sub_type = 4;     /* opened X-spline */
	area_fill = -1;
	fill_color = 0;
    }
    V[3].x = A[0].x;
    V[3].y = A[0].y;
    /* Write first point in line */
    count++;
    p0.x = A[0].x;
    p0.y = A[0].y;
    p1 = figfpt(p0);
    size = sprintf(buf, " %d %d", p1.x, p1.y);
    buf += size;
    /* write subsequent points */
    for (i = 0; i + 3 < n; i += 3) {
	V[0] = V[3];
	for (j = 1; j <= 3; j++) {
	    V[j].x = A[i + j].x;
	    V[j].y = A[i + j].y;
	}
	for (step = 1; step <= BEZIERSUBDIVISION; step++) {
	    count++;
	    p1 = figfpt(Bezier
			(V, 3, (double) step / BEZIERSUBDIVISION, NULL,
			 NULL));
	    size = sprintf(buf, " %d %d", p1.x, p1.y);
	    buf += size;
	}
    }

    fprintf(Output_file, "%d %d %d %d %d %d %d %d %d %.1f %d %d %d %d\n",
	    object_code,
	    sub_type,
	    line_style,
	    thickness,
	    pen_color,
	    fill_color,
	    depth,
	    pen_style,
	    area_fill,
	    style_val, cap_style, forward_arrow, backward_arrow, count);

    fprintf(Output_file, " %s\n", buffer);	/* print points */
    free(buffer);
    for (i = 0; i < count; i++) {
	fprintf(Output_file, " %d", i % (count - 1) ? 1 : 0);	/* -1 on all */
    }
    fprintf(Output_file, "\n");
}

static void fig_polygon(point * A, int n, int filled)
{
    int object_code = 2;	/* always 2 for polyline */
    int sub_type = 3;		/* always 3 for polygon */
    int line_style = cstk[SP].line_style;	/* solid, dotted, dashed */
    int thickness = cstk[SP].penwidth;
    int pen_color = cstk[SP].pencolor_ix;
    int fill_color = cstk[SP].fillcolor_ix;
    int depth = 0;
    int pen_style = 0;		/* not used */
    int area_fill = filled ? 20 : -1;
    double style_val = cstk[SP].style_val;
    int join_style = 0;
    int cap_style = 0;
    int radius = 0;
    int forward_arrow = 0;
    int backward_arrow = 0;
    int npoints = n + 1;

    fprintf(Output_file,
	    "%d %d %d %d %d %d %d %d %d %.1f %d %d %d %d %d %d\n",
	    object_code, sub_type, line_style, thickness, pen_color,
	    fill_color, depth, pen_style, area_fill, style_val, join_style,
	    cap_style, radius, forward_arrow, backward_arrow, npoints);
    figptarray(A, n, 1);	/* closed shape */
}

static void fig_ellipse(point p, int rx, int ry, int filled)
{
    int object_code = 1;	/* always 1 for ellipse */
    int sub_type = 1;		/* ellipse defined by radii */
    int line_style = cstk[SP].line_style;	/* solid, dotted, dashed */
    int thickness = cstk[SP].penwidth;
    int pen_color = cstk[SP].pencolor_ix;
    int fill_color = cstk[SP].fillcolor_ix;
    int depth = 0;
    int pen_style = 0;		/* not used */
    int area_fill = filled ? 20 : -1;
    double style_val = cstk[SP].style_val;
    int direction = 0;
    double angle = 0.0;
    int center_x, center_y, radius_x, radius_y;
    int start_x, start_y, end_x, end_y;

#if 0
    fprintf(stderr, "p %d %d\n", p.x, p.y);
#endif
    if (Rot == 0) {
	start_x = center_x = Scale * p.x;
/* FIXME - why do I need 2 * ??? */
	start_y = center_y = Scale * (2 * PB.UR.y - p.y);
	radius_x = Scale * rx;
	radius_y = Scale * ry;
    } else {
	start_x = center_x = Scale * (2 * PB.UR.x - p.y);
	start_y = center_y = Scale * p.x;
	radius_x = Scale * ry;
	radius_y = Scale * rx;
    }
    end_x = start_x + radius_x;
    end_y = start_y + radius_y;

    fprintf(Output_file,
	    "%d %d %d %d %d %d %d %d %d %.3f %d %.4f %d %d %d %d %d %d %d %d\n",
	    object_code, sub_type, line_style, thickness, pen_color,
	    fill_color, depth, pen_style, area_fill, style_val, direction,
	    angle, center_x, center_y, radius_x, radius_y, start_x,
	    start_y, end_x, end_y);
}

static void fig_polyline(point * A, int n)
{
    int object_code = 2;	/* always 2 for polyline */
    int sub_type = 1;		/* always 1 for polyline */
    int line_style = cstk[SP].line_style;	/* solid, dotted, dashed */
    int thickness = cstk[SP].penwidth;
    int pen_color = cstk[SP].pencolor_ix;
    int fill_color = 0;
    int depth = 0;
    int pen_style = 0;		/* not used */
    int area_fill = 0;
    double style_val = cstk[SP].style_val;
    int join_style = 0;
    int cap_style = 0;
    int radius = 0;
    int forward_arrow = 0;
    int backward_arrow = 0;
    int npoints = n;

    fprintf(Output_file,
	    "%d %d %d %d %d %d %d %d %d %.1f %d %d %d %d %d %d\n",
	    object_code, sub_type, line_style, thickness, pen_color,
	    fill_color, depth, pen_style, area_fill, style_val, join_style,
	    cap_style, radius, forward_arrow, backward_arrow, npoints);
    figptarray(A, n, 0);	/* open shape */
}

static void fig_user_shape(char *name, point * A, int n, int filled)
{
    static bool onetime = TRUE;
    if (onetime) {
	agerr(AGERR, "custom shapes not available with this driver\n");
	onetime = FALSE;
    }
}

codegen_t FIG_CodeGen = {
    fig_reset,
    fig_begin_job, fig_end_job,
    fig_begin_graph, 0,		/* fig_end_graph */
    fig_begin_page, 0,		/* fig_end_page */
    0, /* fig_begin_layer */ 0, /* fig_end_layer */
    0, /* fig_begin_cluster */ 0, /* fig_end_cluster */
    0, /* fig_begin_nodes */ 0,	/* fig_end_nodes */
    0, /* fig_begin_edges */ 0,	/* fig_end_edges */
    0, /* fig_begin_node */  0,	/* fig_end_node */
    0, /* fig_begin_edge */  0,	/* fig_end_edge */
    fig_begin_context, fig_end_context,
    0, /* fig_begin_anchor */ 0,	/* fig_end_anchor */
    fig_set_font, fig_textline,
    fig_set_pencolor, fig_set_fillcolor, fig_set_style,
    fig_ellipse, fig_polygon,
    fig_bezier, fig_polyline,
    0,				/* bezier_has_arrows */
    fig_comment,
    0,				/* fig_textsize */
    fig_user_shape,
    0				/* fig_usershapesize */
};
