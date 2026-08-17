#include "fakes.h"

#include "luma/core/app-context.h"
#include "luma/core/app-manager.h"
#include "luma/core/file-storage.h"
#include "luma/core/in-memory-storage.h"
#include "luma/core/input-manager.h"
#include "luma/apps/about-app.h"
#include "luma/apps/notes-app.h"
#include "luma/apps/settings-app.h"
#include "luma/core/settings.h"
#include "luma/luma.h"
#include "luma/platform/host/host-audio-adapter.h"
#include "luma/ui/components.h"
#include "luma/ui/font.h"
#include "luma/ui/layout.h"
#include "luma/ui/theme.h"

#include <unity.h>

#ifdef _WIN32
#include <direct.h>
#include <process.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <cstring>
#include <string>
#include <vector>

using luma::AppContext;
using luma::AppManager;
using luma::InputAction;
using luma::InputManager;
using luma::Luma;
using luma::Settings;
using luma::layout::appCardBounds;
using luma::layout::kHeaderLogoSize;
using luma::layout::kHeaderLogoX;
using luma::layout::kHeaderLogoY;
using luma::theme::kAccent;
using luma::theme::kTsuyukusa;
using luma::theme::kWakatake;
using luma::theme::kYamabuki;
using luma::test::CountingStorage;
using luma::test::FakeAudio;
using luma::test::FakeClock;
using luma::test::FakeDiagnostics;
using luma::test::FakeDisplay;
using luma::test::FakeInputSource;
using luma::test::RecordingApp;
using luma::test::makeAction;
using luma::test::makeText;

namespace {

std::string nativeTestStorageRoot(const char* suffix) {
#ifdef _WIN32
    const auto process_id = _getpid();
#else
    const auto process_id = getpid();
#endif
    return "build/native-test-storage-" + std::to_string(process_id) + "-" + suffix;
}

struct AppManagerFixture {
    FakeDisplay display;
    Settings settings;
    luma::test::CountingStorage storage;
    FakeClock clock;
    FakeDiagnostics diagnostics;
    AppContext context;
    AppManager manager;
    std::vector<std::string> log;
    RecordingApp launcher;
    RecordingApp about;

    AppManagerFixture()
        : context(display, settings, storage, clock, diagnostics),
          manager(context, diagnostics),
          launcher("launcher", "Launcher", '\0', log),
          about("about", "About", 'a', log) {
        manager.registerApp({&launcher, launcher.id(), launcher.name(), launcher.shortcut()});
        manager.registerApp({&about, about.id(), about.name(), about.shortcut()});
    }
};

}  // namespace

void test_app_manager_lifecycle_order() {
    AppManagerFixture fixture;

    TEST_ASSERT_TRUE(fixture.manager.enter("launcher"));
    fixture.manager.drawIfNeeded();
    TEST_ASSERT_TRUE(fixture.manager.enter("about"));
    fixture.manager.drawIfNeeded();

    TEST_ASSERT_EQUAL_UINT(5, fixture.log.size());
    TEST_ASSERT_EQUAL_STRING("launcher:enter", fixture.log[0].c_str());
    TEST_ASSERT_EQUAL_STRING("launcher:draw", fixture.log[1].c_str());
    TEST_ASSERT_EQUAL_STRING("launcher:exit", fixture.log[2].c_str());
    TEST_ASSERT_EQUAL_STRING("about:enter", fixture.log[3].c_str());
    TEST_ASSERT_EQUAL_STRING("about:draw", fixture.log[4].c_str());
    TEST_ASSERT_TRUE(fixture.diagnostics.contains("[APP] enter launcher"));
    TEST_ASSERT_TRUE(fixture.diagnostics.contains("[APP] exit launcher"));
    TEST_ASSERT_TRUE(fixture.diagnostics.contains("[APP] enter about"));
}

void test_app_manager_back_returns_to_launcher() {
    AppManagerFixture fixture;
    fixture.manager.enter("launcher");
    fixture.manager.drawIfNeeded();
    fixture.manager.enter("about");
    fixture.manager.drawIfNeeded();
    fixture.log.clear();

    fixture.manager.dispatch(makeAction(InputAction::Back));
    fixture.manager.drawIfNeeded();

    TEST_ASSERT_EQUAL_STRING("launcher", fixture.manager.currentId());
    TEST_ASSERT_EQUAL_UINT(4, fixture.log.size());
    TEST_ASSERT_EQUAL_STRING("about:update", fixture.log[0].c_str());
    TEST_ASSERT_EQUAL_STRING("about:exit", fixture.log[1].c_str());
    TEST_ASSERT_EQUAL_STRING("launcher:enter", fixture.log[2].c_str());
    TEST_ASSERT_EQUAL_STRING("launcher:draw", fixture.log[3].c_str());
}

void test_app_manager_launcher_back_is_noop() {
    AppManagerFixture fixture;
    fixture.manager.enter("launcher");
    fixture.manager.drawIfNeeded();
    fixture.log.clear();

    fixture.manager.dispatch(makeAction(InputAction::Back));
    fixture.manager.drawIfNeeded();

    TEST_ASSERT_EQUAL_STRING("launcher", fixture.manager.currentId());
    TEST_ASSERT_EQUAL_UINT(0, fixture.log.size());
}

