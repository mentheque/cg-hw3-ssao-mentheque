#include "FpvCamera.h"
#include <algorithm>
#include <cmath>

#include "utils.h"

FpvCamera::FpvCamera() {
	projection_.perspective(60.0, 1.0, 0.1, 100);
}

void FpvCamera::setPerspective(float fov, float aspect, float near_, float far_){
	projection_.setToIdentity();
	projection_.perspective(fov, aspect, near_, far_);
	updatePv_ = true;

	gAspect_ = aspect;
	gTanHalfFOV_ = std::tan(radFromDeg(fov / 2));
}

float FpvCamera::getAspect() {
	return gAspect_;
}
float FpvCamera::getTanHalfFov()
{
	return gTanHalfFOV_;
}

float FpvCamera::getNear()
{
	return 0.1f;
}

float FpvCamera::getFar()
{
	return 100.0f;
}

const QVector3D & FpvCamera::getDirection()
{
	if (updateDirection_)
	{
		directionVector_ = {std::cos(yaw_) * std::cos(pitch_), std::sin(pitch_),
			std::sin(yaw_) * std::cos(pitch_)};
		directionVector_.normalize();
		updateDirection_ = false;
	}
	return directionVector_;
}

const QVector3D & FpvCamera::rightVector() {
	if (updateRight_)
	{
		rightVector_ = QVector3D::crossProduct(getDirection(), up_).normalized();
		updateRight_ = false;
	}
	return rightVector_;
}

void FpvCamera::move(float right, float forward)
{
	if (right != 0 || forward != 0)
	{
		cameraPos_ += right * rightVector() + forward * getDirection();
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
		view_.lookAt(cameraPos_, cameraPos_ + getDirection(), up_);
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

const QVector3D & FpvCamera::getPosition() {
	return cameraPos_;
}

void FpvCamera::setTransforms(const QVector3D & pos, float pitch, float yaw){
	cameraPos_ = pos;
	pitch_ = pitch;
	yaw_ = yaw;

	updateDirection_ = true;
	updateRight_ = true;
	updatePv_ = true;
	updateView_ = true;
}