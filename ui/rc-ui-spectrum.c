/*
 * RhythmCat UI Spectrum Widget Module
 * A spectrum show widget in the player.
 *
 * rc-ui-spectrum.c
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
 
#include "rc-ui-spectrum.h"

/**
 * SECTION: rc-ui-spectrum
 * @Short_description: A spectrum show widget.
 * @Title: RCUiSpectrumWidget
 * @Include: rc-ui-slabel.h
 *
 * The spectrum show widget. It shows the spectrum gragh in the player.
 */

#define RC_UI_SPECTRUM_STYLE_PARAM_READABLE G_PARAM_READABLE | \
    G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB
    
#define RC_UI_SPECTRUM_BANDS 4096

typedef struct RCUiSpectrumChannel
{
    gfloat *input;
    gfloat *input_tmp;
    GstFFTF32Complex *freqdata;
    gfloat *spect_magnitude;
    GstFFTF32 *fft_ctx;
}RCUiSpectrumChannel;

struct _RCUiSpectrumWidgetPrivate
{
    guint fps;
    RCUiSpectrumStyle style;
    GstBuffer *buffer;
    GMutex buffer_mutex; 
    guint64 spectrum_num_frames;
    guint64 spectrum_num_fft;
    guint64 spectrum_accumulated_error;
    RCUiSpectrumChannel *spectrum_channel_data;
    guint spectrum_rate;
    gfloat spectrum_threshold;
    gfloat *spectrum_data;
    gfloat *spectrum_band_data;
    guint spectrum_data_size;
    guint32 *wave_vdata;
    guint wave_vdata_size;
    guint timeout_id;
    gulong buffer_probe_id;
};

static gpointer rc_ui_spectrum_widget_parent_class = NULL;

#define rc_ui_spectrum_draw_dot(_vd, _s, _x, _y, _st, _c)                 \
    G_STMT_START {                                                        \
    guint p = (_y * _st) + _x;                                            \
    if(p>=_s/sizeof(guint32)) break;                                      \
    _vd[p] = _c;                                                          \
} G_STMT_END

#define rc_ui_spectrum_draw_dot_aa(_vd, _s, _x, _y, _st, _c, _f, _bc)     \
    G_STMT_START {                                                        \
    guint32 _oc, _c1, _c2, _c3;                                           \
    guint p = (_y * _st) + _x;                                            \
    if(p>=_s/sizeof(guint32)) break;                                      \
    _oc = _vd[p];                                                         \
    if(_oc==_bc)                                                          \
    {                                                                     \
        _vd[p] = _c;                                                      \
        break;                                                            \
    }                                                                     \
    _c3 = (_oc & 0xff) + ((_c & 0xff) * _f);                              \
    _c3 = MIN(_c3, 255);                                                  \
    _c2 = ((_oc & 0xff00) >> 8) + (((_c & 0xff00) >> 8) * _f);            \
    _c2 = MIN(_c2, 255);                                                  \
    _c1 = ((_oc & 0xff0000) >> 16) + (((_c & 0xff0000) >> 16) * _f);      \
    _c1 = MIN(_c1, 255);                                                  \
    _vd[p] = (_c1 << 16) | (_c2 << 8) | _c3;                              \
} G_STMT_END

#define rc_ui_spectrum_draw_line(_vd, _s, _x1, _x2, _y1, _y2, _st, _c)    \
    G_STMT_START {                                                        \
    guint _i, _j, _x, _y;                                                 \
    gint _dx = _x2 - _x1, _dy = _y2 - _y1;                                \
    gfloat _f;                                                            \
    _j = abs (_dx) > abs (_dy) ? abs (_dx) : abs (_dy);                   \
    for (_i = 0; _i < _j; _i++)                                           \
    {                                                                     \
        _f = (gfloat) _i / (gfloat) _j;                                   \
        _x = _x1 + _dx * _f;                                              \
        _y = _y1 + _dy * _f;                                              \
        rc_ui_spectrum_draw_dot(_vd, _s, _x, _y, _st, _c);                \
    }                                                                     \
} G_STMT_END

#define rc_ui_spectrum_draw_line_aa(_vd, _s, _x1, _x2, _y1, _y2, _st, _c, \
    _bc) \
    G_STMT_START {                                                        \
    guint _i, _j, _x, _y;                                                 \
    gint _dx = _x2 - _x1, _dy = _y2 - _y1;                                \
    gfloat _f, _rx, _ry, _fx, _fy;                                        \
    _j = abs (_dx) > abs (_dy) ? abs (_dx) : abs (_dy);                   \
    for (_i = 0; _i < _j; _i++)                                           \
    {                                                                     \
        _f = (gfloat) _i / (gfloat) _j;                                   \
        _rx = _x1 + _dx * _f;                                             \
        _ry = _y1 + _dy * _f;                                             \
        _x = (guint)_rx;                                                  \
        _y = (guint)_ry;                                                  \
        _fx = _rx - (gfloat)_x;                                           \
        _fy = _ry - (gfloat)_y;                                           \
        _f = ((1.0 - _fx) + (1.0 - _fy)) / 2.0;                           \
        rc_ui_spectrum_draw_dot_aa(_vd, _s, _x, _y, _st, _c, _f, _bc);    \
        _f = (_fx + (1.0 - _fy)) / 2.0;                                   \
        rc_ui_spectrum_draw_dot_aa (_vd, _s, (_x + 1), _y, _st, _c, _f,   \
            _bc);                                                         \
        _f = ((1.0 - _fx) + _fy) / 2.0;                                   \
        rc_ui_spectrum_draw_dot_aa (_vd, _s, _x, (_y + 1), _st, _c, _f,   \
            _bc);                                                         \
        _f = (_fx + _fy) / 2.0;                                           \
        rc_ui_spectrum_draw_dot_aa (_vd, _s, (_x + 1), (_y + 1), _st,     \
            _c, _f, _bc);                                                      \
    }                                                                     \
} G_STMT_END

