#include "FpvCamera.h"
#include <algorithm>

FpvCamera::FpvCamera() {
	projection_.perspective(60.0, 1.0, 0.1, 100);
}

void FpvCamera::setPerspective(float fov, float aspect, float near, float far){
	projection_.setToIdentity();
	projection_.perspective(fov, aspect, near, far);
	updatePv_ = true;
}

const QVector3D & FpvCamera::directionVector()
{
	if (updateDirection_)
	{
		directionVector_ = {cos(yaw_) * cos(pitch_), sin(pitch_), sin(yaw_) * cos(pitch_)};
		directionVector_.normalize();
		updateDirection_ = false;
	}
	return directionVector_;
}

const QVector3D & FpvCamera::rightVector() {
	if (updateRight_)
	{
		rightVector_ = QVector3D::crossProduct(directionVector(), up_).normalized();
		updateRight_ = false;
	}
	return rightVector_;
}

void FpvCamera::move(float right, float forward)
{
	if (right != 0 || forward != 0)
	{
		cameraPos_ += right * rightVector() + forward * directionVector();
		updateView_ = updatePv_ = true;
	}
}

void FpvCamera::rotate(float pitch, float yaw)
{
	pitch_ = std::clamp(pitch_ + pitch, -1.55f, 1.55f);
	yaw_ = fmod(yaw_ - yaw, 6.28319f);
	updateView_ = updatePv_ = true;
	updateDirection_ = updateRight_ = true;
}

const QMatrix4x4 & FpvCamera::getProjection()
{
	return projection_;
}


const QMatrix4x4 & FpvCamera::getView()
{
	if (updateView_)
	{
		view_.setToIdentity();
		view_.lookAt(cameraPos_, cameraPos_ + directionVector(), up_);
		updateView_ = false;
	}
	return view_;
}

const QMatrix4x4& FpvCamera::getProjectionView() {
	if (updatePv_)
	{
		projectionView_ = getProjection() * getView();
		updatePv_ = false;
	}
	return projectionView_;
}