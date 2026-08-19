#include "fakes.h"

#include "luma/core/app-context.h"
#include "luma/core/app-manager.h"
#include "luma/core/file-storage.h"
#include "luma/core/in-memory-storage.h"
#include "luma/core/input-manager.h"
#include "luma/apps/about-app.h"
#include "luma/apps/notes-app.h"
#include "luma/apps/settings-app.h"
#include "luma/core/network.h"
#include "luma/core/battery.h"
#include "luma/core/battery-types.h"
#include "luma/core/settings.h"
#include "luma/core/time-zone.h"
#include "luma/core/wifi-radio.h"
#include "luma/assets/battery-icons.h"
#include "luma/assets/wifi-icons.h"
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
using luma::CivilTime;
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
using luma::test::ControllableStorage;
using luma::test::CountingStorage;
using luma::test::FakeAudio;
using luma::test::FakeBatterySource;
using luma::test::FakeClock;
using luma::test::FakeDiagnostics;
using luma::test::FakeDisplay;
using luma::test::FakeInputSource;
using luma::test::FakeWifiRadio;
using luma::test::LumaHarness;
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
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    AppContext context;
    AppManager manager;
    std::vector<std::string> log;
    RecordingApp launcher;
    RecordingApp about;

    AppManagerFixture()
        : context(display, settings, storage, clock, diagnostics, network, battery),
          manager(context, diagnostics),
          launcher("launcher", "Launcher", '\0', log),
          about("about", "About", 'a', log) {
        network.attach(radio, storage, diagnostics, clock);
        battery.attach(battery_source, storage, diagnostics, clock);
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
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

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
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

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
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

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
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

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
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

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
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

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
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

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
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

    luma.begin();
    enterLauncher(luma, clock);
    input.push(makeAction(InputAction::Confirm));
    luma.update();

    TEST_ASSERT_EQUAL_STRING("settings", luma.currentAppId());
    TEST_ASSERT_TRUE(display.hasText("SETTINGS"));
    TEST_ASSERT_TRUE(display.hasText("Display"));
    TEST_ASSERT_TRUE(display.hasText("Sound"));
    TEST_ASSERT_TRUE(display.hasText("Network"));
    TEST_ASSERT_TRUE(display.hasText("Time"));
    TEST_ASSERT_TRUE(display.hasText("Battery"));
    TEST_ASSERT_FALSE(display.hasText("System"));
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
    TEST_ASSERT_FALSE(display.hasText("Time zone"));
    TEST_ASSERT_TRUE(display.hasFill({9, 37, 78, 14}, kTsuyukusa));
    TEST_ASSERT_FALSE(display.hasFill({9, 37, 78, 14}, kAccent));
}

void test_stub_back_returns_to_launcher() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    CountingStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

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
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

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
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

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

void test_ui_font_zh_hans_ssid_is_not_question_marks() {
    const luma::font::Glyph house = luma::font::glyphFor(0x5BB6, false);
    TEST_ASSERT_EQUAL(luma::font::kCjkGlyphWidth, house.width);
    TEST_ASSERT_EQUAL_HEX16(0xFF80, house.rows[2]);
    TEST_ASSERT_TRUE(house.rows != luma::font::glyphRows('?', false));
    TEST_ASSERT_EQUAL(10, luma::font::textWidth(u8"家", 1));
    TEST_ASSERT_EQUAL(15, luma::font::textWidth(u8"A家", 1));
    TEST_ASSERT_EQUAL(40, luma::font::textWidth(u8"家里的网", 1));
    TEST_ASSERT_EQUAL(5, luma::font::textWidth(u8"\U0001F600", 1));

    int house_pixels = 0;
    int question_pixels = 0;
    luma::font::drawText({0, 0}, {{255, 255, 255}, 1}, u8"家",
                         [&](int, int, luma::Color) { ++house_pixels; });
    luma::font::drawText({0, 0}, {{255, 255, 255}, 1}, "?",
                         [&](int, int, luma::Color) { ++question_pixels; });
    TEST_ASSERT_TRUE(house_pixels > question_pixels);
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
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

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
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

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
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

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
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

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
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

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
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

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
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

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
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

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
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

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
    TEST_ASSERT_FALSE(display.hasText("Time zone"));

    input.push(makeAction(InputAction::Confirm));
    luma.update();
    TEST_ASSERT_EQUAL_STRING("settings", luma.currentAppId());
    TEST_ASSERT_TRUE(display.hasText("Status"));
    TEST_ASSERT_TRUE(display.hasText("Scan"));

    input.push(makeAction(InputAction::Back));
    luma.update();
    input.push(makeAction(InputAction::Back));
    luma.update();
    input.push(makeAction(InputAction::Down));
    luma.update();
    input.push(makeAction(InputAction::Down));
    luma.update();
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    TEST_ASSERT_TRUE(display.hasText("Battery"));
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    TEST_ASSERT_EQUAL_STRING("settings", luma.currentAppId());
    input.push(makeAction(InputAction::Back));
    luma.update();
    input.push(makeAction(InputAction::Down));
    luma.update();
    TEST_ASSERT_TRUE(display.hasText("System"));
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    TEST_ASSERT_TRUE(display.hasText("About"));
}

void test_header_time_updates_outside_launcher() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    CountingStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

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
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

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
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

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
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

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
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

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
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

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
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

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

void test_civil_time_dst_spring_forward_new_york() {
    const CivilTime before = luma::civilTimeAt(1772953140, "America/New_York");
    const CivilTime after = luma::civilTimeAt(1772953200, "America/New_York");
    TEST_ASSERT_TRUE(before.valid);
    TEST_ASSERT_EQUAL_UINT8(1, before.hour);
    TEST_ASSERT_EQUAL_UINT8(59, before.minute);
    TEST_ASSERT_TRUE(after.valid);
    TEST_ASSERT_EQUAL_UINT8(3, after.hour);
    TEST_ASSERT_EQUAL_UINT8(0, after.minute);
}

