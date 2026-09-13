#include "capture.h"
#include "config.h"
#include <mpd/connection.h>
#include <mpd/response.h>
#include <mpd/capabilities.h>
#include <mpd/queue.h>
#include <mpd/playlist.h>
#include <mpd/database.h>
#include <mpd/search.h>
#include <mpd/player.h>
#include <mpd/mount.h>
#include <mpd/sticker.h>

#include <check.h>

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>

#ifdef HAVE_SETLOCALE
#include <locale.h>
#endif

static void
abort_command(struct test_capture *capture,
	      struct mpd_connection *connection)
{
	test_capture_send(capture, "ACK [5@0] {} cancel\n");
	mpd_response_finish(connection);
	ck_assert(mpd_connection_clear_error(connection));
}

START_TEST(test_quote)
{
	struct test_capture capture;
	struct mpd_connection *c = test_capture_init(&capture);

	ck_assert(mpd_send_add(c, "foo"));
	ck_assert_str_eq(test_capture_receive(&capture), "add \"foo\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_add(c, ""));
	ck_assert_str_eq(test_capture_receive(&capture), "add \"\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_add(c, "'"));
	ck_assert_str_eq(test_capture_receive(&capture), "add \"'\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_add(c, "\""));
	ck_assert_str_eq(test_capture_receive(&capture), "add \"\\\"\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_add(c, "\\"));
	ck_assert_str_eq(test_capture_receive(&capture), "add \"\\\\\"\n");
	abort_command(&capture, c);

	/* CR and LF cannot be represented in the MPD protocol and are
	   thus illegal; libmpdclient will reject them */
	ck_assert(!mpd_send_add(c, "\n"));
	ck_assert(!mpd_send_add(c, "\r"));

	mpd_connection_free(c);
	test_capture_deinit(&capture);
}
END_TEST

START_TEST(test_capabilities_commands)
{
	struct test_capture capture;
	struct mpd_connection *c = test_capture_init(&capture);

	ck_assert(mpd_send_allowed_commands(c));
	ck_assert_str_eq(test_capture_receive(&capture), "commands\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_disallowed_commands(c));
	ck_assert_str_eq(test_capture_receive(&capture), "notcommands\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_list_url_schemes(c));
	ck_assert_str_eq(test_capture_receive(&capture), "urlhandlers\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_list_tag_types(c));
	ck_assert_str_eq(test_capture_receive(&capture), "tagtypes\n");
	abort_command(&capture, c);

	static const enum mpd_tag_type types[] = {
		MPD_TAG_COMMENT,
		MPD_TAG_PERFORMER,
		MPD_TAG_MUSICBRAINZ_RELEASETRACKID,
	};

	ck_assert(mpd_send_disable_tag_types(c, types, 3));
	ck_assert_str_eq(test_capture_receive(&capture), "tagtypes disable Comment Performer MUSICBRAINZ_RELEASETRACKID\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_enable_tag_types(c, types, 3));
	ck_assert_str_eq(test_capture_receive(&capture), "tagtypes enable Comment Performer MUSICBRAINZ_RELEASETRACKID\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_clear_tag_types(c));
	ck_assert_str_eq(test_capture_receive(&capture), "tagtypes \"clear\"\n");
	abort_command(&capture, c);

	mpd_connection_free(c);
	test_capture_deinit(&capture);
}
END_TEST

START_TEST(test_queue_commands)
{
	struct test_capture capture;
	struct mpd_connection *c = test_capture_init(&capture);

	ck_assert(mpd_send_list_queue_meta(c));
	ck_assert_str_eq(test_capture_receive(&capture), "playlistinfo\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_list_queue_range_meta(c, 0, 1));
	ck_assert_str_eq(test_capture_receive(&capture), "playlistinfo \"0:1\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_list_queue_range_meta(c, 42, UINT_MAX));
	ck_assert_str_eq(test_capture_receive(&capture), "playlistinfo \"42:\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_get_queue_song_pos(c, 42));
	ck_assert_str_eq(test_capture_receive(&capture), "playlistinfo \"42\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_queue_changes_meta(c, 42));
	ck_assert_str_eq(test_capture_receive(&capture), "plchanges \"42\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_queue_changes_meta_range(c, 42, 6, 7));
	ck_assert_str_eq(test_capture_receive(&capture), "plchanges \"42\" \"6:7\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_queue_changes_brief(c, 42));
	ck_assert_str_eq(test_capture_receive(&capture), "plchangesposid \"42\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_queue_changes_meta_range(c, 42, 6, UINT_MAX));
	ck_assert_str_eq(test_capture_receive(&capture), "plchanges \"42\" \"6:\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_add_tag_id(c, 42, MPD_TAG_COMMENT, "foo"));
	ck_assert_str_eq(test_capture_receive(&capture), "addtagid \"42\" \"Comment\" \"foo\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_clear_tag_id(c, 42, MPD_TAG_COMMENT));
	ck_assert_str_eq(test_capture_receive(&capture), "cleartagid \"42\" \"Comment\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_clear_all_tags_id(c, 42));
	ck_assert_str_eq(test_capture_receive(&capture), "cleartagid \"42\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_range_id(c, 42, 0, 666));
	ck_assert_str_eq(test_capture_receive(&capture), "rangeid \"42\" \"0.000:666.000\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_range_id(c, 42, 6, -1));
	ck_assert_str_eq(test_capture_receive(&capture), "rangeid \"42\" \"6.000:\"\n");
	abort_command(&capture, c);

	mpd_connection_free(c);
	test_capture_deinit(&capture);
}
END_TEST

START_TEST(test_playlist_commands)
{
	struct test_capture capture;
	struct mpd_connection *c = test_capture_init(&capture);

	ck_assert(mpd_send_list_playlists(c));
	ck_assert_str_eq(test_capture_receive(&capture), "listplaylists\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_list_playlist(c, "foo"));
	ck_assert_str_eq(test_capture_receive(&capture), "listplaylist \"foo\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_list_playlist_meta(c, "foo"));
	ck_assert_str_eq(test_capture_receive(&capture), "listplaylistinfo \"foo\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_playlist_clear(c, "foo"));
	ck_assert_str_eq(test_capture_receive(&capture), "playlistclear \"foo\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_playlist_add(c, "foo", "bar"));
	ck_assert_str_eq(test_capture_receive(&capture), "playlistadd \"foo\" \"bar\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_save(c, "foo"));
	ck_assert_str_eq(test_capture_receive(&capture), "save \"foo\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_load(c, "foo"));
	ck_assert_str_eq(test_capture_receive(&capture), "load \"foo\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_load_range(c, "foo", 2, 5));
	ck_assert_str_eq(test_capture_receive(&capture), "load \"foo\" \"2:5\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_load_range(c, "foo", 2, UINT_MAX));
	ck_assert_str_eq(test_capture_receive(&capture), "load \"foo\" \"2:\"\n");
	abort_command(&capture, c);

	mpd_connection_free(c);
	test_capture_deinit(&capture);
}
END_TEST

START_TEST(test_playlist_search)
{
	struct test_capture capture;
	struct mpd_connection *c = test_capture_init(&capture);

	/* a search without window */
	ck_assert(mpd_playlist_search_begin(c, "foo", "(Artist == \"Queen\")"));
	ck_assert(mpd_playlist_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture),
			 "searchplaylist \"foo\" \"(Artist == \\\"Queen\\\")\"\n");
	abort_command(&capture, c);

	/* with a window */
	ck_assert(mpd_playlist_search_begin(c, "foo", "(Artist == \"Queen\")"));
	ck_assert(mpd_playlist_search_add_window(c, 0, 10));
	ck_assert(mpd_playlist_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture),
			 "searchplaylist \"foo\" \"(Artist == \\\"Queen\\\")\" window 0:10\n");
	abort_command(&capture, c);

	/* an open-ended window */
	ck_assert(mpd_playlist_search_begin(c, "foo", "(Artist == \"Queen\")"));
	ck_assert(mpd_playlist_search_add_window(c, 5, UINT_MAX));
	ck_assert(mpd_playlist_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture),
			 "searchplaylist \"foo\" \"(Artist == \\\"Queen\\\")\" window 5:\n");
	abort_command(&capture, c);

	mpd_connection_free(c);
	test_capture_deinit(&capture);
}
END_TEST

START_TEST(test_playlist_search_quote)
{
	struct test_capture capture;
	struct mpd_connection *c = test_capture_init(&capture);

	/* both the playlist name and the expression must be escaped */
	ck_assert(mpd_playlist_search_begin(c, "double \" quote",
					    "back\\slash"));
	ck_assert(mpd_playlist_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture),
			 "searchplaylist \"double \\\" quote\" \"back\\\\slash\"\n");
	abort_command(&capture, c);

	/* empty strings are passed through as empty quoted arguments */
	ck_assert(mpd_playlist_search_begin(c, "", ""));
	ck_assert(mpd_playlist_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture),
			 "searchplaylist \"\" \"\"\n");
	abort_command(&capture, c);

	mpd_connection_free(c);
	test_capture_deinit(&capture);
}
END_TEST

START_TEST(test_playlist_search_cancel)
{
	struct test_capture capture;
	struct mpd_connection *c = test_capture_init(&capture);

	/* start a search, but cancel it */
	ck_assert(mpd_playlist_search_begin(c, "foo", "(Artist == \"Queen\")"));
	ck_assert(mpd_playlist_search_add_window(c, 0, 10));
	mpd_playlist_search_cancel(c);

	/* after cancelling, a new search can be started, and the
	   cancelled one leaves no remains */
	ck_assert(mpd_playlist_search_begin(c, "bar", "(Album == \"News\")"));
	ck_assert(mpd_playlist_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture),
			 "searchplaylist \"bar\" \"(Album == \\\"News\\\")\"\n");
	abort_command(&capture, c);

	mpd_connection_free(c);
	test_capture_deinit(&capture);
}
END_TEST

START_TEST(test_playlist_search_state)
{
	struct test_capture capture;
	struct mpd_connection *c = test_capture_init(&capture);

	/* committing without a search in progress fails */
	ck_assert(!mpd_playlist_search_commit(c));
	ck_assert_int_eq(mpd_connection_get_error(c), MPD_ERROR_STATE);
	ck_assert(mpd_connection_clear_error(c));

	/* so does adding a window */
	ck_assert(!mpd_playlist_search_add_window(c, 0, 10));
	ck_assert_int_eq(mpd_connection_get_error(c), MPD_ERROR_STATE);
	ck_assert(mpd_connection_clear_error(c));

	/* a second mpd_playlist_search_begin() without commit fails */
	ck_assert(mpd_playlist_search_begin(c, "foo", "(Artist == \"Queen\")"));
	ck_assert(!mpd_playlist_search_begin(c, "bar", "(Album == \"News\")"));
	ck_assert_int_eq(mpd_connection_get_error(c), MPD_ERROR_STATE);
	ck_assert(mpd_connection_clear_error(c));

	/* the first search is still intact and can be committed */
	ck_assert(mpd_playlist_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture),
			 "searchplaylist \"foo\" \"(Artist == \\\"Queen\\\")\"\n");
	abort_command(&capture, c);

	mpd_connection_free(c);
	test_capture_deinit(&capture);
}
END_TEST

START_TEST(test_database_commands)
{
	struct test_capture capture;
	struct mpd_connection *c = test_capture_init(&capture);

	ck_assert(mpd_send_list_files(c, "nfs://foo/bar"));
	ck_assert_str_eq(test_capture_receive(&capture), "listfiles \"nfs://foo/bar\"\n");
	abort_command(&capture, c);

	mpd_connection_free(c);
	test_capture_deinit(&capture);
}
END_TEST

START_TEST(test_search)
{
	struct test_capture capture;
	struct mpd_connection *c = test_capture_init(&capture);

	/* start a search, but cancel it */
	ck_assert(mpd_search_queue_songs(c, false));
	ck_assert(mpd_search_add_uri_constraint(c, MPD_OPERATOR_DEFAULT,
						"foo"));
	mpd_search_cancel(c);

	/* start a new search */
	ck_assert(mpd_search_db_songs(c, true));
	ck_assert(mpd_search_add_base_constraint(c, MPD_OPERATOR_DEFAULT,
						 "foo"));
	ck_assert(mpd_search_add_tag_constraint(c, MPD_OPERATOR_DEFAULT,
						MPD_TAG_ARTIST, "Queen"));
	ck_assert(mpd_search_add_any_tag_constraint(c, MPD_OPERATOR_DEFAULT,
						    "Foo"));
	ck_assert(mpd_search_add_sort_tag(c, MPD_TAG_DATE, false));
	ck_assert(mpd_search_add_window(c, 7, 9));
	ck_assert(mpd_search_commit(c));

	ck_assert_str_eq(test_capture_receive(&capture), "find base \"foo\" Artist \"Queen\" any \"Foo\" sort Date window 7:9\n");
	abort_command(&capture, c);

	/* another search */
	ck_assert(mpd_search_db_songs(c, false));
	ck_assert(mpd_search_add_base_constraint(c, MPD_OPERATOR_DEFAULT,
						 "foo"));
	ck_assert(mpd_search_add_sort_tag(c, MPD_TAG_DATE, false));
	ck_assert(mpd_search_add_window(c, 7, 9));
	ck_assert(mpd_search_commit(c));

	ck_assert_str_eq(test_capture_receive(&capture), "search base \"foo\" sort Date window 7:9\n");
	abort_command(&capture, c);

	/* check backslash escape */
	ck_assert(mpd_search_db_songs(c, false));
	ck_assert(mpd_search_add_tag_constraint(c, MPD_OPERATOR_DEFAULT,
						MPD_TAG_ARTIST, "double quote: \" and backslash: \\"));
	ck_assert(mpd_search_commit(c));

	ck_assert_str_eq(test_capture_receive(&capture), "search Artist \"double quote: \\\" and backslash: \\\\\"\n");
	abort_command(&capture, c);

	mpd_connection_free(c);
	test_capture_deinit(&capture);
}
END_TEST

START_TEST(test_expression)
{
	struct test_capture capture;
	struct mpd_connection *c = test_capture_init(&capture);

	ck_assert(mpd_search_db_songs(c, true));
	ck_assert(mpd_search_add_expression(c, "(Artist == \"Queen\")"));
	ck_assert(mpd_search_commit(c));

	ck_assert_str_eq(test_capture_receive(&capture), "find \"(Artist == \\\"Queen\\\")\"\n");
	abort_command(&capture, c);

	mpd_connection_free(c);
	test_capture_deinit(&capture);
}
END_TEST

START_TEST(test_list)
{
	struct test_capture capture;
	struct mpd_connection *c = test_capture_init(&capture);

	ck_assert(mpd_search_db_tags(c, MPD_TAG_ARTIST));
	ck_assert(mpd_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture), "list Artist\n");
	abort_command(&capture, c);

	ck_assert(mpd_search_db_tags(c, MPD_TAG_ALBUM));
	ck_assert(mpd_search_add_group_tag(c, MPD_TAG_ARTIST));
	ck_assert(mpd_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture), "list Album group Artist\n");
	abort_command(&capture, c);

	mpd_connection_free(c);
	test_capture_deinit(&capture);
}
END_TEST

START_TEST(test_count)
{
	struct test_capture capture;
	struct mpd_connection *c = test_capture_init(&capture);

	ck_assert(mpd_count_db_songs(c));
	ck_assert(mpd_search_add_tag_constraint(c, MPD_OPERATOR_DEFAULT,
						MPD_TAG_ARTIST, "Queen"));
	ck_assert(mpd_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture), "count Artist \"Queen\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_count_db_songs(c));
	ck_assert(mpd_search_add_tag_constraint(c, MPD_OPERATOR_DEFAULT,
						MPD_TAG_ARTIST, "Queen"));
	ck_assert(mpd_search_add_group_tag(c, MPD_TAG_ALBUM));
	ck_assert(mpd_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture), "count Artist \"Queen\" group Album\n");
	abort_command(&capture, c);

	mpd_connection_free(c);
	test_capture_deinit(&capture);
}
END_TEST

START_TEST(test_search_add_db_songs_to_playlist)
{
	struct test_capture capture;
	struct mpd_connection *c = test_capture_init(&capture);

	/* mpd_search_add_db_songs_to_playlist() emits a trailing space;
	   without a constraint it is the last character of the command
	   (this pins the string truncation bug fixed in 4754e8da) */
	ck_assert(mpd_search_add_db_songs_to_playlist(c, "foo"));
	ck_assert(mpd_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture),
			 "searchaddpl \"foo\" \n");
	abort_command(&capture, c);

	/* each constraint adds a leading space of its own, so there are
	   two spaces after the playlist name */
	ck_assert(mpd_search_add_db_songs_to_playlist(c, "foo"));
	ck_assert(mpd_search_add_tag_constraint(c, MPD_OPERATOR_DEFAULT,
						MPD_TAG_ARTIST, "Queen"));
	ck_assert(mpd_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture),
			 "searchaddpl \"foo\"  Artist \"Queen\"\n");
	abort_command(&capture, c);

	/* with sort and window */
	ck_assert(mpd_search_add_db_songs_to_playlist(c, "foo"));
	ck_assert(mpd_search_add_tag_constraint(c, MPD_OPERATOR_DEFAULT,
						MPD_TAG_ARTIST, "Queen"));
	ck_assert(mpd_search_add_sort_tag(c, MPD_TAG_DATE, false));
	ck_assert(mpd_search_add_window(c, 0, 10));
	ck_assert(mpd_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture),
			 "searchaddpl \"foo\"  Artist \"Queen\" sort Date window 0:10\n");
	abort_command(&capture, c);

	/* the playlist name must be escaped */
	ck_assert(mpd_search_add_db_songs_to_playlist(c, "double \" quote"));
	ck_assert(mpd_search_add_expression(c, "(Artist == \"Queen\")"));
	ck_assert(mpd_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture),
			 "searchaddpl \"double \\\" quote\"  \"(Artist == \\\"Queen\\\")\"\n");
	abort_command(&capture, c);

	/* an empty playlist name */
	ck_assert(mpd_search_add_db_songs_to_playlist(c, ""));
	ck_assert(mpd_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture),
			 "searchaddpl \"\" \n");
	abort_command(&capture, c);

	/* a cancelled request leaves no remains */
	ck_assert(mpd_search_add_db_songs_to_playlist(c, "foo"));
	mpd_search_cancel(c);
	ck_assert(mpd_search_add_db_songs_to_playlist(c, "bar"));
	ck_assert(mpd_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture),
			 "searchaddpl \"bar\" \n");
	abort_command(&capture, c);

	/* a second request without commit fails */
	ck_assert(mpd_search_add_db_songs_to_playlist(c, "foo"));
	ck_assert(!mpd_search_add_db_songs_to_playlist(c, "bar"));
	ck_assert_int_eq(mpd_connection_get_error(c), MPD_ERROR_STATE);
	ck_assert(mpd_connection_clear_error(c));
	mpd_search_cancel(c);

	mpd_connection_free(c);
	test_capture_deinit(&capture);
}
END_TEST

START_TEST(test_sticker_search)
{
	struct test_capture capture;
	struct mpd_connection *c = test_capture_init(&capture);

	/* note: unlike the base URI and the name, the sticker type is
	   inserted into the command without quoting */
	ck_assert(mpd_sticker_search_begin(c, "song", NULL, "rating"));
	ck_assert(mpd_sticker_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture),
			 "sticker find song \"\" \"rating\"\n");
	abort_command(&capture, c);

	/* a NULL base URI is equivalent to an empty one */
	ck_assert(mpd_sticker_search_begin(c, "song", "", "rating"));
	ck_assert(mpd_sticker_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture),
			 "sticker find song \"\" \"rating\"\n");
	abort_command(&capture, c);

	/* with a base URI */
	ck_assert(mpd_sticker_search_begin(c, "song", "foo/bar", "rating"));
	ck_assert(mpd_sticker_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture),
			 "sticker find song \"foo/bar\" \"rating\"\n");
	abort_command(&capture, c);

	/* with a value constraint */
	ck_assert(mpd_sticker_search_begin(c, "song", NULL, "rating"));
	ck_assert(mpd_sticker_search_add_value_constraint(c, MPD_STICKER_OP_EQ,
							  "5"));
	ck_assert(mpd_sticker_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture),
			 "sticker find song \"\" \"rating\" = \"5\"\n");
	abort_command(&capture, c);

	/* everything combined */
	ck_assert(mpd_sticker_search_begin(c, "song", "foo/bar", "rating"));
	ck_assert(mpd_sticker_search_add_value_constraint(c,
							  MPD_STICKER_OP_GT_INT,
							  "5"));
	ck_assert(mpd_sticker_search_add_sort(c, MPD_STICKER_SORT_VALUE_INT,
					      false));
	ck_assert(mpd_sticker_search_add_window(c, 0, 10));
	ck_assert(mpd_sticker_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture),
			 "sticker find song \"foo/bar\" \"rating\" gt \"5\" sort value_int window 0:10\n");
	abort_command(&capture, c);

	/* an open-ended window */
	ck_assert(mpd_sticker_search_begin(c, "song", NULL, "rating"));
	ck_assert(mpd_sticker_search_add_window(c, 5, UINT_MAX));
	ck_assert(mpd_sticker_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture),
			 "sticker find song \"\" \"rating\" window 5:\n");
	abort_command(&capture, c);

	mpd_connection_free(c);
	test_capture_deinit(&capture);
}
END_TEST

START_TEST(test_sticker_search_operators)
{
	static const struct {
		enum mpd_sticker_operator oper;
		const char *str;
	} operators[] = {
		{ MPD_STICKER_OP_EQ, "=" },
		{ MPD_STICKER_OP_GT, ">" },
		{ MPD_STICKER_OP_LT, "<" },
		{ MPD_STICKER_OP_EQ_INT, "eq" },
		{ MPD_STICKER_OP_GT_INT, "gt" },
		{ MPD_STICKER_OP_LT_INT, "lt" },
		{ MPD_STICKER_OP_CONTAINS, "contains" },
		{ MPD_STICKER_OP_STARTS_WITH, "starts_with" },
	};

	struct test_capture capture;
	struct mpd_connection *c = test_capture_init(&capture);

	for (size_t i = 0; i < sizeof(operators) / sizeof(operators[0]); ++i) {
		ck_assert(mpd_sticker_search_begin(c, "song", NULL, "rating"));
		ck_assert(mpd_sticker_search_add_value_constraint(c,
								  operators[i].oper,
								  "5"));
		ck_assert(mpd_sticker_search_commit(c));

		char expected[128];
		snprintf(expected, sizeof(expected),
			 "sticker find song \"\" \"rating\" %s \"5\"\n",
			 operators[i].str);
		ck_assert_str_eq(test_capture_receive(&capture), expected);
		abort_command(&capture, c);
	}

	/* an unknown operator is rejected; the request is left intact
	   and must be cancelled by the caller */
	ck_assert(mpd_sticker_search_begin(c, "song", NULL, "rating"));
	ck_assert(!mpd_sticker_search_add_value_constraint(c,
							   MPD_STICKER_OP_UNKNOWN,
							   "5"));
	mpd_sticker_search_cancel(c);

	mpd_connection_free(c);
	test_capture_deinit(&capture);
}
END_TEST

START_TEST(test_sticker_search_sort)
{
	struct test_capture capture;
	struct mpd_connection *c = test_capture_init(&capture);

	ck_assert(mpd_sticker_search_begin(c, "song", NULL, "rating"));
	ck_assert(mpd_sticker_search_add_sort(c, MPD_STICKER_SORT_URI, false));
	ck_assert(mpd_sticker_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture),
			 "sticker find song \"\" \"rating\" sort uri\n");
	abort_command(&capture, c);

	ck_assert(mpd_sticker_search_begin(c, "song", NULL, "rating"));
	ck_assert(mpd_sticker_search_add_sort(c, MPD_STICKER_SORT_VALUE, false));
	ck_assert(mpd_sticker_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture),
			 "sticker find song \"\" \"rating\" sort value\n");
	abort_command(&capture, c);

	/* descending sort is indicated by a "-" prefix */
	ck_assert(mpd_sticker_search_begin(c, "song", NULL, "rating"));
	ck_assert(mpd_sticker_search_add_sort(c, MPD_STICKER_SORT_VALUE_INT,
					      true));
	ck_assert(mpd_sticker_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture),
			 "sticker find song \"\" \"rating\" sort -value_int\n");
	abort_command(&capture, c);

	/* an unknown sort is rejected */
	ck_assert(mpd_sticker_search_begin(c, "song", NULL, "rating"));
	ck_assert(!mpd_sticker_search_add_sort(c, MPD_STICKER_SORT_UNKNOWN,
					       false));
	mpd_sticker_search_cancel(c);

	mpd_connection_free(c);
	test_capture_deinit(&capture);
}
END_TEST

START_TEST(test_sticker_search_quote)
{
	struct test_capture capture;
	struct mpd_connection *c = test_capture_init(&capture);

	/* the base URI and the name must be escaped */
	ck_assert(mpd_sticker_search_begin(c, "song", "double \" quote",
					   "back\\slash"));
	ck_assert(mpd_sticker_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture),
			 "sticker find song \"double \\\" quote\" \"back\\\\slash\"\n");
	abort_command(&capture, c);

	/* so must the constraint value */
	ck_assert(mpd_sticker_search_begin(c, "song", NULL, "rating"));
	ck_assert(mpd_sticker_search_add_value_constraint(c,
							  MPD_STICKER_OP_CONTAINS,
							  "double \" quote"));
	ck_assert(mpd_sticker_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture),
			 "sticker find song \"\" \"rating\" contains \"double \\\" quote\"\n");
	abort_command(&capture, c);

	mpd_connection_free(c);
	test_capture_deinit(&capture);
}
END_TEST

