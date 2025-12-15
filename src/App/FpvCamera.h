#pragma once
#include <QMatrix4x4>

class FpvCamera
{
public:
	FpvCamera();

	const QMatrix4x4 & getView();
	const QMatrix4x4 & getProjection();
	const QMatrix4x4 & getProjectionView();

	void setPerspective(float fov, float aspect, float near, float far);

	void move(float right, float forward);
	void rotate(float pitch, float yaw);

	const QVector3D & getPosition(); 
	const QVector3D & getDirection();

	void setTransforms(const QVector3D & pos, float pitch, float yaw);

private:
	const QVector3D & rightVector();

	QVector3D directionVector_;
	QVector3D rightVector_;
	bool updateDirection_ = true;
	bool updateRight_ = true;

private:
	QVector3D cameraPos_ = {0.0, 0.0, -3.0};
	float pitch_ = 0;
	float yaw_ = -1.5708;

	QMatrix4x4 projection_;
	QMatrix4x4 view_;
	bool updateView_ = true;

	QMatrix4x4 projectionView_;
	bool updatePv_ = true;
	
	const QVector3D up_ = {0.0, 1.0, 0.0};
};