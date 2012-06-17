#include <glib.h>
#include <json-glib/json-glib.h>

/**
 * RCLibDbCatalogType:
 * @RCLIB_DB_CATALOG_TYPE_PLAYLIST: the catalog is a playlist
 *
 * The enum type for catalog type.
 */

typedef enum {
    RCLIB_DB_CATALOG_TYPE_PLAYLIST = 1
}RCLibDbCatalogType;

/**
 * RCLibDbPlaylistType:
 * @RCLIB_DB_PLAYLIST_TYPE_MISSING: the playlist item is missing
 * @RCLIB_DB_PLAYLIST_TYPE_MUSIC: the playlist item is music
 * @RCLIB_DB_PLAYLIST_TYPE_CUE: the playlist item is from CUE sheet
 *
 * The enum type for playlist type.
 */

typedef enum {
    RCLIB_DB_PLAYLIST_TYPE_MISSING = 0,
    RCLIB_DB_PLAYLIST_TYPE_MUSIC = 1,
    RCLIB_DB_PLAYLIST_TYPE_CUE = 2
}RCLibDbPlaylistType;

typedef struct _RCLibDbCatalogData RCLibDbCatalogData;
typedef struct _RCLibDbPlaylistData RCLibDbPlaylistData;
typedef struct _RCLibDb RCLibDb;
typedef struct _RCLibDbClass RCLibDbClass;

/**
 * RCLibDbCatalogData:
 * @playlist: a playlist
 * @name: the name
 * @type: the type of the item in catalog
 * @store: the store, used in UI
 *
 * The structure for catalog item data.
 */

struct _RCLibDbCatalogData {
    /*< private >*/
    gint ref_count;
    
    /*< public >*/
    GSequence *playlist;
    gchar *name;
    RCLibDbCatalogType type;
    gpointer store;
};

/**
 * RCLibDbPlaylistData:
 * @catalog: the catalog item data
 * @type: the type of the item in playlist
 * @uri: the URI
 * @title: the title
 * @artist: the artist
 * @album: the album
 * @ftype: the format information
 * @length: the the duration (length) of the music (unit: nanosecond)
 * @tracknum: the track number
 * @year: the year
 * @rating: the rating level
 * @lyricfile: the lyric binding file path
 * @lyricsecfile: the secondary lyric binding file path
 * @albumfile: the album cover image binding file path
 *
 * The structure for playlist item data.
 */

struct _RCLibDbPlaylistData {
    /*< private >*/
    gint ref_count;

    /*< public >*/
    GSequenceIter *catalog;
    RCLibDbPlaylistType type;
    gchar *uri;
    gchar *title;
    gchar *artist;
    gchar *album;
    gchar *ftype;
    gint64 length;
    gint tracknum;
    gint year;
    guint rating;
    gchar *lyricfile;
    gchar *lyricsecfile;
    gchar *albumfile;
};

static GSequence *conv_catalog = NULL;

static RCLibDbCatalogData *rclib_db_catalog_data_new();
static RCLibDbCatalogData *rclib_db_catalog_data_ref(RCLibDbCatalogData *data);
static void rclib_db_catalog_data_unref(RCLibDbCatalogData *data);
static void rclib_db_catalog_data_free(RCLibDbCatalogData *data);
static RCLibDbPlaylistData *rclib_db_playlist_data_new();
static RCLibDbPlaylistData *rclib_db_playlist_data_ref(RCLibDbPlaylistData *data);
static void rclib_db_playlist_data_unref(RCLibDbPlaylistData *data);
static void rclib_db_playlist_data_free(RCLibDbPlaylistData *data);

static RCLibDbCatalogData *rclib_db_catalog_data_new()
{
    RCLibDbCatalogData *data = g_new0(RCLibDbCatalogData, 1);
    data->ref_count = 1;
    return data;
}

static RCLibDbCatalogData *rclib_db_catalog_data_ref(RCLibDbCatalogData *data)
{
    if(data==NULL) return NULL;
    g_atomic_int_add(&(data->ref_count), 1);
    return data;
}

static void rclib_db_catalog_data_unref(RCLibDbCatalogData *data)
{
    if(data==NULL) return;
    if(g_atomic_int_dec_and_test(&(data->ref_count)))
        rclib_db_catalog_data_free(data);
}

static void rclib_db_catalog_data_free(RCLibDbCatalogData *data)
{
    if(data==NULL) return;
    g_free(data->name);
    if(data->playlist!=NULL) g_sequence_free(data->playlist);
    g_free(data);
}

