#include "FpvCamera.h"
#include <algorithm>

FpvCamera::FpvCamera() {
	perspective_.perspective(60.0, 1.0, 0.1, 100);
}

void FpvCamera::setPerspective(float fov, float aspect, float near, float far){
	perspective_.setToIdentity();
	perspective_.perspective(fov, aspect, near, far);
}

QVector3D FpvCamera::directionVector()
{
	if (updateDirection_)
	{
		directionVector_ = {cos(yaw_) * cos(pitch_), sin(pitch_), sin(yaw_) * cos(pitch_)};
		directionVector_.normalize();
		updateDirection_ = false;
	}
	return directionVector_;
}

QVector3D FpvCamera::rightVector() {
	if (updateRight_)
	{
		rightVector_ = QVector3D::crossProduct(directionVector(), up_).normalized();
		updateRight_ = false;
	}
	return rightVector_;
}

void FpvCamera::move(float right, float forward)
{
	cameraPos_ += right * rightVector() + forward * directionVector();
	updateView_ = true;
}

void FpvCamera::rotate(float pitch, float yaw) {
	pitch_ = std::clamp(pitch_ + pitch, -1.55f, 1.55f);
	yaw_ = fmod(yaw_ - yaw, 6.28319f);
	updateView_ = true;
	updateDirection_ = updateRight_ = true;
}

QMatrix4x4 FpvCamera::getPerspective() {
	return perspective_;
}


QMatrix4x4 FpvCamera::getView()
{
	if (updateView_)
	{
		view_.setToIdentity();
		view_.lookAt(cameraPos_, cameraPos_ + directionVector(), up_);
		updateView_ = false;
	}
	return view_;
}