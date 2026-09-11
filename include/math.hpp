
#pragma once

#include <symengine/expression.h>
#include <symengine/parser.h>
#include <symengine/lambda_double.h>
#include <symengine/derivative.h>

struct Point { 
    
    bool operator<(const Point& other) const;
    float x, y;
};
struct Interval {
    
    Interval(float s, float e, int n) : start(s), end(e), sign(n) {}
    friend std::ostream& operator<<(std::ostream& os, const Interval& interval);

    float start, end; 
    int sign; 
};

class MathFunction {
    public:
        MathFunction(const std::string& formula, const std::string& variable);

        void setFormula(const std::string& formula);
        void generatePoints(float range, float step);

        std::string getDerivative() const;
        std::set<float> getRoots() const;
        std::vector<Interval> getSignIntervals() const;

        static void getAnalytics(const MathFunction& func, const MathFunction& deriv, 
                                 std::string& roots, std::string& signs, std::string& growth);

    private:
        SymEngine::Expression m_variable;
        SymEngine::Expression m_functionExpr;
        mutable SymEngine::LambdaRealDoubleVisitor m_functionVisitor;

        std::vector<Point> m_points;

}; //class MathFunction