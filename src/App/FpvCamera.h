#pragma once
#include <QMatrix4x4>

class FpvCamera
{
public:
	FpvCamera();

	QMatrix4x4 getView();
	QMatrix4x4 getPerspective();

	void setPerspective(float fov, float aspect, float near, float far);

	void move(float right, float forward);
	void rotate(float pitch, float yaw);

private:
	QVector3D directionVector();
	QVector3D rightVector();

	QVector3D directionVector_;
	QVector3D rightVector_;
	bool updateDirection_ = true;
	bool updateRight_ = true;

private:
	QVector3D cameraPos_ = {0.0, 0.0, -3.0};
	float pitch_ = 0;
	float yaw_ = -1.5708;

	QMatrix4x4 perspective_;
	QMatrix4x4 view_;
	bool updateView_ = true;
	
	const QVector3D up_ = {0.0, 1.0, 0.0};
};