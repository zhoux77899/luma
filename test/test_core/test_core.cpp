#include "fakes.h"

#include "luma/core/app-context.h"
#include "luma/core/app-manager.h"
#include "luma/core/file-storage.h"
#include "luma/core/in-memory-storage.h"
#include "luma/core/input-manager.h"
#include "luma/core/settings.h"
#include "luma/luma.h"
#include "luma/platform/host/host-audio-adapter.h"
#include "luma/ui/components.h"
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

#include <string>
#include <vector>

using luma::AppContext;
using luma::AppManager;
using luma::InputAction;
using luma::InputManager;
using luma::Luma;
using luma::Settings;
using luma::layout::appCardBounds;
using luma::theme::kAccent;
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
        : context(display, settings, storage, clock),
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
    TEST_ASSERT_EQUAL_UINT(3, fixture.log.size());
    TEST_ASSERT_EQUAL_STRING("about:exit", fixture.log[0].c_str());
    TEST_ASSERT_EQUAL_STRING("launcher:enter", fixture.log[1].c_str());
    TEST_ASSERT_EQUAL_STRING("launcher:draw", fixture.log[2].c_str());
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
    TEST_ASSERT_TRUE(display.hasText("Settings"));
    TEST_ASSERT_TRUE(display.hasText("About"));
    TEST_ASSERT_TRUE(display.hasText("Notes"));
    TEST_ASSERT_FALSE(display.hasText("Launcher"));
    TEST_ASSERT_FALSE(display.hasText("Enter Open"));
    TEST_ASSERT_TRUE(display.hasText("--:--"));
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
    TEST_ASSERT_TRUE(display.hasStroke(appCardBounds(0, 0), kAccent));
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
    TEST_ASSERT_TRUE(display.hasText("Settings"));
    TEST_ASSERT_TRUE(display.hasText("Coming soon"));
    TEST_ASSERT_TRUE(display.hasText("Esc Back"));
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
    TEST_ASSERT_TRUE(display.hasStroke(appCardBounds(0, 0), kAccent));

    input.push(makeAction(InputAction::Right));
    luma.update();
    TEST_ASSERT_TRUE(display.hasStroke(appCardBounds(1, 0), kAccent));

    input.push(makeAction(InputAction::Left));
    luma.update();
    input.push(makeAction(InputAction::Down));
    luma.update();
    TEST_ASSERT_TRUE(display.hasStroke(appCardBounds(0, 1), kAccent));

    input.push(makeAction(InputAction::Right));
    luma.update();
    TEST_ASSERT_TRUE(display.hasStroke(appCardBounds(0, 1), kAccent));
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
    luma::drawDialog(display, "Title", "Body");

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
    RUN_TEST(test_host_audio_emits_event_log);
    return UNITY_END();
}
