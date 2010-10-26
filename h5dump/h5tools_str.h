/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  Project                 : vtkCSCS                                        *
 *  Module                  : h5dump.h                                       *
 *  Revision of last commit : $Rev: 1460 $                                   *
 *  Author of last commit   : $Author: soumagne $                            *
 *  Date of last commit     : $Date:: 2009-12-02 18:38:09 +0100 #$           *
 *                                                                           *
 *  Copyright (C) CSCS - Swiss National Supercomputing Centre.               *
 *  You may use modify and and distribute this code freely providing         *
 *  1) This copyright notice appears on all copies of source code            *
 *  2) An acknowledgment appears with any substantial usage of the code      *
 *  3) If this code is contributed to any other open source project, it      *
 *  must not be reformatted such that the indentation, bracketing or         *
 *  overall style is modified significantly.                                 *
 *                                                                           *
 *  This software is distributed WITHOUT ANY WARRANTY; without even the      *
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Programmer:  Bill Wendling <wendling@ncsa.uiuc.edu>
 *              Monday, 19. February 2001
 */
#ifndef H5TOOLS_STR_H__
#define H5TOOLS_STR_H__

#include "XdmfGeneratorconfig.h"

typedef struct h5tools_str_t {
    char    *s;     /*allocate string       */
    size_t  len;        /*length of actual value    */
    size_t  nalloc;     /*allocated size of string  */
} h5tools_str_t;

extern void     h5tools_str_close(h5tools_str_t *str);
extern size_t   h5tools_str_len(h5tools_str_t *str);
extern char    *h5tools_str_append(h5tools_str_t *str, const char *fmt, ...);
extern char    *h5tools_str_reset(h5tools_str_t *str);
extern char    *h5tools_str_trunc(h5tools_str_t *str, size_t size);
extern char    *h5tools_str_fmt(h5tools_str_t *str, size_t start, const char *fmt);
extern char    *h5tools_str_prefix(h5tools_str_t *str, const h5tool_format_t *info,
                                   hsize_t elmtno, unsigned ndims, hsize_t min_idx[],
                                   hsize_t max_idx[], h5tools_context_t *ctx);
/* 
 * new functions needed to display region reference data
 */
extern char    *h5tools_str_region_prefix(h5tools_str_t *str, const h5tool_format_t *info,
                                   hsize_t elmtno, hsize_t *ptdata, unsigned ndims, hsize_t min_idx[],
                                   hsize_t max_idx[], h5tools_context_t *ctx);
extern void     h5tools_str_dump_region_blocks(h5tools_str_t *, hid_t, const h5tool_format_t *,
                                   h5tools_context_t *ctx);
extern void     h5tools_str_dump_region_points(h5tools_str_t *, hid_t, const h5tool_format_t *,
                                   h5tools_context_t *ctx);
extern void     h5tools_str_sprint_region(h5tools_str_t *str, const h5tool_format_t *info, hid_t container,
                                   void *vp, h5tools_context_t *ctx);
extern char    *h5tools_str_sprint(h5tools_str_t *str, const h5tool_format_t *info,
                                   hid_t container, hid_t type, void *vp,
                                   h5tools_context_t *ctx);

#endif  /* H5TOOLS_STR_H__ */