void test_app_manager_shortcut_opens_registered_app() {
    AppManagerFixture fixture;
    fixture.manager.enter("launcher");
    fixture.manager.drawIfNeeded();
    fixture.log.clear();

    fixture.manager.dispatch(makeText('a'));
    fixture.manager.drawIfNeeded();

    TEST_ASSERT_EQUAL_STRING("about", fixture.manager.currentId());
    TEST_ASSERT_EQUAL_STRING("launcher:exit", fixture.log[0].c_str());
    TEST_ASSERT_EQUAL_STRING("about:enter", fixture.log[1].c_str());
    TEST_ASSERT_EQUAL_STRING("about:draw", fixture.log[2].c_str());
}

void enterLauncher(Luma& luma, FakeClock& clock) {
    clock.now = 1000;
    luma.update();
}

void test_luma_begin_shows_boot_screen() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    CountingStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    Luma luma(display, input, clock, storage, settings, diagnostics, audio);

    luma.begin();

    TEST_ASSERT_TRUE(display.begun);
    TEST_ASSERT_TRUE(audio.begun);
    TEST_ASSERT_TRUE(storage.begun);
    TEST_ASSERT_EQUAL_STRING("", luma.currentAppId());
    TEST_ASSERT_TRUE(diagnostics.contains("[BOOT] Luma Cardputer ADV started"));
    TEST_ASSERT_FALSE(diagnostics.contains("[APP] enter launcher"));
    TEST_ASSERT_EQUAL_UINT(1, display.bitmaps.size());
    TEST_ASSERT_EQUAL_INT(96, display.bitmaps[0].width);
    TEST_ASSERT_EQUAL_INT(96, display.bitmaps[0].height);
}

void test_luma_enters_launcher_after_boot_timeout() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    CountingStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    Luma luma(display, input, clock, storage, settings, diagnostics, audio);

    luma.begin();
    enterLauncher(luma, clock);

    TEST_ASSERT_EQUAL_STRING("launcher", luma.currentAppId());
    TEST_ASSERT_TRUE(diagnostics.contains("[APP] enter launcher"));
    TEST_ASSERT_TRUE(display.hasText("LUMA"));
    TEST_ASSERT_TRUE(display.hasText("SETTINGS"));
    TEST_ASSERT_TRUE(display.hasText("ABOUT"));
    TEST_ASSERT_TRUE(display.hasText("NOTES"));
    TEST_ASSERT_FALSE(display.hasText("Launcher"));
    TEST_ASSERT_FALSE(display.hasText("LAUNCHER"));
    TEST_ASSERT_EQUAL_UINT8(80, display.brightness);
    TEST_ASSERT_FALSE(display.hasText("Enter Open"));
    TEST_ASSERT_TRUE(display.hasText("--:--"));
    TEST_ASSERT_TRUE(display.hasBitmap({kHeaderLogoX, kHeaderLogoY}, kHeaderLogoSize,
                                      kHeaderLogoSize));
    TEST_ASSERT_TRUE(display.hasFill(appCardBounds(0, 0), kTsuyukusa));
    const auto selected = appCardBounds(0, 0);
    const luma::Rect inner{selected.x + 1, selected.y + 1, selected.w - 2, selected.h - 2};
    TEST_ASSERT_FALSE(display.hasStroke(inner, kTsuyukusa));
}

void test_luma_boot_skips_on_input() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    CountingStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    Luma luma(display, input, clock, storage, settings, diagnostics, audio);

    luma.begin();
    input.push(makeAction(InputAction::Confirm));
    luma.update();

    TEST_ASSERT_EQUAL_STRING("launcher", luma.currentAppId());
    TEST_ASSERT_TRUE(display.hasStroke(appCardBounds(0, 0), kTsuyukusa));
}

void test_luma_draws_only_when_dirty() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    CountingStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    Luma luma(display, input, clock, storage, settings, diagnostics, audio);

    luma.begin();
    enterLauncher(luma, clock);
    const int after_launcher = display.draw_text_count;
    luma.update();
    TEST_ASSERT_EQUAL_INT(after_launcher, display.draw_text_count);
}

void test_luma_processes_deferred_saves_each_update() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    CountingStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    Luma luma(display, input, clock, storage, settings, diagnostics, audio);

    luma.begin();
    TEST_ASSERT_EQUAL_INT(0, storage.flush_count);
    luma.update();
    TEST_ASSERT_EQUAL_INT(1, storage.flush_count);
    luma.update();
    TEST_ASSERT_EQUAL_INT(2, storage.flush_count);
}

void test_luma_routes_input_frame_to_app_manager() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    CountingStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    std::vector<std::string> log;
    RecordingApp extra("extra", "Extra", 'x', log);
    FakeAudio audio;
    Luma luma(display, input, clock, storage, settings, diagnostics, audio);

    luma.registerApp(extra);
    luma.begin();
    enterLauncher(luma, clock);
    input.push(makeText('x'));
    luma.update();

    TEST_ASSERT_EQUAL_STRING("extra", luma.currentAppId());
}

