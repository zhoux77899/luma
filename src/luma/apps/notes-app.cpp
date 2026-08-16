#include "luma/apps/notes-app.h"

#include "luma/apps/placeholder-page.h"
#include "luma/core/app-context.h"

namespace luma {

const char* NotesApp::id() const { return "notes"; }
const char* NotesApp::name() const { return "Notes"; }

void NotesApp::onEnter(AppContext& context) { context_ = &context; }

void NotesApp::update(const InputFrame& input) { (void)input; }

void NotesApp::draw() {
    if (context_ != nullptr) {
        drawPlaceholderPage(*context_, name());
    }
}

}  // namespace luma