static inline void rc_ui_spectrum_flush(RCUiSpectrumWidgetPrivate *priv)
{
    priv->spectrum_num_frames = 0;
    priv->spectrum_num_fft = 0;
    priv->spectrum_accumulated_error = 0;
}

static inline void rc_ui_spectrum_alloc_channel_data(
    RCUiSpectrumWidgetPrivate *priv, guint bands)
{
    RCUiSpectrumChannel *cd;
    guint nfft = 2 * bands - 2;
    if(priv->spectrum_channel_data!=NULL) return;
    priv->spectrum_channel_data = g_new(RCUiSpectrumChannel, 1);
    cd = priv->spectrum_channel_data;
    cd->fft_ctx = gst_fft_f32_new(nfft, FALSE);
    cd->input = g_new0(gfloat, nfft);
    cd->input_tmp = g_new0(gfloat, nfft);
    cd->freqdata = g_new0(GstFFTF32Complex, bands);
    cd->spect_magnitude = g_new0(gfloat, bands);
}

static inline void rc_ui_spectrum_free_channel_data(
    RCUiSpectrumWidgetPrivate *priv)
{
    RCUiSpectrumChannel *cd;
    if(priv->spectrum_channel_data==NULL) return;
    cd = priv->spectrum_channel_data;
    if(cd->fft_ctx!=NULL)
        gst_fft_f32_free(cd->fft_ctx);
    g_free(cd->input);
    g_free(cd->input_tmp);
    g_free(cd->freqdata);
    g_free(cd->spect_magnitude);
    g_free(priv->spectrum_channel_data);
    priv->spectrum_channel_data = NULL;
}

static inline void rc_ui_spectrum_reset_state(
    RCUiSpectrumWidgetPrivate *priv)
{
    rc_ui_spectrum_free_channel_data(priv);
    rc_ui_spectrum_flush(priv);
}

static inline void rc_ui_spectrum_input_data_mixed_float(
    const guint8 *_in, gfloat *out, guint len, guint channels,
    gfloat max_value, guint op, guint nfft)
{
    guint i, j, ip = 0;
    gfloat v;
    gfloat *in = (gfloat *)_in;
    for(j=0;j<len;j++)
    {
        v = in[ip++];
        for(i=1;i<channels;i++)
            v+=in[ip++];
        out[op] = v / channels;
        op = (op + 1)%nfft;
    }
}

static inline void rc_ui_spectrum_input_data_mixed_double(
    const guint8 *_in, gfloat *out, guint len, guint channels,
    gfloat max_value, guint op, guint nfft)
{
    guint i, j, ip = 0;
    gfloat v;
    gdouble *in = (gdouble *)_in;
    for(j=0;j<len;j++)
    {
        v = in[ip++];
        for(i=1;i<channels;i++)
            v+=in[ip++];
        out[op] = v / channels;
        op = (op + 1)%nfft;
    }
}

static inline void rc_ui_spectrum_input_data_mixed_int32(
    const guint8 *_in, gfloat *out, guint len, guint channels,
    gfloat max_value, guint op, guint nfft)
{
    guint i, j, ip = 0;
    gint32 *in = (gint32 *)_in;
    gfloat v;
    for(j=0;j<len;j++)
    {
        v = in[ip++] * 2 + 1;
        for(i=1;i<channels;i++)
            v += in[ip++] * 2 + 1;
        out[op] = v / channels;
        op = (op + 1)%nfft;
    }
}

static inline void rc_ui_spectrum_input_data_mixed_int32_max(
    const guint8 *_in, gfloat *out, guint len, guint channels,
    gfloat max_value, guint op, guint nfft)
{
    guint i, j, ip = 0;
    gint32 *in = (gint32 *) _in;
    gfloat v;
    for(j=0;j<len;j++)
    {
        v = in[ip++] / max_value;
        for(i=1;i<channels;i++)
            v += in[ip++] / max_value;
        out[op] = v / channels;
        op = (op + 1) % nfft;
  }
}

static inline void rc_ui_spectrum_input_data_mixed_int24(
    const guint8 *_in, gfloat *out, guint len, guint channels,
    gfloat max_value, guint op, guint nfft)
{
    guint i, j;
    gfloat v = 0.0;
    gint32 value;
    for(j=0;j<len;j++)
    {
        for(i=0;i<channels;i++)
        {
            #if G_BYTE_ORDER==G_BIG_ENDIAN
                value = GST_READ_UINT24_BE (_in);
            #else
                value = GST_READ_UINT24_LE (_in);
            #endif
           if(value & 0x00800000)
               value |= 0xff000000;
           v += value * 2 + 1;
           _in += 3;
        }
        out[op] = v / channels;
        op = (op + 1)%nfft;
    }
}

static inline void rc_ui_spectrum_input_data_mixed_int24_max(
    const guint8 * _in, gfloat *out, guint len, guint channels,
    gfloat max_value, guint op, guint nfft)
{
    guint i, j;
    gfloat v = 0.0;
    gint32 value;
    for(j=0;j<len;j++)
    {
        for(i=0;i< channels;i++)
        {
            #if G_BYTE_ORDER==G_BIG_ENDIAN
                value = GST_READ_UINT24_BE (_in);
            #else
                value = GST_READ_UINT24_LE (_in);
            #endif
            if(value & 0x00800000)
                value |= 0xff000000;
            v += value / max_value;
            _in += 3;
        }
        out[op] = v / channels;
        op = (op + 1) % nfft;
    }
}

