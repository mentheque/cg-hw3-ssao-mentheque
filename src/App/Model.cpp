#include "Model.h"
#include <QFile>
#include <QOpenGLFunctions>
#include <QDebug>
#include <QColorSpace>
#include <filesystem>

/*
	Окей, то есть меш у нас все-таки примитив, то есть мы вызываем glDrawTriangles для каждого
	треугольника?..
*/

Model::Model() {}

void Model::setShaderProgram(std::shared_ptr<QOpenGLShaderProgram> program) {
	shaderProgram_ = program;

	if (program)
	{
		program->bind();
		mvpUniform_ = program->uniformLocation("mvp");
		normalMatrixUniform_ = program->uniformLocation("normalMatrix");
		modelUniform_ = program->uniformLocation("model");
		cameraPosUniform_ = program->uniformLocation("camera_pos_world");
		material_baseColorUniform_ = program->uniformLocation("material.base_color");
		material_hasTextureUniform_ = program->uniformLocation("material.has_texture");
		material_hasNormalUniform_ = program->uniformLocation("material.has_normalMap");

		program->release();
	}
}

bool Model::loadFromGLTF(const QString& filePath) {
	qDebug() << "started loading\n";

	QFile modelFile(filePath);
	if (!modelFile.open(QIODevice::ReadOnly))
	{
		return false;
	}

	QByteArray modelData = modelFile.readAll();
	modelFile.close();

	if (modelData.isEmpty())
	{
		return false;
	}

	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err, warn;

	qDebug() << "loading file\n";
	bool success = loader.LoadBinaryFromMemory(&model, &err, &warn,
											  reinterpret_cast<const unsigned char *>(modelData.data()),
											  static_cast<unsigned int>(modelData.size()));
	if (!success)
	{
		return false;
	}

	qDebug() << "loaded file\n";

	for (const auto & texture: model.textures)
	{
		if (texture.source >= 0 && texture.source < static_cast<int>(model.images.size()))
		{
			const auto & image = model.images[texture.source];

			QImage qimg;
			if (image.component == 3)
			{
				qDebug() << "3-texture";
				qimg = QImage(image.image.data(), image.width, image.height, QImage::Format_RGB888);
			}
			else if (image.component == 4)
			{
				qDebug() << "4-texture";
				qimg = QImage(image.image.data(), image.width, image.height, QImage::Format_RGBA8888);
			}
			auto tex = std::make_unique<QOpenGLTexture>(qimg);
			tex->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear, QOpenGLTexture::Linear);
			tex->setWrapMode(QOpenGLTexture::Repeat);
			tex->generateMipMaps();

			textures_.push_back(std::move(tex));
		}
	}

	qDebug() << "loaded textures\n";


	for (const auto & mesh: model.meshes)
	{
		Mesh toPushBack;
		for (const auto & primitive: mesh.primitives)
		{
			qDebug() << "\nloading a mesh";
			
			Primitive meshData;

			if (primitive.attributes.find("POSITION") != primitive.attributes.end())
			{
				const auto & accessor = model.accessors[primitive.attributes.at("POSITION")];
				if (accessor.bufferView == -1 && !accessor.sparse.isSparse) {
					qDebug() << "Probably expects some extention, skip.";
					continue;
				}

				const auto & bufferView = model.bufferViews[accessor.bufferView];
				const auto & buffer = model.buffers[bufferView.buffer];

				BufferReader<float> positions = {buffer, bufferView, accessor};

				meshData.vertices.resize(accessor.count);
				for (size_t i = 0; i < accessor.count; ++i)
				{
					meshData.vertices[i].position[0] = positions.get(i, 0);
					meshData.vertices[i].position[1] = positions.get(i, 1);
					meshData.vertices[i].position[2] = positions.get(i, 2);
				}
			}

			if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
			{
				qDebug() << "loading normal attributes\n";


				const auto & accessor = model.accessors[primitive.attributes.at("NORMAL")];
				const auto & bufferView = model.bufferViews[accessor.bufferView];
				const auto & buffer = model.buffers[bufferView.buffer];

				BufferReader<float> normals = {buffer, bufferView, accessor};

				for (size_t i = 0; i < accessor.count && i < meshData.vertices.size(); ++i)
				{
					meshData.vertices[i].normal[0] = normals.get(i, 0);
					meshData.vertices[i].normal[1] = normals.get(i, 1);
					meshData.vertices[i].normal[2] = normals.get(i, 2);
				}
			}

			if (primitive.attributes.find("TANGENT") != primitive.attributes.end())
			{
				qDebug() << "loading tangent attributes\n";

				const auto & accessor = model.accessors[primitive.attributes.at("TANGENT")];
				const auto & bufferView = model.bufferViews[accessor.bufferView];
				const auto & buffer = model.buffers[bufferView.buffer];

				BufferReader<float> tangent = {buffer, bufferView, accessor};

				for (size_t i = 0; i < accessor.count && i < meshData.vertices.size(); ++i)
				{
					meshData.vertices[i].tangent[0] = tangent.get(i, 0);
					meshData.vertices[i].tangent[1] = tangent.get(i, 1);
					meshData.vertices[i].tangent[2] = tangent.get(i, 2);
					meshData.vertices[i].tangent[3] = tangent.get(i, 3);
				}
			}

			if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
			{
				qDebug() << "loading textcoord attributes\n";


				const auto & accessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
				const auto & bufferView = model.bufferViews[accessor.bufferView];
				const auto & buffer = model.buffers[bufferView.buffer];

				BufferReader<float> texCoords = {buffer, bufferView, accessor};

				for (size_t i = 0; i < accessor.count && i < meshData.vertices.size(); ++i)
				{
					meshData.vertices[i].texCoord[0] = texCoords.get(i, 0);
					meshData.vertices[i].texCoord[1] = texCoords.get(i, 1);
				}
			}

			if (primitive.indices >= 0)
			{
				const auto & accessor = model.accessors[primitive.indices];
				const auto & bufferView = model.bufferViews[accessor.bufferView];
				const auto & buffer = model.buffers[bufferView.buffer];

				meshData.indices.resize(accessor.count);

				if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
				{
					BufferReader<uint16_t> indices = {buffer, bufferView, accessor};

					for (size_t i = 0; i < accessor.count; ++i)
					{
						meshData.indices[i] = indices.get(i, 0);
					}
				}
				else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
				{
					BufferReader<uint32_t> indices = {buffer, bufferView, accessor};

					for (size_t i = 0; i < accessor.count; ++i)
					{
						meshData.indices[i] = indices.get(i, 0);
					}
				}
			}

			if (primitive.material >= 0 && primitive.material < static_cast<int>(model.materials.size()))
			{
				const auto & material = model.materials[primitive.material];
				if (material.pbrMetallicRoughness.baseColorTexture.index >= 0)
				{
					meshData.material_.hasTexture = true;
					meshData.textureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
				}
				if (material.normalTexture.index >= 0)
				{
					meshData.material_.hasNormalMap = true;
					meshData.normalIndex = material.normalTexture.index;
				}

				meshData.material_.baseColor = QVector4D(
					static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[0]),
					static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[1]),
					static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[2]),
					static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[3]));
			}

			toPushBack.primitives_.push_back(std::move(meshData));
		}
		meshes_.push_back(toPushBack);
	}
	nodes_ = model.nodes;
	scenes_ = model.scenes;

	qDebug() << "setting up buffers\n";

	setupMeshBuffers();
	qDebug() << "finished loading\n";
	return true;
}

