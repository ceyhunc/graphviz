/* $Id: psgen.c,v 1.36 2006/01/16 19:45:36 ellson Exp $ $Revision: 1.36 $ */
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

/*
    J$: added `pdfmark' URL embedding.  PostScript rendered from
        dot files with URL attributes will get active PDF links
        from Adobe's Distiller.
 */
#define	PDFMAX	14400		/*  Maximum size of PDF page  */

#include "render.h"
#include "ps.h"
#include "agxbuf.h"
#include "gd.h"

#ifndef MSWIN32
#include <unistd.h>
#endif

#include <sys/stat.h>

extern void epsf_define(FILE * of);
void epsf_emit_body(ps_image_t *img, FILE *of);
extern void ps_freeusershapes(void);
extern ps_image_t *ps_usershape(char *shapeimagefile);

extern gdImagePtr gd_getshapeimage(char *name);

static int N_pages, Cur_page;
/* static 	point	Pages; */
static box PB;
static int onetime = TRUE;
static int isLatin1;
static char setupLatin1;
static node_t *Curnode;		/* for user shapes */

static char *Fill = "fill\n";
static char *Stroke = "stroke\n";
static char *Newpath_Moveto = "newpath %d %d moveto\n";
static char **U_lib;

typedef struct grcontext_t {
    char *pencolor, *fillcolor, *font;
    char invis;
    double size;
} grcontext_t;

#define STACKSIZE 8
static grcontext_t S[STACKSIZE];
static int SP = 0;

static void ps_reset(void)
{
    onetime = TRUE;
}

static void
ps_begin_job(FILE * ofp, graph_t * g, char **lib, char *user, char *info[],
	     point pages)
{
    /* Pages = pages; */
    U_lib = lib;
    /* wrong when drawing more than one than one graph - use (atend) */
    N_pages = pages.x * pages.y;
    if (onetime) {
        Cur_page = 0;
        fprintf(Output_file, "%%!PS-Adobe-2.0\n");
        fprintf(Output_file, "%%%%Creator: %s version %s (%s)\n",
	        info[0], info[1], info[2]);
        fprintf(Output_file, "%%%%For: %s\n", user);
        fprintf(Output_file, "%%%%Title: %s\n", g->name);
        fprintf(Output_file, "%%%%Pages: (atend)\n");
    }

    /* remainder is emitted by first begin_graph */
}

static void ps_end_job(void)
{
    fprintf(Output_file, "%%%%Trailer\n");
    fprintf(Output_file, "%%%%Pages: %d\n", Cur_page);
    fprintf(Output_file, "end\nrestore\n");
    fprintf(Output_file, "%%%%EOF\n");
    setupLatin1 = FALSE;
}

static void ps_comment(char *str)
{
    fprintf(Output_file, "%% %s\n", str);
}

static void ps_begin_graph(GVC_t * gvc, graph_t * g, box bb, point pb)
{
    char *s;

    PB = bb;
    if (onetime) {
	if (Show_boxes == NULL)
	    fprintf(Output_file, "%%%%BoundingBox: %d %d %d %d\n",
		PB.LL.x, PB.LL.y, PB.UR.x, PB.UR.y);
	fprintf(Output_file, "%%%%EndComments\nsave\n");
	cat_libfile(Output_file, U_lib, ps_txt);
	epsf_define(Output_file);
 	if (Show_boxes) {
	    char* args[2];
	    args[0] = Show_boxes[0];
	    args[1] = NULL;
	    cat_libfile(Output_file, NULL, args);
        }

	/*  Set base URL for relative links (for Distiller >= 3.0)  */
	if (((s = agget(g, "href")) && s[0])
	    || ((s = agget(g, "URL")) && s[0])) {
	    fprintf(Output_file,
		    "[ {Catalog} << /URI << /Base (%s) >> >>\n"
		    "/PUT pdfmark\n", s);
	}
    }
    
    isLatin1 = (GD_charset(g) == CHAR_LATIN1);
    if (isLatin1 && !setupLatin1) {
	fprintf(Output_file, "setupLatin1\n");	/* as defined in ps header */
	setupLatin1 = TRUE;
    }
}

static void ps_end_graph(void)
{
    ps_freeusershapes();
    onetime = FALSE;
}

