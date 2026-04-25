#pragma once

#include <QVector>
#include <QVector3D>

struct MeshData {
    QVector<float> vertices;    // x,y,z per vertex
    QVector<float> normals;     // nx,ny,nz per vertex
    QVector<unsigned int> indices;
};

class ObjLoader {
public:
    static MeshData loadObj(const QString &filePath);

private:
    struct FaceVertex {
        int vi = -1;
        int ni = -1;
        int ti = -1;
    };
};
