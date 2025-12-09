#include "KeyPressContainer.h"


KeyPressContainer::KeyPressContainer(Qt::Key key)
	: key_(key)
{
}

qint64 KeyPressContainer::touch(qint64 updateTime)
{
	if (pressed_)
	{
		qint64 diff = updateTime - timeCache_;
		timeCache_ = updateTime;
		return diff;
	}
	else if(active_){
		active_ = false;
		return timeCache_;
	}
	return 0;
}

void KeyPressContainer::pressed(qint64 updateTime)
{
	timeCache_ = updateTime;
	active_ = pressed_ = true;
}

void KeyPressContainer::released(qint64 updateTime) {
	timeCache_ = updateTime - timeCache_;
	pressed_ = false;
}

bool KeyPressContainer::isActive() {
	return active_;
}

bool KeyPressContainer::isHeld()
{
	return pressed_;
}