void test_clock_unset_renders_invalid_until_ntp() {
    FakeClock clock;
    clock.use_unix = true;
    clock.unix_utc = 1772953200;
    clock.ntp_succeeds = true;
    TEST_ASSERT_FALSE(clock.localTime().valid);
    clock.synchronize();
    TEST_ASSERT_TRUE(clock.localTime().valid);
    clock.setTimeZone("America/New_York");
    TEST_ASSERT_EQUAL_STRING("America/New_York", clock.timeZoneId());
    TEST_ASSERT_EQUAL_UINT8(3, clock.localTime().hour);
}

void test_clock_keeps_last_valid_time_after_sync() {
    FakeClock clock;
    clock.use_unix = true;
    clock.unix_utc = 1772953200;
    clock.synchronize();
    const CivilTime connected = clock.localTime();
    clock.unix_utc = 1772953200;
    TEST_ASSERT_TRUE(connected.valid);
    TEST_ASSERT_TRUE(clock.localTime().valid);
    TEST_ASSERT_EQUAL_UINT8(connected.hour, clock.localTime().hour);
}

void test_network_scan_lists_ssids_without_blocking() {
    luma::InMemoryStorage storage;
    FakeClock clock;
    FakeDiagnostics diagnostics;
    FakeWifiRadio radio;
    luma::Network network;
    network.attach(radio, storage, diagnostics, clock);
    network.load();

    radio.addHit("Cafe", true, -50);
    network.startScan();
    TEST_ASSERT_TRUE(network.scanInProgress());
    clock.now = 10;
    network.update();
    TEST_ASSERT_TRUE(network.scanInProgress());
    radio.completeScan();
    network.update();
    TEST_ASSERT_FALSE(network.scanInProgress());
    TEST_ASSERT_EQUAL_INT(1, network.publicScanCount());
    luma::WifiScanHit hit;
    TEST_ASSERT_TRUE(network.publicScanAt(0, hit));
    TEST_ASSERT_EQUAL_STRING("Cafe", hit.ssid);
}

void test_network_persists_profile_only_after_success() {
    luma::InMemoryStorage storage;
    FakeClock clock;
    FakeDiagnostics diagnostics;
    FakeWifiRadio radio;
    luma::Network network;
    network.attach(radio, storage, diagnostics, clock);
    network.load();

    network.connect("Home", "secret123");
    radio.fail();
    clock.now = 20;
    network.update();
    TEST_ASSERT_EQUAL_INT(0, network.profileCount());
    TEST_ASSERT_EQUAL(luma::NetworkState::Failed, network.state());

    network.connect("Home", "secret123");
    radio.succeed();
    clock.now = 40;
    network.update();
    TEST_ASSERT_EQUAL_INT(1, network.profileCount());
    TEST_ASSERT_EQUAL_STRING("Home", network.profileSsid(0));
    for (const auto& line : diagnostics.lines) {
        TEST_ASSERT_TRUE(line.find("secret123") == std::string::npos);
    }
}

void test_network_replaces_last_profile_at_capacity() {
    luma::InMemoryStorage storage;
    FakeClock clock;
    FakeDiagnostics diagnostics;
    FakeWifiRadio radio;
    luma::Network network;
    network.attach(radio, storage, diagnostics, clock);
    network.load();

    const char* names[] = {"A", "B", "C", "D", "E", "F"};
    for (int i = 0; i < 6; ++i) {
        network.connect(names[i], "pw");
        radio.succeed();
        clock.now = static_cast<uint32_t>((i + 1) * 20);
        network.update();
        radio.drop();
        network.update();
    }
    TEST_ASSERT_EQUAL_INT(5, network.profileCount());
    TEST_ASSERT_EQUAL_STRING("F", network.profileSsid(0));
    TEST_ASSERT_EQUAL_STRING("B", network.profileSsid(4));
}

void test_network_delete_profile() {
    luma::InMemoryStorage storage;
    FakeClock clock;
    FakeDiagnostics diagnostics;
    FakeWifiRadio radio;
    luma::Network network;
    network.attach(radio, storage, diagnostics, clock);
    network.load();
    network.connect("Home", "pw");
    radio.succeed();
    clock.now = 20;
    network.update();
    network.deleteProfile(0);
    TEST_ASSERT_EQUAL_INT(0, network.profileCount());
}

void test_network_manual_timeout_is_failed() {
    luma::InMemoryStorage storage;
    FakeClock clock;
    FakeDiagnostics diagnostics;
    FakeWifiRadio radio;
    luma::Network network;
    network.attach(radio, storage, diagnostics, clock);
    network.load();
    clock.now = 1000;
    network.connect("Home", "pw");
    clock.now = 1000 + luma::Network::kManualTimeoutMs;
    network.update();
    TEST_ASSERT_EQUAL(luma::NetworkState::Failed, network.state());
}

void test_network_background_reconnect_does_not_fail() {
    luma::InMemoryStorage storage;
    FakeClock clock;
    FakeDiagnostics diagnostics;
    FakeWifiRadio radio;
    luma::Network network;
    network.attach(radio, storage, diagnostics, clock);
    network.load();
    radio.addHit("Home", true, -40);
    network.connect("Home", "pw");
    radio.succeed();
    clock.now = 50;
    network.update();
    TEST_ASSERT_TRUE(network.takeConnectedEdge());

    radio.drop();
    clock.now = 60;
    network.update();
    TEST_ASSERT_EQUAL(luma::NetworkState::Connecting, network.state());
    radio.completeScan();
    network.update();
    radio.fail();
    clock.now = 70;
    network.update();
    TEST_ASSERT_NOT_EQUAL(luma::NetworkState::Failed, network.state());
}