static inline void rc_ui_spectrum_input_data_mixed_int16(
    const guint8 *_in, gfloat *out, guint len, guint channels,
    gfloat max_value, guint op, guint nfft)
{
    guint i, j, ip = 0;
    gint16 *in = (gint16 *)_in;
    gfloat v;
    for(j=0;j<len;j++)
    {
        v = in[ip++] * 2 + 1;
        for(i=1;i<channels;i++)
            v += in[ip++] * 2 + 1;
        out[op] = v / channels;
        op = (op + 1)%nfft;
    }
}

static void rc_ui_spectrum_input_data_mixed_int16_max(
    const guint8 *_in, gfloat *out, guint len, guint channels,
    gfloat max_value, guint op, guint nfft)
{
    guint i, j, ip = 0;
    gint16 *in = (gint16 *)_in;
    gfloat v;
    for(j=0;j<len;j++)
    {
        v = in[ip++] / max_value;
        for(i=1;i<channels;i++)
            v += in[ip++] / max_value;
        out[op] = v / channels;
        op = (op + 1)%nfft;
    }
}

static inline void rc_ui_spectrum_input_data(const gchar *mimetype,
    gint width, gint depth, const guint8 *in, gfloat *out, guint len,
    guint channels, gfloat _max_value, guint op, guint nfft)
{
    gboolean is_float = FALSE;
    guint max_value = (1UL << (depth - 1)) - 1;
    if(g_strcmp0(mimetype, "audio/x-raw-float")==0)
        is_float = TRUE;
    if(is_float)
    {
        if(width==4)
            rc_ui_spectrum_input_data_mixed_float(in, out, len,
                channels, _max_value, op, nfft);
        else if(width==8)
            rc_ui_spectrum_input_data_mixed_double(in, out, len,
                channels, _max_value, op, nfft);
    }
    else
    {
        if(width==4)
        {
            if(max_value)
                rc_ui_spectrum_input_data_mixed_int32_max(in, out, len,
                    channels, _max_value, op, nfft);
            else
                rc_ui_spectrum_input_data_mixed_int32(in, out, len,
                    channels, _max_value, op, nfft);
        }
        else if(width==3)
        {
            if(max_value)
                rc_ui_spectrum_input_data_mixed_int24_max(in, out, len,
                    channels, _max_value, op, nfft);
            else
                rc_ui_spectrum_input_data_mixed_int24(in, out, len,
                    channels, _max_value, op, nfft);
        }
        else if(width==2)
        {
            if(max_value)
                rc_ui_spectrum_input_data_mixed_int16_max(in, out, len,
                    channels, _max_value, op, nfft);
            else
                rc_ui_spectrum_input_data_mixed_int16(in, out, len,
                    channels, _max_value, op, nfft);
        }
    }
}

static inline void rc_ui_spectrum_run_fft(RCUiSpectrumChannel *cd,
    guint bands, gfloat threshold, guint input_pos)
{
    guint i;
    guint nfft = 2 * bands - 2;
    gdouble val;
    gfloat *input = cd->input;
    gfloat *input_tmp = cd->input_tmp;
    gfloat *spect_magnitude = cd->spect_magnitude;
    GstFFTF32Complex *freqdata = cd->freqdata;
    GstFFTF32 *fft_ctx = cd->fft_ctx;
    for(i=0;i<nfft;i++)
        input_tmp[i] = input[(input_pos + i) % nfft];
    gst_fft_f32_window(fft_ctx, input_tmp, GST_FFT_WINDOW_HAMMING);
    gst_fft_f32_fft(fft_ctx, input_tmp, freqdata);
    for(i=0;i<bands;i++)
    {
        val = freqdata[i].r * freqdata[i].r;
        val += freqdata[i].i * freqdata[i].i;
        val /= nfft * nfft;
        val = 10.0 * log10 (val);
        if (val < threshold)
            val = threshold;
        spect_magnitude[i] += val;
    }
}