void test_luma_letter_does_not_open_stub_app() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    CountingStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    Luma luma(display, input, clock, storage, settings, diagnostics, audio);

    luma.begin();
    enterLauncher(luma, clock);
    input.push(makeText('s'));
    luma.update();

    TEST_ASSERT_EQUAL_STRING("launcher", luma.currentAppId());
}

void test_launcher_confirm_opens_settings() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    CountingStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    Luma luma(display, input, clock, storage, settings, diagnostics, audio);

    luma.begin();
    enterLauncher(luma, clock);
    input.push(makeAction(InputAction::Confirm));
    luma.update();

    TEST_ASSERT_EQUAL_STRING("settings", luma.currentAppId());
    TEST_ASSERT_TRUE(display.hasText("SETTINGS"));
    TEST_ASSERT_TRUE(display.hasText("Display"));
    TEST_ASSERT_TRUE(display.hasText("Sound"));
    TEST_ASSERT_TRUE(display.hasText("Network"));
    TEST_ASSERT_TRUE(display.hasText("Power"));
    TEST_ASSERT_TRUE(display.hasText("System"));
    TEST_ASSERT_TRUE(display.hasText("Brightness"));
    TEST_ASSERT_TRUE(display.hasText("80%"));
    TEST_ASSERT_TRUE(display.hasText("Theme"));
    TEST_ASSERT_TRUE(display.hasText("Dark"));
    TEST_ASSERT_TRUE(display.hasText("Ent"));
    TEST_ASSERT_TRUE(display.hasText("ok"));
    TEST_ASSERT_TRUE(display.hasText("Esc"));
    TEST_ASSERT_TRUE(display.hasText("back"));
    TEST_ASSERT_FALSE(display.hasText("On"));
    TEST_ASSERT_FALSE(display.hasText("About"));
    TEST_ASSERT_FALSE(display.hasText("Coming soon"));
    TEST_ASSERT_TRUE(display.hasFill({9, 37, 82, 14}, kTsuyukusa));
    TEST_ASSERT_FALSE(display.hasFill({9, 37, 82, 14}, kAccent));
}

void test_stub_back_returns_to_launcher() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    CountingStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    Luma luma(display, input, clock, storage, settings, diagnostics, audio);

    luma.begin();
    enterLauncher(luma, clock);
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    input.push(makeAction(InputAction::Back));
    luma.update();

    TEST_ASSERT_EQUAL_STRING("launcher", luma.currentAppId());
    TEST_ASSERT_TRUE(diagnostics.contains("[APP] exit settings"));
    TEST_ASSERT_TRUE(diagnostics.contains("[APP] enter launcher"));
}

void test_launcher_navigation_stays_in_bounds() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    CountingStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    Luma luma(display, input, clock, storage, settings, diagnostics, audio);

    luma.begin();
    enterLauncher(luma, clock);
    TEST_ASSERT_TRUE(display.hasStroke(appCardBounds(0, 0), kTsuyukusa));
    TEST_ASSERT_TRUE(display.hasFill(appCardBounds(0, 0), kTsuyukusa));

    input.push(makeAction(InputAction::Right));
    luma.update();
    TEST_ASSERT_TRUE(display.hasStroke(appCardBounds(1, 0), kYamabuki));
    TEST_ASSERT_TRUE(display.hasFill(appCardBounds(1, 0), kYamabuki));

    input.push(makeAction(InputAction::Left));
    luma.update();
    input.push(makeAction(InputAction::Down));
    luma.update();
    TEST_ASSERT_TRUE(display.hasStroke(appCardBounds(0, 1), kWakatake));

    input.push(makeAction(InputAction::Right));
    luma.update();
    TEST_ASSERT_TRUE(display.hasStroke(appCardBounds(0, 1), kWakatake));
}

void test_launcher_header_updates_when_minute_changes() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    CountingStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    Luma luma(display, input, clock, storage, settings, diagnostics, audio);

    clock.civil.hour = 10;
    clock.civil.minute = 0;
    clock.civil.valid = true;
    luma.begin();
    enterLauncher(luma, clock);
    TEST_ASSERT_TRUE(display.hasText("10:00"));

    clock.civil.minute = 1;
    luma.update();
    TEST_ASSERT_TRUE(display.hasText("10:01"));
}

void test_ui_dialog_draws_title_and_body() {
    FakeDisplay display;
    display.beginFrame();
    luma::drawDialog(display, luma::theme::paletteFor(0), "Title", "Body");

    TEST_ASSERT_TRUE(display.hasText("Title"));
    TEST_ASSERT_TRUE(display.hasText("Body"));
    TEST_ASSERT_TRUE(display.hasStroke({30, 30, 180, 75}, kAccent));
}

void test_input_manager_dispatches_fake_source() {
    FakeInputSource source;
    FakeDiagnostics diagnostics;
    InputManager manager(source, diagnostics);

    source.push(makeAction(InputAction::Confirm));
    source.push(makeText('x'));

    luma::InputFrame frame;
    TEST_ASSERT_TRUE(manager.poll(frame));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(InputAction::Confirm),
                            static_cast<uint8_t>(frame.action));
    TEST_ASSERT_TRUE(diagnostics.contains("[KEY] ENTER"));

    TEST_ASSERT_TRUE(manager.poll(frame));
    TEST_ASSERT_EQUAL_INT(1, frame.textLength);
    TEST_ASSERT_EQUAL_INT('x', frame.text[0]);
    TEST_ASSERT_TRUE(diagnostics.contains("[KEY] x"));
}

