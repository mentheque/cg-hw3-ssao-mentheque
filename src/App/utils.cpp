#include "utils.h"

#include <QDebug>
#include <numbers>

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

float cosFromDeg(float deg) {
	return std::cosf(deg * float(std::numbers::pi) / 180.0f);
}