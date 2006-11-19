/*
    This file is part of kdegames
    Copyright (c) 2006 kdegames Team

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/


#ifndef _LIBKOLF_EXPORT_H
#define _LIBKOLF_EXPORT_H

#include <kdemacros.h>

#ifdef Q_WS_WIN

#ifndef KOLFLIB_EXPORT
# ifdef MAKE_KOLFLIB_LIB
#  define KOLFLIB_EXPORT KDE_EXPORT
# else
#  define KOLFLIB_EXPORT KDE_IMPORT
# endif
#endif

#else // not windows

#define KOLFLIB_EXPORT KDE_EXPORT
#endif /* not windows */

#endif /* _LIBKOLF_EXPORT_H */