static inline void rc_ui_spectrum_run(RCUiSpectrumWidget *spectrum,
    GstBuffer *buf, const gchar *mimetype, gint rate, gint channels,
    gint width, gint depth)
{
    RCUiSpectrumWidgetPrivate *priv;
    static guint spectrum_input_pos = 0;
    static guint64 spectrum_error_per_interval = 0;
    static guint64 spectrum_frames_per_interval = 0;
    static guint64 spectrum_frames_todo = 0;
    gint i;
    RCUiSpectrumChannel *cd;
    const guint8 *buf_data = GST_BUFFER_DATA(buf);
    gfloat max_value;
    gfloat threshold = -60;
    guint bands;
    guint nfft;
    guint size = GST_BUFFER_SIZE(buf);
    gint64 interval = GST_SECOND / 10;
    gint frame_size;
    gfloat *input;
    guint input_pos;
    guint fft_todo, msg_todo, block_size;
    gboolean have_full_interval;
    if(spectrum==NULL) return;
    priv = spectrum->priv;
    if(priv==NULL) return;
    width = width / 8;
    frame_size = width * channels;
    max_value = (1UL << (depth - 1)) - 1;
    bands = RC_UI_SPECTRUM_BANDS;
    nfft = 2*bands - 2;
    if(GST_BUFFER_IS_DISCONT(buf)) /* Discontinuity detect */
        rc_ui_spectrum_flush(priv);
    if(priv->spectrum_channel_data==NULL)
    {
        rc_ui_spectrum_alloc_channel_data(priv, bands);
        spectrum_frames_per_interval = gst_util_uint64_scale(interval,
            rate, GST_SECOND);
        spectrum_frames_todo = spectrum_frames_per_interval;
        spectrum_error_per_interval = (interval * rate) % GST_SECOND;
        if(spectrum_frames_per_interval==0)
            spectrum_frames_per_interval = 1;
        spectrum_input_pos = 0;
        rc_ui_spectrum_flush(priv);
    }
    input_pos = spectrum_input_pos;
    while(size>=frame_size)
    {
        fft_todo = nfft - (priv->spectrum_num_frames % nfft);
        msg_todo = spectrum_frames_todo - priv->spectrum_num_frames;
        block_size = msg_todo;
        if(block_size > (size / frame_size))
            block_size = (size / frame_size);
        if(block_size > fft_todo)
            block_size = fft_todo;
        cd = priv->spectrum_channel_data;
        input = cd->input;
        rc_ui_spectrum_input_data(mimetype, width, depth,
            (const guint8 *)buf_data, input, block_size, channels,
            max_value, input_pos, nfft);
        buf_data += block_size * frame_size;
        size -= block_size * frame_size;
        input_pos = (input_pos + block_size) % nfft;
        priv->spectrum_num_frames += block_size;
        have_full_interval = (priv->spectrum_num_frames ==
            spectrum_frames_todo);
        if((priv->spectrum_num_frames % nfft == 0) ||
            (have_full_interval && !priv->spectrum_num_fft))
        { 
            cd = priv->spectrum_channel_data;
            rc_ui_spectrum_run_fft(cd, bands, threshold, input_pos);
            priv->spectrum_num_fft++;
        }
        if(have_full_interval)
        {
            spectrum_frames_todo = spectrum_frames_per_interval;
            if(priv->spectrum_accumulated_error >= GST_SECOND)
            {
                priv->spectrum_accumulated_error -= GST_SECOND;
                spectrum_frames_todo++;
            }
            priv->spectrum_accumulated_error +=
                spectrum_error_per_interval;
            cd = priv->spectrum_channel_data;
            for(i=0;i<bands;i++)
            {
                cd->spect_magnitude[i] /= priv->spectrum_num_fft;
                priv->spectrum_band_data[i] = cd->spect_magnitude[i];
            }
            priv->spectrum_rate = rate;
            priv->spectrum_threshold = threshold;            
            cd = priv->spectrum_channel_data;
            memset(cd->spect_magnitude, 0, bands*sizeof(gfloat));
            priv->spectrum_num_frames = 0;
            priv->spectrum_num_fft = 0;
        }
    }
    spectrum_input_pos = input_pos;
}

static inline gfloat rc_ui_spectrum_wave_get_unit_data(const guint8 *adata,
    guint asize, guint s, gboolean is_float, gint width)
{
    gfloat ret = 0.0;
    if(s*width > asize) return 0.0;
    if(is_float)
    {
        if(width==8)
            ret = (gfloat)((gdouble *)adata)[s];
        else if(width==4)
            ret = (gfloat)((gfloat *)adata)[s];
    }
    else
    {
        switch(width)
        {
            case 4:
                ret = (gfloat)((gint32 *)adata)[s];
                    break;
            case 3:
            {
                gint32 tmp = 0.0;
                #if G_BYTE_ORDER==G_BIG_ENDIAN
                    tmp = GST_READ_UINT24_BE(adata+s*3);
                #else
                    tmp = GST_READ_UINT24_LE(adata+s*3);
                #endif
                if(tmp & 0x00800000)
                    tmp |= 0xFF000000;
                ret = (gfloat)tmp;
                    break;
            }
            case 2:
                ret = (gfloat)((gint16 *)adata)[s];
                break;
            case 1:
                ret = (gfloat)((guint8 *)adata)[s] - 128;
                    break;
            default:
                break;
        }
    }
    return ret;
}

static inline void rc_ui_spectrum_spectrum_calculate(gfloat *sdata,
    guint ssize, gfloat *bands, guint bsize, gint rate, gfloat threshold)
{
    const gdouble maxfreq = 20000;
    const gdouble midfreq = 1000;
    gdouble midlog;
    gdouble level;
    gdouble lower, upper, mid;
    gdouble freq1, freq2;
    gint i, j;
    guint l, u;
    gfloat value;
    midlog = log2(maxfreq/midfreq);
    level = 2 * midlog / (ssize);
    for(i=0;i<ssize;i++) sdata[i] = threshold;
    for(i=0;i<ssize;i++)
    {
        if(i==0)
            lower = 0;
        else
            lower = maxfreq / pow(2, (level * (ssize - i)));
        upper = maxfreq / pow(2, (level * (ssize - i - 1)));
        l = (lower/rate*2) * bsize;
        u = (upper/rate*2) * bsize;
        if(l>=bsize) break;
        value = 0.0;
        if(u-l>1)
        {
            value = bands[l];
            for(j=l;j<u;j++)
            {
                if(j>=bsize) break;
                value = MAX(value, bands[j]);
            }
        }
        else
        {
            mid = (lower + upper) / 2;
            freq1 = l * rate / 2 / bsize;
            freq2 = (l+1) * rate / 2 /bsize;
            freq1 = ABS(freq1 - mid);
            freq2 = ABS(freq2 - mid);
            if(l+1<bsize)
            {
                value = bands[l] * freq2 / (freq1+freq2) +
                    bands[l+1] * freq1 / (freq1+freq2);
            }
            else
                value = bands[l];
        }
        sdata[i] = value;
    }
}