static RCLibDbPlaylistData *rclib_db_playlist_data_new()
{
    RCLibDbPlaylistData *data = g_new0(RCLibDbPlaylistData, 1);
    data->ref_count = 1;
    return data;
}

static RCLibDbPlaylistData *rclib_db_playlist_data_ref(RCLibDbPlaylistData *data)
{
    if(data==NULL) return NULL;
    g_atomic_int_add(&(data->ref_count), 1);
    return data;
}

static void rclib_db_playlist_data_unref(RCLibDbPlaylistData *data)
{
    if(data==NULL) return;
    if(g_atomic_int_dec_and_test(&(data->ref_count)))
        rclib_db_playlist_data_free(data);
}

static void rclib_db_playlist_data_free(RCLibDbPlaylistData *data)
{
    if(data==NULL) return;
    g_free(data->uri);
    g_free(data->title);
    g_free(data->artist);
    g_free(data->album);
    g_free(data->ftype);
    g_free(data->lyricfile);
    g_free(data->lyricsecfile);
    g_free(data->albumfile);
    g_free(data);
}

static gboolean rclib_db_load_library_db(GSequence *catalog,
    const gchar *file)
{
    JsonObject *root_object, *catalog_object, *playlist_object;
    JsonArray *catalog_array, *playlist_array;
    JsonNode *root_node;
    guint i, j;
    guint catalog_len, playlist_len;
    GError *error = NULL;
    GSequence *playlist;
    RCLibDbCatalogData *catalog_data;
    RCLibDbPlaylistData *playlist_data;
    const gchar *str;
    GSequenceIter *catalog_iter;
    JsonParser *parser;
    parser = json_parser_new();
    if(!json_parser_load_from_file(parser, file, &error))
    {
        g_warning("Cannot load playlist: %s", error->message);
        g_error_free(error);
        g_object_unref(parser);
        return FALSE;
    }
    else
        g_message("Loading playlist....");
    root_node = json_parser_get_root(parser); 
    if(root_node==NULL)
    {
        g_warning("Cannot parse playlist!");
        g_object_unref(parser);
        return FALSE;
    }
    G_STMT_START
    {
        root_object = json_node_get_object(root_node);
        if(root_object==NULL) break;
        catalog_array = json_object_get_array_member(root_object,
            "RCMusicCatalog");
        if(catalog_array==NULL) break;
        catalog_len = json_array_get_length(catalog_array);
        for(i=0;i<catalog_len;i++)
        {
            catalog_object = json_array_get_object_element(catalog_array, i);
            if(catalog_object==NULL) continue;
            catalog_data = rclib_db_catalog_data_new();
            str = json_object_get_string_member(catalog_object, "Name");
            if(str==NULL) str = "Unnamed list";
            catalog_data->name = g_strdup(str);
            catalog_data->type = json_object_get_int_member(catalog_object,
                "Type");
            playlist = g_sequence_new((GDestroyNotify)
                rclib_db_playlist_data_unref);
            catalog_data->playlist = playlist;
            catalog_iter = g_sequence_append(catalog, catalog_data);
            playlist_array = json_object_get_array_member(catalog_object,
                "Playlist");
            if(playlist_array==NULL) continue;
            playlist_len = json_array_get_length(playlist_array);
            for(j=0;j<playlist_len;j++)
            {
                playlist_object = json_array_get_object_element(
                    playlist_array, j);
                if(playlist_object==NULL) continue;
                playlist_data = rclib_db_playlist_data_new();
                playlist_data->catalog = catalog_iter;
                playlist_data->type = json_object_get_int_member(
                    playlist_object, "Type");
                if(json_object_has_member(playlist_object, "URI"))
                    str = json_object_get_string_member(playlist_object,
                        "URI");
                else
                    str = NULL;
                playlist_data->uri = g_strdup(str);
                if(json_object_has_member(playlist_object, "Title"))
                    str = json_object_get_string_member(playlist_object,
                        "Title");
                else
                    str = NULL;
                playlist_data->title = g_strdup(str);
                if(json_object_has_member(playlist_object, "Artist"))
                    str = json_object_get_string_member(playlist_object,
                        "Artist");
                else
                    str = NULL;
                playlist_data->artist = g_strdup(str);
                if(json_object_has_member(playlist_object, "Album"))
                    str = json_object_get_string_member(playlist_object,
                        "Album");
                else
                    str = NULL;
                playlist_data->album = g_strdup(str);
                if(json_object_has_member(playlist_object, "FileType"))
                    str = json_object_get_string_member(playlist_object,
                        "FileType");
                else
                    str = NULL;
                playlist_data->ftype = g_strdup(str);
                if(json_object_has_member(playlist_object, "Length"))
                    str = json_object_get_string_member(playlist_object,
                        "Length");
                else
                    str = NULL;
                if(str!=NULL)
                    playlist_data->length = g_ascii_strtoll(str, NULL, 10);
                if(json_object_has_member(playlist_object, "TrackNum"))
                    playlist_data->tracknum = json_object_get_int_member(
                        playlist_object, "TrackNum");
                if(json_object_has_member(playlist_object, "Year"))
                    playlist_data->year = json_object_get_int_member(
                        playlist_object, "Year");
                if(json_object_has_member(playlist_object, "Rating"))
                    playlist_data->rating = json_object_get_int_member(
                        playlist_object, "Rating");
                if(json_object_has_member(playlist_object, "LyricFile"))
                    str = json_object_get_string_member(playlist_object,
                        "LyricFile");
                else
                    str = NULL;
                playlist_data->lyricfile = g_strdup(str);
                if(json_object_has_member(playlist_object, "LyricSecondFile"))
                    str = json_object_get_string_member(playlist_object,
                        "LyricSecondFile");
                else
                    str = NULL;
                playlist_data->lyricsecfile = g_strdup(str);
                if(json_object_has_member(playlist_object, "AlbumFile"))
                    str = json_object_get_string_member(playlist_object,
                        "AlbumFile");
                else
                    str = NULL;
                playlist_data->albumfile = g_strdup(str);
                g_sequence_append(playlist, playlist_data);
            }
        }
    }
    G_STMT_END;
    g_object_unref(parser);
    return TRUE;
}