void test_luma_syncs_clock_on_connected_edge() {
    luma::InMemoryStorage storage;
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);
    clock.use_unix = true;
    clock.unix_utc = 1772953200;
    clock.ntp_succeeds = true;

    luma.begin();
    radio.addHit("Home", true, -40);
    network.connect("Home", "pw");
    radio.succeed();
    luma.update();
    TEST_ASSERT_EQUAL_INT(1, clock.synchronize_count);
    TEST_ASSERT_TRUE(clock.localTime().valid);
}

void test_luma_ntp_failure_keeps_clock_unset() {
    luma::InMemoryStorage storage;
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);
    clock.use_unix = true;
    clock.ntp_succeeds = false;

    luma.begin();
    network.connect("Home", "pw");
    radio.succeed();
    luma.update();
    TEST_ASSERT_EQUAL_INT(1, clock.synchronize_count);
    TEST_ASSERT_FALSE(clock.localTime().valid);
}

void openNetworkWifiEditor(Luma& luma, FakeInputSource& input) {
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    input.push(makeAction(InputAction::Down));
    luma.update();
    input.push(makeAction(InputAction::Down));
    luma.update();
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    input.push(makeAction(InputAction::Confirm));
    luma.update();
}

void openWifiScanDetail(Luma& luma, FakeInputSource& input) {
    openNetworkWifiEditor(luma, input);
    input.push(makeAction(InputAction::Down));
    luma.update();
    input.push(makeAction(InputAction::Down));
    luma.update();
    input.push(makeAction(InputAction::Confirm));
    luma.update();
}

void test_settings_wifi_scan_and_open_network() {
    luma::InMemoryStorage storage;
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);
    radio.addHit("Open-Cafe", false, -70);

    luma.begin();
    enterLauncher(luma, clock);
    openWifiScanDetail(luma, input);
    TEST_ASSERT_TRUE(display.hasText("Scanning"));
    radio.completeScan();
    luma.update();
    TEST_ASSERT_TRUE(display.hasText("Open-Cafe"));
    TEST_ASSERT_FALSE(display.hasText("????????"));
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    TEST_ASSERT_EQUAL_STRING("Open-Cafe", radio.ssid);
    TEST_ASSERT_EQUAL_STRING("", radio.last_password);
    TEST_ASSERT_TRUE(display.hasText("Status"));
    TEST_ASSERT_TRUE(display.hasText("Connecting"));
    TEST_ASSERT_FALSE(display.hasText("**"));
}

void test_settings_wifi_lists_zh_hans_ssid() {
    luma::InMemoryStorage storage;
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);
    radio.addHit(u8"家里的网", false, -70);

    luma.begin();
    enterLauncher(luma, clock);
    openWifiScanDetail(luma, input);
    radio.completeScan();
    luma.update();
    TEST_ASSERT_TRUE(display.hasText(u8"家里的网"));
    TEST_ASSERT_FALSE(display.hasText("????????"));
}

void test_settings_masked_password_and_timezone_directory() {
    luma::InMemoryStorage storage;
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);
    radio.addHit("Cafe", true, -50);

    luma.begin();
    enterLauncher(luma, clock);
    openWifiScanDetail(luma, input);
    radio.completeScan();
    luma.update();
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    TEST_ASSERT_TRUE(display.hasText("Scan"));
    TEST_ASSERT_TRUE(display.hasText("SSID"));
    TEST_ASSERT_TRUE(display.hasText("Cafe"));
    TEST_ASSERT_TRUE(display.hasText("Password"));
    input.push(makeText('s'));
    luma.update();
    input.push(makeText('e'));
    luma.update();
    TEST_ASSERT_TRUE(display.hasText("**"));
    TEST_ASSERT_TRUE(display.hasText("2"));
    TEST_ASSERT_FALSE(display.hasText("se"));
    for (int i = 0; i < 28; ++i) {
        input.push(makeText('x'));
        luma.update();
    }
    TEST_ASSERT_TRUE(display.hasText("30"));
    TEST_ASSERT_TRUE(display.hasText("************************"));
    TEST_ASSERT_FALSE(display.hasText("*************************"));
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    TEST_ASSERT_EQUAL_STRING("sexxxxxxxxxxxxxxxxxxxxxxxxxxxx", radio.last_password);
    TEST_ASSERT_TRUE(display.hasText("Connecting"));
    TEST_ASSERT_FALSE(display.hasText("**"));
}

void test_time_zone_sections_sort_and_utc_labels() {
    TEST_ASSERT_EQUAL_INT(5, luma::timeZoneSectionCount());
    TEST_ASSERT_EQUAL_STRING("UTC", luma::timeZoneSectionLabelAt(0));
    TEST_ASSERT_EQUAL_STRING("America", luma::timeZoneSectionLabelAt(1));
    TEST_ASSERT_EQUAL_STRING("Europe", luma::timeZoneSectionLabelAt(2));
    TEST_ASSERT_EQUAL_STRING("Asia", luma::timeZoneSectionLabelAt(3));
    TEST_ASSERT_EQUAL_STRING("Australia", luma::timeZoneSectionLabelAt(4));
    TEST_ASSERT_EQUAL_INT(0, luma::timeZoneSectionOf("UTC"));
    TEST_ASSERT_EQUAL_INT(3, luma::timeZoneSectionOf("Asia/Shanghai"));
    TEST_ASSERT_EQUAL_STRING("America/Los_Angeles", luma::timeZoneIdInSection(1, 0));
    TEST_ASSERT_EQUAL_STRING("America/New_York", luma::timeZoneIdInSection(1, 3));
    TEST_ASSERT_EQUAL_STRING("Asia/Kolkata", luma::timeZoneIdInSection(3, 0));
    TEST_ASSERT_EQUAL_STRING("Asia/Shanghai", luma::timeZoneIdInSection(3, 1));
    TEST_ASSERT_EQUAL_STRING("Asia/Singapore", luma::timeZoneIdInSection(3, 2));
    TEST_ASSERT_EQUAL_STRING("Asia/Tokyo", luma::timeZoneIdInSection(3, 3));

    char label[12] = {};
    luma::formatTimeZoneUtcLabel("Asia/Kolkata", label, sizeof(label));
    TEST_ASSERT_EQUAL_STRING("UTC+5:30", label);
    luma::formatTimeZoneUtcLabel("America/Los_Angeles", label, sizeof(label));
    TEST_ASSERT_EQUAL_STRING("UTC-8", label);
    luma::formatTimeZoneUtcLabel("UTC", label, sizeof(label));
    TEST_ASSERT_EQUAL_STRING("UTC+0", label);
}