static void
ps_begin_page(graph_t * g, point page, double scale, int rot, point offset)
{
    point sz;

    Cur_page++;
    sz = sub_points(PB.UR,PB.LL);
    fprintf(Output_file, "%%%%Page: %d %d\n", Cur_page, Cur_page);
    if (Show_boxes == NULL) {
	if (rot)
	    fprintf(Output_file, "%%%%PageBoundingBox: %d %d %d %d\n",
	        PB.LL.y, PB.LL.x, PB.UR.y, PB.UR.x);
	else
	    fprintf(Output_file, "%%%%PageBoundingBox: %d %d %d %d\n",
	        PB.LL.x, PB.LL.y, PB.UR.x, PB.UR.y);
    }
    fprintf(Output_file, "%%%%PageOrientation: %s\n",
	    (rot ? "Landscape" : "Portrait"));
    if (Output_lang == PDF)
	fprintf(Output_file, "<< /PageSize [%d %d] >> setpagedevice\n",
	    sz.x, sz.y);
    if (Show_boxes == NULL) {
	if (rot)
	    fprintf(Output_file, "gsave\n%d %d %d %d boxprim clip newpath\n",
	        PB.LL.x, PB.LL.y, sz.y, sz.x);
	else
	    fprintf(Output_file, "gsave\n%d %d %d %d boxprim clip newpath\n",
	        PB.LL.x, PB.LL.y, sz.x, sz.y);
    }
    fprintf(Output_file, "%d %d translate\n", PB.LL.x, PB.LL.y);
    if (rot)
        fprintf(Output_file, "gsave %d %d translate %d rotate\n",
                PB.UR.x - PB.LL.x, 0, rot);
    fprintf(Output_file, "%d %d %d beginpage\n", page.x, page.y, N_pages);
    if (rot)
        fprintf(Output_file, "grestore\n");
    fprintf(Output_file, "%.4f set_scale\n", scale);
    fprintf(Output_file, "%d %d translate %d rotate\n",
	    offset.x, offset.y, rot);
    assert(SP == 0);
    S[SP].font = S[SP].pencolor = S[SP].fillcolor = "";
    S[SP].size = 0.0;

    /*  Define the size of the PS canvas  */
    if (Output_lang == PDF) {
	if (sz.x > PDFMAX || sz.y > PDFMAX)
	    agerr(AGWARN,
		  "canvas size (%d,%d) exceeds PDF limit (%d)\n"
		  "\t(suggest setting a bounding box size, see dot(1))\n",
		  sz.x, sz.y, PDFMAX);
	fprintf(Output_file, "[ /CropBox [%d %d %d %d] /PAGE pdfmark\n",
		PB.LL.x, PB.LL.y, PB.UR.x, PB.UR.y);
    }
}

static void ps_end_page(void)
{
    if (Show_boxes)
	cat_libfile(Output_file, NULL, Show_boxes+1);

    /* the showpage is really a no-op, but at least one PS processor
     * out there needs to see this literal token.  endpage does the real work.
     */
    fprintf(Output_file, "endpage\nshowpage\ngrestore\n");
    fprintf(Output_file, "%%%%PageTrailer\n");
    fprintf(Output_file, "%%%%EndPage: %d\n", Cur_page);
    assert(SP == 0);
}

static void ps_begin_layer(char *s, int n, int Nlayers)
{
    fprintf(Output_file, "%d %d setlayer\n", n, Nlayers);
}

static void ps_begin_cluster(graph_t * g)
{
    fprintf(Output_file, "%% %s\n", g->name);

    /*  Embed information for Distiller to generate hyperlinked PDF  */
    map_begin_cluster(g);
}

static void ps_begin_node(node_t * n)
{
    Curnode = n;

    /*  Embed information for Distiller to generate hyperlinked PDF  */
    map_begin_node(n);
}

static void ps_begin_edge(edge_t * e)
{
    /*  Embed information for Distiller, so it can generate hyperactive PDF  */
    map_begin_edge(e);
}


static void ps_begin_context(void)
{
    fprintf(Output_file, "gsave 10 dict begin\n");
    if (SP == STACKSIZE - 1)
	agerr(AGWARN, "psgen stk ovfl\n");
    else {
	SP++;
	S[SP] = S[SP - 1];
    }
}