static inline void rc_ui_spectrum_wave_render_lines(guint32 *vdata,
    guint vsize, guint vwidth, guint vheight, const guint8 *adata,
    guint asize, const gchar *mimetype, guint num_samples, gint channels,
    gint awidth, gint adepth, guint32 bg_color, guint32 fg_color1,
    guint32 fg_color2, gboolean multi_channel)
{
    guint i, c, s, x = 0, y = 0, oy;
    gfloat dx, dy;
    gint x2 = 0, y2 = 0;
    guint tsize;
    guint32 fg_color;
    gboolean is_float = FALSE;
    gfloat max_value = (1UL << (adepth));
    tsize = vwidth * vheight;
    for(i=0;i<tsize;i++)
        vdata[i] = bg_color;
    if(g_strcmp0(mimetype, "audio/x-raw-float")==0)
    {
        is_float = TRUE;
        max_value = 2.0;
    }
    if(num_samples>=4096)
        num_samples /= 8;
    else if(num_samples>=2048)
        num_samples /= 4;
    else if(num_samples>=1024)
        num_samples /= 2;
    dx = (gfloat) (vwidth - 1) / (gfloat) num_samples;
    dy = (vheight - 1) / max_value;
    oy = (vheight - 1) / 2;
    awidth = awidth / 8;
    if(multi_channel)
    {
        for(c=0;c<channels;c++)
        {
            s = c;
            x2 = 0;
            y2 = (guint)(oy + rc_ui_spectrum_wave_get_unit_data(adata, asize,
                s, is_float, awidth) * dy);
            if(c%2==1)
                fg_color = fg_color2;
            else
                fg_color = fg_color1;
            for(i=1;i<num_samples;i++)
            {
                x = (guint)((gfloat) i * dx);
                y = (guint)(oy + rc_ui_spectrum_wave_get_unit_data(adata,
                    asize, s, is_float, awidth) * dy);
                s += channels;
                x = CLAMP(x, 0, vwidth-1);
                y = CLAMP(y, 0, vheight-1);
                x2 = CLAMP(x2, 0, vwidth-1);
                y2 = CLAMP(y2, 0, vheight-1);
                rc_ui_spectrum_draw_line_aa(vdata, vsize, x2, x, y2, y, vwidth,
                    fg_color, bg_color);
                x2 = x;
                y2 = y;
            }
        }
    }
    else
    {
        s = 0;
        x2 = 0;
        y2 = 0;
        for(c=0;c<channels;c++)
        {
            y2 += (guint)(oy + rc_ui_spectrum_wave_get_unit_data(adata, asize,
                s+c, is_float, awidth) * dy);
        }
        y2 = y2 / channels;
        for(i=1;i<num_samples;i++)
        {
            x = (guint)((gfloat) i * dx);
            y = 0;
            for(c=0;c<channels;c++)
            {
                y += (guint)(oy + rc_ui_spectrum_wave_get_unit_data(adata,
                    asize, s+c, is_float, awidth) * dy);
            }
            y = y / channels;
            s += channels;
            x = CLAMP(x, 0, vwidth-1);
            y = CLAMP(y, 0, vheight-1);
            x2 = CLAMP(x2, 0, vwidth-1);
            y2 = CLAMP(y2, 0, vheight-1);
            rc_ui_spectrum_draw_line_aa(vdata, vsize, x2, x, y2, y, vwidth,
                fg_color1, bg_color);
            x2 = x;
            y2 = y;
        }
    }
}

static void rc_ui_spectrum_buffer_probe_cb(RCLibCore *core, GstBuffer *buf,
    gpointer data)
{
    /* WARNING: This function is not called in main thread! */
    RCUiSpectrumWidget *spectrum;
    RCUiSpectrumWidgetPrivate *priv;
    gint rate = 0;
    gint channels = 0;
    gint width = 0;
    gint depth = 0;
    const gchar *mimetype;
    GstCaps *caps;
    GstStructure *structure;
    if(data==NULL) return;
    spectrum = RC_UI_SPECTRUM_WIDGET(data);
    if(spectrum==NULL) return;
    priv = spectrum->priv;
    if(priv==NULL) return;
    if(buf==NULL) return;
    g_mutex_lock(&(priv->buffer_mutex));
    gst_buffer_replace(&(priv->buffer), buf);
    g_mutex_unlock(&(priv->buffer_mutex));
    if(priv->style==RC_UI_SPECTRUM_STYLE_SPECTRUM)
    {
        caps = gst_buffer_get_caps(buf);
        if(caps==NULL) return;
        structure = gst_caps_get_structure(caps, 0);
        mimetype = gst_structure_get_name(structure);
        gst_structure_get_int(structure, "rate", &rate);
        gst_structure_get_int(structure, "channels", &channels);
        gst_structure_get_int(structure, "width", &width);
        gst_structure_get_int(structure, "depth", &depth);
        gst_caps_unref(caps);
        if(depth==0) depth = width;
        if(rate==0 || width==0 || channels==0) return;
        rc_ui_spectrum_run(spectrum, buf, mimetype, rate, channels,
            width, depth);
    }
}

static void rc_ui_spectrum_widget_realize(GtkWidget *widget)
{
    RCUiSpectrumWidget *spectrum;
    GdkWindowAttr attributes;
    GtkAllocation allocation;
    GdkWindow *window, *parent;
    gint attr_mask;
    GtkStyleContext *context;
    g_return_if_fail(widget!=NULL);
    g_return_if_fail(IS_RC_UI_SPECTRUM_WIDGET(widget));
    spectrum = RC_UI_SPECTRUM_WIDGET(widget);
    gtk_widget_set_realized(widget, TRUE);
    gtk_widget_get_allocation(widget, &allocation);
    attributes.x = allocation.x;
    attributes.y = allocation.y;
    attributes.width = allocation.width;
    attributes.height = allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.event_mask = gtk_widget_get_events(widget);
    attributes.event_mask |= (GDK_EXPOSURE_MASK);
    attributes.visual = gtk_widget_get_visual(widget);
    attr_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;
    gtk_widget_set_has_window(widget, TRUE);
    parent = gtk_widget_get_parent_window(widget);
    window = gdk_window_new(parent, &attributes, attr_mask);
    gtk_widget_set_window(widget, window);
    gdk_window_set_user_data(window, spectrum);
    gdk_window_set_background_pattern(window, NULL);
    context = gtk_widget_get_style_context(widget);
    gtk_style_context_set_background(context, window);
    gdk_window_show(window);
}

