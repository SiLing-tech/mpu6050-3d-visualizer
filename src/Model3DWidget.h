#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QMatrix4x4>
#include <QVector3D>
#include "ObjLoader.h"

class Model3DWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

    QOpenGLShaderProgram *shader_ = nullptr;
    QOpenGLBuffer vbo_{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer ebo_{QOpenGLBuffer::IndexBuffer};

    QVector<float> vertices_;
    QVector<unsigned int> indices_;
    int indexCount_ = 0;

    QMatrix4x4 proj_, view_, model_;
    float roll_ = 0, pitch_ = 0, yaw_ = 0;

    QVector3D lightPos_{5, 5, 5};
    QVector3D lightColor_{1, 1, 1};

public:
    explicit Model3DWidget(QWidget *parent = nullptr);
    ~Model3DWidget() override;

    void setRotation(float roll, float pitch, float yaw);
    bool loadObjModel(const QString &path);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    void buildCubeGeometry();
    void uploadMesh(const MeshData &mesh);
};