static void ps_end_context(void)
{
    if (SP == 0)
	agerr(AGWARN, "psgen stk undfl\n");
    else
	SP--;
    fprintf(Output_file, "end grestore\n");
}

static void ps_set_font(char *name, double size)
{
    if (strcmp(S[SP].font, name) || (size != S[SP].size)) {
	fprintf(Output_file, "%.2f /%s set_font\n", size, name);
	S[SP].font = name;
	S[SP].size = size;
    }
}

static void ps_set_color(char *name)
{
    static char *op[] = { "graph", "node", "edge", "sethsb" };
    gvcolor_t color;

    colorxlate(name, &color, HSV_DOUBLE);
    fprintf(Output_file, "%.3f %.3f %.3f %scolor\n",
	    color.u.HSV[0], color.u.HSV[1], color.u.HSV[2], op[Obj]);
}

static void ps_set_pencolor(char *name)
{
    if (streq(name, "transparent"))
	S[SP].pencolor = "";
    else if (strcmp(name, S[SP].pencolor)) {
	ps_set_color(name);	/* change pen color immediately */
	S[SP].pencolor = name;
    }
}

static void ps_set_fillcolor(char *name)
{
    /* defer changes to fill color to shape */
    if (streq(name, "transparent"))
	S[SP].fillcolor = "";
    else
	S[SP].fillcolor = name;
}

static void ps_set_style(char **s)
{
    char *line, *p;

    while ((p = line = *s++)) {
	while (*p)
	    p++;
	p++;
	while (*p) {
	    fprintf(Output_file, "%s ", p);
	    while (*p)
		p++;
	    p++;
	}
	if (streq(line, "invis"))
	    S[SP].invis = TRUE;
	fprintf(Output_file, "%s\n", line);
    }
}

static unsigned char psbuf[BUFSIZ];

char *ps_string(char *ins, int latin)
{
    char *s;
    char *base;
    static agxbuf  xb;

    if (latin)
	base = utf8ToLatin1 (ins);
    else
	base = ins;

    if (xb.buf == NULL)
	agxbinit (&xb, BUFSIZ, psbuf);

    agxbputc (&xb, LPAREN);
    s = base;
    while (*s) {
	if ((*s == LPAREN) || (*s == RPAREN) || (*s == '\\')) {
	    agxbputc (&xb, '\\');
	}
	agxbputc (&xb, *s++);
    }
    agxbputc (&xb, RPAREN);
    if (base != ins) free (base);
    return agxbuse(&xb);
}

static void ps_textline(point p, textline_t * line)
{
    double adj;

    if (S[SP].invis)
	return;
    if (*S[SP].pencolor == '\0')
	return;
    if (line->xshow) {
	switch (line->just) {
	case 'l':
	    break;
	case 'r':
	    p.x -= line->width;
	    break;
	default:
	case 'n':
	    p.x -= line->width / 2;
	    break;
	}
	fprintf(Output_file, "%d %d moveto\n%s\n[%s]\nxshow\n",
		p.x, p.y, ps_string(line->str,isLatin1), line->xshow);
    } else {
	switch (line->just) {
	case 'l':
	    adj = 0.0;
	    break;
	case 'r':
	    adj = -1.0;
	    break;
	default:
	case 'n':
	    adj = -0.5;
	    break;
	}
	fprintf(Output_file, "%d %d moveto %.1f %.1f %s alignedtext\n",
		p.x, p.y, line->width, adj, ps_string(line->str,isLatin1));
    }
}