void Model::setupMeshBuffers()
{
	if (!shaderProgram_)
		return;

	for (auto & mesh: meshes_)
	{
		for (const auto & primitive: mesh.primitives_)
		{
			auto vao = std::make_unique<QOpenGLVertexArrayObject>();
			vao->create();
			vao->bind();

			auto vbo = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::Type::VertexBuffer);
			vbo->create();
			vbo->bind();
			vbo->setUsagePattern(QOpenGLBuffer::StaticDraw);
			vbo->allocate(primitive.vertices.data(), static_cast<int>(primitive.vertices.size() * sizeof(Vertex)));

			auto ibo = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::Type::IndexBuffer);
			ibo->create();
			ibo->bind();
			ibo->setUsagePattern(QOpenGLBuffer::StaticDraw);
			ibo->allocate(primitive.indices.data(), static_cast<int>(primitive.indices.size() * sizeof(uint32_t)));

			shaderProgram_->bind();

			shaderProgram_->enableAttributeArray(0);
			shaderProgram_->setAttributeBuffer(0, GL_FLOAT, offsetof(Vertex, position), 3, sizeof(Vertex));

			shaderProgram_->enableAttributeArray(1);
			shaderProgram_->setAttributeBuffer(1, GL_FLOAT, offsetof(Vertex, normal), 3, sizeof(Vertex));

			shaderProgram_->enableAttributeArray(2);
			shaderProgram_->setAttributeBuffer(2, GL_FLOAT, offsetof(Vertex, texCoord), 2, sizeof(Vertex));

			shaderProgram_->enableAttributeArray(3);
			shaderProgram_->setAttributeBuffer(3, GL_FLOAT, offsetof(Vertex, tangent), 4, sizeof(Vertex));

			shaderProgram_->release();
			vao->release();

			mesh.vaos_.push_back(vaos_.size());
			vaos_.push_back(std::move(vao));
			vbos_.push_back(std::move(vbo));
			ibos_.push_back(std::move(ibo));
		}
	}
}


