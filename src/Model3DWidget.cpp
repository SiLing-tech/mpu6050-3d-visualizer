#include "Model3DWidget.h"
#include <QOpenGLVersionFunctionsFactory>
#include <array>

static const char *vertexShaderSrc = R"(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;

uniform mat4 uMVP;
uniform mat4 uModel;

out vec3 vNormal;
out vec3 vColor;
out vec3 vWorldPos;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    vColor = aColor;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

static const char *fragmentShaderSrc = R"(#version 330 core
in vec3 vNormal;
in vec3 vColor;
in vec3 vWorldPos;

uniform vec3 uLightPos;
uniform vec3 uLightColor;

out vec4 FragColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(uLightPos - vWorldPos);
    float diff = max(dot(normal, lightDir), 0.0);
    float ambient = 0.25;
    vec3 lit = (ambient + diff * uLightColor) * vColor;
    FragColor = vec4(lit, 1.0);
}
)";

Model3DWidget::Model3DWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
}

Model3DWidget::~Model3DWidget()
{
    makeCurrent();
    delete shader_;
    vbo_.destroy();
    ebo_.destroy();
    doneCurrent();
}

void Model3DWidget::setRotation(float roll, float pitch, float yaw)
{
    roll_ = roll;
    pitch_ = pitch;
    yaw_ = yaw;
    update();
}

bool Model3DWidget::loadObjModel(const QString &path)
{
    MeshData mesh = ObjLoader::loadObj(path);
    if (mesh.vertices.isEmpty())
        return false;

    vertices_.clear();
    indices_.clear();
    vertices_.reserve(mesh.vertices.size() * 6 / 3); // pos(3) + normal(3) + dummy color(3)? need 9

    for (int i = 0; i < mesh.vertices.size() / 3; ++i) {
        vertices_.append(mesh.vertices[i * 3 + 0]);
        vertices_.append(mesh.vertices[i * 3 + 1]);
        vertices_.append(mesh.vertices[i * 3 + 2]);

        if (i * 3 + 2 < mesh.normals.size()) {
            vertices_.append(mesh.normals[i * 3 + 0]);
            vertices_.append(mesh.normals[i * 3 + 1]);
            vertices_.append(mesh.normals[i * 3 + 2]);
        } else {
            vertices_.append(0.0f);
            vertices_.append(0.0f);
            vertices_.append(1.0f);
        }

        vertices_.append(0.7f);
        vertices_.append(0.7f);
        vertices_.append(0.7f);
    }

    indices_ = mesh.indices;
    indexCount_ = static_cast<int>(indices_.size());

    makeCurrent();
    vbo_.bind();
    vbo_.allocate(vertices_.constData(), vertices_.size() * static_cast<int>(sizeof(float)));
    ebo_.bind();
    ebo_.allocate(indices_.constData(), indexCount_ * static_cast<int>(sizeof(unsigned int)));
    doneCurrent();

    update();
    return true;
}

void Model3DWidget::initializeGL()
{
    initializeOpenGLFunctions();

    shader_ = new QOpenGLShaderProgram(this);
    shader_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSrc);
    shader_->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSrc);
    shader_->link();

    vbo_.create();
    ebo_.create();

    buildCubeGeometry();

    vbo_.bind();
    vbo_.allocate(vertices_.constData(), vertices_.size() * static_cast<int>(sizeof(float)));

    ebo_.bind();
    ebo_.allocate(indices_.constData(), indexCount_ * static_cast<int>(sizeof(unsigned int)));

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
}

void Model3DWidget::buildCubeGeometry()
{
    struct FaceDef {
        float nx, ny, nz;
        float r, g, b;
    };

    const std::array<FaceDef, 6> faces = {{
        { 0, 0,  1,  1.0f, 0.3f, 0.3f}, // Front: +Z red
        { 0, 0, -1,  0.3f, 0.3f, 1.0f}, // Back: -Z blue
        { 1, 0,  0,  0.3f, 1.0f, 0.3f}, // Right: +X green
        {-1, 0,  0,  1.0f, 1.0f, 0.3f}, // Left: -X yellow
        { 0, 1,  0,  0.9f, 0.9f, 0.9f}, // Top: +Y white
        { 0, -1, 0,  1.0f, 0.3f, 1.0f}, // Bottom: -Y magenta
    }};

    // Generate per-face quad corners in CCW order for correct normals
    auto addFace = [&](const FaceDef &f) {
        // Find two tangent vectors orthogonal to normal
        QVector3D n(f.nx, f.ny, f.nz);
        QVector3D u, v;
        if (fabs(f.ny) < 0.99) {
            u = QVector3D::crossProduct(n, QVector3D(0, 1, 0)).normalized();
        } else {
            u = QVector3D::crossProduct(n, QVector3D(1, 0, 0)).normalized();
        }
        v = QVector3D::crossProduct(n, u);

        auto addVertex = [&](float su, float sv) {
            QVector3D pos = n + u * su + v * sv;
            vertices_.append(pos.x());
            vertices_.append(pos.y());
            vertices_.append(pos.z());
            vertices_.append(f.nx);
            vertices_.append(f.ny);
            vertices_.append(f.nz);
            vertices_.append(f.r);
            vertices_.append(f.g);
            vertices_.append(f.b);
        };

        addVertex(-1,  1);
        addVertex(-1, -1);
        addVertex( 1, -1);
        addVertex( 1,  1);

        int base = static_cast<int>(indices_.size()) / 6 * 4;
        indices_.append(base + 0);
        indices_.append(base + 1);
        indices_.append(base + 2);
        indices_.append(base + 0);
        indices_.append(base + 2);
        indices_.append(base + 3);
    };

    for (const auto &face : faces)
        addFace(face);

    indexCount_ = static_cast<int>(indices_.size());
}

void Model3DWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    float aspect = float(w) / float(h > 0 ? h : 1);
    proj_.setToIdentity();
    proj_.perspective(45.0f, aspect, 0.1f, 100.0f);

    view_.setToIdentity();
    view_.lookAt(QVector3D(0, 0, 6), QVector3D(0, 0, 0), QVector3D(0, 1, 0));
}

void Model3DWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!shader_->isLinked())
        return;

    model_.setToIdentity();
    model_.rotate(roll_,   1, 0, 0);
    model_.rotate(pitch_,  0, 1, 0);
    model_.rotate(yaw_,    0, 0, 1);

    QMatrix4x4 mvp = proj_ * view_ * model_;

    shader_->bind();
    shader_->setUniformValue("uMVP", mvp);
    shader_->setUniformValue("uModel", model_);
    shader_->setUniformValue("uLightPos", lightPos_);
    shader_->setUniformValue("uLightColor", lightColor_);

    vbo_.bind();
    ebo_.bind();

    int stride = 9 * static_cast<int>(sizeof(float));
    shader_->enableAttributeArray(0);
    shader_->setAttributeBuffer(0, GL_FLOAT, 0, 3, stride);
    shader_->enableAttributeArray(1);
    shader_->setAttributeBuffer(1, GL_FLOAT, 3 * static_cast<int>(sizeof(float)), 3, stride);
    shader_->enableAttributeArray(2);
    shader_->setAttributeBuffer(2, GL_FLOAT, 6 * static_cast<int>(sizeof(float)), 3, stride);

    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);

    shader_->disableAttributeArray(0);
    shader_->disableAttributeArray(1);
    shader_->disableAttributeArray(2);
    shader_->release();
}
