
#ifndef __rclib_marshal_MARSHAL_H__
#define __rclib_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:INT64 (./rclib-marshal.list:1) */
extern void rclib_marshal_VOID__INT64 (GClosure     *closure,
                                       GValue       *return_value,
                                       guint         n_param_values,
                                       const GValue *param_values,
                                       gpointer      invocation_hint,
                                       gpointer      marshal_data);

/* VOID:POINTER,INT64 (./rclib-marshal.list:2) */
extern void rclib_marshal_VOID__POINTER_INT64 (GClosure     *closure,
                                               GValue       *return_value,
                                               guint         n_param_values,
                                               const GValue *param_values,
                                               gpointer      invocation_hint,
                                               gpointer      marshal_data);

/* VOID:POINTER,STRING (./rclib-marshal.list:3) */
extern void rclib_marshal_VOID__POINTER_STRING (GClosure     *closure,
                                                GValue       *return_value,
                                                guint         n_param_values,
                                                const GValue *param_values,
                                                gpointer      invocation_hint,
                                                gpointer      marshal_data);

/* VOID:STRING,POINTER (./rclib-marshal.list:4) */
extern void rclib_marshal_VOID__STRING_POINTER (GClosure     *closure,
                                                GValue       *return_value,
                                                guint         n_param_values,
                                                const GValue *param_values,
                                                gpointer      invocation_hint,
                                                gpointer      marshal_data);

/* VOID:POINTER,POINTER (./rclib-marshal.list:5) */
extern void rclib_marshal_VOID__POINTER_POINTER (GClosure     *closure,
                                                 GValue       *return_value,
                                                 guint         n_param_values,
                                                 const GValue *param_values,
                                                 gpointer      invocation_hint,
                                                 gpointer      marshal_data);

/* VOID:UINT,INT64,POINTER (./rclib-marshal.list:6) */
extern void rclib_marshal_VOID__UINT_INT64_POINTER (GClosure     *closure,
                                                    GValue       *return_value,
                                                    guint         n_param_values,
                                                    const GValue *param_values,
                                                    gpointer      invocation_hint,
                                                    gpointer      marshal_data);

/* VOID:UINT,UINT,FLOAT,POINTER (./rclib-marshal.list:7) */
extern void rclib_marshal_VOID__UINT_UINT_FLOAT_POINTER (GClosure     *closure,
                                                         GValue       *return_value,
                                                         guint         n_param_values,
                                                         const GValue *param_values,
                                                         gpointer      invocation_hint,
                                                         gpointer      marshal_data);

G_END_DECLS

#endif /* __rclib_marshal_MARSHAL_H__ */