static inline GString *rclib_db_build_xml_data(GSequence *catalog)
{
    RCLibDbCatalogData *catalog_data;
    RCLibDbPlaylistData *playlist_data;
    GSequenceIter *catalog_iter, *playlist_iter;
    GString *data_str;
    gchar *tmp;
    extern guint rclib_major_version;
    extern guint rclib_minor_version;
    extern guint rclib_micro_version;
    data_str = g_string_new("<?xml version=\"1.0\" standalone=\"yes\"?>\n");
    g_string_append_printf(data_str, "<rclibdb version=\"1.9.4\">\n");
    for(catalog_iter = g_sequence_get_begin_iter(catalog);
        !g_sequence_iter_is_end(catalog_iter);
        catalog_iter = g_sequence_iter_next(catalog_iter))
    {
        catalog_data = g_sequence_get(catalog_iter);
        tmp = g_markup_printf_escaped("  <playlist name=\"%s\" type=\"%u\">\n",
            catalog_data->name, catalog_data->type);
        g_string_append(data_str, tmp);
        g_free(tmp);
        for(playlist_iter = g_sequence_get_begin_iter(catalog_data->playlist);
            !g_sequence_iter_is_end(playlist_iter);
            playlist_iter = g_sequence_iter_next(playlist_iter))
        {
            playlist_data = g_sequence_get(playlist_iter);
            g_string_append_printf(data_str, "    <item type=\"%u\" ",
                playlist_data->type);
            if(playlist_data->uri!=NULL)
            {
                tmp = g_markup_printf_escaped("uri=\"%s\" ",
                    playlist_data->uri);
                g_string_append(data_str, tmp);
                g_free(tmp);
            }
            if(playlist_data->title!=NULL)
            {
                tmp = g_markup_printf_escaped("title=\"%s\" ",
                    playlist_data->title);
                g_string_append(data_str, tmp);
                g_free(tmp);
            }
            if(playlist_data->artist!=NULL)
            {
                tmp = g_markup_printf_escaped("artist=\"%s\" ",
                    playlist_data->artist);
                g_string_append(data_str, tmp);
                g_free(tmp);
            }
            if(playlist_data->album!=NULL)
            {
                tmp = g_markup_printf_escaped("album=\"%s\" ",
                    playlist_data->album);
                g_string_append(data_str, tmp);
                g_free(tmp);
            }
            if(playlist_data->ftype!=NULL)
            {
                tmp = g_markup_printf_escaped("filetype=\"%s\" ",
                    playlist_data->ftype);
                g_string_append(data_str, tmp);
                g_free(tmp);
            }
            tmp = g_markup_printf_escaped("length=\"%"G_GINT64_FORMAT"\" ",
                playlist_data->length);
            g_string_append(data_str, tmp);
            g_free(tmp);
            tmp = g_markup_printf_escaped("tracknum=\"%d\" year=\"%d\" "
                "rating=\"%f\" ", playlist_data->tracknum,
                playlist_data->year, playlist_data->rating);
            g_string_append(data_str, tmp);
            g_free(tmp);
            if(playlist_data->lyricfile!=NULL)
            {
                tmp = g_markup_printf_escaped("lyricfile=\"%s\" ",
                    playlist_data->lyricfile);
                g_string_append(data_str, tmp);
                g_free(tmp);
            }
            if(playlist_data->albumfile!=NULL)
            {
                tmp = g_markup_printf_escaped("albumfile=\"%s\" ",
                    playlist_data->albumfile);
                g_string_append(data_str, tmp);
                g_free(tmp);
            }
            if(playlist_data->lyricsecfile!=NULL)
            {
                tmp = g_markup_printf_escaped("lyricsecondfile=\"%s\" ",
                    playlist_data->lyricsecfile);
                g_string_append(data_str, tmp);
                g_free(tmp);
            }
            g_string_append(data_str, "/>\n");
        }
        g_string_append(data_str, "  </playlist>\n");
    }
    g_string_append(data_str, "</rclibdb>\n");
    return data_str;
}

