#include "ScreenspacePipeline.h"

#include <QDebug>

#include "utils.h"

void ScreenspacePipeline::ScreenQuad::init()
{
	vao_ = std::make_unique<QOpenGLVertexArrayObject>();
	vao_->create();
	vao_->bind();

	vbo_ = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::Type::VertexBuffer);
	vbo_->create();
	vbo_->bind();
	vbo_->setUsagePattern(QOpenGLBuffer::StaticDraw);
	vbo_->allocate(vertices_.data(), static_cast<int>(vertices_.size() * sizeof(GLfloat)));

	ibo_ = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::Type::IndexBuffer);
	ibo_->create();
	ibo_->bind();
	ibo_->setUsagePattern(QOpenGLBuffer::StaticDraw);
	ibo_->allocate(indices_.data(), static_cast<int>(indices_.size() * sizeof(GLuint)));

	vao_->release();
	ibo_->release();
	vbo_->release();
}

void ScreenspacePipeline::ScreenQuad::setAttributeBuffers(
	std::shared_ptr<QOpenGLShaderProgram> shaderProgram)
{
	vao_->bind();
	vbo_->bind();
	ibo_->bind();
	shaderProgram->bind();
	// screenspace coordinates
	shaderProgram->enableAttributeArray(0);
	shaderProgram->setAttributeBuffer(0, GL_FLOAT, 0, 2, 4 * sizeof(GLfloat));

	// texture coordinates
	shaderProgram->enableAttributeArray(1);
	shaderProgram->setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(GLfloat), 2, 4 * sizeof(GLfloat));
	shaderProgram->release();
	vao_->release();
	vbo_->release();
	ibo_->release();
}

void ScreenspacePipeline::ScreenQuad::resize(size_t width, size_t height)
{
	height_ = height;
	width_ = width;
	// For possible implementation of textures of specific size
	// rather than adjusting to screen size. Then will need to add
	// update logic for textCoords.
	return;
}

size_t ScreenspacePipeline::ScreenQuad::height() {
	return height_;
}
size_t ScreenspacePipeline::ScreenQuad::width() {
	return width_;
}

TextureParams::TextureParams(QOpenGLTexture::TextureFormat format,
			  QOpenGLTexture::PixelFormat pixelFormat,
			  QOpenGLTexture::PixelType pixelType,
			  QOpenGLTexture::Filter minFilter,
			  QOpenGLTexture::Filter magFilter,
			  QOpenGLTexture::WrapMode wrapMode)
	: format_(format)
	, pixelFormat_(pixelFormat)
	, pixelType_(pixelType)
	, minFilter_(minFilter)
	, magFilter_(magFilter)
	, wrapMode_(wrapMode)
{
}

ScreenspacePipeline::FramebufferHelper::FramebufferHelper() {}

