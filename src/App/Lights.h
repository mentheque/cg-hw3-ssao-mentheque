#pragma once

#include <QVector3D>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLExtraFunctions>
#include <QOpenGLBuffer>
#include <QOpenGlShaderProgram>

#include <memory>

struct DirectionalLight
{
	alignas(16) QVector3D direction_; // 12
	float _padding0 = 0; // 16
	alignas(16) QVector3D color_; // 28
	alignas(4) float ambientStrength; // 32
	float diffuseStrength; // 36
	float specularStrength; // 40
	float _padding1 = 0; // 44
	float _padding2 = 0;// 48

public:
	void setColor(QVector3D color);
};

struct SpotLight : public DirectionalLight {
	// 48 for super's data
	
	alignas(16) QVector3D position_; // 60
	float innerCutoff_; // 64
	float outerCutoff_; // 68
	float _padding1 = 0; // 72
	float _padding2 = 0; // 76
	float _padding3 = 0; // 80

public:
	void move(QVector3D translation);
};

template<size_t directionalSize, size_t spotSize>
struct LightUniformData {
	DirectionalLight directionals[directionalSize];
	SpotLight spotlights[spotSize];

	size_t size() const{
		return sizeof(DirectionalLight) * directionalSize + sizeof(SpotLight) * spotSize;
	}
};

template<size_t directionalSize, size_t spotSize>
class LightUBOManager {
	LightUniformData<directionalSize, spotSize> data_;
	QOpenGLBuffer ubo_;
	GLint bindingPoint_;

	int err_;
public:
	LightUBOManager(GLint bindingPoint)
		: bindingPoint_(bindingPoint)
	{}

	LightUBOManager()
		: LightUBOManager(0)
	{}

	DirectionalLight& directional(size_t i) {
		return data_.directionals[i];
	}

	SpotLight& spot(size_t i) {
		return data_.spotlights[i];
	}

	const DirectionalLight & directional(size_t i) const
	{
		return data_.directionals[i];
	}

	const SpotLight & spot(size_t i) const
	{
		return data_.spotlights[i];
	}

	size_t size() const{
		return data_.size();
	}

private:
	void write() {
		write(0, data_.size());
	}

	void write(size_t offset, size_t len) {
		ubo_.write(offset, reinterpret_cast<const unsigned char *>(&data_) + offset, len);
	}

public:
	void initialise() {
		auto glExtra = QOpenGLContext::currentContext()->extraFunctions();
		ubo_.create();
		ubo_.bind();
		ubo_.setUsagePattern(QOpenGLBuffer::DynamicDraw);
		ubo_.allocate(data_.size());
		write();
		glExtra->glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint_, ubo_.bufferId());
		ubo_.release();
	}

	void update() {
		ubo_.bind();
		write();
		ubo_.release();
	}

	void updateDirectional(size_t i) {
		if (i > directionalSize) {
			err_ = -1; // breakless error handling. Could have done silent, but 
			return; // this is probably cleaner.
		}

		data_.directionals[i].direction_.normalize();

		ubo_.bind();
		ubo_.write(sizeof(DirectionalLight) * i, data_.directionals + i,
				   sizeof(DirectionalLight)); // forgoing write() to not use casts
		ubo_.release();
	}

	void updateSpot(size_t i)
	{
		if (i > spotSize) {
			err_ = -2;
			return;
		}

		data_.spotlights[i].direction_.normalize();

		ubo_.bind();
		ubo_.write(sizeof(DirectionalLight) * directionalSize + sizeof(SpotLight) * i,
				   data_.spotlights + i, sizeof(SpotLight)); // forgoing write() to not use casts
		ubo_.release();
	}

	void bindToShader(std::shared_ptr<QOpenGLShaderProgram> program, const GLchar * blockName)
	{
		auto glExtra = QOpenGLContext::currentContext()->extraFunctions();
		GLuint blockIndex = glExtra->glGetUniformBlockIndex(program->programId(), blockName);

		program->bind();
		glExtra->glUniformBlockBinding(program->programId(), blockIndex, bindingPoint_);
		program->release();
	}
};