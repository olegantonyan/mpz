#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>

#include "playlist/cueparser.h"
#include "track.h"

#ifndef CUE_FIXTURES_DIR
#  error "CUE_FIXTURES_DIR must be defined by the build system"
#endif

using Playlist::CueParser;

// These tests run a curated set of real-world CUE sheets from various
// rippers/encodings through CueParser and assert on the parsed Track output.
// Expected strings for non-UTF-8 fixtures lock in the parser's CP1251 fallback
// behavior — those values were derived once by running `iconv -f CP1251` on
// the source files, not by guessing. If a parser change shifts the decoded
// output, update the constant deliberately.

namespace {

bool touch(const QString& path) {
  QFile f(path);
  return f.open(QIODevice::WriteOnly) && f.flush();
}

QString fixturePath(const char* fname) {
  return QStringLiteral(CUE_FIXTURES_DIR) + QChar('/') + QString::fromLatin1(fname);
}

// Copy a fixture cue into tempDir verbatim; touch each named audio stub so
// the parser's resolve_audio_file finds them. The stub filenames are passed
// as UTF-8 (some contain non-ASCII characters like ø, ß, ä, Cyrillic).
QString stage(const QTemporaryDir& dir,
              const char* fixture,
              std::initializer_list<const char*> audioStubs) {
  const QString dst = dir.filePath(QString::fromLatin1(fixture));
  if (!QFile::copy(fixturePath(fixture), dst)) {
    qWarning() << "stage(): copy failed for" << fixture;
    return {};
  }
  for (const char* s : audioStubs) {
    const QString p = dir.filePath(QString::fromUtf8(s));
    if (!touch(p)) {
      qWarning() << "stage(): touch failed for" << p;
      return {};
    }
  }
  return dst;
}

} // namespace

class TestCueParserFixtures : public QObject {
  Q_OBJECT
private slots:
  void init();

  void asciiSatyriconDeep();
  void asciiPainIAm();
  void asciiBurzumThuleanCd1();
  void asciiBeyondDawnElectric();
  void asciiBeyondDawnPity();
  void utf8bomBeyondDawnElectric_matchesAsciiTwin();
  void utf8bomDodheimsgardBmc();
  void utf8bomBthvnCd026();
  void utf8bomBthvnCd027();
  void utf8NachtreichSturmgang_noBom();
  void cp1252BthvnCd010_decodedAsCp1251();
  void cp1252BthvnCd005_decodedAsCp1251();
  void cp1251Wagner();
  void cp1251JungleBook();
  void multifileVibrasphereArchipelago();
  void multifileVibrasphereTributaries();
  void multifileNightwishImaginaerum_oneTrackPerFile();
  void multifileTherionFleurs_tracksGroupedPerFile();
  void multifileChapuisNoel_gapsAppendedCrossFile();
  void mp3BseDrivingInsane();
  void mp3BseCruelAndUnusual();
  void backslashDjTiestoTraffic();
  void isrcBorknagarFall();

private:
  QTemporaryDir tempDir;
};

void TestCueParserFixtures::init() {
  // Same pattern as tst_cueparser.cpp::init — wipe tempDir between slots so
  // stem-fallback lookups don't see stale files.
  QVERIFY(tempDir.isValid());
  QDir d(tempDir.path());
  const QStringList all = d.entryList(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot);
  for (const QString& f : all) {
    QVERIFY(d.remove(f));
  }
}

// ---------- pure-ASCII single-FILE cues ----------

void TestCueParserFixtures::asciiSatyriconDeep() {
  const QString cue = stage(tempDir, "ascii_satyricon_deep.cue",
                            {"Satyricon - Deep Calleth Upon Deep.flac"});
  QVERIFY(!cue.isEmpty());

  const auto tracks = CueParser(cue).tracks_list();
  QCOMPARE(tracks.size(), 8);
  QCOMPARE(tracks.first().album(),  QStringLiteral("Deep Calleth Upon Deep "));
  QCOMPARE(tracks.first().artist(), QStringLiteral("Satyricon"));
  QCOMPARE(tracks.first().year(),   quint16{2017});
  QCOMPARE(tracks.first().title(),  QStringLiteral("Midnight Serpent"));
  QCOMPARE(tracks.first().begin(),  quint32{0});
  QCOMPARE(tracks.at(1).title(),    QStringLiteral("Blood Cracks Open the Ground"));
  QCOMPARE(tracks.at(1).begin(),    quint32{380653});  // 06:20:49
  QCOMPARE(tracks.last().title(),   QStringLiteral("Burial Rite"));
  QCOMPARE(tracks.last().begin(),   quint32{2270320});  // 37:50:24
  QVERIFY(tracks.first().path().endsWith(QStringLiteral("Satyricon - Deep Calleth Upon Deep.flac")));
  for (const Track& t : tracks) QVERIFY(t.isCue());
}

