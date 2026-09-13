#include "config.h"
#include <mpd/pair.h>
#include <mpd/parser.h>
#include <mpd/song.h>

#include <check.h>

#include <stdlib.h>
#include <limits.h>
#include <time.h>

#ifdef HAVE_SETLOCALE
#include <locale.h>
#endif

START_TEST(test_song_parser)
{
	ck_assert_ptr_null(mpd_song_begin(&(const struct mpd_pair){"invalid", "dummy"}));

	struct mpd_song *song = mpd_song_begin(&(const struct mpd_pair){"file", "foo.ogg"});
	ck_assert_ptr_nonnull(song);

	ck_assert_str_eq(mpd_song_get_uri(song), "foo.ogg");
	ck_assert_ptr_null(mpd_song_get_real_uri(song));
	ck_assert_uint_eq(mpd_song_get_id(song), 0);
	ck_assert_uint_eq(mpd_song_get_duration(song), 0);
	ck_assert_uint_eq(mpd_song_get_duration_ms(song), 0);
	ck_assert_uint_eq(mpd_song_get_start_ms(song), 0);
	ck_assert_uint_eq(mpd_song_get_start(song), 0);
	ck_assert_uint_eq(mpd_song_get_end_ms(song), 0);
	ck_assert_uint_eq(mpd_song_get_end(song), 0);

	ck_assert(mpd_song_feed(song, &(const struct mpd_pair){"Range", "3-4.1"}));
	ck_assert_uint_eq(mpd_song_get_start_ms(song), 3000);
	ck_assert_uint_eq(mpd_song_get_start(song), 3);
	ck_assert_uint_eq(mpd_song_get_end_ms(song), 4100);
	ck_assert_uint_eq(mpd_song_get_end(song), 4);

	ck_assert(mpd_song_feed(song, &(const struct mpd_pair){"Range", "-5"}));
	ck_assert_uint_eq(mpd_song_get_start_ms(song), 0);
	ck_assert_uint_eq(mpd_song_get_start(song), 0);
	ck_assert_uint_eq(mpd_song_get_end_ms(song), 5000);
	ck_assert_uint_eq(mpd_song_get_end(song), 5);

	ck_assert(mpd_song_feed(song, &(const struct mpd_pair){"RealUri", "bar.ogg"}));
	ck_assert_str_eq(mpd_song_get_real_uri(song), "bar.ogg");
	ck_assert(mpd_song_feed(song, &(const struct mpd_pair){"RealUri", "leak.ogg"}));
	ck_assert_str_eq(mpd_song_get_real_uri(song), "leak.ogg");

	ck_assert(mpd_song_feed(song, &(const struct mpd_pair){"Id", "42"}));
	ck_assert_uint_eq(mpd_song_get_id(song), 42);

	ck_assert(mpd_song_feed(song, &(const struct mpd_pair){"duration", "12.3456"}));
	ck_assert_uint_eq(mpd_song_get_duration_ms(song), 12346);
	ck_assert_uint_eq(mpd_song_get_duration(song), 12);

	ck_assert(mpd_song_feed(song, &(const struct mpd_pair){"duration", "34.5"}));
	ck_assert_uint_eq(mpd_song_get_duration_ms(song), 34500);
	ck_assert_uint_eq(mpd_song_get_duration(song), 35);

	ck_assert(mpd_song_feed(song, &(const struct mpd_pair){"Time", "33"}));
	ck_assert_uint_eq(mpd_song_get_duration_ms(song), 34500);
	ck_assert_uint_eq(mpd_song_get_duration(song), 33);

	mpd_song_free(song);
}
END_TEST

#ifdef HAVE_SETLOCALE

START_TEST(test_locale)
{
	/* choosing a locale that uses the comma instead of the dot as
	   the decimal separator to see if this affects libmpdclient
	   (it should not because MPD expects the dot) */
	setlocale(LC_ALL, "de_DE@UTF-8");

	struct mpd_song *song = mpd_song_begin(&(const struct mpd_pair){"file", "foo.ogg"});
	ck_assert_ptr_nonnull(song);

	ck_assert(mpd_song_feed(song, &(const struct mpd_pair){"duration", "12.3456"}));
	ck_assert_uint_eq(mpd_song_get_duration_ms(song), 12346);
	ck_assert_uint_eq(mpd_song_get_duration(song), 12);

	mpd_song_free(song);

	setlocale(LC_ALL, "C");
}
END_TEST

#endif // HAVE_SETLOCALE

static Suite *
create_suite(void)
{
	Suite *s = suite_create("commands");

	TCase *tc_song = tcase_create("song");
	tcase_add_test(tc_song, test_song_parser);
	suite_add_tcase(s, tc_song);

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
