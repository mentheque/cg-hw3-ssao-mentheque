#pragma once

#include <QVector3D>
#include <QOpenGLExtraFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

#include <memory>
#include <cmath>

#include "Model.h"
#include "FpvCamera.h"

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

enum class LightType
{
	Directional,
	Spot
};

template<size_t directionalSize, size_t spotSize>
class LightUBOManager {
	LightUniformData<directionalSize, spotSize> data_;
	QOpenGLBuffer ubo_;
	GLint bindingPoint_;

	int err_ = 0;
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

template <size_t directionalSize, size_t spotSize>
class lightModelManager
{
	LightUBOManager<directionalSize, spotSize> * const lights_;
	const bool spot_;
	const size_t idx_;

	Model * model_;

	Instance instance_;
	float directionalOffset_ = 0.0;

	GLint lightIdxUniform_ = -1;
	GLint lightTypeUniform_ = -1;
	GLint radiusUniform_ = -1;

	float scale_ = 1.0;

	QVector3D cachedColor_;

	inline static const QVector3D _deactivatedColor_ = {0.0, 0.0, 0.0};
public: 
	lightModelManager(LightUBOManager<directionalSize, spotSize> * lights,
					  LightType lightType,
					  size_t idx,
					  Model * model,
					  const GLchar * blockName,
					  float directionalOffset = 0)
		: lights_(lights)
		, spot_(lightType == LightType::Spot)
		, idx_(idx)
		, model_(model)
		, directionalOffset_(directionalOffset)
	{
		ensureUBOBinding(blockName);
		cachedColor_ = _deactivatedColor_;
		update();
	}

	void ensureUBOBinding(const GLchar * blockName)
	{
		auto program = model_->getShaderProgram();
		lights_->bindToShader(program, blockName);

		lightIdxUniform_ = program->uniformLocation("shineThrough.idx");
		lightTypeUniform_ = program->uniformLocation("shineThrough.spot");
		radiusUniform_ = program->uniformLocation("radius");
	}

	void changeModel(Model * model, const GLchar * blockName)
	{
		model_ = model;
		ensureUBOBinding(blockName);
	}

	void activateLight() {
		if (!instance_.active_)
		{
			if (spot_) {
				lights_->spot(idx_).color_ = cachedColor_;
			}
			else {
				lights_->directional(idx_).color_ = cachedColor_;
			}
			cachedColor_ = _deactivatedColor_;
			instance_.active_ = true;
		}
	}

	void deactivateLight() {
		if (instance_.active_)
		{
			if (spot_)
			{
				cachedColor_ = lights_->spot(idx_).color_;
			}
			else
			{
				cachedColor_ = lights_->directional(idx_).color_;
			}
			instance_.active_ = false;
		}
	}

	float getDirectionalOffset()
	{
		return directionalOffset_;
	}

	void scale(float scale) {
		scale_ = scale;
	}

	void setDirectionalOffset(float directionalOffset)
	{
		directionalOffset_ = directionalOffset;
	}

	void update() {
		QVector3D direction, position;

		if (spot_) {
			direction = lights_->spot(idx_).direction_;
			position = lights_->spot(idx_).position_;
		}
		else {
			direction = lights_->directional(idx_).direction_;
			position = {0, 0, 0};
		}
		direction.normalize();
		position += direction * directionalOffset_;

		QQuaternion rotation = QQuaternion::rotationTo({0, -1, 0}, direction);

		instance_.transform_.setToIdentity();
		instance_.transform_.translate(position);
		instance_.transform_.rotate(rotation);
		instance_.transform_.scale(scale_);		
	}

	void render(FpvCamera & camera)
	{
		if (lightIdxUniform_ >= 0 && lightTypeUniform_ >= 0) {
			auto program = model_->getShaderProgram();
			program->bind();
			program->setUniformValue(lightIdxUniform_, (GLint)idx_);
			program->setUniformValue(lightTypeUniform_, spot_);

			if (radiusUniform_ >= 0 && spot_) {
				float cos = lights_->spot(idx_).outerCutoff_; // outer for now 
				float sin = std::sqrt(1 - cos * cos);
				float radius = 2 * (sin / cos); // base height of cone is 2
				program->setUniformValue(radiusUniform_, radius);
			}

			program->release(); // excessive, but less room for mistakes I guess
		}
		model_->render(camera, {&instance_});
	}

};