static gboolean rclib_db_save_library_db(GSequence *catalog,
    const gchar *file)
{
    GString *xml_str;
    GZlibCompressor *compressor;
    GFileOutputStream *file_ostream;
    GOutputStream *compress_ostream;
    GFile *output_file;
    gssize write_size;
    GError *error = NULL;
    gboolean flag = TRUE;
    output_file = g_file_new_for_path(file);
    if(output_file==NULL) return FALSE;
    xml_str = rclib_db_build_xml_data(catalog);
    if(xml_str==NULL) return FALSE;
    file_ostream = g_file_replace(output_file, NULL, FALSE,
        G_FILE_CREATE_PRIVATE, NULL, &error);
    g_object_unref(output_file);
    if(file_ostream==NULL)
    {
        g_warning("Cannot open output file stream: %s", error->message);
        g_error_free(error);
        error = NULL;
        return FALSE;
    }
    compressor = g_zlib_compressor_new(G_ZLIB_COMPRESSOR_FORMAT_ZLIB, 5);
    compress_ostream = g_converter_output_stream_new(G_OUTPUT_STREAM(
        file_ostream), G_CONVERTER(compressor));
    g_object_unref(file_ostream);
    g_object_unref(compressor);
    write_size = g_output_stream_write(compress_ostream, xml_str->str,
        xml_str->len, NULL, &error);
    if(error!=NULL)
    {
        g_warning("Cannot save playlist: %s", error->message);
        g_error_free(error);
        error = NULL;
        flag = FALSE;
    }
    g_output_stream_close(compress_ostream, NULL, &error);
    if(error!=NULL)
    {
        g_warning("Cannot close file stream: %s", error->message);
        g_error_free(error);
        error = NULL;
        flag = FALSE;
    }
    g_object_unref(compress_ostream);
    if(flag)
    {
        g_message("Playlist saved, wrote %ld bytes.", (glong)write_size);
    } 
    return flag;
}

int main()
{
    const gchar *home_dir = NULL;
    gchar *db_file, *zdb_file;
    gboolean flag;
    g_type_init();
    conv_catalog = g_sequence_new((GDestroyNotify)
        rclib_db_catalog_data_unref);
    g_message("Loading playlist data...");
    home_dir = g_getenv("HOME");
    if(home_dir==NULL)
        home_dir = g_get_home_dir();
    db_file = g_build_filename(home_dir, ".RhythmCat2", "library.db", NULL);
    flag = rclib_db_load_library_db(conv_catalog, db_file);
    g_free(db_file);
    if(!flag)
    {
        g_error("Cannot read playlist database data!");
        return 1;
    }
    g_message("Playlist data loaded.");
    zdb_file = g_build_filename(home_dir, ".RhythmCat2", "library.zdb", NULL);
    flag = rclib_db_save_library_db(conv_catalog, zdb_file);
    g_free(zdb_file);
    g_sequence_free(conv_catalog);
    if(!flag)
    {
        g_error("Cannot save compressed playlist database data!");
        return 1;
    }
    return 0;
}

