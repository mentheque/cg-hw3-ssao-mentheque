#pragma once

#include <QLayout>
#include <QOpenGLFunctions>
#include <QSlider>
#include <QVector3D>
#include <QString>
#include <QColor>

#include "ColorButton.h"

QLayout * addAll(QLayout * addTo);

// Adds all widgest to the given layout.
// Not sure how precise the template usage is, but it works.
template<typename... Rest>
QLayout * addAll(QLayout * addTo, QWidget * first, Rest... remainingWidgets)
{
	addTo->addWidget(first);
	return addAll(addTo, remainingWidgets...);
}

template <typename... Rest>
QLayout* addAllH(Rest... widgets)
{
	return addAll(new QHBoxLayout(), widgets...);
}

template <typename... Rest>
QLayout * addAllV(Rest... widgets)
{
	return addAll(new QVBoxLayout(), widgets...);
}

// Maps slider's value
float mapSlider(int value, int steps, int newRangeZeroAt, float newRangeLen);

// Creates new slider, and maps it's value on update into mapTo using mapSlider
QSlider * createSlider(QWidget * parent, int maximum, int newRangeZeroAt,
	float newRangeLen, GLfloat * mapTo, int defaultFalue);

// Creates new slider, and maps it's value on update into mapTo using mapSlider
// Default value is same as newRangeZeroAt
QSlider * createSlider(QWidget * parent, int maximum, int newRangeZeroAt,
	float newRangeLen, GLfloat * mapTo);

QString v3ToS(QString prefix, QVector3D vec);

QVector3D colorToV3(QColor color);

float cosFromDeg(float deg);