#pragma once

// =============================================================================
// Launchpad Dice - a digital dice / roulette on the Launchpad Mini Mk3
// =============================================================================
//   Press any pad (or arrow) to roll. The 8x8 grid shows a digital 1..6 that
//   flickers randomly with roulette blips, decelerates over ~3 s, then lands on
//   a random 1..6 and plays a result tune (6 = jackpot, 1 = womp-womp).
//
//   The screen mirrors the grid 1:1. Everything is driven by the device.
//
// Threading: onPad/onArrow fire on libremidi's input thread; a single precise
// async timer (fineTick) clocks the roll on the scheduler thread. `mtx_` guards
// the state they share with draw().
// =============================================================================

#include <TrussC.h>

#include "LaunchpadMk3.h"
#include "PadGrid.h"
#include "DiceFont.h"
#include "DiceSound.h"

#include <mutex>
#include <random>

using namespace tc;
using namespace tcx;
using namespace std;

class tcApp : public App {
public:
    void setup() override;
    void update() override;
    void draw() override;
    void cleanup() override;

    // Roll from the computer keyboard too (handy without the device).
    void keyPressed(int key) override;

private:
    enum class State { Idle, Rolling, Result };

    void onTrigger();      // any pad/arrow press: start a roll (input thread)
    void startRoll();      // assumes mtx_ held
    void fineTick();       // scheduler thread: advance the roll
    void finishRoll();     // assumes mtx_ held: land + play melody
    void render();         // state -> grid_ (assumes mtx_ held)
    void flushLeds();      // diff grid_ -> device LEDs (assumes mtx_ held)

    int  randomDigit();
    int  randomDigitOtherThan(int prev);

    // Screen mirror helpers (row 0 = bottom).
    float cellSize() const;
    float gridOriginX() const;
    float gridOriginY() const;

    LaunchpadMk3 lp_;
    DiceSound    sound_;
    PadGrid      grid_;
    PadGrid      lastSent_;

    State  state_ = State::Idle;
    int    shown_ = 1;     // digit currently on the grid
    int    result_ = 0;    // final landed digit (0 = none yet)

    // Deceleration clock (seconds), advanced by fineTick.
    double stepInterval_ = 0.0;  // gap between number changes; grows as it slows
    double sinceStep_    = 0.0;   // time since the last number change
    double rollElapsed_  = 0.0;   // total time spent in the current roll

    std::mt19937 rng_{std::random_device{}()};

    bool started_         = false;
    bool deviceConnected_ = false;

    std::mutex mtx_;
};
