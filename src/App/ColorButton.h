#pragma once

#include <QPushButton>
#include <QColor>

// Just some UI stuff, copypasted from StackOverflow 
class ColorButton : public QPushButton
{
	Q_OBJECT
public:
	ColorButton(QWidget * parent);

private:
	QColor color;
public slots:
	void changeColor();

public:
	void setColor(const QColor& newColor);
	QColor getColor() const;
};