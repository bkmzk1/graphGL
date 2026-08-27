
#include <format>

#include "../include/scene.hpp"
#include "../include/analyzer.hpp"

constexpr char kFormula[] = "x^3";

int main() {

    veil::LogTimer::toggle(false);

    MathFunction function(kFormula, "x");
    function.generatePoints(20.0f, 0.25f);

    MathFunction derivative(function.getDerivative(), "x");
    derivative.generatePoints(20.0f, 0.25f);
    
    const auto& roots = function.getRoots();
    const auto& constIntervals = function.getSignIntervals();
    const auto& growthIntervals = derivative.getSignIntervals();

    std::cout << "Roots:\n\t";
    for (const auto& root : roots)
        std::cout << std::format("{:.2f}, ", root);
    std::cout << std::endl;

    std::cout << "Constant sign intervals:\n\t";
    for (const auto& interval : constIntervals) 
        std::cout << interval << ",  ";
    std::cout << std::endl;

    std::cout << "Increase/decrease intervals:\n\t";
    for (const auto& interval : growthIntervals) 
        std::cout << interval << ",  ";
    std::cout << std::endl;

    Scene scene(kFormula, "x");
    int code = scene.startLoop();
    
    std::_Exit(code);
}