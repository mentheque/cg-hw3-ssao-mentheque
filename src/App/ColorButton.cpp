#include "ColorButton.h"

#include <QColorDialog>

ColorButton::ColorButton(QWidget * parent)
	: QPushButton(parent)
{
	connect(this, &QPushButton::clicked, this, &ColorButton::changeColor);
	setColor(Qt::black);
}

void ColorButton::changeColor() {
	QColor newColor = QColorDialog::getColor(color, parentWidget());
	if (newColor.isValid() && newColor != color) {
		setColor(newColor);
	}
}

void ColorButton::setColor(const QColor& newColor) {
	color = newColor;
	setStyleSheet("background-color: " + color.name());
}

QColor ColorButton::getColor() const{
	return color;
}