void test_in_memory_storage_round_trips_notes() {
    luma::InMemoryStorage storage;
    TEST_ASSERT_TRUE(storage.begin());

    const char payload[] = "hello notes";
    TEST_ASSERT_TRUE(storage.writeFileAtomic("/apps/notes/notes.txt", payload, 11));

    char buffer[32] = {};
    size_t length = 0;
    TEST_ASSERT_TRUE(storage.readFile("/apps/notes/notes.txt", buffer, sizeof(buffer), length));
    TEST_ASSERT_EQUAL_UINT(11, length);
    TEST_ASSERT_EQUAL_STRING("hello notes", buffer);
}

void test_file_storage_persists_across_instances() {
    const std::string root = nativeTestStorageRoot("persistence");
    luma::FileStorage first(root.c_str());
    TEST_ASSERT_TRUE(first.begin());
    TEST_ASSERT_TRUE(first.writeFileAtomic("/apps/notes/notes.txt", "persist-me", 10));

    luma::FileStorage second(root.c_str());
    TEST_ASSERT_TRUE(second.begin());

    char buffer[32] = {};
    size_t length = 0;
    TEST_ASSERT_TRUE(second.readFile("/apps/notes/notes.txt", buffer, sizeof(buffer), length));
    TEST_ASSERT_EQUAL_UINT(10, length);
    TEST_ASSERT_EQUAL_STRING("persist-me", buffer);
}

void test_file_storage_keeps_previous_file_when_replace_fails() {
    const std::string root = nativeTestStorageRoot("replace");
    luma::FileStorage storage(root.c_str());
    TEST_ASSERT_TRUE(storage.begin());
    TEST_ASSERT_TRUE(storage.writeFileAtomic("/apps/notes/notes.txt", "keep-me", 7));

#ifdef _WIN32
    const std::string blocked_path = root + "/apps/notes/notes.txt.tmp";
    _mkdir(blocked_path.c_str());
#else
    const std::string blocked_path = root + "/apps/notes/notes.txt.tmp";
    mkdir(blocked_path.c_str(), 0755);
#endif

    TEST_ASSERT_FALSE(storage.writeFileAtomic("/apps/notes/notes.txt", "overwrite", 9));

    char buffer[32] = {};
    size_t length = 0;
    TEST_ASSERT_TRUE(storage.readFile("/apps/notes/notes.txt", buffer, sizeof(buffer), length));
    TEST_ASSERT_EQUAL_UINT(7, length);
    TEST_ASSERT_EQUAL_STRING("keep-me", buffer);
}

void test_ui_font_lowercase_is_not_shifted_by_backslash_comment() {
    // A trailing "// \" comment splices out the next glyph and shifts a-z by +1
    // (Settings -> Sfuujoht, About -> Acpvu).
    TEST_ASSERT_EQUAL_HEX16(0x6000, luma::font::glyphRows('e', false)[4]);
    TEST_ASSERT_EQUAL_HEX16(0xF000, luma::font::glyphRows('e', false)[6]);
    TEST_ASSERT_EQUAL_HEX16(0xE000, luma::font::glyphRows(']', false)[1]);
    TEST_ASSERT_EQUAL_HEX16(0x8000, luma::font::glyphRows('b', false)[2]);
}

void test_settings_loads_valid_keys_independently() {
    luma::InMemoryStorage storage;
    FakeDiagnostics diagnostics;
    FakeClock clock;
    uint8_t brightness = 40;
    uint8_t volume = 20;
    uint8_t theme = 1;
    uint8_t schema = 1;
    TEST_ASSERT_TRUE(storage.savePref("brightness", &brightness, 1));
    TEST_ASSERT_TRUE(storage.savePref("volume", &volume, 1));
    TEST_ASSERT_TRUE(storage.savePref("theme", &theme, 1));
    TEST_ASSERT_TRUE(storage.savePref("schema", &schema, 1));

    Settings settings;
    settings.attach(storage, diagnostics, clock);
    settings.load();

    TEST_ASSERT_EQUAL_UINT8(40, settings.brightness());
    TEST_ASSERT_EQUAL_UINT8(20, settings.volume());
    TEST_ASSERT_EQUAL_UINT8(1, settings.theme());
    TEST_ASSERT_EQUAL_UINT8(1, settings.schema());
}

void test_settings_invalid_key_falls_back_without_wiping_others() {
    luma::InMemoryStorage storage;
    FakeDiagnostics diagnostics;
    FakeClock clock;
    uint8_t brightness = 40;
    uint8_t theme = 9;
    uint8_t volume = 200;
    TEST_ASSERT_TRUE(storage.savePref("brightness", &brightness, 1));
    TEST_ASSERT_TRUE(storage.savePref("theme", &theme, 1));
    TEST_ASSERT_TRUE(storage.savePref("volume", &volume, 1));

    Settings settings;
    settings.attach(storage, diagnostics, clock);
    settings.load();

    TEST_ASSERT_EQUAL_UINT8(40, settings.brightness());
    TEST_ASSERT_EQUAL_UINT8(80, settings.volume());
    TEST_ASSERT_EQUAL_UINT8(0, settings.theme());
    TEST_ASSERT_EQUAL_UINT8(1, settings.schema());
}