void test_settings_time_category_timezone_directory() {
    luma::InMemoryStorage storage;
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

    luma.begin();
    enterLauncher(luma, clock);
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    for (int i = 0; i < 3; ++i) {
        input.push(makeAction(InputAction::Down));
        luma.update();
    }
    TEST_ASSERT_TRUE(display.hasText("Time"));
    TEST_ASSERT_TRUE(display.hasText("Time zone"));
    TEST_ASSERT_TRUE(display.hasText("UTC"));
    TEST_ASSERT_FALSE(display.hasText("Wi-Fi"));
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    TEST_ASSERT_TRUE(display.hasText("UTC"));
    TEST_ASSERT_TRUE(display.hasText("America"));
    TEST_ASSERT_TRUE(display.hasText("UTC+0"));
    input.push(makeAction(InputAction::Down));
    luma.update();
    input.push(makeAction(InputAction::Down));
    luma.update();
    input.push(makeAction(InputAction::Down));
    luma.update();
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    TEST_ASSERT_TRUE(display.hasText("Asia/Kolkata"));
    input.push(makeAction(InputAction::Down));
    luma.update();
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    TEST_ASSERT_EQUAL_STRING("Asia/Shanghai", clock.timeZoneId());
    TEST_ASSERT_TRUE(display.hasText("Time zone"));
    TEST_ASSERT_TRUE(display.hasText("Asia/Shanghai"));
}

void test_header_network_glyphs_use_icons_not_spectrum_colors() {
    FakeDisplay display;
    const auto palette = luma::theme::paletteFor(0);
    luma::drawNetworkGlyph(display, palette, {0, 0}, luma::NetworkState::Disconnected,
                           luma::SignalStrength::None);
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kWifiDisconnected, palette.primary_text));
    display.beginFrame();
    luma::drawNetworkGlyph(display, palette, {0, 0}, luma::NetworkState::Connecting,
                           luma::SignalStrength::None);
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kWifiDisconnected, palette.primary_text));
    display.beginFrame();
    luma::drawNetworkGlyph(display, palette, {0, 0}, luma::NetworkState::Failed,
                           luma::SignalStrength::None);
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kWifiDisconnected, palette.primary_text));
    display.beginFrame();
    luma::drawNetworkGlyph(display, palette, {0, 0}, luma::NetworkState::Unknown,
                           luma::SignalStrength::None);
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kWifiDisconnected, palette.primary_text));
    display.beginFrame();
    luma::drawNetworkGlyph(display, palette, {0, 0}, luma::NetworkState::Connected,
                           luma::SignalStrength::Weakest);
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kWifiListDot, palette.secondary_text));
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kWifiListArc2, palette.secondary_text));
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kWifiListArc3, palette.secondary_text));
    display.beginFrame();
    luma::drawNetworkGlyph(display, palette, {0, 0}, luma::NetworkState::Connected,
                           luma::SignalStrength::Strong);
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kWifiListArc3, palette.primary_text));
}

void test_network_scan_dedups_ssid_keeping_strongest_rssi() {
    luma::InMemoryStorage storage;
    FakeClock clock;
    FakeDiagnostics diagnostics;
    FakeWifiRadio radio;
    luma::Network network;
    network.attach(radio, storage, diagnostics, clock);
    network.load();
    radio.addHit("Cafe", true, -80);
    radio.addHit("Cafe", true, -40);
    radio.addHit("Other", false, -60);
    network.startScan();
    radio.completeScan();
    network.update();
    TEST_ASSERT_EQUAL_INT(2, network.publicScanCount());
    luma::WifiScanHit hit;
    TEST_ASSERT_TRUE(network.publicScanAt(0, hit));
    TEST_ASSERT_EQUAL_STRING("Cafe", hit.ssid);
    TEST_ASSERT_EQUAL_INT8(-40, hit.rssi);
    TEST_ASSERT_TRUE(network.publicScanAt(1, hit));
    TEST_ASSERT_EQUAL_STRING("Other", hit.ssid);
}

void test_network_station_ip_and_disconnect_hold() {
    luma::InMemoryStorage storage;
    FakeClock clock;
    FakeDiagnostics diagnostics;
    FakeWifiRadio radio;
    luma::Network network;
    network.attach(radio, storage, diagnostics, clock);
    network.load();
    radio.addHit("Home", true, -40);
    network.connect("Home", "pw");
    radio.succeed();
    clock.now = 20;
    network.update();
    TEST_ASSERT_EQUAL(luma::NetworkState::Connected, network.state());
    char ip[16] = {};
    network.stationIp(ip, sizeof(ip));
    TEST_ASSERT_EQUAL_STRING("192.168.1.10", ip);
    TEST_ASSERT_EQUAL_INT8(-55, network.rssi());

    const int connects = radio.connect_calls;
    network.disconnect();
    TEST_ASSERT_EQUAL(luma::NetworkState::Disconnected, network.state());
    network.stationIp(ip, sizeof(ip));
    TEST_ASSERT_EQUAL_STRING("", ip);
    clock.now = 20 + luma::Network::kBackgroundIdleMs + 1000;
    network.update();
    TEST_ASSERT_EQUAL(luma::NetworkState::Disconnected, network.state());
    TEST_ASSERT_EQUAL_INT(connects, radio.connect_calls);

    network.connect("Home", "pw");
    TEST_ASSERT_EQUAL(luma::NetworkState::Connecting, network.state());
}