START_TEST(test_sticker_search_state)
{
	struct test_capture capture;
	struct mpd_connection *c = test_capture_init(&capture);

	/* committing without a search in progress fails */
	ck_assert(!mpd_sticker_search_commit(c));
	ck_assert_int_eq(mpd_connection_get_error(c), MPD_ERROR_STATE);
	ck_assert(mpd_connection_clear_error(c));

	/* so do the "add" functions */
	ck_assert(!mpd_sticker_search_add_value_constraint(c,
							   MPD_STICKER_OP_EQ,
							   "5"));
	ck_assert_int_eq(mpd_connection_get_error(c), MPD_ERROR_STATE);
	ck_assert(mpd_connection_clear_error(c));

	ck_assert(!mpd_sticker_search_add_sort(c, MPD_STICKER_SORT_URI, false));
	ck_assert_int_eq(mpd_connection_get_error(c), MPD_ERROR_STATE);
	ck_assert(mpd_connection_clear_error(c));

	ck_assert(!mpd_sticker_search_add_window(c, 0, 10));
	ck_assert_int_eq(mpd_connection_get_error(c), MPD_ERROR_STATE);
	ck_assert(mpd_connection_clear_error(c));

	/* a cancelled request leaves no remains */
	ck_assert(mpd_sticker_search_begin(c, "song", NULL, "rating"));
	mpd_sticker_search_cancel(c);

	/* a second mpd_sticker_search_begin() without commit fails */
	ck_assert(mpd_sticker_search_begin(c, "song", NULL, "rating"));
	ck_assert(!mpd_sticker_search_begin(c, "playlist", NULL, "foo"));
	ck_assert_int_eq(mpd_connection_get_error(c), MPD_ERROR_STATE);
	ck_assert(mpd_connection_clear_error(c));

	/* the first search is still intact and can be committed */
	ck_assert(mpd_sticker_search_commit(c));
	ck_assert_str_eq(test_capture_receive(&capture),
			 "sticker find song \"\" \"rating\"\n");
	abort_command(&capture, c);

	mpd_connection_free(c);
	test_capture_deinit(&capture);
}
END_TEST