void TestCueParserFixtures::asciiPainIAm() {
  const QString cue = stage(tempDir, "ascii_pain_i_am.cue", {"Pain - I Am.flac"});
  QVERIFY(!cue.isEmpty());

  const auto tracks = CueParser(cue).tracks_list();
  QCOMPARE(tracks.size(), 11);
  QCOMPARE(tracks.first().album(),  QStringLiteral("I Am"));
  QCOMPARE(tracks.first().artist(), QStringLiteral("Pain"));
  QCOMPARE(tracks.first().year(),   quint16{2024});
  QCOMPARE(tracks.first().title(),  QStringLiteral("I Just Dropped By (to say goodbye)"));
  QCOMPARE(tracks.first().begin(),  quint32{0});
  QCOMPARE(tracks.last().title(),   QStringLiteral("Fair Game"));
  QCOMPARE(tracks.last().begin(),   quint32{2214840});  // 36:54:63
}

void TestCueParserFixtures::asciiBurzumThuleanCd1() {
  const QString cue = stage(tempDir, "ascii_burzum_thulean_cd1.cue",
                            {"Burzum - Thulean Mysteries CD1 (BYE013CD).flac"});
  QVERIFY(!cue.isEmpty());

  const auto tracks = CueParser(cue).tracks_list();
  QCOMPARE(tracks.size(), 17);
  QCOMPARE(tracks.first().album(),  QStringLiteral("Thulean Mysteries CD1 (BYE013CD)"));
  QCOMPARE(tracks.first().artist(), QStringLiteral("Burzum"));
  QCOMPARE(tracks.first().year(),   quint16{2020});
  QCOMPARE(tracks.first().title(),  QStringLiteral("The Sacred Well"));
  QCOMPARE(tracks.at(1).begin(),    quint32{176680});  // 02:56:51 → 13251/75*1000
  QCOMPARE(tracks.last().title(),   QStringLiteral("Thulean Sorcery"));
  QCOMPARE(tracks.last().begin(),   quint32{2851946});  // 47:31:71
}

void TestCueParserFixtures::asciiBeyondDawnElectric() {
  const QString cue = stage(tempDir, "ascii_beyond_dawn_electric.cue",
                            {"Beyond Dawn - Electric Sulking Machine.flac"});
  QVERIFY(!cue.isEmpty());

  const auto tracks = CueParser(cue).tracks_list();
  QCOMPARE(tracks.size(), 10);
  QCOMPARE(tracks.first().album(),  QStringLiteral("Electric Sulking Machine"));
  QCOMPARE(tracks.first().year(),   quint16{1999});
  QCOMPARE(tracks.first().title(),  QStringLiteral("Violence Heals"));
  // INDEX 01 00:00:32 (32 frames) wins over INDEX 00 00:00:00.
  QCOMPARE(tracks.first().begin(),  quint32{426});
  // Track 6 ASCII version has plain "Aage" (no diacritic).
  QCOMPARE(tracks.at(5).title(),    QStringLiteral("Aage"));
  QCOMPARE(tracks.last().title(),   QStringLiteral("Hairy Liquor (Mer kraft i hver draabe)"));
  QCOMPARE(tracks.last().begin(),   quint32{2653093});  // 44:13:07
}

void TestCueParserFixtures::asciiBeyondDawnPity() {
  const QString cue = stage(tempDir, "ascii_beyond_dawn_pity.cue",
                            {"Beyond Dawn - Pity Love.flac"});
  QVERIFY(!cue.isEmpty());

  const auto tracks = CueParser(cue).tracks_list();
  QCOMPARE(tracks.size(), 9);
  QCOMPARE(tracks.first().album(),  QStringLiteral("Pity Love"));
  QCOMPARE(tracks.first().year(),   quint16{1995});
  QCOMPARE(tracks.first().title(),  QStringLiteral("When Beauty Dies"));
  // Track 2 has only INDEX 01 (no pregap). 04:56:20 = (4*60+56)*75+20 = 22220 → 296266ms.
  QCOMPARE(tracks.at(1).title(),    QStringLiteral("The Penance"));
  QCOMPARE(tracks.at(1).begin(),    quint32{296266});
  QCOMPARE(tracks.last().begin(),   quint32{2650826});  // 44:10:62
}

void TestCueParserFixtures::utf8bomBeyondDawnElectric_matchesAsciiTwin() {
  const QString cue = stage(tempDir, "utf8bom_beyond_dawn_electric.cue",
                            {"Beyond Dawn - Electric Sulking Machine.flac"});
  QVERIFY(!cue.isEmpty());

  const auto tracks = CueParser(cue).tracks_list();
  QCOMPARE(tracks.size(), 10);
  QCOMPARE(tracks.first().album(),  QStringLiteral("Electric Sulking Machine"));
  QCOMPARE(tracks.first().year(),   quint16{1999});
  // Same album as ascii_beyond_dawn_electric, but track 6 carries é.
  QCOMPARE(tracks.at(5).title(),    QString::fromUtf8("Aagé"));
  // Begins identical to the ASCII twin — encoding doesn't change INDEX math.
  QCOMPARE(tracks.first().begin(),  quint32{426});
  QCOMPARE(tracks.last().begin(),   quint32{2653093});
}

