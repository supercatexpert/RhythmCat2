/*
 * RhythmCat Library String Module
 * Reference countable string for the library.
 *
 * rclib-string.c
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

#include "rclib-string.h"

/**
 * SECTION: rclib-string
 * @Short_description: Reference countable string
 * @Title: Refable String
 * @Include: rclib-string.h
 *
 * Simple reference countable string data structure.
 */

struct _RCLibString
{
    gint ref_count;
    GString *str;
};

GType rclib_string_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_boxed_type_register_static(
            g_intern_static_string("RCLibString"),
            (GBoxedCopyFunc)rclib_string_ref,
            (GBoxedFreeFunc)rclib_string_unref);
        g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

/**
 * rclib_string_new:
 * @init: the initial string
 * 
 * Create a #RCLibString data structure.
 *
 * Returns: (transfer full): The #RCLibString, #NULL if any errors occured.
 */
 
RCLibString *rclib_string_new(const gchar *init)
{
    RCLibString *str = g_new0(RCLibString, 1);
    if(init==NULL) init = "";
    str->str = g_string_new(init);
    str->ref_count = 1;
    return str;
}

/**
 * rclib_string_free:
 * @str: the #RCLibString structure
 * 
 * Free the #RCLibString data structure.
 */

void rclib_string_free(RCLibString *str)
{
    if(str==NULL) return;
    if(str->str!=NULL)
    {
        g_string_free(str->str, TRUE);
        str->str = NULL;
    }
    str->ref_count = 0;
    g_free(str);
}

/**
 * rclib_string_ref:
 * @str: the #RCLibString structure
 * 
 * Increase the reference count of #RCLibString by 1.
 *
 * Returns: (transfer full): The #RCLibString.
 */

RCLibString *rclib_string_ref(RCLibString *str)
{
    if(str==NULL) return NULL;
    g_atomic_int_add(&(str->ref_count), 1);
    return str;
}


/**
 * rclib_string_unref:
 * @str: the #RCLibString structure
 *
 * Decrease the reference of #RCLibString by 1.
 * If the reference down to zero, the structure will be freed.
 */

void rclib_string_unref(RCLibString *str)
{
    if(str==NULL) return;
    if(g_atomic_int_dec_and_test(&(str->ref_count)))
        rclib_string_free(str);
}

/**
 * rclib_string_set:
 * @str: the #RCLibString structure
 * @text: the C string to set
 * 
 * Set the @text to @str, replace the string inside @str if exists.
 */

void rclib_string_set(RCLibString *str, const gchar *text)
{
    if(str==NULL) return;
    if(text==NULL) text = "";
    (void)g_string_assign(str->str, text);
}

/**
 * rclib_string_get:
 * @str: the #RCLibString structure
 * 
 * Get the C string inside @str
 * 
 * Returns: The C string inside @str, do not free or modify it.
 */

const gchar *rclib_string_get(const RCLibString *str)
{
    if(str==NULL) return NULL;
    return str->str->str;
}

/**
 * rclib_string_dup:
 * @str: the #RCLibString structure
 * 
 * Duplicate the C string inside @str
 * 
 * Returns: The C string deplicated from @str, use #g_free() to free it.
 */

gchar *rclib_string_dup(RCLibString *str)
{
    if(str==NULL) return NULL;
    return g_strdup(str->str->str);
}

/**
 * rclib_string_length:
 * @str: the #RCLibString structure
 * 
 * Get the string length inside @str
 * 
 * Returns: The string length.
 */

gsize rclib_string_length(RCLibString *str)
{
    if(str==NULL) return 0;
    return str->str->len;
}

/**
 * rclib_string_printf:
 * @str: the #RCLibString structure
 * @format: the string format. See the printf() documentation
 * @...: the parameters to insert into the format string
 * 
 * Write a formatted string into @str, just similar to the standard sprintf()
 * function.
 */

void rclib_string_printf(RCLibString *str, const gchar *format, ...)
{
    va_list valist;
    if(str==NULL || format==NULL) return;
    va_start(valist, format);
    g_string_vprintf(str->str, format, valist);
    va_end(valist);
}

/**
 * rclib_string_vprintf:
 * @str: the #RCLibString structure
 * @format: the string format. See the printf() documentation
 * @args: the list of arguments to insert in the output
 * 
 * Write a formatted string into @str, just similar to the standard vsprintf()
 * function.
 */

void rclib_string_vprintf(RCLibString *str, const gchar *format, va_list args)
{
    if(str==NULL || format==NULL) return;
    g_string_vprintf(str->str, format, args);
}

/**
 * rclib_string_append:
 * @str: the #RCLibString structure
 * @text: the text to append
 * 
 * Append @text to @str.
 */

void rclib_string_append(RCLibString *str, const gchar *text)
{
    if(str==NULL || text==NULL) return;
    g_string_append(str->str, text);
}

/**
 * rclib_string_append_len:
 * @str: the #RCLibString structure
 * @text: the text to append
 * @len: number of bytes of @text to use
 * 
 * Append @len bytes of @text to @str.
 */

void rclib_string_append_len(RCLibString *str, const gchar *text, gssize len)
{
    if(str==NULL || text==NULL) return;
    g_string_append_len(str->str, text, len);
}

