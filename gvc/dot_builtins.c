/* $Id: dot_builtins.c,v 1.1 2005/10/18 19:09:58 ellson Exp $ $Revision: 1.1 $ */
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

#include "gvplugin.h"

extern gvplugin_library_t *gvplugin_dot_layout_LTX_library;
extern gvplugin_library_t *gvplugin_neato_layout_LTX_library;
extern gvplugin_library_t *gvplugin_usershape_gd_LTX_library;

const lt_symlist_t lt_preloaded_symbols[] = {
	{ "gvplugin_dot_layout_LTX_library", (void*)(&gvplugin_dot_layout_LTX_library) },
	{ "gvplugin_neato_layout_LTX_library", (void*)(&gvplugin_neato_layout_LTX_library) },
	{ "gvplugin_usershape_gd_LTX_library", (void*)(&gvplugin_usershape_gd_LTX_library) },
	{ 0, 0 }
};