void TestCueParserFixtures::utf8bomDodheimsgardBmc() {
  // FILE references contain Norwegian ø — UTF-8 BOM, so decode goes through
  // the BOM-detected UTF-8 path and the stub filename must match exactly.
  const QString cue = stage(tempDir, "utf8bom_dodheimsgard_bmc.cue",
                            {"D\xC3\xB8""dheimsgard - Black Medium Current.flac"});
  QVERIFY(!cue.isEmpty());

  const auto tracks = CueParser(cue).tracks_list();
  QCOMPARE(tracks.size(), 9);
  QCOMPARE(tracks.first().album(),  QString::fromUtf8("Black Medium Current"));
  QCOMPARE(tracks.first().artist(), QString::fromUtf8("Dødheimsgard"));
  QCOMPARE(tracks.first().year(),   quint16{2023});
  QCOMPARE(tracks.first().title(),  QStringLiteral("Et Smelter"));
  QCOMPARE(tracks.first().begin(),  quint32{0});
  // Track 7 carries ø and Ø-adjacent characters in the title.
  QCOMPARE(tracks.at(6).title(),    QString::fromUtf8("Det Tomme Kalde Mørke"));
  QCOMPARE(tracks.last().title(),   QStringLiteral("Requiem Aeternum"));
  QCOMPARE(tracks.last().begin(),   quint32{3867160});  // 64:27:12
  QVERIFY(tracks.first().path().endsWith(QString::fromUtf8("Dødheimsgard - Black Medium Current.flac")));
}

void TestCueParserFixtures::utf8bomBthvnCd026() {
  const QString cue = stage(tempDir, "utf8bom_bthvn_cd026.cue",
                            {"BTHVN 2020 - CD 026.flac"});
  QVERIFY(!cue.isEmpty());

  const auto tracks = CueParser(cue).tracks_list();
  QCOMPARE(tracks.size(), 21);
  QCOMPARE(tracks.first().artist(), QStringLiteral("Ludwig van Beethoven"));
  QCOMPARE(tracks.first().album(),
           QStringLiteral("BTHVN 2020: CD 026 Egmont / The Ruins of Athens / Gratulations-Menuett"));
  QCOMPARE(tracks.first().year(),   quint16{2019});
  // Title contains "Egmont" with proper UTF-8 curly quotes (no umlauts).
  QCOMPARE(tracks.first().title(),
           QString::fromUtf8("Music to Johann Wolfgang von Goethe's Tragedy \xE2\x80\x9C" "Egmont\xE2\x80\x9D, op. 84: Overture. Sostenuto, ma non troppo - Allegro"));
  QCOMPARE(tracks.last().begin(),   quint32{4658880});  // 77:38:66
}

void TestCueParserFixtures::utf8bomBthvnCd027() {
  const QString cue = stage(tempDir, "utf8bom_bthvn_cd027.cue",
                            {"BTHVN 2020 - CD 027.flac"});
  QVERIFY(!cue.isEmpty());

  const auto tracks = CueParser(cue).tracks_list();
  QCOMPARE(tracks.size(), 19);
  QCOMPARE(tracks.first().artist(), QStringLiteral("Ludwig van Beethoven"));
  QCOMPARE(tracks.first().year(),   quint16{2019});
  QCOMPARE(tracks.first().begin(),  quint32{0});
  // Last track INDEX 01 70:30:32 → 317282 frames → 4230426ms (truncated).
  QCOMPARE(tracks.last().begin(),   quint32{4230426});
}

void TestCueParserFixtures::utf8NachtreichSturmgang_noBom() {
  // UTF-8 *without* BOM (and so QStringConverter::encodingForData returns
  // empty; strict-UTF-8 path is taken). Also exercises one-FILE-per-track
  // layout where each TRACK's INDEX 01 is 0.
  const QString cue = stage(tempDir, "utf8_nachtreich_sturmgang.cue", {
    "01 Im Sturmwind.wav",
    "02 In die Ferne.wav",
    "03 Abschied (Ein letzter Gru\xC3\x9F...).wav",       // ß
    "04 Der verlorene Weg.wav",
    "05 Vom Sturm getrieben.wav",
    "06 Erinnerungen.wav",
    "07 Abendd\xC3\xA4mmerung.wav",                       // ä
    "08 Lust der Sturmnacht.wav"});
  QVERIFY(!cue.isEmpty());

  const auto tracks = CueParser(cue).tracks_list();
  QCOMPARE(tracks.size(), 8);
  QCOMPARE(tracks.first().album(),  QStringLiteral("Sturmgang"));
  QCOMPARE(tracks.first().artist(), QStringLiteral("Nachtreich"));
  QCOMPARE(tracks.first().year(),   quint16{2019});
  QCOMPARE(tracks.first().title(),  QStringLiteral("Im Sturmwind"));
  // Each FILE has one TRACK starting at INDEX 01 00:00:00.
  for (const Track& t : tracks) {
    QCOMPARE(t.begin(), quint32{0});
  }
  QCOMPARE(tracks.at(6).title(),    QString::fromUtf8("Abenddämmerung"));
  QCOMPARE(tracks.at(2).title(),    QString::fromUtf8("Abschied (Ein letzter Gruß...)"));
  QVERIFY(tracks.at(6).path().endsWith(QString::fromUtf8("07 Abenddämmerung.wav")));
}

// ---------- non-UTF-8 cues: CP1251 fallback decode ----------

