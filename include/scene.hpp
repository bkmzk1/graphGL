
#pragma once

#include <veil/veil.hpp>

#include <memory>

#include <symengine/expression.h>
#include <symengine/parser.h>
#include <symengine/lambda_double.h>

class Graph {
    public:
        Graph(const std::string& formula, const std::string& variable);
        ~Graph();

        void buildMesh(float range, int n);

        inline float getCurrentRange() const { return m_range; }
        inline const veil::Mesh& getMesh() const { return *m_mesh; }
        inline veil::MeshInstance& getDrawable() { return *m_drawable; }

    private:
        float m_range = 0.0f;
        std::unique_ptr<veil::Mesh> m_mesh;
        std::unique_ptr<veil::MeshInstance> m_drawable;

        const SymEngine::Expression m_variable;
        const SymEngine::Expression m_functionExpr;
        mutable SymEngine::LambdaRealDoubleVisitor m_functionLambda;
}; //class Graph

class Axis {
    public:
        Axis(const veil::ShaderProgram& instancedShader, std::string_view attribName);
        ~Axis();

        void updateRange(float range);
        
        inline veil::InstancedMesh& getAxisDrawable() { return *m_axisDrawable; }
        inline veil::InstancedText& getRangeTextDrawable() { return *m_rangeTextDrawable; }

    private:
        std::unique_ptr<veil::Font> m_font;

        std::array<veil::Matrix4, 2> m_axisMatrices;
        std::array<veil::Matrix4, 2> m_rangeTextMatrices;

        std::unique_ptr<veil::Mesh> m_axisMesh;
        std::unique_ptr<veil::Text> m_rangeText;

        std::unique_ptr<veil::InstancedMesh> m_axisDrawable;
        std::unique_ptr<veil::InstancedText> m_rangeTextDrawable;
}; //class Axis