void ScreenspacePipeline::FramebufferHelper::init(){
	auto glExtra = QOpenGLContext::currentContext()->extraFunctions();
	glExtra->glGenFramebuffers(1, &fbo_);
}
void ScreenspacePipeline::FramebufferHelper::bind()
{
	auto glExtra = QOpenGLContext::currentContext()->extraFunctions();
	glExtra->glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
}
void ScreenspacePipeline::FramebufferHelper::release() {
	auto glExtra = QOpenGLContext::currentContext()->extraFunctions();
	glExtra->glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

ScreenspaceEffect::TexUniformPair::TexUniformPair(std::string textureName, GLint textureUniform)
	: textureName_(textureName)
	, textureUniform_(textureUniform)
{}

ScreenspaceEffect::ScreenspaceEffect(std::shared_ptr<QOpenGLShaderProgram> shaderProgram,
	std::vector<TexUniformPair> inTextures,
	std::vector<TexUniformPair> outTextures,
	std::function<void()> preparation)
	: shaderProgram_(shaderProgram)
	, preparation_(preparation)
	, outTextures_(outTextures)
	, inTextures_(inTextures){
	shaderProgram_->bind();
	for (auto& texturePair : inTextures_) {
		if (texturePair.textureUniform_ == -1) {
			texturePair.textureUniform_ = 
				shaderProgram_->uniformLocation(QString::fromStdString(texturePair.textureName_));
		}
	}
	shaderProgram_->release();
}

ScreenspaceEffect ScreenspaceEffect::last(std::shared_ptr<QOpenGLShaderProgram> shaderProgram,
										   std::vector<TexUniformPair> inTextures,
										   std::function<void()> preparation)
{
	return ScreenspaceEffect{shaderProgram, inTextures, {}, preparation};
}

bool ScreenspacePipeline::initPipeline(
	std::map<std::string, TextureParams> textureDefsMap,
	std::vector<ScreenspaceEffect::TexUniformPair> initialOuts,
	std::vector<ScreenspaceEffect> effects)
{
	if (!effects.back().outTextures_.empty())
	{
		qDebug() << "Last effect shouldn't have render targets";
		return false;
	}

	std::map<std::string, size_t> textureIdxs; // texture name to idx
	textureDefs_.clear(); // texture defs available by idx
	for (auto & entry: textureDefsMap)
	{
		textureIdxs.insert({entry.first, textureDefs_.size()});
		textureDefs_.push_back(entry.second);
	}
	const size_t texSize = textureDefs_.size();
	textures_.resize(texSize);
	texturesSpare_.resize(texSize);
	needSpare_.resize(texSize, false);
	needSwap_.resize(texSize * effects.size(), false);

	// Framebuffer setup for first outside fill
	initialOuts_.resize(texSize, -1);
	drawBuffers_.resize(effects.size());
	for (auto & texturePair: initialOuts)
	{
		initialOuts_[textureIdxs.at(texturePair.textureName_)] =
			texturePair.textureUniform_;
		if (isColorAttachment(texturePair.textureUniform_)) {
			drawBuffers_[0].push_back(texturePair.textureUniform_);
		}
	}

	pipelineInOuts_.resize(2 * texSize * effects.size(), -1); // last doesn't need outs, but keeping for now

	for (size_t i = 0;i < effects.size();i++) {
		ScreenspaceEffect & effect = effects[i];
		const size_t inoutsPrefix = texSize * 2 * i;

		shaderPrograms_.push_back(effect.shaderProgram_);
		preparations_.push_back(effect.preparation_);
		for (auto& texturePair : effect.inTextures_) {
			pipelineInOuts_[inoutsPrefix + textureIdxs.at(texturePair.textureName_)] =
				texturePair.textureUniform_;
		}
		for (auto & texturePair: effect.outTextures_)
		{
			pipelineInOuts_[inoutsPrefix + texSize + textureIdxs.at(texturePair.textureName_)] =
				texturePair.textureUniform_;
			if (isColorAttachment(texturePair.textureUniform_))
			{
				// should work fine as last shouldn't have attachemnts
				drawBuffers_[i + 1].push_back(texturePair.textureUniform_);
			}
		}

		for (size_t t = 0; t < texSize; t++) {
			if (pipelineInOuts_[inoutsPrefix + t] != -1
				&& pipelineInOuts_[inoutsPrefix + texSize + t] != -1)
			{
				needSwap_[texSize * i + t] = true;
				needSpare_[t] = true;
			}
		}

		screenQuad_.setAttributeBuffers(shaderPrograms_.back());
	}



	return true;
}

std::unique_ptr<QOpenGLTexture> ScreenspacePipeline::genTexture(TextureParams& params,
	const size_t width, const size_t height) {
	auto texture = std::make_unique<QOpenGLTexture>(QOpenGLTexture::Target2D);

	texture->setSize(width, height);
	texture->setFormat(params.format_);
	texture->setMinificationFilter(params.minFilter_);
	texture->setMagnificationFilter(params.magFilter_);
	texture->setWrapMode(params.wrapMode_);

	texture->allocateStorage(params.pixelFormat_, params.pixelType_);
	texture->generateMipMaps();

	return texture;
}

void ScreenspacePipeline::resize(size_t width, size_t height) {
	width *= sizeMultiplier_;
	height *= sizeMultiplier_;
	screenQuad_.resize(width, height);
	for (size_t i = 0; i < textures_.size(); i++) {
		textures_[i] = genTexture(textureDefs_[i], width, height);
		if (needSpare_[i]) {
			texturesSpare_[i] = genTexture(textureDefs_[i], width, height);
		}
	}
}

void ScreenspacePipeline::execute()
{
	auto glExtra = QOpenGLContext::currentContext()->extraFunctions();
	auto glFunc = QOpenGLContext::currentContext()->functions();
	fbo_.bind();

	superViewport(glFunc);
	glFunc->glDisable(GL_DEPTH_TEST);

	for (size_t e = 0; e < shaderPrograms_.size(); e++) {
		if (e == shaderPrograms_.size() - 1)
		{
			fbo_.release();
			unsuperViewport(glFunc);
		}
		else {
			// TODO: write initial outs to the place of the last's outAttachments, and 
			// bind them here with bindFbo_(e + 1) or something.
			glExtra->glDrawBuffers(drawBuffers_[e + 1].size(), drawBuffers_[e + 1].data());
		}

		auto & shaderProgram = shaderPrograms_[e];
		shaderProgram->bind(); // binding shader
		preparations_[e]();	// doing effect-specific uniforms

		const size_t texSize = textures_.size();
		GLint * inUniforms = pipelineInOuts_.data() + 2 * texSize * e;
		GLint * outAttachments = inUniforms + texSize;

		// binding in textures:
		for (size_t t = 0; t < texSize; t++)
		{
			if (inUniforms[t] != -1)
			{
				shaderProgram->setUniformValue(inUniforms[t], (GLint)t);
				textures_[t]->bind(t);
			}
		}

		// binding out attachments:
		for (size_t t = 0; t < texSize; t++)
		{
			if (outAttachments[t] != -1) {
				GLuint texture = textures_[t]->textureId();
				if (needSwap_[texSize * e + t]) {
					texture = texturesSpare_[t]->textureId();
				}
				//	TODO: do not reattach if no swap and last was the same
				glExtra->glFramebufferTexture2D(GL_FRAMEBUFFER,
												outAttachments[t], GL_TEXTURE_2D, texture, 0);  
			}
		}

		screenQuad_.vao_->bind();

		glFunc->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

		screenQuad_.vao_->release();

		// releasing textures, swapping if needed
		for (size_t t = 0; t < texSize; t++)
		{
			if (inUniforms[t] != -1)
			{
				textures_[t]->release();

				if (needSwap_[texSize * e + t])
				{
					std::swap(textures_[t], texturesSpare_[t]);
				}
			}
		}
		shaderProgram->release();
	}
	glFunc->glEnable(GL_DEPTH_TEST);
}

void ScreenspacePipeline::bindFbo() {	
	auto glExtra = QOpenGLContext::currentContext()->extraFunctions();
	auto glFunc = QOpenGLContext::currentContext()->functions();

	fbo_.bind();
	const size_t texSize = textures_.size();
	// binding out attachments:
	// TODO: fix copypaste from execute
	for (size_t t = 0; t < texSize; t++)
	{
		if (initialOuts_[t] != -1)
		{
			GLuint texture = textures_[t]->textureId();
			//	TODO: do not reattach if no swap and last was the same
			// Just keep last attachemnt for every exture, even -1
			glExtra->glFramebufferTexture2D(GL_FRAMEBUFFER,
											initialOuts_[t], GL_TEXTURE_2D, texture, 0);
		}
	}
	glExtra->glDrawBuffers(drawBuffers_[0].size(), drawBuffers_[0].data());

	GLenum status = glExtra->glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		qDebug() << "FBO not complete in bindFbo()! Status:" << status;
	}

	superViewport(glFunc);
}
void ScreenspacePipeline::releaseFbo()
{
	auto glFunc = QOpenGLContext::currentContext()->functions();
	fbo_.release();

	unsuperViewport(glFunc);
}

void ScreenspacePipeline::initBuffers()
{
	screenQuad_.init();
	fbo_.init();
}

void ScreenspacePipeline::supersample(size_t mult) {
	if (sizeMultiplier_ != mult) {
		size_t oldMult = sizeMultiplier_;
		sizeMultiplier_ = mult;
		resize(screenQuad_.width() / oldMult, screenQuad_.height() / oldMult);
	}
}

void ScreenspacePipeline::superViewport(QOpenGLFunctions * glFunc)
{
	if (sizeMultiplier_ != 1)
	{
		glFunc->glViewport(0, 0, screenQuad_.width(), screenQuad_.height());
	}
}
void ScreenspacePipeline::unsuperViewport(QOpenGLFunctions * glFunc)
{
	if (sizeMultiplier_ != 1)
	{
		glFunc->glViewport(0, 0, screenQuad_.width() / sizeMultiplier_, screenQuad_.height() / sizeMultiplier_);
	}
}