void TestCueParserFixtures::cp1252BthvnCd010_decodedAsCp1251() {
  // Source is Windows-1252 (Beethoven liner notes with curly quotes 0x93/0x94
  // and German umlauts 0xF6/...). Qt6 parser path falls back to CP1251 decode;
  // Qt5 path falls back to lenient UTF-8 and emits U+FFFD per invalid byte.
  // Either way the structural fields (count, INDEX timings) match.
  const QString cue = stage(tempDir, "cp1252_bthvn_cd010.cue",
                            {"BTHVN 2020 - CD 010.flac"});
  QVERIFY(!cue.isEmpty());

  const auto tracks = CueParser(cue).tracks_list();
  QCOMPARE(tracks.size(), 5);
  QCOMPARE(tracks.first().artist(), QStringLiteral("Ludwig van Beethoven"));
  QCOMPARE(tracks.first().album(),
           QStringLiteral("BTHVN 2020: CD 010 Symphony no. 9 (Vienna Cycle)"));
  QCOMPARE(tracks.first().year(),   quint16{2019});
  QCOMPARE(tracks.first().title(),
           QStringLiteral("Symphony no. 9 in d, op. 125: I. Allegro ma non troppo, un poco maestoso"));
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
  // Qt6: CP1251 fallback. 0x93/0x94/0x96 share punctuation slots with CP1252
  // and decode correctly to U+201C/U+201D/U+2013; 0xF6 mangles ö → ц (U+0446).
  QCOMPARE(tracks.last().title(),
           QString::fromUtf8(u8"Symphony no. 9 in d, op. 125: V. Presto - “O Freunde, nicht diese Tцne!” – Allegro assai - Allegro assai: “Freude, schцner Gцtterfunken” - Allegro assai vivace. Alla marcia - Andante maestoso - Allegro energico, sempre ben marcato - Allegro ma non tanto - Prestissimo"));
#else
  // Qt5: lenient UTF-8 fallback. All 8 high bytes (0x93×3, 0x94×3, 0x96, 0xF6×3)
  // emit U+FFFD replacement characters.
  QCOMPARE(tracks.last().title(),
           QString::fromUtf8(u8"Symphony no. 9 in d, op. 125: V. Presto - �O Freunde, nicht diese T�ne!� � Allegro assai - Allegro assai: �Freude, sch�ner G�tterfunken� - Allegro assai vivace. Alla marcia - Andante maestoso - Allegro energico, sempre ben marcato - Allegro ma non tanto - Prestissimo"));
#endif
  QCOMPARE(tracks.last().begin(),   quint32{3125760});  // 52:05:57
}

void TestCueParserFixtures::cp1252BthvnCd005_decodedAsCp1251() {
  const QString cue = stage(tempDir, "cp1252_bthvn_cd005.cue",
                            {"BTHVN 2020 - CD 005.flac"});
  QVERIFY(!cue.isEmpty());

  const auto tracks = CueParser(cue).tracks_list();
  QCOMPARE(tracks.size(), 5);
  QCOMPARE(tracks.first().artist(), QStringLiteral("Ludwig van Beethoven"));
  QCOMPARE(tracks.first().year(),   quint16{1984});
  // INDEX 01 00:00:33 (33 frames at 75fps) → 440 ms exact.
  QCOMPARE(tracks.first().begin(),  quint32{440});
  QCOMPARE(tracks.first().title(),
           QStringLiteral("Symphonie No. 9 d-Moll, Op. 125: I. Allegro ma non troppo, un poco maestoso"));
  // Track 5 has ASCII straight quotes (unchanged) and one 0xF6 byte (ö in CP1252).
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
  // Qt6 CP1251 fallback: ö → ц (U+0446).
  QCOMPARE(tracks.last().title(),
           QString::fromUtf8(u8"Symphonie No. 9 d-Moll, Op. 125: V. Presto - 'O Freunde, nicht diese Tцne!' - Allegro assai - Allegro assai vivace. Alla marcia - Andante maestoso - Allegro energico, sempre ben marcato"));
#else
  // Qt5 lenient UTF-8: ö → U+FFFD.
  QCOMPARE(tracks.last().title(),
           QString::fromUtf8(u8"Symphonie No. 9 d-Moll, Op. 125: V. Presto - 'O Freunde, nicht diese T�ne!' - Allegro assai - Allegro assai vivace. Alla marcia - Andante maestoso - Allegro energico, sempre ben marcato"));
#endif
  QCOMPARE(tracks.last().begin(),   quint32{2938840});  // 48:58:63
}