static void rc_ui_spectrum_widget_size_allocate(GtkWidget *widget,
    GtkAllocation *allocation)
{
    GdkWindow *window;
    g_return_if_fail(widget!=NULL);
    g_return_if_fail(IS_RC_UI_SPECTRUM_WIDGET(widget));
    gtk_widget_set_allocation(widget, allocation);
    window = gtk_widget_get_window(widget);
    if(gtk_widget_get_realized(widget))
    {
        gdk_window_move_resize(window, allocation->x, allocation->y,
            allocation->width, allocation->height);
    }
}

static void rc_ui_spectrum_widget_get_preferred_height(GtkWidget *widget,
    gint *min_height, gint *nat_height)
{
    *min_height = 15;
    *nat_height = 24;
}

static gboolean rc_ui_spectrum_widget_draw(GtkWidget *widget, cairo_t *cr)
{
    RCUiSpectrumWidget *spectrum;
    RCUiSpectrumWidgetPrivate *priv;
    GstCaps *caps;
    GstStructure *structure;
    GtkAllocation allocation;
    GstBuffer *buffer = NULL;
    const guint8 *adata;
    guint asize;
    guint num_samples = 0;
    gint rate = 0, channels = 0, width = 0, depth = 0;
    const gchar *mimetype = NULL;
    gfloat percent;
    gint i;
    GdkRGBA color;
    GdkRGBA bg_color;
    GdkColor *wave_channel_color1 = NULL;
    GdkColor *wave_channel_color2 = NULL;
    GtkStyleContext *style_context;
    g_return_val_if_fail(widget!=NULL || cr!=NULL, FALSE);
    g_return_val_if_fail(IS_RC_UI_SPECTRUM_WIDGET(widget), FALSE);
    spectrum = RC_UI_SPECTRUM_WIDGET(widget);
    priv = spectrum->priv;
    if(priv==NULL) return FALSE;
    gtk_widget_get_allocation(widget, &allocation);
    if(allocation.width<=0 || allocation.height<=0) return FALSE;
    style_context = gtk_widget_get_style_context(widget);
    gtk_style_context_get_color(style_context, GTK_STATE_FLAG_NORMAL,
        &color);
    gtk_style_context_get_background_color(style_context,
        GTK_STATE_FLAG_NORMAL, &bg_color);
    g_mutex_lock(&(priv->buffer_mutex));
    if(priv->buffer!=NULL)
        buffer = gst_buffer_ref(priv->buffer);
    g_mutex_unlock(&(priv->buffer_mutex));
    G_STMT_START
    {
        if(buffer==NULL) break;
        caps = gst_buffer_get_caps(buffer);
        if(caps==NULL) break;
        structure = gst_caps_get_structure(caps, 0);
        if(structure==NULL)
        {
            gst_caps_unref(caps);
            break;
        }
        mimetype = gst_structure_get_name(structure);
        gst_structure_get_int(structure, "rate", &rate);
        gst_structure_get_int(structure, "channels", &channels);
        gst_structure_get_int(structure, "width", &width);
        gst_structure_get_int(structure, "depth", &depth);
        gst_caps_unref(caps);
        if(depth==0) depth = width;
        if(rate==0 || width==0 || channels==0) break;
        adata = GST_BUFFER_DATA(buffer);
        asize = GST_BUFFER_SIZE(buffer);
        num_samples = GST_BUFFER_SIZE(buffer) / (channels * width / 8);
        switch(priv->style)
        {
            case RC_UI_SPECTRUM_STYLE_NONE:
                break;
            case RC_UI_SPECTRUM_STYLE_WAVE_MONO:
            case RC_UI_SPECTRUM_STYLE_WAVE_MULTI:
            {
                guint vsize;
                guint32 bg_icolor;
                gboolean multi_channel;
                guint32 c1_icolor = 0xFF00FF00, c2_icolor = 0xFFFF0000;
                cairo_surface_t *surface;
                vsize = sizeof(guint32) * allocation.width *
                    allocation.height;
                if(priv->wave_vdata==NULL)
                {
                    priv->wave_vdata = g_new(guint32, vsize/sizeof(guint32));
                    priv->wave_vdata_size = vsize;
                }
                if(priv->wave_vdata_size!=vsize)
                {
                    priv->wave_vdata = g_renew(guint32, priv->wave_vdata,
                        vsize/sizeof(guint32));
                    priv->wave_vdata_size = vsize;
                }
                bg_icolor = 0xFF000000 +
                    ((guint32)(0xFF * bg_color.red)<<16) +
                    ((guint32)(0xFF * bg_color.green)<<8) +
                    (guint32)(0xFF * bg_color.blue);
                gtk_widget_style_get(widget, "wave-channel1-color",
                    &wave_channel_color1, "wave-channel2-color",
                    &wave_channel_color2, NULL);
                if(wave_channel_color1!=NULL)
                {
                    c1_icolor = 0xFF000000 +
                        ((guint32)(wave_channel_color1->red>>8)<<16) +
                        ((guint32)(wave_channel_color1->green>>8)<<8) +
                        (guint32)(wave_channel_color1->blue>>8);
                    gdk_color_free(wave_channel_color1);
                }
                if(wave_channel_color2!=NULL)
                {
                    c2_icolor = 0xFF000000 +
                        ((guint32)(wave_channel_color2->red>>8)<<16) +
                        ((guint32)(wave_channel_color2->green>>8)<<8) +
                        (guint32)(wave_channel_color2->blue>>8);
                    gdk_color_free(wave_channel_color2);
                }
                multi_channel =
                    (priv->style==RC_UI_SPECTRUM_STYLE_WAVE_MULTI);
                rc_ui_spectrum_wave_render_lines(priv->wave_vdata, vsize,
                    allocation.width, allocation.height, adata, asize,
                    mimetype, num_samples, channels, width, depth,
                    bg_icolor, c1_icolor, c2_icolor, multi_channel);
                surface = cairo_image_surface_create_for_data(
                    (guchar *)priv->wave_vdata, CAIRO_FORMAT_ARGB32,
                    allocation.width, allocation.height,
                    cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,
                    allocation.width));
                cairo_set_source_surface(cr, surface, 0, 0);
                cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
                cairo_paint(cr);
                cairo_surface_destroy(surface);
                break;
            }
            case RC_UI_SPECTRUM_STYLE_SPECTRUM:
            {
                guint num;
                guint swidth = 4;
                guint sheight;
                guint space = 1;
                num = allocation.width / (swidth + space);
                cairo_set_source_rgba(cr, color.red, color.green,
                    color.blue, color.alpha);
                if(priv->spectrum_band_data==NULL) break;
                if(priv->spectrum_data==NULL)
                {
                    priv->spectrum_data = g_new0(gfloat, num);
                    priv->spectrum_data_size = num;
                }
                if(priv->spectrum_data_size!=num)
                {
                    priv->spectrum_data = g_renew(gfloat,
                        priv->spectrum_data, num);
                    priv->spectrum_data_size = num;
                }
                rc_ui_spectrum_spectrum_calculate(priv->spectrum_data, num,
                    priv->spectrum_band_data, RC_UI_SPECTRUM_BANDS,
                    priv->spectrum_rate, priv->spectrum_threshold);
                for(i=0;i<num;i++)
                {
                    percent = (priv->spectrum_data[i] -
                        priv->spectrum_threshold)/30;
                    if(percent>1) percent = 1.0;
                    sheight = (gfloat)allocation.height * percent;
                    cairo_rectangle(cr, (swidth+space)*i,
                        allocation.height-sheight, swidth, sheight);
                    cairo_fill(cr);
                }
                break;               
            }
            default:
                break;
        }
    }
    G_STMT_END;
    if(buffer!=NULL) gst_buffer_unref(buffer);
    return TRUE;
}