void test_settings_ignores_legacy_sound_key() {
    luma::InMemoryStorage storage;
    FakeDiagnostics diagnostics;
    FakeClock clock;
    uint8_t sound = 0;
    TEST_ASSERT_TRUE(storage.savePref("sound", &sound, 1));

    Settings settings;
    settings.attach(storage, diagnostics, clock);
    settings.load();

    TEST_ASSERT_EQUAL_UINT8(80, settings.volume());
}

void test_settings_debounce_then_flush() {
    luma::InMemoryStorage storage;
    FakeDiagnostics diagnostics;
    FakeClock clock;
    Settings settings;
    settings.attach(storage, diagnostics, clock);

    clock.now = 100;
    settings.setBrightness(70);
    settings.processDeferredSaves(400);
    uint8_t stored = 0;
    TEST_ASSERT_FALSE(storage.loadPref("brightness", &stored, 1));

    settings.processDeferredSaves(600);
    TEST_ASSERT_TRUE(storage.loadPref("brightness", &stored, 1));
    TEST_ASSERT_EQUAL_UINT8(70, stored);
    TEST_ASSERT_TRUE(diagnostics.contains("[SETTINGS] saved"));
}

void test_settings_app_steps_brightness_and_applies() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    luma::InMemoryStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    Luma luma(display, input, clock, storage, settings, diagnostics, audio);

    luma.begin();
    enterLauncher(luma, clock);
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    audio.events.clear();
    input.push(makeAction(InputAction::Right));
    luma.update();

    TEST_ASSERT_EQUAL_UINT8(90, settings.brightness());
    TEST_ASSERT_EQUAL_UINT8(90, display.brightness);
    TEST_ASSERT_TRUE(display.hasText("90%"));
    TEST_ASSERT_EQUAL_UINT(1, audio.events.size());
    TEST_ASSERT_EQUAL_STRING("click", audio.events[0].c_str());
}

void test_settings_flush_on_exit_before_debounce() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    luma::InMemoryStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    Luma luma(display, input, clock, storage, settings, diagnostics, audio);

    luma.begin();
    enterLauncher(luma, clock);
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    clock.now = 2000;
    input.push(makeAction(InputAction::Right));
    luma.update();
    input.push(makeAction(InputAction::Back));
    luma.update();
    input.push(makeAction(InputAction::Back));
    luma.update();

    uint8_t stored = 0;
    TEST_ASSERT_TRUE(storage.loadPref("brightness", &stored, 1));
    TEST_ASSERT_EQUAL_UINT8(90, stored);
    TEST_ASSERT_TRUE(diagnostics.contains("[SETTINGS] saved"));
}

void test_settings_volume_zero_does_not_play_click() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    luma::InMemoryStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    Luma luma(display, input, clock, storage, settings, diagnostics, audio);

    luma.begin();
    enterLauncher(luma, clock);
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    input.push(makeAction(InputAction::Down));
    luma.update();
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    for (int i = 0; i < 8; ++i) {
        input.push(makeAction(InputAction::Left));
        luma.update();
    }
    input.push(makeAction(InputAction::Back));
    luma.update();
    audio.events.clear();
    input.push(makeAction(InputAction::Confirm));
    luma.update();

    TEST_ASSERT_EQUAL_UINT8(0, settings.volume());
    TEST_ASSERT_EQUAL_UINT8(0, audio.volume);
    TEST_ASSERT_EQUAL_UINT(0, audio.events.size());
}

void test_settings_volume_steps_and_applies() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    luma::InMemoryStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    Luma luma(display, input, clock, storage, settings, diagnostics, audio);

    luma.begin();
    enterLauncher(luma, clock);
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    input.push(makeAction(InputAction::Down));
    luma.update();
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    audio.events.clear();
    input.push(makeAction(InputAction::Right));
    luma.update();

    TEST_ASSERT_EQUAL_UINT8(90, settings.volume());
    TEST_ASSERT_EQUAL_UINT8(90, audio.volume);
    TEST_ASSERT_TRUE(display.hasText("Volume"));
    TEST_ASSERT_TRUE(display.hasText("90%"));
    TEST_ASSERT_EQUAL_UINT(1, audio.events.size());
    TEST_ASSERT_EQUAL_STRING("click", audio.events[0].c_str());
}