void TestCueParserFixtures::cp1251Wagner() {
  // Cue and FILE clause are pure CP1251 Cyrillic. On Qt6, the parser's CP1251
  // fallback decodes both successfully and finds the stubbed UTF-8 audio file.
  // On Qt5, lenient UTF-8 decode produces U+FFFD for every Cyrillic byte, so
  // resolve_audio_file can't match the stub and the whole cue yields no
  // tracks — different behaviour, but lock both in as regression baselines.
  const QString cue = stage(tempDir, "cp1251_wagner.cue",
                            {u8"Вагнер Рихард - Великие композиторы.wav"});
  QVERIFY(!cue.isEmpty());

  const auto tracks = CueParser(cue).tracks_list();
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
  QCOMPARE(tracks.size(), 6);
  QCOMPARE(tracks.first().album(),  QString::fromUtf8(u8"Великие композиторы"));
  QCOMPARE(tracks.first().artist(), QString::fromUtf8(u8"Вагнер Рихард"));
  QCOMPARE(tracks.first().year(),   quint16{0});  // No REM DATE in this cue.
  QCOMPARE(tracks.first().title(),  QString::fromUtf8(u8"[Валькирия] Полет валькирий"));
  QCOMPARE(tracks.first().begin(),  quint32{0});
  // Last track INDEX 01 53:44:68 → 241868 frames → 3224906ms.
  QCOMPARE(tracks.last().begin(),   quint32{3224906});
  QVERIFY(tracks.first().path().endsWith(QString::fromUtf8(u8"Вагнер Рихард - Великие композиторы.wav")));
#else
  QCOMPARE(tracks.size(), 0);
#endif
}

void TestCueParserFixtures::cp1251JungleBook() {
  // CP1251 album title, ASCII track titles, multi-FILE EAC "gaps appended"
  // layout (each track's INDEX 00 pregap sits at the tail of the previous file,
  // its INDEX 01 audio at the head of its own file), plus the PREGAP directive
  // (silently ignored). The cue's FILE 03 line carries a CP1251 byte 0xDC (Ь)
  // inside the filename — on Qt6 the CP1251 fallback decodes it and the stub
  // matches; on Qt5 lenient UTF-8 turns it into U+FFFD, the FILE 03 stub can't
  // be resolved, and the track whose audio lives in FILE 03 (track 3) gets
  // dropped — 7 surviving tracks.
  const QString cue = stage(tempDir, "cp1251_jungle_book.cue", {
    "01 - Mega-Phone - Hard Dramma.wav",
    "02 - Plexus - Protein.wav",
    "03 - Radiotrance vs DJ Dan - Byaz\xD0\xAC (Poison).wav",  // CP1251 0xDC → U+042C 'Ь'
    "04 - Aqualung - Chillout XTC.wav",
    "05 - Djungl vs Dad D - Sessions.wav",
    "06 - The Crude Project - Another Side Of The Sun.wav",
    "07 - DJ Dan - Gun Power.wav",
    "08 - The Crude Project - Base Problems.wav"});
  QVERIFY(!cue.isEmpty());

  const auto tracks = CueParser(cue).tracks_list();
  QCOMPARE(tracks.first().artist(), QStringLiteral("Mega-Phone"));  // Per-track PERFORMER wins.
  QCOMPARE(tracks.first().year(),   quint16{1997});
  QCOMPARE(tracks.first().title(),  QStringLiteral("Hard Dramma"));
  QCOMPARE(tracks.first().begin(),  quint32{0});
  // Track 2's audio is the whole of FILE 02 (INDEX 01 00:00:00); its INDEX 00
  // pregap at 05:29:56 belongs to the tail of FILE 01 and is discarded.
  QCOMPARE(tracks.at(1).title(),    QStringLiteral("Protein"));
  QCOMPARE(tracks.at(1).artist(),   QStringLiteral("Plexus"));
  QCOMPARE(tracks.at(1).begin(),    quint32{0});
  QVERIFY(tracks.at(1).path().endsWith(QStringLiteral("02 - Plexus - Protein.wav")));
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
  QCOMPARE(tracks.size(), 8);
  QCOMPARE(tracks.first().album(),  QString::fromUtf8(u8"Книга джунглей. Том 1"));
  QCOMPARE(tracks.at(7).artist(),   QStringLiteral("The Crude Project"));
#else
  // Qt5: FILE 03 unresolved → track 4 dropped. tracks.last() is track 8.
  QCOMPARE(tracks.size(), 7);
  QCOMPARE(tracks.last().artist(),  QStringLiteral("The Crude Project"));
#endif
}

// ---------- multi-FILE cues ----------