static gboolean rc_ui_spectrum_draw_timeout_cb(gpointer data)
{
    RCUiSpectrumWidget *widget = (RCUiSpectrumWidget *)data;
    if(data==NULL) return FALSE;
    gtk_widget_queue_draw(GTK_WIDGET(widget));
    return TRUE;
}

static void rc_ui_spectrum_widget_init(RCUiSpectrumWidget *spectrum)
{
    RCUiSpectrumWidgetPrivate *priv;
    priv = G_TYPE_INSTANCE_GET_PRIVATE(spectrum, RC_UI_SPECTRUM_WIDGET_TYPE,
        RCUiSpectrumWidgetPrivate);
    spectrum->priv = priv;
    priv->fps = 10;
    priv->style = RC_UI_SPECTRUM_STYLE_WAVE_MULTI;
    priv->spectrum_band_data = g_new(gfloat, RC_UI_SPECTRUM_BANDS);
    g_mutex_init(&(priv->buffer_mutex));
    priv->timeout_id = g_timeout_add(1000/priv->fps, (GSourceFunc)
        rc_ui_spectrum_draw_timeout_cb, spectrum);
    priv->buffer_probe_id = rclib_core_signal_connect("buffer-probe",
        G_CALLBACK(rc_ui_spectrum_buffer_probe_cb), spectrum);
}

static void rc_ui_spectrum_widget_finalize(GObject *object)
{
    RCUiSpectrumWidget *spectrum = RC_UI_SPECTRUM_WIDGET(object);
    RCUiSpectrumWidgetPrivate *priv;
    priv = spectrum->priv;
    spectrum->priv = NULL;
    rc_ui_spectrum_reset_state(priv);
    if(priv->buffer_probe_id>0)
        rclib_core_signal_disconnect(priv->buffer_probe_id);
    g_mutex_lock(&(priv->buffer_mutex));
    if(priv->buffer!=NULL)
        gst_buffer_unref(priv->buffer);
    g_mutex_unlock(&(priv->buffer_mutex));
    g_mutex_clear(&(priv->buffer_mutex));
    if(priv->timeout_id>0)
        g_source_remove(priv->timeout_id);
    g_free(priv->wave_vdata);
    g_free(priv->spectrum_band_data);
    G_OBJECT_CLASS(rc_ui_spectrum_widget_parent_class)->finalize(object);
}

static void rc_ui_spectrum_widget_class_init(RCUiSpectrumWidgetClass *klass)
{
    GObjectClass *object_class;
    GtkWidgetClass *widget_class;
    rc_ui_spectrum_widget_parent_class = g_type_class_peek_parent(klass);
    object_class = (GObjectClass *)klass;
    widget_class = (GtkWidgetClass *)klass;
    object_class->set_property = NULL;
    object_class->get_property = NULL;
    object_class->finalize = rc_ui_spectrum_widget_finalize;
    widget_class->realize = rc_ui_spectrum_widget_realize;
    widget_class->size_allocate = rc_ui_spectrum_widget_size_allocate;
    widget_class->draw = rc_ui_spectrum_widget_draw;
    widget_class->get_preferred_height =
        rc_ui_spectrum_widget_get_preferred_height;
    g_type_class_add_private(klass, sizeof(RCUiSpectrumWidgetPrivate));
    gtk_widget_class_install_style_property(widget_class,
        g_param_spec_boxed("wave-channel1-color",
            "Wave scope channel 1 color",
            "The color of the line of channel 1 in wave scope mode",
            GDK_TYPE_COLOR, RC_UI_SPECTRUM_STYLE_PARAM_READABLE));
    gtk_widget_class_install_style_property(widget_class,
        g_param_spec_boxed("wave-channel2-color",
            "Wave scope channel 2 color",
            "The color of the line of channel 2 in wave scope mode",
            GDK_TYPE_COLOR, RC_UI_SPECTRUM_STYLE_PARAM_READABLE));
}