static void
ps_bezier(point * A, int n, int arrow_at_start, int arrow_at_end, int filled)
{
    int j;
    if (S[SP].invis)
	return;
    if (filled && *S[SP].fillcolor) {
	ps_set_color(S[SP].fillcolor);
	fprintf(Output_file, Newpath_Moveto, A[0].x, A[0].y);
	for (j = 1; j < n; j += 3)
	    fprintf(Output_file, "%d %d %d %d %d %d curveto\n",
		A[j].x, A[j].y, A[j + 1].x, A[j + 1].y, A[j + 2].x,
		A[j + 2].y);
	fprintf(Output_file, "closepath\n");
	fprintf(Output_file, Fill);
	if (*S[SP].pencolor)
	    ps_set_color(S[SP].pencolor);
    }
    if (*S[SP].pencolor == '\0')
	return;
    if (arrow_at_start || arrow_at_end)
	agerr(AGERR, "ps_bezier illegal arrow args\n");
    fprintf(Output_file, Newpath_Moveto, A[0].x, A[0].y);
    for (j = 1; j < n; j += 3)
	fprintf(Output_file, "%d %d %d %d %d %d curveto\n",
		A[j].x, A[j].y, A[j + 1].x, A[j + 1].y, A[j + 2].x,
		A[j + 2].y);
    fprintf(Output_file, Stroke);
}

static void ps_polygon(point * A, int n, int filled)
{
    int j;

    if (S[SP].invis)
	return;
    if (filled && *S[SP].fillcolor) {
	ps_set_color(S[SP].fillcolor);
	fprintf(Output_file, Newpath_Moveto, A[0].x, A[0].y);
	for (j = 1; j < n; j++)
	    fprintf(Output_file, "%d %d lineto\n", A[j].x, A[j].y);
	fprintf(Output_file, "closepath\n");
	fprintf(Output_file, Fill);
	if (*S[SP].pencolor)
	    ps_set_color(S[SP].pencolor);
    }
    if (*S[SP].pencolor == '\0')
	return;
    fprintf(Output_file, Newpath_Moveto, A[0].x, A[0].y);
    for (j = 1; j < n; j++)
	fprintf(Output_file, "%d %d lineto\n", A[j].x, A[j].y);
    fprintf(Output_file, "closepath\n");
    fprintf(Output_file, Stroke);
}

static void ps_ellipse(point p, int rx, int ry, int filled)
{
    if (S[SP].invis)
	return;
    if (filled && *S[SP].fillcolor) {
	ps_set_color(S[SP].fillcolor);
	fprintf(Output_file, "%d %d %d %d ellipse_path\n", p.x, p.y, rx,
		ry);
	fprintf(Output_file, Fill);
	if (*S[SP].pencolor)
	    ps_set_color(S[SP].pencolor);
    }
    if (*S[SP].pencolor == '\0')
	return;
    if (!filled || (filled && strcmp(S[SP].fillcolor, S[SP].pencolor))) {
	fprintf(Output_file, "%d %d %d %d ellipse_path\n", p.x, p.y, rx,
		ry);
	fprintf(Output_file, Stroke);
    }
}

static void ps_polyline(point * A, int n)
{
    int j;

    if (S[SP].invis)
	return;
    if (*S[SP].pencolor == '\0')
	return;
    fprintf(Output_file, Newpath_Moveto, A[0].x, A[0].y);
    for (j = 1; j < n; j++)
	fprintf(Output_file, "%d %d lineto\n", A[j].x, A[j].y);
    fprintf(Output_file, Stroke);
}

static void writePSBitmap (gdImagePtr im, point p, point sz)
{
    int x, y, px;

    fprintf(Output_file, "gsave\n");

    /* this sets the position of the image */
    fprintf(Output_file, "%d %d translate %% lower-left coordinate\n", p.x, p.y);

    /* this sets the rendered size, from 'pixels' to points (1/72 inch) */
    fprintf(Output_file,"%d %d scale\n", sz.x, sz.y);

    /* xsize ysize bits-per-sample [matrix] */
    fprintf(Output_file, "%d %d 8 [%d 0 0 %d 0 %d]\n", im->sx, im->sy, 
			im->sx, -(im->sy), im->sy);

    fprintf(Output_file, "{<\n");
    for (y = 0; y < im->sy; y++) {
	for (x = 0; x < im->sx; x++) {
	    if (im->trueColor) {
		px = gdImageTrueColorPixel(im, x, y);
		fprintf(Output_file, "%02x%02x%02x",
		    gdTrueColorGetRed(px),
		    gdTrueColorGetGreen(px),
		    gdTrueColorGetBlue(px));
	    }
	    else {
		px = gdImagePalettePixel(im, x, y);
		fprintf(Output_file, "%02x%02x%02x",
		    im->red[px],
		    im->green[px],
		    im->blue[px]);
	    }
	}
	fprintf(Output_file, "\n");
    }

    fprintf(Output_file, ">}\n");
    fprintf(Output_file, "false 3 colorimage\n");

    fprintf(Output_file, "grestore\n");

}

