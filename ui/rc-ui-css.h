/*
 * RhythmCat UI CSS Data Declaration
 *
 * rc-ui-css.h
 * This file is part of RhythmCat Music Player (GTK+ Version)
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

#ifndef HAVE_RC_UI_CSS_H
#define HAVE_RC_UI_CSS_H

gchar rc_ui_css_default[] = 
    "@define-color theme_base_color #4f524f;"
    "@define-color theme_bg_color #363b3b;"
    "GtkWindow#RC2MainWindow.background\n"
    "{\n"
    "    background-color: @theme_bg_color;\n"
    "    background-image: -gtk-gradient (linear, left top, right bottom,\n"
    "        from (@theme_bg_color), to (@theme_bg_color));\n"
    "}\n"
    "GtkWindow#RC2MainWindow GtkLabel#RC2TitleLabel,\n"
    "GtkWindow#RC2MainWindow GtkLabel#RC2ArtistLabel,\n"
    "GtkWindow#RC2MainWindow GtkLabel#RC2AlbumLabel,\n"
    "GtkWindow#RC2MainWindow GtkLabel#RC2InfoLabel,\n"
    "GtkWindow#RC2MainWindow GtkLabel#RC2LengthLabel,\n"
    "GtkWindow#RC2MiniWindow GtkLabel\n"
    "{\n"
    "    color: #F0F0F0;\n"
    "    font: Cantarell, Dejavu Sans, Wenquanyi Microhei, Wenquanyi Zenhei\n"
    "}\n"
    "GtkWindow#RC2MainWindow GtkLabel#RC2TimeLabel\n"
    "{\n"
    "    color: #6CD02F;\n"
    "    font: Cantarell, Dejavu Sans, Wenquanyi Microhei, Wenquanyi Zenhei\n"
    "}\n"
    "GtkWindow#RC2MainWindow RCUiScrollableLabel#RC2Lyric1ScrollableLabel,\n"
    "GtkWindow#RC2MainWindow RCUiScrollableLabel#RC2Lyric2ScrollableLabel\n"
    "{\n"
    "    background-color: @theme_bg_color;\n"
    "    background-image: -gtk-gradient (linear, left top, right bottom,\n"
    "        from (@theme_bg_color), to (@theme_bg_color));\n"
    "    color: #F0F0F0;\n"
    "    font: Cantarell, Dejavu Sans, Wenquanyi Microhei, Wenquanyi Zenhei\n"
    "}\n"
    "GtkWindow#RC2MainWindow RCUiSpectrumWidget#RC2SpectrumWidget\n"
    "{\n"
    "    background-color: @theme_bg_color;\n"
    "    background-image: -gtk-gradient (linear, left top, right bottom,\n"
    "        from (@theme_bg_color), to (@theme_bg_color));\n"
    "    color: #00A0F0;\n"
    "}\n"
    "";

#endif

