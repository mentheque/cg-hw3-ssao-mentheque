#pragma once

#include <QVector3D>
#include <QOpenGLExtraFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

#include <memory>
#include <cmath>
#include <map>
#include <functional>

#include "Model.h"
#include "FpvCamera.h"

/*  
	Okay, so this is what I need : say I rendered already
	to this framebuffer, and then I want different screen-space effects. 
	For some of them I'll need to ping-pong textures, so 

	Короче, один класс на это все.
	Передаем список слотов в буффере
	Передаем шейдерную программу для fill
	Передаем список из программ, для которых отдельно специфицируется:
		какие текустуры они хотят на вход и в какие слоты 
		какие текстуры они хотят на выход и в какие слоты
	Если программа хочет одну и ту же текстуру на вход и на выход, делаем
	пингпонг.

	Наверное имеет смысл следующее - один большой метод для фреймбуффера который
	принимает следующее - 
	1 - словарь с названиями текстур и их параметрами для создания
	2 - вектор программ, 
		для каждой дополнительно
		1 - вектор названий текстур на вход, считаем что их названия юниформов совпадают
		с названиями текстур которые нам сначала дали, ну или сделать возможно задавать это, в целом насрать
		2 - словарь названий текстур на выход, и в какие локации мы ожидаем что они буду забайдены.


	Отдельно - класс для экранного прямоугольника, чтобы при ресайзе он пересоздавал свои буферы
	чтобы они всегда были static draw, считаем что resize случается крайне редко.

	Однако следующий момент - некоторым из программ хотелось бы задавать свои юниформы помимо
	вышеупомянутых структур. Так что видимо
		3 - лямбду или отдельно специфицированный класс? Начну наверное с лямбды, в моменте
		мне это надо чтобы передавать размеры экрана, как будто бы для этого этого достаточно.
*/

/*
	План на ближайшие 30 минут - написать экранный quad, написать костяк метода который собирал
	бы информацию о программах и кодировал их.
*/

/*
	Out attachments are unclear if GLenum is needed or GLint will be ok 
*/


struct ScreenspaceEffect {
	struct TexUniformPair {
		std::string textureName_;
		GLint textureUniform_;

		TexUniformPair(std::string textureName, GLint textureUniform = -1);
	};

	std::shared_ptr<QOpenGLShaderProgram> shaderProgram_;
	std::vector<TexUniformPair> inTextures_;
	std::vector<TexUniformPair> outTextures_; // uniform = attachemnt, like COLOR_ATTAHMENT0
	std::function<void()> preparation_; // should expect it's shader probramm to be bound when called,
	// should leave it same.

	ScreenspaceEffect(std::shared_ptr<QOpenGLShaderProgram> shaderProgram,
					  std::vector<TexUniformPair> inTextures,
					  std::vector<TexUniformPair> outTextures,
					  std::function<void()> preparation);
	static ScreenspaceEffect last(std::shared_ptr<QOpenGLShaderProgram> shaderProgram,
								  std::vector<TexUniformPair> inTextures,
								   std::function<void()> preparation);
};

struct TextureParams {
	QOpenGLTexture::TextureFormat format_;
	QOpenGLTexture::PixelFormat pixelFormat_;
	QOpenGLTexture::PixelType pixelType_;
	QOpenGLTexture::Filter minFilter_;
	QOpenGLTexture::Filter magFilter_;
	QOpenGLTexture::WrapMode wrapMode_;

	TextureParams(QOpenGLTexture::TextureFormat format,
				  QOpenGLTexture::PixelFormat pixelFormat,
				  QOpenGLTexture::PixelType pixelType,
				  QOpenGLTexture::Filter minFilter = QOpenGLTexture::Linear,
				  QOpenGLTexture::Filter magFilter = QOpenGLTexture::Linear,
				  QOpenGLTexture::WrapMode wrapMode = QOpenGLTexture::Repeat);
};


class ScreenspacePipeline
{
	/*
		Usage:
		Init:
		pipeline.initialise()
		pipeline.resize()

		Render:
		pipeline.bindFbo()
		// rendering first pass
		pipeline.releaseFBO() // optional?
		pipeline.execute()
	*/
	struct ScreenQuad {
		std::unique_ptr<QOpenGLVertexArrayObject> vao_;

		void init();

		void setAttributeBuffers(std::shared_ptr<QOpenGLShaderProgram> shaderProgram);

		void resize(size_t width, size_t height);

		size_t height();
		size_t width();

	private:
		size_t height_ = 0;
		size_t width_ = 0;

		std::unique_ptr<QOpenGLBuffer> vbo_;
		std::unique_ptr<QOpenGLBuffer> ibo_;
		
		std::vector<GLfloat> vertices_ = {
			-1.0f, -1.0f, 0.0, 0.0,// bottom left
			1.0f, -1.0f, 1, 0.0,// bottom right
			1.0f, 1.0f, 1, 1, // top right
			-1.0f, 1.0f, 0.0, 1  // top left
		};
		static constexpr std::array<GLuint, 6u> indices_ = {
			0, 1, 2,// BL -> BR -> TR (counter-clockwise)
			0, 2, 3 // BL -> TR -> TL (counter-clockwise)
		};
	} screenQuad_;

	struct FramebufferHelper {
		GLuint fbo_;

		FramebufferHelper();
		void init();
		void bind();
		void release();
	} fbo_;

	// Initialise actual textures only on resize?

	std::vector<TextureParams> textureDefs_;
	std::vector<std::unique_ptr<QOpenGLTexture>> textures_;
	std::vector<std::unique_ptr<QOpenGLTexture>> texturesSpare_;
	std::vector<bool> needSpare_;
	std::vector<GLint> pipelineInOuts_;
	std::vector<std::shared_ptr<QOpenGLShaderProgram>> shaderPrograms_;
	std::vector<std::function<void()>> preparations_;
	std::vector<bool> needSwap_;
	std::vector<GLint> initialOuts_;

	std::vector<std::vector<GLenum>> drawBuffers_;
public:
	bool initPipeline(
		std::map<std::string, TextureParams> textureDefsMap,
		std::vector<ScreenspaceEffect::TexUniformPair> initialOuts,
		std::vector<ScreenspaceEffect> effects
	);

	static std::unique_ptr<QOpenGLTexture> genTexture(TextureParams & params,
		const size_t width, const size_t height);

	void resize(size_t width, size_t height);

	void execute();
	
	void initBuffers();
	void bindFbo();
	void releaseFbo();

private:
	size_t sizeMultiplier_ = 1;

	void superViewport(QOpenGLFunctions * glFunc);
	void unsuperViewport(QOpenGLFunctions * glFunc);

public:
	void supersample(size_t mult);

	QVector2D textureSize();
};