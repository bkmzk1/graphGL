
#include "../include/scene.hpp"
#include "../include/analyzer.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

static void getAnalytics(const MathFunction& func, const MathFunction& deriv, 
                         std::string& roots, std::string& signs, std::string& growth);

int main() {

    bool cursorDisabled = false;
    char formula[128] = "x";
    std::string roots, signs, growth;

    MathFunction function(formula, "x");
    function.generatePoints(20.0f, 0.25f);
    MathFunction derivative(function.getDerivative(), "x");
    derivative.generatePoints(20.0f, 0.25f);
    
    veil::Window window("graphGL", {800.0f, 800.0f});
    window.setInputMode(GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    veil::initGL(&window);
    veil::toggleGLFlags(&window, {GL_DEPTH_TEST, GL_CULL_FACE, GL_BLEND, GL_PRIMITIVE_RESTART}, true);

    veil::GLCamera camera({0.0f, 0.0f, 5.5f}, {0.0f, 1.0f, 0.0f}, window.getAspectRatio(), 90.0f);

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

    Axis axis(*instancedShader, "aModel");
    axis.updateRange(20.0f);

    Graph graph(formula, "x");
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

            if (cursorDisabled)
                camera.calculateAttitude(xpos, ypos);
        }
    );
    window.setScrollCallback(
        [&](double xoff, double yoff) {

            if (!cursorDisabled)
                return;

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

            static bool cKeyWasDown = false;
            bool cKeyIsDown = ke.keysDown[GLFW_KEY_C];

            if (cKeyIsDown && !cKeyWasDown) {

                cursorDisabled = !cursorDisabled;
                if (cursorDisabled) {
                    window.setInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
                }
                else {
                    window.setInputMode(GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
                }
                camera.resyncMouse();
            }
            cKeyWasDown = cKeyIsDown;

            if (!cursorDisabled)
                return; 

            float dt = window.getClock().getDeltaTime();
            static float speed = 2.0f;

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

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::SetNextWindowPos(ImVec2(5.0f, 5.0f), ImGuiCond_Always);
            ImGui::Begin("Formula", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
            if (ImGui::InputText("Enter Function", formula, IM_ARRAYSIZE(formula), ImGuiInputTextFlags_EnterReturnsTrue)) {

                formula[sizeof(formula) - 1] = '\0';
                
                graph.setFormula(formula);
                graph.buildMesh(graph.getCurrentRange(), 1500);

                function.setFormula(formula);
                function.generatePoints(20.0f, 0.25f);
                derivative.setFormula(function.getDerivative());
                derivative.generatePoints(20.0f, 0.25f);    

                getAnalytics(function, derivative, roots, signs, growth);
            }
            ImGui::End();

            ImGui::Begin("Analytics");
            if (ImGui::CollapsingHeader("Roots")) {

                ImGui::BeginChild("Roots", ImVec2(0.0, 100.0), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
                ImGui::TextUnformatted(roots.c_str());
                ImGui::EndChild();
            }
            if (ImGui::CollapsingHeader("Signs")) {

                ImGui::BeginChild("Signs", ImVec2(0.0, 100.0), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
                ImGui::TextUnformatted(signs.c_str());
                ImGui::EndChild();
            }
            if (ImGui::CollapsingHeader("Growth")) {

                ImGui::BeginChild("Growth", ImVec2(0.0, 100.0), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
                ImGui::TextUnformatted(growth.c_str());
                ImGui::EndChild();
            }
            ImGui::End();

            renderer.callbackUniforms();
            renderer.callbackRender();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
    );

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.MouseDrawCursor = false;

    ImGui_ImplGlfw_InitForOpenGL(window.getNativeHandle(), true);
    ImGui_ImplOpenGL3_Init("#version 460");

    int code = window.startUpdateLoop();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    std::_Exit(code);
}

void getAnalytics(const MathFunction& func, const MathFunction& deriv, 
                         std::string& roots, std::string& signs, std::string& growth) {
    roots = "Roots:\n";
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

    signs = "Sign intervals:\n";
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

    growth = "Growth intervals:\n";
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