void test_settings_wifi_nested_split_and_scan_scroll() {
    luma::InMemoryStorage storage;
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);
    radio.addHit("Net0", false, -50);
    radio.addHit("Net1", false, -51);
    radio.addHit("Net2", false, -52);
    radio.addHit("Net3", false, -53);
    radio.addHit("Net4", false, -54);
    radio.addHit("Net5", false, -55);

    luma.begin();
    enterLauncher(luma, clock);
    openNetworkWifiEditor(luma, input);
    TEST_ASSERT_TRUE(display.hasText("Status"));
    TEST_ASSERT_TRUE(display.hasText("Saved"));
    TEST_ASSERT_TRUE(display.hasText("Scan"));
    TEST_ASSERT_TRUE(display.hasText("Disconnected"));
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    TEST_ASSERT_TRUE(display.hasText("State"));
    input.push(makeAction(InputAction::Back));
    luma.update();
    input.push(makeAction(InputAction::Down));
    luma.update();
    input.push(makeAction(InputAction::Down));
    luma.update();
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    TEST_ASSERT_TRUE(display.hasText("Scanning"));
    radio.completeScan();
    luma.update();
    TEST_ASSERT_TRUE(display.hasText("Scan"));
    TEST_ASSERT_TRUE(display.hasText("Net0"));
    for (int i = 0; i < 5; ++i) {
        input.push(makeAction(InputAction::Down));
        luma.update();
    }
    TEST_ASSERT_TRUE(display.hasText("Net5"));
    TEST_ASSERT_FALSE(display.hasText("Net0"));
}

void test_settings_wifi_password_back_keeps_ssid() {
    luma::InMemoryStorage storage;
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);
    radio.addHit("Cafe", true, -50);

    luma.begin();
    enterLauncher(luma, clock);
    openWifiScanDetail(luma, input);
    radio.completeScan();
    luma.update();
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    TEST_ASSERT_TRUE(display.hasText("Password"));
    input.push(makeText('x'));
    luma.update();
    TEST_ASSERT_TRUE(display.hasText("*"));
    input.push(makeAction(InputAction::Back));
    luma.update();
    TEST_ASSERT_TRUE(display.hasText("Cafe"));
    TEST_ASSERT_FALSE(display.hasText("Key"));
    const auto palette = luma::theme::paletteFor(0);
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kLockClosed, palette.primary_text));
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kWifiListArc3, palette.primary_text));
    TEST_ASSERT_FALSE(display.hasText("*"));
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    TEST_ASSERT_EQUAL_STRING("", radio.last_password);
    TEST_ASSERT_EQUAL_STRING("Cafe", radio.ssid);
}

void test_settings_wifi_saved_reconnect_skips_password() {
    luma::InMemoryStorage storage;
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);
    radio.addHit("Open-Cafe", false, -70);

    luma.begin();
    enterLauncher(luma, clock);
    openWifiScanDetail(luma, input);
    radio.completeScan();
    luma.update();
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    radio.succeed();
    luma.update();
    TEST_ASSERT_TRUE(display.hasText("Connected"));
    TEST_ASSERT_TRUE(display.hasText("Open-Cafe"));
    input.push(makeAction(InputAction::Back));
    luma.update();
    input.push(makeAction(InputAction::Down));
    luma.update();
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    TEST_ASSERT_TRUE(display.hasText("Saved"));
    TEST_ASSERT_FALSE(display.hasText("**"));
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    TEST_ASSERT_EQUAL_STRING("Open-Cafe", radio.ssid);
    TEST_ASSERT_TRUE(display.hasText("Connecting"));
    TEST_ASSERT_FALSE(display.hasText("**"));
}

void test_header_time_and_title_do_not_clip() {
    FakeDisplay display;
    const auto palette = luma::theme::paletteFor(0);
    luma::drawAppHeader(display, palette, nullptr, "SETTINGS", "--:--");
    TEST_ASSERT_TRUE(display.hasText("SETTINGS"));
    TEST_ASSERT_TRUE(display.hasText("--:--"));
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kWifiDisconnected, palette.primary_text));
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kBatteryOutline, palette.secondary_text));
    bool stacked = false;
    for (const auto& mono : display.monos) {
        if (mono.rows == luma::assets::kWifiDisconnected && mono.origin.x == 212 &&
            mono.origin.y == 5 && mono.width == 10 && mono.height == 10) {
            stacked = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(stacked);
    bool battery = false;
    for (const auto& mono : display.monos) {
        if (mono.rows == luma::assets::kBatteryOutline && mono.origin.x == 224 &&
            mono.origin.y == 5 && mono.width == 10 && mono.height == 10) {
            battery = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(battery);
}

void test_battery_glyph_fill_and_band_color() {
    FakeDisplay display;
    const auto palette = luma::theme::paletteFor(0);
    luma::BatteryReading reading;
    reading.percent = 50;
    reading.percent_valid = true;
    luma::drawBatteryGlyph(display, palette, {0, 0}, reading);
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kBatteryOutline, luma::theme::kWakatake));
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kBatteryFill1, luma::theme::kWakatake));
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kBatteryFill2, luma::theme::kWakatake));
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kBatteryFill3, luma::theme::kWakatake));
    TEST_ASSERT_FALSE(display.hasMono(luma::assets::kBatteryFill4, luma::theme::kWakatake));

    display.beginFrame();
    reading.percent = 25;
    luma::drawBatteryGlyph(display, palette, {0, 0}, reading);
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kBatteryOutline, luma::theme::kYamabuki));
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kBatteryFill2, luma::theme::kYamabuki));

    display.beginFrame();
    reading.percent = 10;
    luma::drawBatteryGlyph(display, palette, {0, 0}, reading);
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kBatteryOutline, luma::theme::kBenihi));
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kBatteryFill1, luma::theme::kBenihi));
}

