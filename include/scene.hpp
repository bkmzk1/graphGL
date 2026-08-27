
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
        
        inline const veil::Mesh& getMesh() const { return *m_mesh; }
        inline veil::InstancedMesh& getDrawable() { return *m_drawable; }

    private:
        std::array<veil::Matrix4, 2> m_axisMatrices;
        std::unique_ptr<veil::Mesh> m_mesh;
        std::unique_ptr<veil::InstancedMesh> m_drawable;
}; //class Axis

class Scene {
    public:
        Scene(const std::string& formula, const std::string& variable);
        ~Scene();

        int startLoop();

    private:
        std::unique_ptr<veil::Window>   m_window;
        std::unique_ptr<veil::GLCamera> m_camera;
        std::unique_ptr<veil::Renderer> m_renderer;

        std::unique_ptr<Axis>  m_axis;
        std::unique_ptr<Graph> m_graph;

        void initWindow();
        void initDrawables(const std::string& formula, const std::string& variable);
        void initLoop();
}; //class Scene