START_TEST(test_player_commands)
{
	struct test_capture capture;
	struct mpd_connection *c = test_capture_init(&capture);

	ck_assert(mpd_send_seek_pos(c, 2, 120));
	ck_assert_str_eq(test_capture_receive(&capture), "seek \"2\" \"120\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_seek_id(c, 2, 120));
	ck_assert_str_eq(test_capture_receive(&capture), "seekid \"2\" \"120\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_seek_id_float(c, 2, 120.5));
	ck_assert_str_eq(test_capture_receive(&capture), "seekid \"2\" \"120.500\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_seek_current(c, 42, false));
	ck_assert_str_eq(test_capture_receive(&capture), "seekcur \"42.000\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_seek_current(c, 42, true));
	ck_assert_str_eq(test_capture_receive(&capture), "seekcur \"+42.000\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_seek_current(c, -42, false));
	ck_assert_str_eq(test_capture_receive(&capture), "seekcur \"-42.000\"\n");
	abort_command(&capture, c);

	mpd_connection_free(c);
	test_capture_deinit(&capture);
}
END_TEST

START_TEST(test_mount_commands)
{
	struct test_capture capture;
	struct mpd_connection *c = test_capture_init(&capture);

	ck_assert(mpd_send_list_mounts(c));
	ck_assert_str_eq(test_capture_receive(&capture), "listmounts\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_mount(c, "foo", "nfs://server/share"));
	ck_assert_str_eq(test_capture_receive(&capture), "mount \"foo\" \"nfs://server/share\"\n");
	abort_command(&capture, c);

	ck_assert(mpd_send_unmount(c, "foo"));
	ck_assert_str_eq(test_capture_receive(&capture), "unmount \"foo\"\n");
	abort_command(&capture, c);

	mpd_connection_free(c);
	test_capture_deinit(&capture);
}
END_TEST

#ifdef HAVE_SETLOCALE

START_TEST(test_locale)
{
	/* choosing a locale that uses the comma instead of the dot as
	   the decimal separator to see if this affects libmpdclient
	   (it should not because MPD expects the dot) */
	setlocale(LC_ALL, "de_DE@UTF-8");

	struct test_capture capture;
	struct mpd_connection *c = test_capture_init(&capture);

	ck_assert(mpd_send_seek_current(c, -42.5, false));
	ck_assert_str_eq(test_capture_receive(&capture), "seekcur \"-42.500\"\n");
	abort_command(&capture, c);

	mpd_connection_free(c);
	test_capture_deinit(&capture);

	setlocale(LC_ALL, "C");
}
END_TEST

#endif // HAVE_SETLOCALE

static Suite *
create_suite(void)
{
	Suite *s = suite_create("commands");

	TCase *tc_quote = tcase_create("quote");
	tcase_add_test(tc_quote, test_quote);
	suite_add_tcase(s, tc_quote);

	TCase *tc_capabilities = tcase_create("capabilities");
	tcase_add_test(tc_capabilities, test_capabilities_commands);
	suite_add_tcase(s, tc_capabilities);

	TCase *tc_queue = tcase_create("queue");
	tcase_add_test(tc_queue, test_queue_commands);
	suite_add_tcase(s, tc_queue);

	TCase *tc_playlist = tcase_create("playlist");
	tcase_add_test(tc_playlist, test_playlist_commands);
	suite_add_tcase(s, tc_playlist);

	TCase *tc_playlist_search = tcase_create("playlist_search");
	tcase_add_test(tc_playlist_search, test_playlist_search);
	tcase_add_test(tc_playlist_search, test_playlist_search_quote);
	tcase_add_test(tc_playlist_search, test_playlist_search_cancel);
	tcase_add_test(tc_playlist_search, test_playlist_search_state);
	suite_add_tcase(s, tc_playlist_search);

	TCase *tc_database = tcase_create("database");
	tcase_add_test(tc_database, test_database_commands);
	suite_add_tcase(s, tc_database);

	TCase *tc_search = tcase_create("search");
	tcase_add_test(tc_search, test_search);
	tcase_add_test(tc_search, test_expression);
	tcase_add_test(tc_search, test_list);
	tcase_add_test(tc_search, test_count);
	tcase_add_test(tc_search, test_search_add_db_songs_to_playlist);
	suite_add_tcase(s, tc_search);

	TCase *tc_sticker = tcase_create("sticker");
	tcase_add_test(tc_sticker, test_sticker_search);
	tcase_add_test(tc_sticker, test_sticker_search_operators);
	tcase_add_test(tc_sticker, test_sticker_search_sort);
	tcase_add_test(tc_sticker, test_sticker_search_quote);
	tcase_add_test(tc_sticker, test_sticker_search_state);
	suite_add_tcase(s, tc_sticker);

	TCase *tc_player = tcase_create("player");
	tcase_add_test(tc_player, test_player_commands);
	suite_add_tcase(s, tc_player);

	TCase *tc_mount = tcase_create("mount");
	tcase_add_test(tc_mount, test_mount_commands);
	suite_add_tcase(s, tc_mount);

#ifdef HAVE_SETLOCALE
	TCase *tc_locale = tcase_create("locale");
	tcase_add_test(tc_locale, test_locale);
	suite_add_tcase(s, tc_locale);
#endif // HAVE_SETLOCALE

	return s;
}

int
main(void)
{
	Suite *s = create_suite();
	SRunner *sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return number_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