void test_theme_palette_inverts_for_light() {
    const auto dark = luma::theme::paletteFor(0);
    const auto light = luma::theme::paletteFor(1);
    TEST_ASSERT_TRUE(luma::colorsEqual(dark.canvas, luma::theme::kKuro));
    TEST_ASSERT_TRUE(luma::colorsEqual(dark.card, luma::theme::kSumi));
    TEST_ASSERT_TRUE(luma::colorsEqual(dark.primary_text, luma::theme::kGofun));
    TEST_ASSERT_TRUE(luma::colorsEqual(light.canvas, luma::theme::kGofun));
    TEST_ASSERT_TRUE(luma::colorsEqual(light.card, luma::theme::kShironezumi));
    TEST_ASSERT_TRUE(luma::colorsEqual(dark.secondary_text, luma::theme::kGinnezumi));
    TEST_ASSERT_TRUE(luma::colorsEqual(light.secondary_text, luma::theme::kGinnezumi));
    TEST_ASSERT_TRUE(luma::colorsEqual(light.primary_text, luma::theme::kSumi));
    TEST_ASSERT_TRUE(luma::colorsEqual(light.boot_canvas, luma::theme::kGofun));
    TEST_ASSERT_TRUE(luma::colorsEqual(dark.accent, kAccent));
    TEST_ASSERT_TRUE(luma::colorsEqual(light.accent, kAccent));
}

void test_app_accent_is_identity_color() {
    luma::SettingsApp settings_app;
    luma::AboutApp about_app;
    luma::NotesApp notes_app;
    std::vector<std::string> log;
    RecordingApp extra("extra", "Extra", 'x', log);

    TEST_ASSERT_TRUE(luma::colorsEqual(settings_app.accent(), kTsuyukusa));
    TEST_ASSERT_TRUE(luma::colorsEqual(about_app.accent(), kYamabuki));
    TEST_ASSERT_TRUE(luma::colorsEqual(notes_app.accent(), kWakatake));
    TEST_ASSERT_TRUE(luma::colorsEqual(extra.accent(), kAccent));
}

void test_settings_opens_about_with_build_identity() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    CountingStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    Luma luma(display, input, clock, storage, settings, diagnostics, audio);

    luma.begin();
    enterLauncher(luma, clock);
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    input.push(makeAction(InputAction::Down));
    luma.update();
    input.push(makeAction(InputAction::Down));
    luma.update();
    input.push(makeAction(InputAction::Down));
    luma.update();
    input.push(makeAction(InputAction::Down));
    luma.update();
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    input.push(makeAction(InputAction::Confirm));
    luma.update();

    TEST_ASSERT_EQUAL_STRING("about", luma.currentAppId());
    TEST_ASSERT_TRUE(display.hasText("ABOUT"));
    TEST_ASSERT_TRUE(display.hasText("0.1"));
    TEST_ASSERT_TRUE(display.hasText("Cardputer ADV"));
    TEST_ASSERT_TRUE(display.hasText("native"));
    TEST_ASSERT_TRUE(display.hasText("MIT"));
}

void test_settings_category_left_right_does_not_change_brightness() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    luma::InMemoryStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    Luma luma(display, input, clock, storage, settings, diagnostics, audio);

    luma.begin();
    enterLauncher(luma, clock);
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    input.push(makeAction(InputAction::Right));
    luma.update();
    input.push(makeAction(InputAction::Left));
    luma.update();

    TEST_ASSERT_EQUAL_STRING("settings", luma.currentAppId());
    TEST_ASSERT_EQUAL_UINT8(80, settings.brightness());
    TEST_ASSERT_TRUE(display.hasText("80%"));
}

void test_settings_detail_back_stays_in_settings() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    luma::InMemoryStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    Luma luma(display, input, clock, storage, settings, diagnostics, audio);

    luma.begin();
    enterLauncher(luma, clock);
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    input.push(makeAction(InputAction::Right));
    luma.update();
    input.push(makeAction(InputAction::Back));
    luma.update();
    input.push(makeAction(InputAction::Right));
    luma.update();

    TEST_ASSERT_EQUAL_STRING("settings", luma.currentAppId());
    TEST_ASSERT_EQUAL_UINT8(90, settings.brightness());
    TEST_ASSERT_FALSE(diagnostics.contains("[APP] exit settings"));
}

void test_settings_detail_up_down_stays_bounded() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    luma::InMemoryStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    Luma luma(display, input, clock, storage, settings, diagnostics, audio);

    luma.begin();
    enterLauncher(luma, clock);
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    input.push(makeAction(InputAction::Down));
    luma.update();
    input.push(makeAction(InputAction::Down));
    luma.update();
    input.push(makeAction(InputAction::Confirm));
    luma.update();

    TEST_ASSERT_EQUAL_STRING("settings", luma.currentAppId());
    TEST_ASSERT_EQUAL_UINT8(1, settings.theme());
    TEST_ASSERT_TRUE(display.hasText("Light"));
    TEST_ASSERT_EQUAL_UINT8(80, settings.brightness());
}

void test_settings_placeholder_confirm_stays() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    luma::InMemoryStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    Luma luma(display, input, clock, storage, settings, diagnostics, audio);

    luma.begin();
    enterLauncher(luma, clock);
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    input.push(makeAction(InputAction::Down));
    luma.update();
    input.push(makeAction(InputAction::Down));
    luma.update();
    input.push(makeAction(InputAction::Confirm));
    luma.update();

    TEST_ASSERT_TRUE(display.hasText("Wi-Fi"));
    TEST_ASSERT_TRUE(display.hasText("Time zone"));
    TEST_ASSERT_TRUE(display.hasText("--"));

    input.push(makeAction(InputAction::Confirm));
    luma.update();
    TEST_ASSERT_EQUAL_STRING("settings", luma.currentAppId());

    input.push(makeAction(InputAction::Back));
    luma.update();
    input.push(makeAction(InputAction::Down));
    luma.update();
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    TEST_ASSERT_TRUE(display.hasText("Battery"));
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    TEST_ASSERT_EQUAL_STRING("settings", luma.currentAppId());
}