void TestCueParserFixtures::multifileVibrasphereArchipelago() {
  // Capitalized .CUE extension; interleaved INDEX 00 / FILE / INDEX 01 layout
  // where the pregap of the next track is declared under the previous FILE.
  const QString cue = stage(tempDir, "multifile_vibrasphere_archipelago.CUE", {
    "01-Vibrasphere - Tierra azul-2006.wav",
    "02-Vibrasphere - Sweet september-2006.wav",
    "03-Vibrasphere - Reservoir-2006.wav",
    "04-Vibrasphere - Landmark-2006.wav",
    "05-Vibrasphere - Seven days to daylight-2006.wav",
    "06-Vibrasphere - Morning breath-2006.wav",
    "07-Vibrasphere - Sudden comfort-2006.wav",
    "08-Vibrasphere - Baltic resonance-2006.wav",
    "09-Vibrasphere - Late winter storms-2006.wav"});
  QVERIFY(!cue.isEmpty());

  const auto tracks = CueParser(cue).tracks_list();
  QCOMPARE(tracks.size(), 9);
  QCOMPARE(tracks.first().album(),  QStringLiteral("Archipelago"));
  QCOMPARE(tracks.first().artist(), QStringLiteral("Vibrasphere"));
  QCOMPARE(tracks.first().year(),   quint16{2006});
  QCOMPARE(tracks.first().title(),  QStringLiteral("Tierra azul"));
  // Track 2's audio is the whole of file 02 (INDEX 01 0); its INDEX 00 06:55:59
  // pregap rode at the tail of file 01 and is discarded.
  QCOMPARE(tracks.at(1).title(),    QStringLiteral("Sweet september"));
  QCOMPARE(tracks.at(1).begin(),    quint32{0});
  QVERIFY(tracks.at(1).path().endsWith(QStringLiteral("02-Vibrasphere - Sweet september-2006.wav")));
  // Track 3 jumps to file=03 at INDEX 01 0.
  QCOMPARE(tracks.at(2).title(),    QStringLiteral("Reservoir"));
  QCOMPARE(tracks.at(2).begin(),    quint32{0});
  QVERIFY(tracks.at(2).path().endsWith(QStringLiteral("03-Vibrasphere - Reservoir-2006.wav")));
  // Last track on file=09.
  QCOMPARE(tracks.last().title(),   QStringLiteral("Late winter storm"));
  QCOMPARE(tracks.last().begin(),   quint32{0});
  QVERIFY(tracks.last().path().endsWith(QStringLiteral("09-Vibrasphere - Late winter storms-2006.wav")));
}

void TestCueParserFixtures::multifileVibrasphereTributaries() {
  const QString cue = stage(tempDir, "multifile_vibrasphere_tributaries.CUE", {
    "01 - Unknown Artist - Track01.wav",
    "02 - Unknown Artist - Track02.wav",
    "03 - Unknown Artist - Track03.wav",
    "04 - Unknown Artist - Track04.wav",
    "05 - Unknown Artist - Track05.wav",
    "06 - Unknown Artist - Track06.wav",
    "07 - Unknown Artist - Track07.wav",
    "08 - Unknown Artist - Track08.wav",
    "09 - Unknown Artist - Track09.wav",
    "10 - Unknown Artist - Track10.wav",
    "11 - Unknown Artist - Track11.wav"});
  QVERIFY(!cue.isEmpty());

  const auto tracks = CueParser(cue).tracks_list();
  QCOMPARE(tracks.size(), 11);
  QCOMPARE(tracks.first().album(),  QStringLiteral("Unknown Title"));
  QCOMPARE(tracks.first().artist(), QStringLiteral("Unknown Artist"));
  QCOMPARE(tracks.first().title(),  QStringLiteral("Track01"));
  // Track 11's audio is the whole of file 11 (INDEX 01 0); its INDEX 00 06:31:27
  // pregap rode at the tail of file 10 and is discarded.
  QCOMPARE(tracks.at(10).title(),   QStringLiteral("Track11"));
  QCOMPARE(tracks.at(10).begin(),   quint32{0});
  QVERIFY(tracks.at(10).path().endsWith(QStringLiteral("11 - Unknown Artist - Track11.wav")));
}

void TestCueParserFixtures::multifileNightwishImaginaerum_oneTrackPerFile() {
  // One FILE per TRACK, every track at INDEX 01 00:00:00 — each track is a whole
  // standalone file. This is the layout that made multi-file playback stop after
  // every file (each track is the last/only track in its file).
  const QString cue = stage(tempDir, "multifile_nightwish_imaginaerum.cue", {
    "01. Taikatalvi.flac",
    "02. Storytime.flac",
    "03. Ghost River.flac",
    "04. Slow, Love, Slow.flac",
    "05. I Want My Tears Back.flac",
    "06. Scaretale.flac",
    "07. Arabesque.flac",
    "08. Turn Loose the Mermaids.flac",
    "09. Rest Calm.flac",
    "10. The Crow, the Owl and the Dove.flac",
    "11. Last Ride of the Day.flac",
    "12. Song of Myself.flac",
    "13. Imaginaerum.flac",
    "14. The Heart Asks Pleasure First (Theme from the Movie 'Piano') [Michael Nyman cover] [bonus track].flac"});
  QVERIFY(!cue.isEmpty());

  const auto tracks = CueParser(cue).tracks_list();
  QCOMPARE(tracks.size(), 14);
  QCOMPARE(tracks.first().album(),  QStringLiteral("Imaginaerum"));
  QCOMPARE(tracks.first().artist(), QStringLiteral("Nightwish"));
  QCOMPARE(tracks.first().year(),   quint16{2011});
  QCOMPARE(tracks.first().title(),  QStringLiteral("Taikatalvi"));
  QCOMPARE(tracks.at(1).title(),    QStringLiteral("Storytime"));
  // Every track is its own FILE, each starting at 0; assert distinct per-file paths.
  for (int i = 0; i < tracks.size(); ++i) {
    QCOMPARE(tracks.at(i).begin(), quint32{0});
  }
  QVERIFY(tracks.first().path().endsWith(QStringLiteral("01. Taikatalvi.flac")));
  QVERIFY(tracks.at(11).path().endsWith(QStringLiteral("12. Song of Myself.flac")));
  // Bonus track: brackets and apostrophes in both title and filename parse intact.
  QCOMPARE(tracks.last().title(),
           QStringLiteral("The Heart Asks Pleasure First (Theme from the Movie 'Piano') [Michael Nyman cover] [bonus track]"));
  QVERIFY(tracks.last().path().endsWith(
            QStringLiteral("14. The Heart Asks Pleasure First (Theme from the Movie 'Piano') [Michael Nyman cover] [bonus track].flac")));
}

