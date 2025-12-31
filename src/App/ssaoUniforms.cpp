#include "ssaoUniforms.h"

std::vector<QVector3D> genKernel(size_t size)
{
	std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
	std::default_random_engine generator;
	std::vector<QVector3D> out;

	for (size_t i = 0; i < size; i++)
	{
		QVector3D sample = {randomFloats(generator) * 2 - 1,
							randomFloats(generator) * 2 - 1,
							randomFloats(generator)};
		sample.normalize();
		sample *= std::lerp(0.1f, 1.0f, ((float)i * i) / ((float)size * size));
		out.push_back(sample);
	}
	return out;
}

std::unique_ptr<QOpenGLTexture> genNoiseTexture()
{
	std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
	std::default_random_engine generator;
	std::vector<QVector3D> noise;
	for (size_t i = 0; i < 16; i++)
	{
		QVector3D sample = {randomFloats(generator) * 2 - 1,
							randomFloats(generator) * 2 - 1,
							0.0f};
		noise.push_back(sample);
	}

	auto texture = std::make_unique<QOpenGLTexture>(QOpenGLTexture::Target2D);

	texture->setSize(4, 4);
	texture->setFormat(QOpenGLTexture::RGB16F);
	texture->setMinificationFilter(QOpenGLTexture::Nearest);
	texture->setMagnificationFilter(QOpenGLTexture::Nearest);
	texture->setWrapMode(QOpenGLTexture::Repeat);

	texture->allocateStorage(QOpenGLTexture::RGB, QOpenGLTexture::Float16);
	texture->setData(QOpenGLTexture::RGB, QOpenGLTexture::Float32,
					 reinterpret_cast<const float *>(noise.data()));
	return texture;
}