QMatrix4x4 Model::getTransform() {
	return transform_;
}
void Model::setTransform(QMatrix4x4 transform) {
	transform_ = transform;
}

void Model::render(FpvCamera & camera, std::vector<Instance *> instances)
{
	if (!shaderProgram_ || scenes_.empty())
		return;

	shaderProgram_->bind();

	if (cameraPosUniform_ >= 0)
		shaderProgram_->setUniformValue(cameraPosUniform_, camera.getView().inverted().column(3).toVector3D());

	for (Instance* instance: instances)
	{
		if (!instance->active_)
			continue;
		// Not sure this is how it is supposed to work, but rendering all for now.
		for (auto scene : scenes_) {
			for (int nodeIdx : scene.nodes) {
				renderNode(camera, nodes_[nodeIdx], instance->transform_);
			}
		}
	}

	shaderProgram_->release();
}

// Assuming shaderProgram and camera related stuff is already set up? 
void Model::renderNode(FpvCamera& camera, tinygltf::Node node, QMatrix4x4 transform) {

	// TODO: pass pv matrix initially and see how performance improoves compared to 
	// calculating it here and passing there each mesh.

	if (!node.matrix.empty()) {
		std::vector<float> floatMatrix = {node.matrix.begin(), node.matrix.end()};
		transform = transform * QMatrix4x4(floatMatrix.data()).transposed();
	}
	else
	{
		if (!node.scale.empty())
		{
			transform.scale(node.scale[0], node.scale[1], node.scale[2]);
		}
		if (!node.rotation.empty())
		{
			transform.rotate(QQuaternion(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]));
		}
		if (!node.translation.empty())
		{
			transform.translate(node.translation[0], node.translation[1], node.translation[2]);
		}
	}


	if (node.mesh != -1) {
		const auto mvp = camera.getProjectionView() * transform;
		const auto normalMatrix = transform.normalMatrix();

		if (mvpUniform_ >= 0)
			shaderProgram_->setUniformValue(mvpUniform_, mvp);
		if (normalMatrixUniform_ >= 0)
			shaderProgram_->setUniformValue(normalMatrixUniform_, normalMatrix);
		if (modelUniform_ >= 0)
			shaderProgram_->setUniformValue(modelUniform_, transform);

		const auto & mesh = meshes_[node.mesh];
		for (size_t i = 0;i < mesh.primitives_.size();i++)
		{
			if (i < mesh.vaos_.size() && mesh.vaos_[i])
			{
				vaos_[mesh.vaos_[i]]->bind();
				const auto & primitive = mesh.primitives_[i];

				if (primitive.material_.hasTexture)
				{
					textures_[primitive.textureIndex]->bind(0);
				}
				if (primitive.material_.hasNormalMap)
				{
					textures_[primitive.normalIndex]->bind(1);
				}

				if (material_baseColorUniform_ >= 0)
					shaderProgram_->setUniformValue(
						material_baseColorUniform_, primitive.material_.baseColor);
				if (material_hasTextureUniform_ >= 0)
					shaderProgram_->setUniformValue(
						material_hasTextureUniform_, primitive.material_.hasTexture);
				if (material_hasNormalUniform_ >= 0)
					shaderProgram_->setUniformValue(
						material_hasNormalUniform_, primitive.material_.hasNormalMap);

				QOpenGLContext::currentContext()
					->functions()
					->glDrawElements(
						GL_TRIANGLES,
						static_cast<GLsizei>(primitive.indices.size()),
						GL_UNSIGNED_INT,
						nullptr);

				if (primitive.material_.hasTexture)
				{
					textures_[primitive.textureIndex]->release();
				}
				if (primitive.material_.hasNormalMap)
				{
					textures_[primitive.normalIndex]->release();
				}

				vaos_[mesh.vaos_[i]]->release();
			}
		}
	}

	for (int childIdx : node.children) {
		renderNode(camera, nodes_[childIdx], transform);
	}
}