GType rc_ui_spectrum_widget_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    static const GTypeInfo object_info = {
        .class_size = sizeof(RCUiSpectrumWidgetClass),
        .base_init = NULL,
        .base_finalize = NULL,
        .class_init = (GClassInitFunc)rc_ui_spectrum_widget_class_init,
        .class_finalize = NULL, 
        .class_data = NULL,
        .instance_size = sizeof(RCUiSpectrumWidget),
        .n_preallocs = 0,
        .instance_init = (GInstanceInitFunc)rc_ui_spectrum_widget_init
    };
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_type_register_static(GTK_TYPE_WIDGET,
            g_intern_static_string("RCUiSpectrumWidget"), &object_info, 0);
        g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

/**
 * rc_ui_spectrum_widget_new:
 *
 * Create a new #RCUiSpectrumWidget widget.
 *
 * Returns: A new #RCUiSpectrumWidget widget.
 */

GtkWidget *rc_ui_spectrum_widget_new()
{
    return GTK_WIDGET(g_object_new(rc_ui_spectrum_widget_get_type(),
        NULL));
}

/**
 * rc_ui_spectrum_widget_set_fps:
 * @spectrum: the #RCUiSpectrumWidget widget
 * @fps: the update frequency (frames per second), from 10 to 60
 *
 * Set the refresh frequency of the widget.
 */

void rc_ui_spectrum_widget_set_fps(RCUiSpectrumWidget *spectrum, guint fps)
{
    RCUiSpectrumWidgetPrivate *priv;
    if(spectrum==NULL || fps<10 || fps>60) return;
    priv = RC_UI_SPECTRUM_WIDGET(spectrum)->priv;
    if(priv==NULL) return;
    if(priv->timeout_id>0)
        g_source_remove(priv->timeout_id);
    priv->fps = fps;
    priv->timeout_id = g_timeout_add(1000/fps, (GSourceFunc)
        rc_ui_spectrum_draw_timeout_cb, spectrum);
}

/**
 * rc_ui_spectrum_widget_set_style:
 * @spectrum: the #RCUiSpectrumWidget widget
 * @style: the spectrum show style
 *
 * Set the spectrum style of the spectrum widget.
 */

void rc_ui_spectrum_widget_set_style(RCUiSpectrumWidget *spectrum,
    RCUiSpectrumStyle style)
{
    RCUiSpectrumWidgetPrivate *priv;
    priv = RC_UI_SPECTRUM_WIDGET(spectrum)->priv;
    if(priv==NULL) return;
    priv->style = style;
    rc_ui_spectrum_widget_clean(spectrum);
}

/**
 * rc_ui_spectrum_widget_get_fps:
 * @spectrum: the #RCUiSpectrumWidget widget
 *
 * Get the refresh frequency (frames per second) of the widget.
 *
 * Returns: The refresh frequency.
 */

guint rc_ui_spectrum_widget_get_fps(RCUiSpectrumWidget *spectrum)
{
    RCUiSpectrumWidgetPrivate *priv;
    if(spectrum==NULL) return 0;
    priv = RC_UI_SPECTRUM_WIDGET(spectrum)->priv;
    if(priv==NULL) return 0;
    return priv->fps;
}

/**
 * rc_ui_spectrum_widget_get_style:
 * @spectrum: the #RCUiSpectrumWidget widget
 *
 * Get the spectrum style of the spectrum widget.
 *
 * Returns: The spectrum style.
 */

RCUiSpectrumStyle rc_ui_spectrum_widget_get_style(
    RCUiSpectrumWidget *spectrum)
{
    RCUiSpectrumWidgetPrivate *priv;
    if(spectrum==NULL) return RC_UI_SPECTRUM_STYLE_NONE;
    priv = RC_UI_SPECTRUM_WIDGET(spectrum)->priv;
    if(priv==NULL) return RC_UI_SPECTRUM_STYLE_NONE;
    return priv->style;
}

/**
 * rc_ui_spectrum_widget_clean:
 * @spectrum: the #RCUiSpectrumWidget widget
 *
 * Clean the spectrum widget.
 */

void rc_ui_spectrum_widget_clean(RCUiSpectrumWidget *spectrum)
{
    RCUiSpectrumWidgetPrivate *priv;
    if(spectrum==NULL) return;
    GstBuffer *buffer;
    priv = RC_UI_SPECTRUM_WIDGET(spectrum)->priv;
    if(priv==NULL) return;
    g_mutex_lock(&(priv->buffer_mutex));
    buffer = priv->buffer;
    priv->buffer = NULL;
    if(buffer!=NULL)
        gst_buffer_unref(buffer);
    g_mutex_unlock(&(priv->buffer_mutex));
    g_free(priv->wave_vdata);
    g_free(priv->spectrum_data);
    priv->wave_vdata = NULL;
    priv->spectrum_data = NULL;
    priv->wave_vdata_size = 0;
    priv->spectrum_data_size = 0;
    gtk_widget_queue_draw(GTK_WIDGET(spectrum));
}