void TestCueParserFixtures::multifileTherionFleurs_tracksGroupedPerFile() {
  // Mixed layout: one cue, four files (a/b/c/d.flac), several tracks per file.
  // Each file's first track starts at 0; later tracks are mid-file (same FILE).
  const QString cue = stage(tempDir, "multifile_therion_fleurs.cue",
                            {"a.flac", "b.flac", "c.flac", "d.flac"});
  QVERIFY(!cue.isEmpty());

  const auto tracks = CueParser(cue).tracks_list();
  QCOMPARE(tracks.size(), 15);
  QCOMPARE(tracks.first().album(),  QStringLiteral("Les Fleurs Du Mal"));
  QCOMPARE(tracks.first().artist(), QStringLiteral("Therion"));
  QCOMPARE(tracks.first().year(),   quint16{2012});

  // Per-file grouping: 1-4 -> a, 5-7 -> b, 8-11 -> c, 12-15 -> d.
  for (int i = 0; i < 4; ++i) {
    QVERIFY(tracks.at(i).path().endsWith(QStringLiteral("a.flac")));
  }
  for (int i = 4; i < 7; ++i) {
    QVERIFY(tracks.at(i).path().endsWith(QStringLiteral("b.flac")));
  }
  for (int i = 7; i < 11; ++i) {
    QVERIFY(tracks.at(i).path().endsWith(QStringLiteral("c.flac")));
  }
  for (int i = 11; i < 15; ++i) {
    QVERIFY(tracks.at(i).path().endsWith(QStringLiteral("d.flac")));
  }

  // First track of each file starts at 0.
  QCOMPARE(tracks.at(0).begin(),  quint32{0});
  QCOMPARE(tracks.at(4).begin(),  quint32{0});
  QCOMPARE(tracks.at(7).begin(),  quint32{0});
  QCOMPARE(tracks.at(11).begin(), quint32{0});
  // Sampled mid-file INDEX 01 02:50:48 = (2*60+50)*75+48 = 12798 frames -> 170640 ms.
  QCOMPARE(tracks.at(1).title(),  QStringLiteral("Une Fleur Dans Le C?ur"));
  QCOMPARE(tracks.at(1).begin(),  quint32{170640});
}

void TestCueParserFixtures::multifileChapuisNoel_gapsAppendedCrossFile() {
  // The exact cue from the bug report: a 12-track EAC "gaps appended to previous
  // file" rip, one FLAC per track. Every track 2..12 has its INDEX 00 pregap at
  // the tail of the previous file and its INDEX 01 audio at 0 in its own file.
  // Pre-fix, tracks 2..12 bound to the previous file at the pregap offset and
  // played a few seconds of trailing silence; each must bind to its own file at
  // begin 0. FILE names are NFC; we touch identical stubs and assert on them.
  static const char* files[] = {
    "01 Noël étranger (Noël VIII).flac",
    "02 Noël en dialogue, duo, trio (Noël II).flac",
    "03 A minuit fut fait un Réveil.flac",
    "04 Quoy ma Voisine es tu faché.flac",
    "05 A minuit fut fait un Réveil.flac",
    "06 Puer nobis nascitur.flac",
    "07 Allons voir ce divin Gage.flac",
    "08 Chanton de Voix Hautaine.flac",
    "09 Prélude - A la venue de Noël.flac",
    "10 Joseph est bien marié.flac",
    "11 Où s'en vont Ces gais bergers.flac",
    "12 Au jô deu de pubelle - Grand déi, ribon ribeine.flac",
  };
  const QString cue = tempDir.filePath(QStringLiteral("multifile_chapuis_noel.cue"));
  QVERIFY(QFile::copy(fixturePath("multifile_chapuis_noel.cue"), cue));
  for (const char* f : files) {
    QVERIFY(touch(tempDir.filePath(QString::fromUtf8(f))));
  }

  const auto tracks = CueParser(cue).tracks_list();
  QCOMPARE(tracks.size(), 12);
  for (int i = 0; i < 12; ++i) {
    QCOMPARE(tracks.at(i).begin(), quint32{0});
    QVERIFY(tracks.at(i).path().endsWith(QString::fromUtf8(files[i])));
  }
  QCOMPARE(tracks.first().artist(), QStringLiteral("Michel Chapuis"));
  QCOMPARE(tracks.first().year(),   quint16{1968});
  QVERIFY(tracks.first().album().contains(QStringLiteral("Organ Music for Christmas")));
}

// ---------- MP3-as-WAVE cues ----------

