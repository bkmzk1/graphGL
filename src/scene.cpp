
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

void Graph::setFormula(const std::string& formula) {

    m_functionExpr = SymEngine::parse(formula);
    m_functionLambda.init({m_variable}, m_functionExpr);
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

        veil::Matrix4 axisModel{1.0f};
        axisModel.rotate(90.0f * i, {0.0f, 0.0f, 1.0f});
        axisModel.scale({5.0f, 5.0f, 5.0f});

        float moveX[] = {1.0f, 0.0f};
        float moveY[] = {0.0f, 1.0f};

        veil::Matrix4 rangeTextModel{1.0f};
        rangeTextModel.translate({5.05f*moveX[i], 5.05f*moveY[i], 0.0f});
        rangeTextModel.scale({0.003f, 0.003f, 0.003f});

        m_axisMatrices[i] = axisModel;
        m_rangeTextMatrices[i] = rangeTextModel;
    }

    m_font = std::make_unique<veil::Font>(
        "/usr/share/fonts/google-noto/NotoSans-Regular.ttf", 75
    );
    m_rangeText = std::make_unique<veil::Text>(
        *m_font
    );
    m_rangeText->setText("0.0");
    
    m_axisMesh = std::make_unique<veil::Mesh>(
        std::vector<veil::Vertex>{ 
            veil::Vertex{.position={-1.0f, 0.0f, 0.0f}}, 
            veil::Vertex{.position={1.0f, 0.0f, 0.0f}} 
        },
        std::vector<unsigned int>{0, 1},
        veil::Material{}
    );

    m_axisDrawable = std::make_unique<veil::InstancedMesh>(*m_axisMesh, 2);
    m_axisDrawable->setInstanceAttribute(instancedShader, attribName);
    m_axisDrawable->setInstances(m_axisMatrices);
    m_axisDrawable->setDrawingMode(GL_LINES);

    m_rangeTextDrawable = std::make_unique<veil::InstancedText>(*m_rangeText, 2);
    m_rangeTextDrawable->setInstanceAttribute(instancedShader, attribName);
    m_rangeTextDrawable->setInstances(m_rangeTextMatrices);
    m_rangeTextDrawable->setDrawingMode(GL_TRIANGLES);
}

Axis::~Axis() {

    m_font.reset();

    m_axisDrawable.reset();
    m_axisMesh.reset();

    m_rangeTextDrawable.reset();
    m_rangeText.reset();
}

void Axis::updateRange(float range) {

    m_rangeText->setText(std::format("{:.2f}", range));
}

AnalyticsDisplayer::AnalyticsDisplayer(const MathFunction& function, const MathFunction& derivative) {

    std::string roots = "Roots: ";
    const auto& rootArr = function.getRoots();
    if (rootArr.empty())
        roots += "None";
    else
        for (const auto& root : rootArr) 
            roots += std::format("{:.2f}, ", root);

    std::string sign = "Sign intervals: ";
    const auto& signArr = function.getSignIntervals();
    if (signArr.empty())
        sign += "None";
    else
        for (const auto& interval : signArr)
            sign += std::format("{}({:.2f}; {:.2f}),  ", (interval.sign > 0 ? '+' : '-'), interval.start, interval.end);

    std::string growth = "Growth intervals: ";
    const auto& growthArr = derivative.getSignIntervals();
    if (growthArr.empty())
        growth += "None";
    else
        for (const auto& interval : growthArr)
            growth += std::format("{}({:.2f}; {:.2f}),  ", (interval.sign > 0 ? '+' : '-'), interval.start, interval.end);

    m_font = std::make_unique<veil::Font>(
        "/usr/share/fonts/google-noto/NotoSans-Regular.ttf", 75
    );

    m_texts[0] = std::make_unique<veil::Text>(*m_font);
    m_texts[0]->setText(roots);

    m_texts[1] = std::make_unique<veil::Text>(*m_font);
    m_texts[1]->setText(sign);

    m_texts[2] = std::make_unique<veil::Text>(*m_font);
    m_texts[2]->setText(growth);

    m_textDrawables[0] = std::make_unique<veil::TextInstance>(*m_texts[0]);
    m_textDrawables[1] = std::make_unique<veil::TextInstance>(*m_texts[1]);
    m_textDrawables[2] = std::make_unique<veil::TextInstance>(*m_texts[2]);

    for (int i = 0; i < 3; ++i) {
        m_textDrawables[i]->translate({5.6f, 4.5f - 2.25*i, 0.0f});
        m_textDrawables[i]->rotate(90.0f, {0.0f, 1.0f, 0.0f});
        m_textDrawables[i]->scale({0.003f, 0.003f, 0.003f});
        m_textDrawables[i]->setDrawingMode(GL_TRIANGLES);
    }
}
AnalyticsDisplayer::~AnalyticsDisplayer() {

    m_font.reset();

    for (int i = 0; i < 3; ++i) {

        m_textDrawables[i].reset();
        m_texts[i].reset();
    }
}