void test_header_time_updates_outside_launcher() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    CountingStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    Luma luma(display, input, clock, storage, settings, diagnostics, audio);

    clock.civil.hour = 10;
    clock.civil.minute = 0;
    clock.civil.valid = true;
    luma.begin();
    enterLauncher(luma, clock);
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    TEST_ASSERT_TRUE(display.hasText("10:00"));

    clock.civil.minute = 1;
    luma.update();
    TEST_ASSERT_TRUE(display.hasText("10:01"));
}

void enterNotes(Luma& luma, FakeInputSource& input) {
    input.push(makeAction(InputAction::Down));
    luma.update();
    input.push(makeAction(InputAction::Confirm));
    luma.update();
}

void typeText(Luma& luma, FakeInputSource& input, const char* text) {
    for (const char* cursor = text; *cursor != '\0'; ++cursor) {
        input.push(makeText(*cursor));
        luma.update();
    }
}

void test_notes_round_trips_multiline_text() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    luma::InMemoryStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    Luma luma(display, input, clock, storage, settings, diagnostics, audio);

    luma.begin();
    enterLauncher(luma, clock);
    enterNotes(luma, input);
    typeText(luma, input, "ab");
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    typeText(luma, input, "cd");
    input.push(makeAction(InputAction::Back));
    luma.update();
    enterNotes(luma, input);

    TEST_ASSERT_TRUE(display.hasText("ab"));
    TEST_ASSERT_TRUE(display.hasText("cd"));

    char buffer[16] = {};
    size_t length = 0;
    TEST_ASSERT_TRUE(storage.readFile("/apps/notes/notes.txt", buffer, sizeof(buffer), length));
    TEST_ASSERT_EQUAL_UINT(5, length);
    TEST_ASSERT_EQUAL_INT(0, std::memcmp(buffer, "ab\ncd", 5));
}

void test_notes_deletes_in_the_middle() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    luma::InMemoryStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    Luma luma(display, input, clock, storage, settings, diagnostics, audio);

    luma.begin();
    enterLauncher(luma, clock);
    enterNotes(luma, input);
    typeText(luma, input, "hello");
    input.push(makeAction(InputAction::Left));
    luma.update();
    input.push(makeAction(InputAction::Left));
    luma.update();
    input.push(makeAction(InputAction::Delete));
    luma.update();
    input.push(makeAction(InputAction::Back));
    luma.update();

    char buffer[16] = {};
    size_t length = 0;
    TEST_ASSERT_TRUE(storage.readFile("/apps/notes/notes.txt", buffer, sizeof(buffer), length));
    TEST_ASSERT_EQUAL_UINT(4, length);
    TEST_ASSERT_EQUAL_INT(0, std::memcmp(buffer, "helo", 4));
}

void test_notes_up_keeps_column() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    luma::InMemoryStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    Luma luma(display, input, clock, storage, settings, diagnostics, audio);

    luma.begin();
    enterLauncher(luma, clock);
    enterNotes(luma, input);
    typeText(luma, input, "ab");
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    typeText(luma, input, "cd");
    input.push(makeAction(InputAction::Up));
    luma.update();
    input.push(makeText('X'));
    luma.update();
    input.push(makeAction(InputAction::Back));
    luma.update();

    char buffer[16] = {};
    size_t length = 0;
    TEST_ASSERT_TRUE(storage.readFile("/apps/notes/notes.txt", buffer, sizeof(buffer), length));
    TEST_ASSERT_EQUAL_UINT(6, length);
    TEST_ASSERT_EQUAL_INT(0, std::memcmp(buffer, "abX\ncd", 6));
}

void test_notes_rejects_overflow_and_shows_full() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    luma::InMemoryStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    Luma luma(display, input, clock, storage, settings, diagnostics, audio);

    luma.begin();
    enterLauncher(luma, clock);
    enterNotes(luma, input);
    for (size_t i = 0; i < luma::NotesApp::kMaxLength; ++i) {
        input.push(makeText('a'));
        luma.update();
    }
    TEST_ASSERT_TRUE(display.hasText("FULL"));
    input.push(makeText('b'));
    luma.update();
    input.push(makeAction(InputAction::Back));
    luma.update();

    char buffer[luma::NotesApp::kMaxLength + 4] = {};
    size_t length = 0;
    TEST_ASSERT_TRUE(storage.readFile("/apps/notes/notes.txt", buffer, sizeof(buffer), length));
    TEST_ASSERT_EQUAL_UINT(luma::NotesApp::kMaxLength, length);
    TEST_ASSERT_EQUAL_INT('a', buffer[0]);
    TEST_ASSERT_EQUAL_INT('a', buffer[luma::NotesApp::kMaxLength - 1]);
}

