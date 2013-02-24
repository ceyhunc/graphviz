/* $Id: gvplugin_device.h,v 1.6 2005/10/18 18:47:22 ellson Exp $ $Revision: 1.6 $ */
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

#ifndef GVDEVICE_PLUGIN_H
#define GVDEVICE_PLUGIN_H

#include "gvplugin.h"
#include "gvcjob.h"

#ifdef __cplusplus
extern "C" {
#endif

    struct gvdevice_engine_s {
	void (*finalize) (GVJ_t * firstjob);
    };

#ifdef __cplusplus
}
#endif
#endif				/* GVDEVICE_PLUGIN_H */
