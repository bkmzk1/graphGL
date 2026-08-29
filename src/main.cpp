
#include <format>

#include "../include/scene.hpp"
#include "../include/analyzer.hpp"

constexpr char kFormula[] = "1/x";

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

    veil::Window window("graphGL", {800.0f, 800.0f});
    window.setInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    veil::initGL(&window);
    veil::toggleGLFlags(&window, {GL_DEPTH_TEST, GL_CULL_FACE, GL_BLEND, GL_PRIMITIVE_RESTART}, true);

    veil::Storage<veil::ShaderStorage>().loadShader(
        "basic", { {"shader/vertex.vert", GL_VERTEX_SHADER}, {"shader/fragment.frag", GL_FRAGMENT_SHADER} } 
    );
    veil::Storage<veil::ShaderStorage>().loadShader(
        "instanced", { {"shader/instanced.vert", GL_VERTEX_SHADER}, {"shader/fragment.frag", GL_FRAGMENT_SHADER} } 
    );
    veil::Storage<veil::ShaderStorage>().loadShader(
        "font", { {"shader/instanced.vert", GL_VERTEX_SHADER}, {"shader/font.frag", GL_FRAGMENT_SHADER} } 
    );
    veil::Storage<veil::UniformBufferStorage>().loadUBO<veil::GLCamera::Attitude>(
        0
    );
    const veil::ShaderProgram* basicShader = veil::Storage<veil::ShaderStorage>().getShader("basic");
    const veil::ShaderProgram* instancedShader = veil::Storage<veil::ShaderStorage>().getShader("instanced");
    const veil::ShaderProgram* fontShader = veil::Storage<veil::ShaderStorage>().getShader("font");
    const veil::UniformBuffer* attitudeUBO = veil::Storage<veil::UniformBufferStorage>().getUBO(0);

    veil::GLCamera camera(
        {0.0f, 0.0f, 5.5f}, {0.0f, 1.0f, 0.0f}, window.getAspectRatio(), 90.0f 
    );

    Axis axis(*instancedShader, "aModel");
    axis.updateRange(20.0f);

    Graph graph(kFormula, "x");
    graph.buildMesh(20.0f, 1500);
    graph.getDrawable().scale({5.0f, 5.0f, 5.0f});
    graph.getDrawable().setDrawingMode(GL_LINE_STRIP);

    veil::Renderer renderer;
    renderer.setForTargetCallback(
        [&](const veil::ShaderProgram* shader, const veil::Drawable* drawable) {

            if (drawable->getType() == veil::DrawableType::MESH_SINGULAR) {
                const veil::MeshInstance* mesh = static_cast<const veil::MeshInstance*>(drawable);
                renderer.uploadUniformDirect(*shader, "uColor", veil::Vector3{1.0f, 0.0f, 0.0f});
                renderer.uploadUniformDirect(*shader, "uModel", mesh->getModelMat());
            }
            if (drawable->getType() == veil::DrawableType::MESH_INSTANCED) {
                const veil::InstancedMesh* mesh = static_cast<const veil::InstancedMesh*>(drawable);
                renderer.uploadUniformDirect(*shader, "uColor", veil::Vector3{1.0f, 1.0f, 1.0f});
            }
        }
    );
    renderer.reserveShaders(
        { basicShader, instancedShader, fontShader }
    );
    renderer.addTargets({ 
        { *basicShader,     graph.getDrawable() },
        { *instancedShader, axis.getAxisDrawable() },
        { *fontShader,      axis.getRangeTextDrawable() }
    });
    renderer.uploadUniformBuffers( 
        std::make_pair(attitudeUBO, [&]() { return camera.getAttitude(); }) 
    );

    window.setFramebufferCallback(
        [&]() {
            camera.updateProjection(90.0f, window.getAspectRatio());
            camera.resyncMouse();
        }
    );
    window.setMouseCallback(
        [&](double xpos, double ypos) {
            camera.calculateAttitude(xpos, ypos);
        }
    );
    window.setScrollCallback(
        [&](double xoff, double yoff) {

            float range = graph.getCurrentRange();
            if (yoff > 0) 
                graph.buildMesh((std::clamp(range/1.1f, 0.5f, 50.0f)), 1500);
            if (yoff < 0 && range <= 70.0f) 
                graph.buildMesh((std::clamp(range*1.1f, 0.5f, 50.0f)), 1500);

            axis.updateRange(graph.getCurrentRange());
        }   
    );
    window.setKeyCallback(
        [&](const veil::KeyEvents& ke) {

            float dt = window.getClock().getDeltaTime();
            float speed = 2.0f;

            if(ke.keysDown[GLFW_KEY_W])
                camera.move(+camera.getFront() * dt * speed);
            if(ke.keysDown[GLFW_KEY_S])
                camera.move(-camera.getFront() * dt * speed);
            if(ke.keysDown[GLFW_KEY_A])
                camera.move(-veil::Vector3::cross(camera.getFront(), camera.getUp()) * dt * speed);
            if(ke.keysDown[GLFW_KEY_D])
                camera.move(+veil::Vector3::cross(camera.getFront(), camera.getUp()) * dt * speed);
        }
    );
    window.setUpdateCallback(
        [&]() {
            renderer.callbackUniforms();
            renderer.callbackRender();
        }
    );

    int code = window.startUpdateLoop();
    std::_Exit(code);
}