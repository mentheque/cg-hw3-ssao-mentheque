#include "Lights.h"

void DirectionalLight::setColor(QVector3D color) {
	color_ = color;
}

void SpotLight::move(QVector3D translation) {
	position_ += translation;
}