void test_notes_failed_save_keeps_previous_and_memory() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    luma::test::ControllableStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    Luma luma(display, input, clock, storage, settings, diagnostics, audio);

    TEST_ASSERT_TRUE(storage.writeFileAtomic("/apps/notes/notes.txt", "keep", 4));
    luma.begin();
    enterLauncher(luma, clock);
    enterNotes(luma, input);
    clock.now = 4000;
    typeText(luma, input, "Z");
    storage.write_succeeds = false;
    clock.now = 4500;
    luma.update();

    TEST_ASSERT_TRUE(diagnostics.contains("[ERROR] notes save failed"));
    TEST_ASSERT_TRUE(display.hasText("SAVE FAIL"));
    TEST_ASSERT_TRUE(display.hasText("keepZ"));
    char buffer[16] = {};
    size_t length = 0;
    TEST_ASSERT_TRUE(storage.readFile("/apps/notes/notes.txt", buffer, sizeof(buffer), length));
    TEST_ASSERT_EQUAL_UINT(4, length);
    TEST_ASSERT_EQUAL_INT(0, std::memcmp(buffer, "keep", 4));
}

void test_notes_autosaves_after_idle() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    luma::InMemoryStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    Luma luma(display, input, clock, storage, settings, diagnostics, audio);

    luma.begin();
    enterLauncher(luma, clock);
    enterNotes(luma, input);
    clock.now = 3000;
    typeText(luma, input, "hi");
    clock.now = 3500;
    luma.update();

    char buffer[16] = {};
    size_t length = 0;
    TEST_ASSERT_TRUE(storage.readFile("/apps/notes/notes.txt", buffer, sizeof(buffer), length));
    TEST_ASSERT_EQUAL_UINT(2, length);
    TEST_ASSERT_EQUAL_INT(0, std::memcmp(buffer, "hi", 2));
}

void test_host_audio_emits_event_log() {
    FakeDiagnostics diagnostics;
    luma::HostAudioAdapter audio(diagnostics);

    audio.play("click");
    TEST_ASSERT_TRUE(diagnostics.contains("[AUDIO] click"));
}

void setUp() {}
void tearDown() {}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_app_manager_lifecycle_order);
    RUN_TEST(test_app_manager_back_returns_to_launcher);
    RUN_TEST(test_app_manager_launcher_back_is_noop);
    RUN_TEST(test_app_manager_shortcut_opens_registered_app);
    RUN_TEST(test_luma_begin_shows_boot_screen);
    RUN_TEST(test_luma_enters_launcher_after_boot_timeout);
    RUN_TEST(test_luma_boot_skips_on_input);
    RUN_TEST(test_luma_draws_only_when_dirty);
    RUN_TEST(test_luma_processes_deferred_saves_each_update);
    RUN_TEST(test_luma_routes_input_frame_to_app_manager);
    RUN_TEST(test_luma_letter_does_not_open_stub_app);
    RUN_TEST(test_launcher_confirm_opens_settings);
    RUN_TEST(test_stub_back_returns_to_launcher);
    RUN_TEST(test_launcher_navigation_stays_in_bounds);
    RUN_TEST(test_launcher_header_updates_when_minute_changes);
    RUN_TEST(test_ui_dialog_draws_title_and_body);
    RUN_TEST(test_input_manager_dispatches_fake_source);
    RUN_TEST(test_in_memory_storage_round_trips_notes);
    RUN_TEST(test_file_storage_persists_across_instances);
    RUN_TEST(test_file_storage_keeps_previous_file_when_replace_fails);
    RUN_TEST(test_ui_font_lowercase_is_not_shifted_by_backslash_comment);
    RUN_TEST(test_settings_loads_valid_keys_independently);
    RUN_TEST(test_settings_invalid_key_falls_back_without_wiping_others);
    RUN_TEST(test_settings_ignores_legacy_sound_key);
    RUN_TEST(test_settings_debounce_then_flush);
    RUN_TEST(test_settings_app_steps_brightness_and_applies);
    RUN_TEST(test_settings_flush_on_exit_before_debounce);
    RUN_TEST(test_settings_volume_zero_does_not_play_click);
    RUN_TEST(test_settings_volume_steps_and_applies);
    RUN_TEST(test_theme_palette_inverts_for_light);
    RUN_TEST(test_app_accent_is_identity_color);
    RUN_TEST(test_settings_opens_about_with_build_identity);
    RUN_TEST(test_settings_category_left_right_does_not_change_brightness);
    RUN_TEST(test_settings_detail_back_stays_in_settings);
    RUN_TEST(test_settings_detail_up_down_stays_bounded);
    RUN_TEST(test_settings_placeholder_confirm_stays);
    RUN_TEST(test_header_time_updates_outside_launcher);
    RUN_TEST(test_notes_round_trips_multiline_text);
    RUN_TEST(test_notes_deletes_in_the_middle);
    RUN_TEST(test_notes_up_keeps_column);
    RUN_TEST(test_notes_rejects_overflow_and_shows_full);
    RUN_TEST(test_notes_failed_save_keeps_previous_and_memory);
    RUN_TEST(test_notes_autosaves_after_idle);
    RUN_TEST(test_host_audio_emits_event_log);
    return UNITY_END();
}
