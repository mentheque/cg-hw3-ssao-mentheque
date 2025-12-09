#pragma once
#include <QDateTime>

class KeyPressContainer
{
private:
	qint64 timeCache_ = 0;
	bool active_ = false;
	Qt::Key key_;

	bool pressed_ = false;
public:
	KeyPressContainer(Qt::Key key);

	void pressed(qint64 updateTime);
	void released(qint64 updateTime);
	bool isActive();
	bool isHeld();
	qint64 touch(qint64 updateTime);
};