void test_header_redraws_when_battery_changes() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    luma::InMemoryStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

    luma.begin();
    enterLauncher(luma, clock);
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kBatteryFill5, luma::theme::kWakatake));

    battery_source.reading.percent = 10;
    luma.update();
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kBatteryFill1, luma::theme::kBenihi));
    TEST_ASSERT_FALSE(display.hasMono(luma::assets::kBatteryFill5, luma::theme::kWakatake));
}

void test_battery_history_bars_mark_bands_and_gaps() {
    FakeDisplay display;
    const auto palette = luma::theme::paletteFor(0);
    luma::BatterySample samples[2] = {};
    samples[0].reading.percent = 10;
    samples[0].reading.percent_valid = true;
    samples[0].run_id = 1;
    samples[1].reading.percent = 50;
    samples[1].reading.percent_valid = true;
    samples[1].run_id = 2;
    luma::drawBatteryHistory(display, palette, {0, 0, 120, 62}, samples, 2);
    TEST_ASSERT_TRUE(display.hasFill({0, 0, 120, 1}, palette.secondary_text));
    TEST_ASSERT_TRUE(display.hasFill({0, 25, 120, 1}, palette.secondary_text));
    TEST_ASSERT_TRUE(display.hasFill({0, 49, 120, 1}, palette.secondary_text));
    TEST_ASSERT_TRUE(display.hasFill({114, 45, 1, 5}, luma::theme::kBenihi));
    TEST_ASSERT_TRUE(display.hasFill({118, 25, 1, 25}, luma::theme::kWakatake));
    TEST_ASSERT_FALSE(display.hasFill({0, 45, 1, 5}, luma::theme::kBenihi));
    TEST_ASSERT_FALSE(display.hasFill({1, 45, 1, 5}, luma::theme::kBenihi));
    TEST_ASSERT_FALSE(display.hasFill({2, 45, 1, 5}, luma::theme::kBenihi));
    TEST_ASSERT_TRUE(display.hasText("-60"));
    TEST_ASSERT_TRUE(display.hasText("-30"));
    TEST_ASSERT_TRUE(display.hasText("0"));
}

void test_settings_battery_pane_shows_reading() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    luma::InMemoryStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

    luma.begin();
    enterLauncher(luma, clock);
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    for (int i = 0; i < 4; ++i) {
        input.push(makeAction(InputAction::Down));
        luma.update();
    }
    TEST_ASSERT_TRUE(display.hasText("Battery"));
    TEST_ASSERT_TRUE(display.hasText("87%"));
    TEST_ASSERT_TRUE(display.hasText("4.12 V"));
    TEST_ASSERT_FALSE(display.hasText("Charging"));
    TEST_ASSERT_FALSE(display.hasText("Not charging"));
}

void test_signal_strength_from_rssi_bins() {
    TEST_ASSERT_EQUAL(luma::SignalStrength::Strong, luma::signalStrengthFromRssi(-60));
    TEST_ASSERT_EQUAL(luma::SignalStrength::Mid, luma::signalStrengthFromRssi(-70));
    TEST_ASSERT_EQUAL(luma::SignalStrength::Weak, luma::signalStrengthFromRssi(-80));
    TEST_ASSERT_EQUAL(luma::SignalStrength::Weakest, luma::signalStrengthFromRssi(-81));
}

void test_ellipsize_appends_ascii_dots() {
    char out[40] = {};
    luma::ellipsizeToWidth("Short", out, sizeof(out), 100);
    TEST_ASSERT_EQUAL_STRING("Short", out);
    luma::ellipsizeToWidth("ABCDEFGHIJKLMNOPQRSTUVWXYZ012345", out, sizeof(out), 40);
    TEST_ASSERT_EQUAL_STRING("ABCDE...", out);
}

void test_wifi_list_glyph_weakest_is_all_secondary() {
    FakeDisplay display;
    const auto palette = luma::theme::paletteFor(0);
    luma::drawWifiListGlyph(display, palette, {0, 0}, luma::SignalStrength::Weakest);
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kWifiListDot, palette.secondary_text));
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kWifiListArc2, palette.secondary_text));
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kWifiListArc3, palette.secondary_text));
}

void test_settings_wifi_status_signal_is_dbm_only() {
    luma::InMemoryStorage storage;
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);
    radio.addHit("Open-Cafe", false, -40);
    radio.rssi_dbm = -42;

    luma.begin();
    enterLauncher(luma, clock);
    openWifiScanDetail(luma, input);
    radio.completeScan();
    luma.update();
    input.push(makeAction(InputAction::Confirm));
    luma.update();
    radio.succeed();
    luma.update();
    TEST_ASSERT_TRUE(display.hasText("-42 dBm"));
    TEST_ASSERT_FALSE(display.hasText("Strong"));
}