void TestCueParserFixtures::mp3BseDrivingInsane() {
  const QString cue = stage(tempDir, "mp3_bse_driving_insane.cue",
                            {"201-va_-_driving_insane_mixed_by_black_sun_empire-boss.mp3"});
  QVERIFY(!cue.isEmpty());

  const auto tracks = CueParser(cue).tracks_list();
  QCOMPARE(tracks.size(), 22);
  QCOMPARE(tracks.first().album(),  QStringLiteral("Driving Insane-Retail 2CD"));
  QCOMPARE(tracks.first().artist(), QStringLiteral("Black Sun Empire"));
  QCOMPARE(tracks.first().year(),   quint16{2004});
  QCOMPARE(tracks.first().title(),  QStringLiteral("Cryogenic (BSE remix)"));
  // 61:55:55 = (61*60+55)*75+55 = 278680 → 3715733ms (truncated from .333).
  QCOMPARE(tracks.last().title(),   QStringLiteral("Epilogue VIP"));
  QCOMPARE(tracks.last().begin(),   quint32{3715733});
  QVERIFY(tracks.first().path().endsWith(QStringLiteral(".mp3")));
}

void TestCueParserFixtures::mp3BseCruelAndUnusual() {
  const QString cue = stage(tempDir, "mp3_bse_cruel_and_unusual.cue",
                            {"201-va-cruel_and_unusual.mp3"});
  QVERIFY(!cue.isEmpty());

  const auto tracks = CueParser(cue).tracks_list();
  QCOMPARE(tracks.size(), 24);
  QCOMPARE(tracks.first().album(),  QStringLiteral("Cruel and Unusual"));
  QCOMPARE(tracks.first().artist(), QStringLiteral("Black Sun Empire"));  // Per-track PERFORMER.
  QCOMPARE(tracks.first().year(),   quint16{2005});
  // Last INDEX 01 65:33:20 = (65*60+33)*75+20 = 295010 → 3933466ms.
  QCOMPARE(tracks.last().title(),   QStringLiteral("Beautiful Morning (Black Sun Empire Remix)"));
  QCOMPARE(tracks.last().begin(),   quint32{3933266});
}

// ---------- file-resolution edge cases ----------

void TestCueParserFixtures::backslashDjTiestoTraffic() {
  // Cue references "DJ Tiлsto\Traffic\01-...wav" (CP1251 + Windows backslashes
  // and a subpath that doesn't exist). On Unix, backslashes get rewritten to
  // forward slashes, the canonical path lookup fails, the parent-dir scan
  // fails, and the parser falls back to looking up the bare basename in the
  // cue's directory — that's where our stubs live.
  const QString cue = stage(tempDir, "backslash_dj_tiesto_traffic.cue", {
    "01-Traffic (Radio Edit).wav",
    "02-Traffic (Original Mix).wav",
    "03-Traffic (Max Walder Remix).wav"});
  QVERIFY(!cue.isEmpty());

  const auto tracks = CueParser(cue).tracks_list();
  QCOMPARE(tracks.size(), 3);
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
  // Qt6 CP1251 fallback: "ë" (byte 0xEB) decodes to "л" (U+043B).
  QCOMPARE(tracks.first().artist(), QString::fromUtf8(u8"DJ Tiлsto"));
#else
  // Qt5 lenient UTF-8: byte 0xEB is invalid → U+FFFD.
  QCOMPARE(tracks.first().artist(), QString::fromUtf8(u8"DJ Ti�sto"));
#endif
  QCOMPARE(tracks.first().album(),  QStringLiteral("Traffic"));
  QCOMPARE(tracks.first().year(),   quint16{2003});
  QCOMPARE(tracks.first().title(),  QStringLiteral("Traffic (Radio Edit)"));
  QVERIFY(tracks.first().path().endsWith(QStringLiteral("01-Traffic (Radio Edit).wav")));
  QVERIFY(tracks.last().path().endsWith(QStringLiteral("03-Traffic (Max Walder Remix).wav")));
  // All three tracks start at INDEX 01 00:00:00 — each is its own FILE.
  for (const Track& t : tracks) {
    QCOMPARE(t.begin(), quint32{0});
  }
}

void TestCueParserFixtures::isrcBorknagarFall() {
  // ISRC and per-track REM COMPOSER lines should be silently ignored.
  const QString cue = stage(tempDir, "isrc_borknagar_fall.cue",
                            {"Borknagar - Fall.flac"});
  QVERIFY(!cue.isEmpty());

  const auto tracks = CueParser(cue).tracks_list();
  QCOMPARE(tracks.size(), 8);
  QCOMPARE(tracks.first().album(),  QStringLiteral("Fall"));
  QCOMPARE(tracks.first().artist(), QStringLiteral("Borknagar"));
  QCOMPARE(tracks.first().year(),   quint16{2024});
  QCOMPARE(tracks.first().title(),  QStringLiteral("Summits"));
  QCOMPARE(tracks.first().begin(),  quint32{0});
  // Track 2 INDEX 01 07:58:41 = (7*60+58)*75+41 = 35891 → 478546ms.
  QCOMPARE(tracks.at(1).begin(),    quint32{478546});
  QCOMPARE(tracks.last().title(),   QStringLiteral("Northward"));
  // INDEX 01 44:33:71 = (44*60+33)*75+71 = 200546 → 2673946ms.
  QCOMPARE(tracks.last().begin(),   quint32{2673946});
}

QTEST_GUILESS_MAIN(TestCueParserFixtures)
#include "tst_cueparser_fixtures.moc"
