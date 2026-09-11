
#include "../include/math.hpp"
#include <format>

bool Point::operator<(const Point& other) const {
    if (x != other.x) 
        return x < other.x;
    return y < other.y;
}

std::ostream& operator<<(std::ostream& os, const Interval& interval) {

    char sign = (interval.sign > 0) ? '+' : '-';
    std::cout << std::format("{}({:.2f}; {:.2f})", sign, interval.start, interval.end);

    return os;
}

MathFunction::MathFunction(const std::string& formula, const std::string& variable) 
    : m_functionExpr(SymEngine::parse(formula)), m_variable(SymEngine::symbol(variable)) {

    m_functionVisitor.init({m_variable}, m_functionExpr);
}

void MathFunction::setFormula(const std::string& formula) {

    m_functionExpr = SymEngine::parse(formula);
    m_functionVisitor.init({m_variable}, m_functionExpr);
}

void MathFunction::generatePoints(float range, float step) {

    m_points.clear();   

    for (float i = -range/2; i <= range/2; i+=step) {

        float xVal = i;
        float yVal = m_functionVisitor.call({xVal});

        if (std::isnan(yVal) || std::isinf(yVal))
            continue;

        m_points.push_back(Point{.x=xVal, .y=yVal});
    }
    std::sort(m_points.begin(), m_points.end());
}

std::string MathFunction::getDerivative() const {

    return SymEngine::str(m_functionExpr.diff(m_variable));
}

std::set<float> MathFunction::getRoots() const {

    std::set<float> roots;

    for (size_t i = 0; i < m_points.size() - 1; ++i) {

        if (m_points[i].y == 0) {

            roots.insert(m_points[i].x);
            continue;
        }
        else if (std::signbit(m_points[i].y) != std::signbit(m_points[i+1].y)) {

            float xRoot = m_points[i].x - m_points[i].y * 
                         ((m_points[i+1].x - m_points[i].x) / (m_points[i+1].y - m_points[i].y));

            float yRoot = m_functionVisitor.call({xRoot});
            
            if (!std::isnan(yRoot) && !std::isinf(yRoot) && std::abs(yRoot) < 0.005f)
                roots.insert(xRoot);
        }
        else
            continue;
    }
    if (m_points.back().y == 0)
        roots.insert(m_points.back().x);

    return roots;
}

std::vector<Interval> MathFunction::getSignIntervals() const {

    auto getSign = [](float y) -> int { 
        if (y > 0.0f) return 1; 
        if (y < 0.0f) return -1; 
        return 0; 
    };

    std::vector<Interval> intervals;

    float intervalStartX = m_points[0].x;
    int currentSign = getSign(m_points[0].y);

    for (size_t i = 1; i < m_points.size(); ++i) {

        int pointSign = getSign(m_points[i].y);

        if (pointSign == 0) 
            continue;
        if (currentSign == 0) {    
            currentSign = pointSign;
            continue;
        }

        if (pointSign != currentSign) {

            Point one(m_points[i-1].x, m_points[i-1].y);
            Point two(m_points[i].x, m_points[i].y);

            float zeroCrossingX;
            if (two.y != one.y)
                zeroCrossingX = one.x - one.y * (two.x - one.x) / (two.y - one.y);
            else 
                zeroCrossingX = (one.x + two.x) / 2.0f;

            intervals.emplace_back(Interval{intervalStartX, zeroCrossingX, currentSign});
            intervalStartX = zeroCrossingX;
            currentSign = pointSign;
        }
    }
    if (intervals.empty() || intervals.back().end != m_points.back().x)
        intervals.emplace_back(Interval{intervalStartX, m_points.back().x, currentSign});

    return intervals;
}

void MathFunction::getAnalytics(const MathFunction& func, const MathFunction& deriv, 
                                std::string& roots, std::string& signs, std::string& growth) {
    roots = "";
    const auto& rootArr = func.getRoots();
    if (rootArr.empty())
        roots += "None";
    else {
        int count = 0;
        for (const auto& root : rootArr) {
            roots += std::format("{:.2f}, ", root);
            if (++count % 3 == 0)
                roots += "\n";
        }
    }

    signs = "";
    const auto& signArr = func.getSignIntervals();
    if (signArr.empty())
        signs += "None";
    else {
        int count = 0;
        for (const auto& interval : signArr) {
            signs += std::format("{}({:.2f}; {:.2f}),  ", (interval.sign > 0 ? '+' : '-'), interval.start, interval.end);
            if (++count % 2 == 0)
                signs += "\n";
        }
    }

    growth = "";
    const auto& growthArr = deriv.getSignIntervals();
    if (growthArr.empty())
        growth += "None";
    else {
        int count = 0;
        for (const auto& interval : growthArr) {
            growth += std::format("{}({:.2f}; {:.2f}),  ", (interval.sign > 0 ? '+' : '-'), interval.start, interval.end);
            if (++count % 2 == 0)
                growth += "\n";
        }
    }
}