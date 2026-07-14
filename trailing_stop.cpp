// Project Hail Mary — quad trailing stop-loss, C++ port of the live Python logic.

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <iomanip>

enum class Direction { BUY, SHORT };

struct TrailLevel {
    double trigger;
    double newStop;
};

// round to 2 decimals; std::round only does whole numbers
double round2(double value) {
    return std::round(value * 100.0) / 100.0;
}

// splits entry->target into (nSteps+1) equal chunks, returns nSteps trigger/stop pairs
std::vector<TrailLevel> buildTrailLevels(double entry, double initialStop,
                                          double target, Direction direction,
                                          int nSteps = 4) {
    double orSize = std::fabs(target - entry);
    double step = orSize / (nSteps + 1);

    std::vector<TrailLevel> levels;

    for (int k = 1; k <= nSteps; k++) {
        double trigger, newStop;
        if (direction == Direction::BUY) {
            trigger  = round2(entry + step * k);
            newStop  = round2(initialStop + step * k);
        } else {
            trigger  = round2(entry - step * k);
            newStop  = round2(initialStop - step * k);
        }
        levels.push_back(TrailLevel{trigger, newStop});
    }
    return levels;
}

// ratchets the stop forward on a new price tick, never backward (k > trailLevel check)
void updateTrail(double price, const std::vector<TrailLevel>& levels,
                  Direction direction, int& trailLevel, double& activeStop) {
    int k = 1;
    for (const auto& level : levels) {
        bool crossed = (direction == Direction::BUY)
                           ? (price >= level.trigger)
                           : (price <= level.trigger);

        if (crossed && k > trailLevel) {
            trailLevel = k;
            activeStop = level.newStop;
        }
        k++;
    }
}

std::string directionName(Direction d) {
    return d == Direction::BUY ? "BUY" : "SHORT";
}

// runs one scenario tick by tick, printing the stop after each and checking exits
void runScenario(const std::string& label, double orSize, double entry,
                  double initialStop, double target, Direction direction,
                  const std::vector<double>& priceTicks) {
    std::vector<TrailLevel> levels = buildTrailLevels(entry, initialStop, target, direction);

    std::cout << "\n=== " << label << " (" << directionName(direction) << ") ===\n";
    std::cout << "Opening range size: Rs" << orSize << "\n";
    std::cout << "Entry: " << entry << " | Initial stop: " << initialStop
              << " | Target: " << target << "\n";
    std::cout << "Trail levels (trigger -> new stop): ";
    for (const auto& lvl : levels) {
        std::cout << "[" << lvl.trigger << " -> " << lvl.newStop << "] ";
    }
    std::cout << "\n";

    int trailLevel = 0;
    double activeStop = initialStop;

    std::cout << std::fixed << std::setprecision(2);
    for (double price : priceTicks) {
        int levelBefore = trailLevel;
        updateTrail(price, levels, direction, trailLevel, activeStop);

        std::cout << "  Price " << price << " -> active stop " << activeStop
                   << " (trail level " << trailLevel << " of 4)";
        if (trailLevel > levelBefore) {
            std::cout << "  <-- TRAIL " << trailLevel << " ACTIVATED";
        }
        std::cout << "\n";

        bool targetHit = (direction == Direction::BUY) ? (price >= target) : (price <= target);
        bool stopHit   = (direction == Direction::BUY) ? (price <= activeStop) : (price >= activeStop);

        if (targetHit) {
            std::cout << "  >>> TARGET HIT — trade closed for a win\n";
            return;
        }
        if (stopHit) {
            std::string label2 = (trailLevel > 0)
                ? "TRAIL STOP HIT (level " + std::to_string(trailLevel) + " of 4)"
                : "FULL STOP HIT (initial stop, no trail ever activated)";
            std::cout << "  >>> " << label2 << " — trade closed\n";
            return;
        }
    }
}

int main() {
    // stop/target both Rs4 off entry -> strict 1:1, same as the live OR-based sizing
    const double OR_SIZE = 4.0;
    const double ENTRY    = 600.0;

    const double BUY_STOP   = ENTRY - OR_SIZE;
    const double BUY_TARGET = ENTRY + OR_SIZE;
    const double SHT_STOP   = ENTRY + OR_SIZE;
    const double SHT_TARGET = ENTRY - OR_SIZE;

    runScenario(
        "Scenario 1: BUY, direct stop-loss hit (no trail ever activates)",
        OR_SIZE, ENTRY, BUY_STOP, BUY_TARGET, Direction::BUY,
        {599.0, 598.0, 595.5}
    );

    runScenario(
        "Scenario 2: BUY, steady climb to target",
        OR_SIZE, ENTRY, BUY_STOP, BUY_TARGET, Direction::BUY,
        {600.8, 601.6, 602.4, 603.2, 604.0}
    );

    runScenario(
        "Scenario 3: BUY, partial climb then reversal",
        OR_SIZE, ENTRY, BUY_STOP, BUY_TARGET, Direction::BUY,
        {600.8, 601.6, 600.9, 599.9}
    );

    runScenario(
        "Scenario 4: SHORT, direct stop-loss hit (no trail ever activates)",
        OR_SIZE, ENTRY, SHT_STOP, SHT_TARGET, Direction::SHORT,
        {601.0, 602.0, 604.5}
    );

    runScenario(
        "Scenario 5: SHORT, steady fall to target",
        OR_SIZE, ENTRY, SHT_STOP, SHT_TARGET, Direction::SHORT,
        {599.2, 598.4, 597.6, 596.8, 596.0}
    );

    runScenario(
        "Scenario 6: SHORT, price gaps past two levels in one tick",
        OR_SIZE, ENTRY, SHT_STOP, SHT_TARGET, Direction::SHORT,
        {598.0, 597.0}
    );

    return 0;
}
