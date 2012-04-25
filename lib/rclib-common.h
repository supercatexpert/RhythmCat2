/*
 * RhythmCat Library Common Header Declaration
 *
 * rclib-common.h
 * This file is part of RhythmCat Library (LibRhythmCat)
 *
 * Copyright (C) 2012 - SuperCat, license: GPL v3
 *
 * RhythmCat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * RhythmCat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RhythmCat; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#ifndef HAVE_RCLIB_COMMON_H
#define HAVE_RCLIB_COMMON_H

#include <glib.h>
#include <glib/gi18n.h>

G_BEGIN_DECLS

#ifdef G_LOG_DOMAIN
#undef G_LOG_DOMAIN
#endif

#define G_LOG_DOMAIN "LibRhythmCat2"
#define GETTEXT_PACKAGE "RhythmCat2"
#define PACKAGE "RhythmCat2"

G_END_DECLS

#endif

