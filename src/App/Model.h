#pragma once

#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <memory>
#include <vector>
#include <tiny_gltf.h>

#include "FpvCamera.h"

#include <QDebug>
 


struct Vertex {
	float position[3];
	float normal[3];
	float texCoord[2];
	float tangent[4];
};

struct Primitive {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	int textureIndex = -1;
	int normalIndex = -1;

	struct Material {
		QVector4D baseColor;
		bool hasTexture = false;
		bool hasNormalMap = false;
	} material_;
};

struct Instance {
	QMatrix4x4 transform_;
	bool active_ = true;
};

struct Mesh {
	std::vector<Primitive> primitives_;
	std::vector<size_t> vaos_;
};

class Model
{
public:
	Model();
	void setShaderProgram(std::shared_ptr<QOpenGLShaderProgram> program);

	void initaliseTexture(const size_t textureIdx,
						  QOpenGLTexture::TextureFormat format, const tinygltf::Model & model);
	bool loadFromGLTF(const QString & filePath);
private:
	QMatrix4x4 transform_;

public:
	QMatrix4x4 getTransform();
	void setTransform(QMatrix4x4 transform);

public:
	void render(FpvCamera & camera, std::vector<Instance*> instances);

private:
	void renderNode(FpvCamera & camera, tinygltf::Node node, QMatrix4x4 transform);

private:
	std::vector<tinygltf::Node> nodes_;
	std::vector<tinygltf::Scene> scenes_;

	std::shared_ptr<QOpenGLShaderProgram> shaderProgram_;
	std::vector<std::unique_ptr<QOpenGLTexture>> textures_;
	std::vector<Mesh> meshes_;

	std::vector<std::unique_ptr<QOpenGLBuffer>> vbos_;
	std::vector<std::unique_ptr<QOpenGLBuffer>> ibos_;
	std::vector<std::unique_ptr<QOpenGLVertexArrayObject>> vaos_;

	void setupMeshBuffers();

private:
	GLint mvpUniform_ = -1;
	GLint normalMatrixUniform_ = -1;
	GLint modelUniform_ = -1;
	GLint cameraPosUniform_ = -1;
	GLint material_baseColorUniform_ = -1;
	GLint material_hasTextureUniform_ = -1;
	GLint material_hasNormalUniform_ = -1;

private:
	template<typename T>
	class BufferReader
	{
		const unsigned char * data_;
		const int byteStride_;
	public:
		BufferReader(
			const tinygltf::Buffer & buffer,
			const tinygltf::BufferView & bufferView,
			const tinygltf::Accessor & accessor)
			: BufferReader(buffer.data.data() + bufferView.byteOffset + accessor.byteOffset,
				accessor.ByteStride(bufferView)) 
		{
		}

		BufferReader(const unsigned char * data, int byteStride)
			:
			data_(data), byteStride_(byteStride){
		}

		const T get(size_t i, size_t j) {
			return *reinterpret_cast<const T *>(data_ + i * byteStride_ + j * sizeof(T));
		}
	};
};