void test_settings_wifi_scan_icons_and_long_ssid() {
    luma::InMemoryStorage storage;
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);
    radio.addHit("ABCDEFGHIJKLMNOPQRSTUVWXYZ012345", true, -50);

    luma.begin();
    enterLauncher(luma, clock);
    openWifiScanDetail(luma, input);
    radio.completeScan();
    luma.update();
    const auto palette = luma::theme::paletteFor(0);
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kLockClosed, palette.primary_text));
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kWifiListArc3, palette.primary_text));
    TEST_ASSERT_FALSE(display.hasText("Key"));
    bool found_ellipsis = false;
    for (const auto& text : display.texts) {
        if (text.size() >= 3 && text.compare(text.size() - 3, 3, "...") == 0) {
            found_ellipsis = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(found_ellipsis);
}

void test_battery_band_from_percent() {
    TEST_ASSERT_EQUAL(luma::BatteryBand::Critical, luma::batteryBand(0));
    TEST_ASSERT_EQUAL(luma::BatteryBand::Critical, luma::batteryBand(20));
    TEST_ASSERT_EQUAL(luma::BatteryBand::Warn, luma::batteryBand(21));
    TEST_ASSERT_EQUAL(luma::BatteryBand::Warn, luma::batteryBand(30));
    TEST_ASSERT_EQUAL(luma::BatteryBand::Ok, luma::batteryBand(31));
    TEST_ASSERT_EQUAL(luma::BatteryBand::Ok, luma::batteryBand(100));
}

void test_header_redraws_when_battery_band_changes() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    luma::InMemoryStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    FakeWifiRadio radio;
    luma::Network network;
    FakeBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

    luma.begin();
    enterLauncher(luma, clock);
    battery_source.reading.percent = 25;
    luma.update();
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kBatteryFill2, luma::theme::kYamabuki));

    battery_source.reading.percent = 35;
    luma.update();
    TEST_ASSERT_TRUE(display.hasMono(luma::assets::kBatteryFill2, luma::theme::kWakatake));
    TEST_ASSERT_FALSE(display.hasMono(luma::assets::kBatteryFill2, luma::theme::kYamabuki));
}

void test_battery_percent_from_voltage_mv() {
    TEST_ASSERT_EQUAL_UINT8(0, luma::batteryPercentFromVoltageMv(0));
    TEST_ASSERT_EQUAL_UINT8(0, luma::batteryPercentFromVoltageMv(3300));
    TEST_ASSERT_EQUAL_UINT8(100, luma::batteryPercentFromVoltageMv(4100));
    TEST_ASSERT_EQUAL_UINT8(100, luma::batteryPercentFromVoltageMv(4150));
}

void test_battery_fill_levels_bucket_percent() {
    luma::BatteryReading reading;
    reading.percent_valid = true;
    reading.percent = 0;
    TEST_ASSERT_EQUAL_UINT8(0, luma::batteryFillLevel(reading));
    reading.percent = 1;
    TEST_ASSERT_EQUAL_UINT8(1, luma::batteryFillLevel(reading));
    reading.percent = 20;
    TEST_ASSERT_EQUAL_UINT8(1, luma::batteryFillLevel(reading));
    reading.percent = 21;
    TEST_ASSERT_EQUAL_UINT8(2, luma::batteryFillLevel(reading));
    reading.percent = 40;
    TEST_ASSERT_EQUAL_UINT8(2, luma::batteryFillLevel(reading));
    reading.percent = 41;
    TEST_ASSERT_EQUAL_UINT8(3, luma::batteryFillLevel(reading));
    reading.percent = 60;
    TEST_ASSERT_EQUAL_UINT8(3, luma::batteryFillLevel(reading));
    reading.percent = 61;
    TEST_ASSERT_EQUAL_UINT8(4, luma::batteryFillLevel(reading));
    reading.percent = 80;
    TEST_ASSERT_EQUAL_UINT8(4, luma::batteryFillLevel(reading));
    reading.percent = 81;
    TEST_ASSERT_EQUAL_UINT8(5, luma::batteryFillLevel(reading));
    reading.percent = 100;
    TEST_ASSERT_EQUAL_UINT8(5, luma::batteryFillLevel(reading));
    reading.percent_valid = false;
    TEST_ASSERT_EQUAL_UINT8(0, luma::batteryFillLevel(reading));
}

void test_battery_samples_once_per_minute_and_rolls_over() {
    luma::InMemoryStorage storage;
    FakeClock clock;
    FakeDiagnostics diagnostics;
    FakeBatterySource source;
    luma::Battery battery;
    battery.attach(source, storage, diagnostics, clock);
    clock.now = 1000;
    clock.unix_utc = 1700000000;
    battery.begin();
    TEST_ASSERT_EQUAL(1, battery.sampleCount());
    TEST_ASSERT_EQUAL_UINT8(87, battery.current().percent);

    clock.now = 1000 + luma::Battery::kSampleMs - 1;
    battery.update();
    TEST_ASSERT_EQUAL(1, battery.sampleCount());

    source.reading.percent = 70;
    clock.now = 1000 + luma::Battery::kSampleMs;
    battery.update();
    TEST_ASSERT_EQUAL(2, battery.sampleCount());
    luma::BatterySample second;
    TEST_ASSERT_TRUE(battery.sampleAt(1, second));
    TEST_ASSERT_EQUAL_UINT8(70, second.reading.percent);
    TEST_ASSERT_EQUAL_UINT32(1700000000, second.unix_utc);

    for (int i = 0; i < 59; ++i) {
        clock.now += luma::Battery::kSampleMs;
        source.reading.percent = static_cast<uint8_t>(i);
        battery.update();
    }
    TEST_ASSERT_EQUAL(luma::Battery::kMaxSamples, battery.sampleCount());
    luma::BatterySample oldest;
    TEST_ASSERT_TRUE(battery.sampleAt(0, oldest));
    TEST_ASSERT_EQUAL_UINT8(70, oldest.reading.percent);
}