/* ps_user_shape:
 * Images for postscript are complicated by the old epsf shape, as
 * well as user-defined shapes using postscript code.
 * If the name is custom, we look for the image stored in the
 * current node's shapefile attribute.
 * Else we see if name is a user-defined postscript function
 * Else we assume name is the name of the image. This occurs when
 * the image is part of an html label.
 */
static void ps_user_shape(char *name, point * A, int sides, int filled)
{
    int j;
    gdImagePtr bmimg; 
    point offset;
    char *shapeimagefile = NULL;
    char *suffix;

    if (S[SP].invis)
	return;
    if (streq(name, "custom")) {
	shapeimagefile = agget(Curnode, "shapefile");
    }
    else if (find_user_shape(name)) {
	if (filled) {
	    ps_begin_context();
	    ps_set_color(S[SP].fillcolor);
	    fprintf(Output_file, "[ ");
	    for (j = 0; j < sides; j++)
		fprintf(Output_file, "%d %d ", A[j].x, A[j].y);
	    fprintf(Output_file, "%d %d ", A[0].x, A[0].y);
	    fprintf(Output_file, "]  %d true %s\n", sides, name);
	    ps_end_context();
	}
	fprintf(Output_file, "[ ");
	for (j = 0; j < sides; j++)
	    fprintf(Output_file, "%d %d ", A[j].x, A[j].y);
	fprintf(Output_file, "%d %d ", A[0].x, A[0].y);
	fprintf(Output_file, "]  %d false %s\n", sides, name);
	return;
    }
    else
	shapeimagefile = name;

    assert (shapeimagefile);
    suffix = strrchr(shapeimagefile, '.');
    if (suffix) {
	suffix++;
	if (streq(suffix, "ps")) {
	    ps_image_t *img = 0;
	    img = ps_usershape(shapeimagefile);
	    if (!img) /* problems would have been reported by image_size */
		return; 
	    ps_begin_context();
	    offset.x = -img->origin.x - (img->size.x) / 2;
	    offset.y = -img->origin.y - (img->size.y) / 2;
	    fprintf(Output_file, "%d %d translate newpath\n",
		ND_coord_i(Curnode).x + offset.x,
		ND_coord_i(Curnode).y + offset.y);
	    if (img->must_inline) epsf_emit_body(img,Output_file);
	    else fprintf(Output_file,"user_shape_%d\n",img->macro_id);
	    ps_end_context();
	}
	else if ((bmimg = gd_getshapeimage(shapeimagefile))) {
	    point sz;
	    sz.x = A[0].x - A[2].x;
	    sz.y = A[0].y - A[2].y;
	    writePSBitmap (bmimg, A[2], sz);
	}
	else {  /* some other type of image */
	    agerr(AGERR, "image type \"%s\" of file %s unsupported in PostScript output\n",
		suffix, shapeimagefile);
	}
    }
    /* if !suffix, already reported by image_size */
}

codegen_t PS_CodeGen = {
    ps_reset,
    ps_begin_job, ps_end_job,
    ps_begin_graph, ps_end_graph,
    ps_begin_page, ps_end_page,
    ps_begin_layer, 0,		/* ps_end_layer */
    ps_begin_cluster, 0,	/* ps_end_cluster */
    0, /* ps_begin_nodes */ 0,	/* ps_end_nodes */
    0, /* ps_begin_edges */ 0,	/* ps_end_edges */
    ps_begin_node, 0,		/* ps_end_node */
    ps_begin_edge, 0,		/* ps_end_edge */
    ps_begin_context, ps_end_context,
    0, /* ps_begin_anchor */ 0,	/* ps_end_anchor */
    ps_set_font, ps_textline,
    ps_set_pencolor, ps_set_fillcolor, ps_set_style,
    ps_ellipse, ps_polygon,
    ps_bezier, ps_polyline,
    0,				/* bezier_has_arrows */
    ps_comment,
    0,				/* ps_textsize */
    ps_user_shape,
    0				/* usershapesize */
};