/**
 * rclib_string_append_printf:
 * @str: the #RCLibString structure
 * @format: the string format. See the printf() documentation
 * @...: the parameters to insert into the format string
 * 
 * Appends a formatted string onto the end of @str, just similar to
 * #rclib_string_printf() except that the text is appended to the @str.
 */

void rclib_string_append_printf(RCLibString *str, const gchar *format, ...)
{
    va_list valist;
    if(str==NULL || format==NULL) return;
    va_start(valist, format);
    g_string_append_vprintf(str->str, format, valist);
    va_end(valist);
}

/**
 * rclib_string_append_vprintf:
 * @str: the #RCLibString structure
 * @format: the string format. See the printf() documentation
 * @args: the list of arguments to insert in the output
 * 
 * Appends a formatted string onto the end of @str, just similar to
 * #rclib_string_printf() except that the text is appended to the @str.
 */

void rclib_string_append_vprintf(RCLibString *str, const gchar *format,
    va_list args)
{
    if(str==NULL || format==NULL) return;
    g_string_append_vprintf(str->str, format, args);
}

/**
 * rclib_string_prepend:
 * @str: the #RCLibString structure
 * @text: the text to prepend
 * 
 * Add @text on to the start of @str.
 */

void rclib_string_prepend(RCLibString *str, const gchar *text)
{
    if(str==NULL || text==NULL) return;
    g_string_prepend(str->str, text);
}

/**
 * rclib_string_prepend_len:
 * @str: the #RCLibString structure
 * @text: the text to prepend
 * @len: number of bytes of @text to use
 * 
 * Add @len bytes of @text on to the start of @str.
 */

void rclib_string_prepend_len(RCLibString *str, const gchar *text, gssize len)
{
    if(str==NULL || text==NULL) return;
    g_string_prepend_len(str->str, text, len);
}

/**
 * rclib_string_insert:
 * @str: the #RCLibString structure
 * @pos: the position to insert the copy of the string
 * @text: the text to insert
 * 
 * Inserts a copy of a string into @str at @pos, expanding it if necessary.
 */

void rclib_string_insert(RCLibString *str, gssize pos, const gchar *text)
{
    if(str==NULL || text==NULL) return;
    g_string_insert(str->str, pos, text);
}

/**
 * rclib_string_insert_len:
 * @str: the #RCLibString structure
 * @pos: the position to insert the copy of the string
 * @text: the text to insert
 * @len: number of bytes of @text to insert
 * 
 * Inserts @len bytes of @text into @str at @pos, expanding it if necessary.
 */

void rclib_string_insert_len(RCLibString *str, gssize pos, const gchar *text,
    gssize len)
{
    if(str==NULL || text==NULL) return;
    g_string_insert_len(str->str, pos, text, len);
}

/**
 * rclib_string_overwrite:
 * @str: the #RCLibString structure
 * @pos: the position at which to start overwriting
 * @text: the text for overwriting
 * 
 * Overwrites part of a string, lengthening it if necessary.
 */

void rclib_string_overwrite(RCLibString *str, gsize pos, const gchar *text)
{
    if(str==NULL || text==NULL) return;
    g_string_overwrite(str->str, pos, text);
}

/**
 * rclib_string_overwrite_len:
 * @str: the #RCLibString structure
 * @pos: the position at which to start overwriting
 * @text: the text for overwriting
 * @len: the number of bytes to write from @text
 * 
 * Overwrites part of a string, lengthening it if necessary. 
 */

void rclib_string_overwrite_len(RCLibString *str, gsize pos, const gchar *text,
    gssize len)
{
    if(str==NULL || text==NULL) return;
    g_string_overwrite_len(str->str, pos, text, len);
}

/**
 * rclib_string_erase:
 * @str: the #RCLibString structure
 * @pos: the position of the content to remove
 * @len: the number of bytes to remove, or -1 to remove all following bytes
 * 
 * Removes @len bytes from @str, starting at position @pos. The rest of
 *     the @str is shifted down to fill the gap.
 */

void rclib_string_erase(RCLibString *str, gssize pos, gssize len)
{
    if(str==NULL) return;
    g_string_erase(str->str, pos, len);
}

/**
 * rclib_string_truncate:
 * @str: the #RCLibString structure
 * @len: the new size of @str
 * 
 * Cuts off the end of the @str, leaving the first @len bytes.
 */

void rclib_string_truncate(RCLibString *str, gsize len)
{
    if(str==NULL) return;
    g_string_truncate(str->str, len);
}

/**
 * rclib_string_hash:
 * @str: the #RCLibString structure
 * 
 * Creates a hash code for @str; for use with #GHashTable.
 * 
 * Returns: Hash code for @str.
 */

guint rclib_string_hash(const RCLibString *str)
{
    if(str==NULL) return 0;
    return g_string_hash(str->str);
}

/**
 * rclib_string_equal:
 * @str1: the first #RCLibString
 * @str2: the second #RCLibString
 * 
 * Compares two strings for equality, returning TRUE if they are equal.
 *     For use with #GHashTable.
 * 
 * Returns: #TRUE if they strings are the same length and contain
 *     the same bytes.
 */

gboolean rclib_string_equal(const RCLibString *str1, const RCLibString *str2)
{
    if(str1==NULL || str2==NULL) return FALSE;
    return g_string_equal(str1->str, str2->str);
}
