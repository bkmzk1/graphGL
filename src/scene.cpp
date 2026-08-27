
#include "../include/scene.hpp"

Graph::Graph(const std::string& formula, const std::string& variable)
    : m_functionExpr(SymEngine::parse(formula)), m_variable(SymEngine::symbol(variable)) {

    m_functionLambda.init({m_variable}, m_functionExpr);

    m_mesh = std::make_unique<veil::Mesh>(
        std::vector<veil::Vertex>{},
        std::vector<unsigned int>{},
        veil::Material{}
    );
    m_drawable = std::make_unique<veil::MeshInstance>(*m_mesh);
}
Graph::~Graph() {

    m_drawable.reset();
    m_mesh.reset();
}

void Graph::buildMesh(float range, int n) {

    m_range = range;

    std::vector<veil::Vertex> vertices;
    std::vector<unsigned int> indices;

    float spacing = range / n;
    float yLimit = range / 2.0f; 

    float prevX = 0.0f;
    float prevY = 0.0f;

    bool lastPointValid = false;

    for (int i = 0; i <= n; ++i) {

        float xVal = -range/2 + spacing*i;
        float yVal = m_functionLambda.call({xVal});

        if (std::isnan(yVal) || std::isinf(yVal)) {

            if (lastPointValid) 
                indices.push_back(veil::g_primitiveRestartIndex);
            lastPointValid = false;
            continue;
        }
        bool currentPointValid = (yVal <= yLimit && yVal >= -yLimit);

        if (lastPointValid != currentPointValid && i > 0) {

            float outOfBoundsY = !currentPointValid ? yVal : prevY;
            float targetY = (outOfBoundsY > yLimit) ? yLimit : -yLimit;

            float t = (targetY - prevY) / (yVal - prevY);
            float intersectX = prevX + t * (xVal - prevX);

            float xNorm = (intersectX / range) * 2.0f;
            float yNorm = (targetY / range) * 2.0f;

            vertices.push_back(veil::Vertex{.position={xNorm, yNorm, 0.0f}});
            indices.push_back(vertices.size() - 1);

            if (!currentPointValid) 
                indices.push_back(veil::g_primitiveRestartIndex);
        }

        else if (i > 0 && !lastPointValid && !currentPointValid && (prevY * yVal < 0.0f)) {

            float exitY = (prevY > 0.0f) ? yLimit : -yLimit;
            float t1 = (exitY - prevY) / (yVal - prevY);
            float exitX = prevX + t1 * (xVal - prevX);

            vertices.push_back(veil::Vertex{.position={(exitX / range) * 2.0f, (exitY / range) * 2.0f, 0.0f}});
            indices.push_back(vertices.size() - 1);

            indices.push_back(veil::g_primitiveRestartIndex);

            float enterY = (yVal > 0.0f) ? yLimit : -yLimit;
            float t2 = (enterY - prevY) / (yVal - prevY);
            float enterX = prevX + t2 * (xVal - prevX);

            vertices.push_back(veil::Vertex{.position={(enterX / range) * 2.0f, (enterY / range) * 2.0f, 0.0f}});
            indices.push_back(vertices.size() - 1);

            indices.push_back(veil::g_primitiveRestartIndex);
        }

        if (currentPointValid) {
            
            float xNorm = (xVal / range) * 2.0f;
            float yNorm = (yVal / range) * 2.0f;

            vertices.push_back(veil::Vertex{.position={xNorm, yNorm, 0.0f}});
            indices.push_back(vertices.size() - 1);
        }

        prevX = xVal;
        prevY = yVal;
        lastPointValid = currentPointValid;
    }

    m_mesh->setData(vertices, indices);
}

Axis::Axis(const veil::ShaderProgram& instancedShader, std::string_view attribName) {

    for (int i = 0; i < 2; ++i) {

        veil::Matrix4 model{1.0f};
        model.rotate(90.0f * i, {0.0f, 0.0f, 1.0f});
        model.scale({5.0f, 5.0f, 5.0f});

        m_axisMatrices[i] = model;
    }
    
    m_mesh = std::make_unique<veil::Mesh>(
        std::vector<veil::Vertex>{ 
            veil::Vertex{.position={-1.0f, 0.0f, 0.0f}}, 
            veil::Vertex{.position={1.0f, 0.0f, 0.0f}} 
        },
        std::vector<unsigned int>{0, 1},
        veil::Material{}
    );
    m_drawable = std::make_unique<veil::InstancedMesh>(*m_mesh, 2);
    m_drawable->setInstanceAttribute(instancedShader, attribName);
    m_drawable->setInstances(m_axisMatrices);
    m_drawable->setDrawingMode(GL_LINES);
}

Axis::~Axis() {

    m_drawable.reset();
    m_mesh.reset();
}

Scene::Scene(const std::string& formula, const std::string& variable) {

    initWindow();

    veil::initGL(m_window.get());
    veil::toggleGLFlags(m_window.get(), { GL_DEPTH_TEST, GL_PRIMITIVE_RESTART }, true);

    initDrawables(formula, variable);
    initLoop();
}

