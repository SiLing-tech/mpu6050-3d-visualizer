#include "ObjLoader.h"
#include <QFile>
#include <QTextStream>
#include <QVector3D>

MeshData ObjLoader::loadObj(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    QVector<QVector3D> positions;
    QVector<QVector3D> normals;
    QVector<FaceVertex> faces;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        const QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (parts.isEmpty())
            continue;

        if (parts[0] == "v") {
            positions.append(QVector3D(parts[1].toFloat(), parts[2].toFloat(), parts[3].toFloat()));
        } else if (parts[0] == "vn") {
            normals.append(QVector3D(parts[1].toFloat(), parts[2].toFloat(), parts[3].toFloat()));
        } else if (parts[0] == "f") {
            for (int i = 1; i < parts.size(); ++i) {
                const QStringList indices = parts[i].split('/');
                FaceVertex fv;
                fv.vi = indices[0].toInt() - 1;
                if (indices.size() > 2 && !indices[2].isEmpty())
                    fv.ni = indices[2].toInt() - 1;
                faces.append(fv);
            }
        }
    }

    file.close();

    MeshData mesh;
    for (const auto &fv : faces) {
        QVector3D p = positions.value(fv.vi);
        mesh.vertices.append({p.x(), p.y(), p.z()});

        QVector3D n = normals.value(fv.ni, QVector3D(0, 0, 1));
        mesh.normals.append({n.x(), n.y(), n.z()});

        mesh.indices.append(static_cast<unsigned int>(mesh.indices.size()));
    }

    return mesh;
}
