/* $Id: gvtextlayout.c,v 1.13 2005/10/18 18:56:21 ellson Exp $ $Revision: 1.13 $ */
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
 *  textlayout engine wrapper
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include "const.h"
#include "types.h"
#include "gvplugin_textlayout.h"
#include "gvcproc.h"

int gvtextlayout_select(GVC_t * gvc, char *textlayout)
{
    gvplugin_available_t *plugin;
    gvplugin_installed_t *typeptr;

    plugin = gvplugin_load(gvc, API_textlayout, textlayout);
    if (plugin) {
	typeptr = plugin->typeptr;
	gvc->textlayout.engine = (gvtextlayout_engine_t *) (typeptr->engine);
	return GVRENDER_PLUGIN;  /* FIXME - need more suitable success code */
    }
    return NO_SUPPORT;
}

double gvtextlayout_width(GVC_t *gvc, textline_t *textline, char *fontname, double fontsize, char *fontpath)
{
    gvtextlayout_engine_t *gvte = gvc->textlayout.engine;

    if (gvte && gvte->width) {
	gvte->width(textline, fontname, fontsize, fontpath);
    }
    return textline->width;
}