Scene::~Scene() {

    veil::Storage<veil::UniformBufferStorage>().shutdown();
    veil::Storage<veil::ShaderStorage>().shutdown();
}

void Scene::initWindow() {

    m_window = std::make_unique<veil::Window>(
        "graphGL", 
        veil::Vector2{700.0f, 700.0f}
    );
    m_window->setInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Scene::initDrawables(const std::string& formula, const std::string& variable) {

    veil::Storage<veil::UniformBufferStorage>().loadUBO<veil::GLCamera::Attitude>(
        0
    );
    veil::Storage<veil::ShaderStorage>().loadShader(
        "basicShader", 
        { { "shader/vertex.vert", GL_VERTEX_SHADER }, { "shader/fragment.frag", GL_FRAGMENT_SHADER } }
    );
    veil::Storage<veil::ShaderStorage>().loadShader(
        "instancedShader", 
        { { "shader/instanced.vert", GL_VERTEX_SHADER }, { "shader/fragment.frag", GL_FRAGMENT_SHADER } }
    );
    const veil::UniformBuffer* attitudeUBO = veil::Storage<veil::UniformBufferStorage>().getUBO(0);
    const veil::ShaderProgram* basicShader = veil::Storage<veil::ShaderStorage>().getShader("basicShader");
    const veil::ShaderProgram* instancedShader = veil::Storage<veil::ShaderStorage>().getShader("instancedShader");

    m_axis = std::make_unique<Axis>(*instancedShader, "aModel");

    m_graph = std::make_unique<Graph>(formula, variable);
    m_graph->buildMesh(20.0f, 2000);
    m_graph->getDrawable().scale({5.0f, 5.0f, 5.0f});
    m_graph->getDrawable().setDrawingMode(GL_LINE_STRIP);

    m_camera = std::make_unique<veil::GLCamera>(
        veil::Vector3{0.0f, 0.0f, 5.5f}, 
        veil::Vector3{0.0f, 1.0f, 0.0f}, 
        m_window->getAspectRatio(), 
        90.0f 
    );

    m_renderer = std::make_unique<veil::Renderer>();
    
    m_renderer->setForTargetCallback(
        [&](const veil::ShaderProgram* shader, const veil::Drawable* drawable) {

            if (drawable->getType() == veil::DrawableType::MESH_SINGULAR) {
                const veil::MeshInstance* mesh = static_cast<const veil::MeshInstance*>(drawable);
                m_renderer->uploadUniformDirect(*shader, "uColor", veil::Vector3{1.0f, 0.0f, 0.0f});
                m_renderer->uploadUniformDirect(*shader, "uModel", mesh->getModelMat());
            }
            if (drawable->getType() == veil::DrawableType::MESH_INSTANCED) {
                const veil::InstancedMesh* mesh = static_cast<const veil::InstancedMesh*>(drawable);
                m_renderer->uploadUniformDirect(*shader, "uColor", veil::Vector3{1.0f, 1.0f, 1.0f});
            }
        }
    );
    m_renderer->reserveShaders(
        { basicShader, instancedShader }
    );
    m_renderer->addTargets({ 
        { *basicShader,     m_graph->getDrawable() },
        { *instancedShader, m_axis->getDrawable() } 
    });
    m_renderer->uploadUniformBuffers( 
        std::make_pair(attitudeUBO, [&]() { return m_camera->getAttitude(); }) 
    );
}

void Scene::initLoop() {

    m_window->setFramebufferCallback(
        [&]() {
            m_camera->updateProjection(90.0f, m_window->getAspectRatio());
            m_camera->resyncMouse();
        }
    );
    m_window->setMouseCallback(
        [&](double xpos, double ypos) {
            m_camera->calculateAttitude(xpos, ypos);
        }
    );
    m_window->setScrollCallback(
        [&](double xoff, double yoff) {

            float range = m_graph->getCurrentRange();
            if (yoff > 0) 
                m_graph->buildMesh((std::clamp(range/1.1f, 0.5f, 50.0f)), 2000);
            if (yoff < 0 && range <= 70.0f) 
                m_graph->buildMesh((std::clamp(range*1.1f, 0.5f, 50.0f)), 2000);
        }   
    );
    m_window->setKeyCallback(
        [&](const veil::KeyEvents& ke) {

            float dt = m_window->getClock().getDeltaTime();
            float speed = 2.0f;

            if(ke.keysDown[GLFW_KEY_W])
                m_camera->move(+m_camera->getFront() * dt * speed);
            if(ke.keysDown[GLFW_KEY_S])
                m_camera->move(-m_camera->getFront() * dt * speed);
            if(ke.keysDown[GLFW_KEY_A])
                m_camera->move(-veil::Vector3::cross(m_camera->getFront(), m_camera->getUp()) * dt * speed);
            if(ke.keysDown[GLFW_KEY_D])
                m_camera->move(+veil::Vector3::cross(m_camera->getFront(), m_camera->getUp()) * dt * speed);
        }
    );
    m_window->setUpdateCallback(
        [&]() {
            m_renderer->callbackUniforms();
            m_renderer->callbackRender();
        }
    );
}

int Scene::startLoop() {

    return m_window->startUpdateLoop();
}