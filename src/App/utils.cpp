#include "utils.h"

#include <QDebug>
#include <numbers>
#include <cmath>

void saveTextureToFile(QOpenGLTexture * texture, const QString & filename)
{
	qDebug() << "=== SAVE TEXTURE DEBUG ===";

	if (!texture)
	{
		qDebug() << "Texture is null!";
		return;
	}
	if (!texture->isCreated())
	{
		qDebug() << "Texture is not created!";
		return;
	}

	qDebug() << "Texture ID:" << texture->textureId();
	qDebug() << "Texture size:" << texture->width() << "x" << texture->height();
	qDebug() << "Texture format:" << texture->format();

	if (!texture || !texture->isCreated())
		return;

	int width = texture->width();
	int height = texture->height();

	// Create temporary FBO to read from texture
	QOpenGLFramebufferObject fbo(width, height);
	fbo.bind();

	auto gl = QOpenGLContext::currentContext()->functions();
	gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
							   GL_TEXTURE_2D, texture->textureId(), 0);

	// Read pixels from FBO
	QImage image(width, height, QImage::Format_RGBA8888);
	gl->glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, image.bits());

	// Flip vertically (OpenGL has origin at bottom-left)
	image = image.mirrored(false, true);

	fbo.release();

	// Save to file
	image.save(filename);
	qDebug() << "Saved texture to:" << filename;
}

QLayout * addAll(QLayout * addTo)
{
	return addTo;
}

float mapSlider(int value, int steps, int newRangeZeroAt, float newRangeLen)
{
	return (float(value - newRangeZeroAt) / steps) * newRangeLen;
}

QSlider * createSlider(QWidget * parent, int maximum, int newRangeZeroAt,
	float newRangeLen, GLfloat * mapTo, int defaultValue)
{
	QSlider * slider = new QSlider(parent);
	slider->setRange(0, maximum);
	slider->setOrientation(Qt::Horizontal);
	slider->setTickPosition(QSlider::TicksAbove);
	slider->setTracking(true);
	QObject::connect(slider, &QSlider::valueChanged, [=](int value) {
		(*mapTo) = mapSlider(value, maximum, newRangeZeroAt, newRangeLen);
	});
	slider->setValue(defaultValue);
	// somehow here update() alone doesn't do the trick. 
	*mapTo = mapSlider(defaultValue, maximum, newRangeZeroAt, newRangeLen);
	// It does though when I do this whole thing without helper function. Weird.
	slider->update();
	return slider;
}

QSlider* createSlider(QWidget* parent, int maximum, int newRangeZeroAt,
	float newRangeLen, GLfloat* mapTo, int defaultValue, bool* updated) {
	QSlider * slider = createSlider(parent, maximum, newRangeZeroAt,
		newRangeLen, mapTo, defaultValue);
	QObject::connect(slider, &QSlider::valueChanged, [=](int value) {
		(*updated) = true;
	});
	return slider;
}


QSlider* createSlider(QWidget* parent, int maximum,
	int newRangeZeroAt, float newRangeLen, GLfloat* mapTo) {
	return createSlider(parent, maximum, newRangeZeroAt, newRangeLen,
		mapTo, newRangeZeroAt);
}

QString v3ToS(QString prefix, QVector3D vec)
{
	QString out;
	QDebug(&out) << vec;
	return prefix + out;
}

QVector3D colorToV3(QColor color)
{
	QVector3D out;
	out[0] = color.redF();
	out[1] = color.greenF();
	out[2] = color.blueF();
	return out;
}

float radFromDeg(float deg) {
	return deg * float(std::numbers::pi) / 180.0f;
}

float cosFromDeg(float deg) {
	return std::cos(radFromDeg(deg));
}

bool isColorAttachment(GLint att)
{
	return (att != GL_DEPTH_ATTACHMENT) && (att != GL_STENCIL_ATTACHMENT)
		&& (att != GL_DEPTH_STENCIL_ATTACHMENT) && (att != GL_STENCIL_ATTACHMENT_EXT)
		&& (att != GL_DEPTH_ATTACHMENT_EXT);
}