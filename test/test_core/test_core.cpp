#include "fakes.h"

#include "luma/core/app-context.h"
#include "luma/core/app-manager.h"
#include "luma/core/input-manager.h"
#include "luma/core/settings.h"
#include "luma/luma.h"

#include <unity.h>

#include <string>
#include <vector>

using luma::AppContext;
using luma::AppManager;
using luma::InputAction;
using luma::InputManager;
using luma::Luma;
using luma::Settings;
using luma::test::CountingStorage;
using luma::test::FakeClock;
using luma::test::FakeDiagnostics;
using luma::test::FakeDisplay;
using luma::test::FakeInputSource;
using luma::test::RecordingApp;
using luma::test::makeAction;
using luma::test::makeText;

namespace {

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

void test_luma_begin_enters_launcher() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    CountingStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    Luma luma(display, input, clock, storage, settings, diagnostics);

    luma.begin();

    TEST_ASSERT_TRUE(display.begun);
    TEST_ASSERT_TRUE(storage.begun);
    TEST_ASSERT_EQUAL_STRING("launcher", luma.currentAppId());
    TEST_ASSERT_TRUE(diagnostics.contains("[BOOT] Luma Cardputer ADV started"));
    TEST_ASSERT_TRUE(diagnostics.contains("[APP] enter launcher"));
    TEST_ASSERT_EQUAL_STRING("Luma / Cardputer ADV", display.texts[0].c_str());
    TEST_ASSERT_EQUAL_STRING("Launcher", display.texts[1].c_str());
}

void test_luma_draws_only_when_dirty() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    CountingStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    Luma luma(display, input, clock, storage, settings, diagnostics);

    luma.begin();
    const int after_begin = display.draw_text_count;
    luma.update();
    TEST_ASSERT_EQUAL_INT(after_begin, display.draw_text_count);
}

void test_luma_processes_deferred_saves_each_update() {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    CountingStorage storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    Luma luma(display, input, clock, storage, settings, diagnostics);

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
    RecordingApp about("about", "About", 'a', log);
    Luma luma(display, input, clock, storage, settings, diagnostics);

    luma.registerApp(about);
    luma.begin();
    input.push(makeText('a'));
    luma.update();

    TEST_ASSERT_EQUAL_STRING("about", luma.currentAppId());
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

void setUp() {}
void tearDown() {}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_app_manager_lifecycle_order);
    RUN_TEST(test_app_manager_back_returns_to_launcher);
    RUN_TEST(test_app_manager_launcher_back_is_noop);
    RUN_TEST(test_app_manager_shortcut_opens_registered_app);
    RUN_TEST(test_luma_begin_enters_launcher);
    RUN_TEST(test_luma_draws_only_when_dirty);
    RUN_TEST(test_luma_processes_deferred_saves_each_update);
    RUN_TEST(test_luma_routes_input_frame_to_app_manager);
    RUN_TEST(test_input_manager_dispatches_fake_source);
    return UNITY_END();
}
