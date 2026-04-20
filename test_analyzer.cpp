#include "../src/PitchAnalyzer.h"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "🧪 Running Unit Tests for PitchAnalyzer..." << std::endl;
    
    PitchAnalyzer analyzer;

    // Test 1: Standard A4 frequency
    NoteDef res1 = analyzer.identifyNote(440.0);
    assert(res1.name == "A4");
    std::cout << "✅ Test 1 Passed: 440.0Hz -> A4" << std::endl;

    // Test 2: Standard C4 with slight error (should still pass due to 4% tolerance)
    NoteDef res2 = analyzer.identifyNote(262.5);
    assert(res2.name == "C4");
    std::cout << "✅ Test 2 Passed: 262.5Hz -> C4 (Tolerance matching)" << std::endl;

    // Test 3: High Note B6
    NoteDef res3 = analyzer.identifyNote(1975.5);
    assert(res3.name == "B6");
    std::cout << "✅ Test 3 Passed: 1975.5Hz -> B6" << std::endl;

    // Test 4: Unknown frequency (noise)
    NoteDef res4 = analyzer.identifyNote(50.0);
    assert(res4.name == "Unknown");
    std::cout << "✅ Test 4 Passed: 50.0Hz -> Unknown" << std::endl;

    std::cout << "🎉 All Unit Tests Passed Successfully!" << std::endl;
    return 0;
}
