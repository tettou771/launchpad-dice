#include "tcApp.h"

#include <algorithm>

// =============================================================================
// Tuning
// =============================================================================
namespace {
constexpr double kFineTick     = 0.02;   // scheduler tick (50 Hz)
constexpr double kStartGap     = 0.045;  // first gap between number changes
constexpr double kDecel        = 1.16;   // each step the gap grows by this much
constexpr double kMinRollTime  = 2.0;    // don't land before this many seconds
constexpr double kStopGap      = 0.30;   // ...and not until the gap exceeds this

// Result palette: 1 (low) -> 6 (jackpot).
int numberColor(int n) {
    switch (n) {
        case 1: return lp::color::Red;
        case 2: return lp::color::Orange;
        case 3: return lp::color::Yellow;
        case 4: return lp::color::Lime;
        case 5: return lp::color::Cyan;
        case 6: return lp::color::Green;
        default: return lp::color::White;
    }
}
}  // namespace

// =============================================================================
// Lifecycle
// =============================================================================
void tcApp::setup() {
    sound_.setup();

    // Any pad or arrow press starts a roll. These fire on libremidi's input
    // thread; onTrigger locks mtx_ itself.
    lp_.onPad   = [this](int, int, bool pressed, int) { if (pressed) onTrigger(); };
    lp_.onArrow = [this](lp::Arrow, bool pressed)     { if (pressed) onTrigger(); };

    for (auto& row : lastSent_.color) row.fill(-1);  // force a full LED refresh
    {
        std::lock_guard<std::mutex> lock(mtx_);
        render();  // show the idle digit
    }

    // One precise clock for the whole app life; it only does work while rolling.
    callEveryAsync(kFineTick, [this] { fineTick(); });
    // The device is connected in update() once midiReady() (deferred on web).
}

void tcApp::update() {
    if (started_ || !midiReady()) return;
    started_ = true;

#if defined(__EMSCRIPTEN__)
    logNotice("dice") << "web build: Launchpad needs sysex (native only)";
#else
    std::lock_guard<std::mutex> lock(mtx_);
    deviceConnected_ = lp_.connect();
    if (deviceConnected_) {
        logNotice("dice") << "Connected: press any pad to roll";
        for (auto& row : lastSent_.color) row.fill(-1);  // force full refresh
        flushLeds();
    } else {
        logWarning("dice") << "No Launchpad found (connect one over USB and relaunch)";
    }
#endif
}

void tcApp::draw() {
    std::lock_guard<std::mutex> lock(mtx_);

    clear(0.08f);

    const float cs = cellSize();
    const float ox = gridOriginX();
    const float oy = gridOriginY();

    // Header.
    setColor(1.0f);
    string head;
    switch (state_) {
        case State::Idle:    head = "LAUNCHPAD DICE - press any pad to roll"; break;
        case State::Rolling: head = "rolling..."; break;
        case State::Result:
            head = "result: " + std::to_string(result_) + (result_ == 6 ? "   JACKPOT!" : "");
            break;
    }
    drawBitmapString(head, 20, 26);

    // 8x8 grid mirror (row 0 at the bottom).
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            float x = ox + c * cs;
            float y = oy + (7 - r) * cs;
            setColor(screenColor(grid_.get(c, r)));
            drawRect(x, y, cs - 4, cs - 4);
        }
    }

    // Footer.
    setColor(0.6f);
    float fy = oy + 8 * cs + 24;
    drawBitmapString(deviceConnected_ ? "device connected"
                                      : "no device - connect a Launchpad Mini Mk3",
                     20, fy);
}

void tcApp::cleanup() {
    cancelAllAsyncTimers();  // stop the clock (no mtx_ held) before teardown
    lp_.disconnect();        // return the device to Live mode
}

void tcApp::keyPressed(int) {
    onTrigger();
}

// =============================================================================
// Roll logic
// =============================================================================
void tcApp::onTrigger() {
    std::lock_guard<std::mutex> lock(mtx_);
    if (state_ == State::Rolling) return;  // already spinning - ignore
    startRoll();
}

void tcApp::startRoll() {
    state_        = State::Rolling;
    result_       = 0;
    stepInterval_ = kStartGap;
    sinceStep_    = 0.0;
    rollElapsed_  = 0.0;
    shown_        = randomDigit();
    sound_.blip(shown_ * 2);
    render();
    flushLeds();
}

// Scheduler thread: grow the gap, change the number when a gap elapses, and
// land once we've spun long enough and slowed down enough.
void tcApp::fineTick() {
    std::lock_guard<std::mutex> lock(mtx_);
    if (state_ != State::Rolling) return;

    sinceStep_   += kFineTick;
    rollElapsed_ += kFineTick;
    if (sinceStep_ < stepInterval_) return;

    sinceStep_    = 0.0;
    stepInterval_ *= kDecel;

    if (rollElapsed_ >= kMinRollTime && stepInterval_ >= kStopGap) {
        finishRoll();
        return;
    }

    shown_ = randomDigitOtherThan(shown_);
    sound_.blip(shown_ * 2);
    render();
    flushLeds();
}

void tcApp::finishRoll() {
    result_ = randomDigit();
    shown_  = result_;
    state_  = State::Result;
    render();
    flushLeds();
    sound_.playMelody(result_);  // AudioEngine::play is fine on the scheduler thread
}

// =============================================================================
// Rendering
// =============================================================================
void tcApp::render() {
    grid_.clear();
    int color = (state_ == State::Result) ? numberColor(result_)
              : (state_ == State::Rolling) ? lp::color::White
                                           : lp::color::DimBlue;  // idle
    dicefont::drawDigit(grid_, shown_, color);
}

void tcApp::flushLeds() {
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            int color = grid_.get(c, r);
            if (color != lastSent_.color[r][c]) {
                lp_.setPad(c, r, color);
                lastSent_.color[r][c] = color;
            }
        }
    }
}

// =============================================================================
// Helpers
// =============================================================================
int tcApp::randomDigit() {
    return std::uniform_int_distribution<int>(1, 6)(rng_);
}

int tcApp::randomDigitOtherThan(int prev) {
    int n = prev;
    while (n == prev) n = randomDigit();
    return n;
}

float tcApp::cellSize() const {
    float avail = std::min((float)getWindowWidth() - 40.0f,
                           (float)getWindowHeight() - 110.0f);
    return avail / 8.0f;
}
float tcApp::gridOriginX() const {
    return (getWindowWidth() - cellSize() * 8.0f) * 0.5f;
}
float tcApp::gridOriginY() const {
    return 44.0f;
}