void test_battery_checkpoint_restores_and_new_run_is_a_gap() {
    luma::InMemoryStorage storage;
    FakeClock clock;
    FakeDiagnostics diagnostics;
    FakeBatterySource source;
    luma::Battery battery;
    battery.attach(source, storage, diagnostics, clock);
    clock.now = 0;
    battery.begin();
    const uint8_t first_run = battery.runId();
    for (int i = 0; i < luma::Battery::kCheckpointSamples - 1; ++i) {
        clock.now += luma::Battery::kSampleMs;
        battery.update();
    }
    TEST_ASSERT_EQUAL(luma::Battery::kCheckpointSamples, battery.sampleCount());

    luma::Battery restored;
    restored.attach(source, storage, diagnostics, clock);
    restored.load();
    TEST_ASSERT_EQUAL(luma::Battery::kCheckpointSamples, restored.sampleCount());
    luma::BatterySample last_restored;
    TEST_ASSERT_TRUE(restored.sampleAt(restored.sampleCount() - 1, last_restored));
    TEST_ASSERT_EQUAL_UINT8(first_run, last_restored.run_id);

    clock.now += luma::Battery::kSampleMs;
    restored.begin();
    TEST_ASSERT_TRUE(restored.runId() != first_run);
    luma::BatterySample newest;
    TEST_ASSERT_TRUE(restored.sampleAt(restored.sampleCount() - 1, newest));
    TEST_ASSERT_TRUE(luma::batteryHistoryGap(last_restored, newest));
}

void test_battery_checkpoint_failure_keeps_memory_and_emits_error() {
    ControllableStorage storage;
    FakeClock clock;
    FakeDiagnostics diagnostics;
    FakeBatterySource source;
    luma::Battery battery;
    battery.attach(source, storage, diagnostics, clock);
    clock.now = 0;
    battery.begin();
    storage.save_pref_succeeds = false;
    for (int i = 0; i < luma::Battery::kCheckpointSamples - 1; ++i) {
        clock.now += luma::Battery::kSampleMs;
        battery.update();
    }
    TEST_ASSERT_EQUAL(luma::Battery::kCheckpointSamples, battery.sampleCount());
    TEST_ASSERT_TRUE(diagnostics.contains("[ERROR] battery checkpoint failed"));

    luma::BatterySample kept;
    TEST_ASSERT_TRUE(battery.sampleAt(0, kept));
    TEST_ASSERT_EQUAL_UINT8(87, kept.reading.percent);
}

void test_battery_unknown_values_are_not_fabricated() {
    luma::InMemoryStorage storage;
    FakeClock clock;
    FakeDiagnostics diagnostics;
    FakeBatterySource source;
    source.reading = luma::BatteryReading{};
    luma::Battery battery;
    battery.attach(source, storage, diagnostics, clock);
    battery.begin();
    TEST_ASSERT_FALSE(battery.current().percent_valid);
    TEST_ASSERT_FALSE(battery.current().voltage_valid);
    TEST_ASSERT_FALSE(battery.current().charging_valid);
    luma::BatterySample sample;
    TEST_ASSERT_TRUE(battery.sampleAt(0, sample));
    TEST_ASSERT_FALSE(sample.reading.percent_valid);
    TEST_ASSERT_EQUAL_UINT32(0, sample.unix_utc);
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
    RUN_TEST(test_ui_font_zh_hans_ssid_is_not_question_marks);
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
    RUN_TEST(test_civil_time_dst_spring_forward_new_york);
    RUN_TEST(test_clock_unset_renders_invalid_until_ntp);
    RUN_TEST(test_clock_keeps_last_valid_time_after_sync);
    RUN_TEST(test_network_scan_lists_ssids_without_blocking);
    RUN_TEST(test_network_scan_dedups_ssid_keeping_strongest_rssi);
    RUN_TEST(test_network_station_ip_and_disconnect_hold);
    RUN_TEST(test_network_persists_profile_only_after_success);
    RUN_TEST(test_network_replaces_last_profile_at_capacity);
    RUN_TEST(test_network_delete_profile);
    RUN_TEST(test_network_manual_timeout_is_failed);
    RUN_TEST(test_network_background_reconnect_does_not_fail);
    RUN_TEST(test_luma_syncs_clock_on_connected_edge);
    RUN_TEST(test_luma_ntp_failure_keeps_clock_unset);
    RUN_TEST(test_settings_wifi_scan_and_open_network);
    RUN_TEST(test_settings_wifi_lists_zh_hans_ssid);
    RUN_TEST(test_settings_masked_password_and_timezone_directory);
    RUN_TEST(test_time_zone_sections_sort_and_utc_labels);
    RUN_TEST(test_settings_time_category_timezone_directory);
    RUN_TEST(test_settings_wifi_nested_split_and_scan_scroll);
    RUN_TEST(test_settings_wifi_password_back_keeps_ssid);
    RUN_TEST(test_settings_wifi_saved_reconnect_skips_password);
    RUN_TEST(test_header_network_glyphs_use_icons_not_spectrum_colors);
    RUN_TEST(test_header_time_and_title_do_not_clip);
    RUN_TEST(test_battery_glyph_fill_and_band_color);
    RUN_TEST(test_header_redraws_when_battery_changes);
    RUN_TEST(test_header_redraws_when_battery_band_changes);
    RUN_TEST(test_battery_history_bars_mark_bands_and_gaps);
    RUN_TEST(test_settings_battery_pane_shows_reading);
    RUN_TEST(test_signal_strength_from_rssi_bins);
    RUN_TEST(test_ellipsize_appends_ascii_dots);
    RUN_TEST(test_wifi_list_glyph_weakest_is_all_secondary);
    RUN_TEST(test_settings_wifi_status_signal_is_dbm_only);
    RUN_TEST(test_settings_wifi_scan_icons_and_long_ssid);
    RUN_TEST(test_battery_band_from_percent);
    RUN_TEST(test_battery_percent_from_voltage_mv);
    RUN_TEST(test_battery_fill_levels_bucket_percent);
    RUN_TEST(test_battery_samples_once_per_minute_and_rolls_over);
    RUN_TEST(test_battery_checkpoint_restores_and_new_run_is_a_gap);
    RUN_TEST(test_battery_checkpoint_failure_keeps_memory_and_emits_error);
    RUN_TEST(test_battery_unknown_values_are_not_fabricated);
    return UNITY_END();
}
