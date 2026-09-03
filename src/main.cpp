
#include "../include/scene.hpp"
#include "../include/analyzer.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

constexpr char kFormula[] = "x^2+4*x";

int main() {

    bool cursorDisabled = true;

    MathFunction function(kFormula, "x");
    function.generatePoints(20.0f, 0.25f);
    
    MathFunction derivative(function.getDerivative(), "x");
    derivative.generatePoints(20.0f, 0.25f);
    
    veil::Window window("graphGL", {800.0f, 800.0f});
    window.setInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    veil::initGL(&window);
    veil::toggleGLFlags(&window, {GL_DEPTH_TEST, GL_CULL_FACE, GL_BLEND, GL_PRIMITIVE_RESTART}, true);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.MouseDrawCursor = false;

    ImGui_ImplGlfw_InitForOpenGL(window.getNativeHandle(), false);
    ImGui_ImplOpenGL3_Init("#version 460");

    veil::Storage<veil::ShaderStorage>().loadShader(
        "basic", { {"shader/vertex.vert", GL_VERTEX_SHADER}, {"shader/fragment.frag", GL_FRAGMENT_SHADER} } 
    );
    veil::Storage<veil::ShaderStorage>().loadShader(
        "instanced", { {"shader/instanced.vert", GL_VERTEX_SHADER}, {"shader/fragment.frag", GL_FRAGMENT_SHADER} } 
    );
    veil::Storage<veil::ShaderStorage>().loadShader(
        "basicFont", { {"shader/vertex.vert", GL_VERTEX_SHADER}, {"shader/font.frag", GL_FRAGMENT_SHADER} } 
    );
    veil::Storage<veil::ShaderStorage>().loadShader(
        "instancedFont", { {"shader/instanced.vert", GL_VERTEX_SHADER}, {"shader/font.frag", GL_FRAGMENT_SHADER} } 
    );
    veil::Storage<veil::UniformBufferStorage>().loadUBO<veil::GLCamera::Attitude>(
        0
    );
    const veil::ShaderProgram* basicShader = veil::Storage<veil::ShaderStorage>().getShader("basic");
    const veil::ShaderProgram* instancedShader = veil::Storage<veil::ShaderStorage>().getShader("instanced");
    const veil::ShaderProgram* basicFontShader = veil::Storage<veil::ShaderStorage>().getShader("basicFont");
    const veil::ShaderProgram* instancedFontShader = veil::Storage<veil::ShaderStorage>().getShader("instancedFont");
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

            if (drawable->getType() == veil::DrawableType::TEXT_SINGULAR) {
                const veil::TextInstance* text = dynamic_cast<const veil::TextInstance*>(drawable);
                renderer.uploadUniformDirect(*shader, "uModel", text->getModelMat());
            }
            if (drawable->getType() == veil::DrawableType::MESH_SINGULAR) {
                const veil::MeshInstance* mesh = dynamic_cast<const veil::MeshInstance*>(drawable);
                renderer.uploadUniformDirect(*shader, "uColor", veil::Vector3{1.0f, 0.0f, 0.0f});
                renderer.uploadUniformDirect(*shader, "uModel", mesh->getModelMat());
            }
            if (drawable->getType() == veil::DrawableType::MESH_INSTANCED) {
                const veil::InstancedMesh* mesh = dynamic_cast<const veil::InstancedMesh*>(drawable);
                renderer.uploadUniformDirect(*shader, "uColor", veil::Vector3{1.0f, 1.0f, 1.0f});
            }
        }
    );
    renderer.reserveShaders(
        { basicShader, instancedShader, basicFontShader, instancedFontShader }
    );
    renderer.addTargets({ 
        { *basicShader,         graph.getDrawable() },
        { *instancedShader,     axis.getAxisDrawable() },
        { *instancedFontShader, axis.getRangeTextDrawable() }
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
            ImGui_ImplGlfw_CursorPosCallback(window.getNativeHandle(), xpos, ypos);

            if (cursorDisabled)
                camera.calculateAttitude(xpos, ypos);
        }
    );
    window.setScrollCallback(
        [&](double xoff, double yoff) {
            ImGui_ImplGlfw_ScrollCallback(window.getNativeHandle(), xoff, yoff);

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

            if (ke.keysDown[GLFW_KEY_C]) {
                if (cursorDisabled) {
                    window.setInputMode(GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    camera.resyncMouse();
                    cursorDisabled = false;
                }
            } 
            else {
                if (!cursorDisabled) {
                    window.setInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    camera.resyncMouse(); 
                    cursorDisabled = true;
                }
            }

            if (!cursorDisabled)
                return; 

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
    window.setMouseButtonCallback(
        [&](const veil::KeyEvents& mbe) {
            static bool prevDown[GLFW_MOUSE_BUTTON_LAST + 1] = {};

            for (int b = 0; b <= GLFW_MOUSE_BUTTON_LAST; ++b) {
                bool down = mbe.keysDown[b];
                if (down != prevDown[b]) {
                    ImGui_ImplGlfw_MouseButtonCallback(window.getNativeHandle(), b, down ? GLFW_PRESS : GLFW_RELEASE, 0);
                    prevDown[b] = down;
                }
            }
        }
    );

    window.setUpdateCallback(
        [&]() {

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::Begin("My Debug Window");
            ImGui::Text("Hello, Worldssssssssssssssssssss!");
            ImGui::End();

            renderer.callbackUniforms();
            renderer.callbackRender();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
    );

    int code = window.startUpdateLoop();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    std::_Exit(code);
}