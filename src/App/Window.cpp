#include "Window.h"

#include <QMouseEvent>
#include <QLabel>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QVBoxLayout>
#include <QScreen>
#include <QPushButton>

#include <array>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

#include "projectionPointSearch.h"
#include "utils.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <tinygltf/tiny_gltf.h>

namespace
{
}// namespace

Window::Window() noexcept
{
	// Just some QT stuff, didn't really bother with anything pretty here

	QWidget * settingsWidget = new QWidget(this);
	settingsWidget->setStyleSheet("background-color: lightblue;");

	/*
	auto dialLabel = new QLabel(settingsWidget);
	dialLabel->setText(tr("a: "));
	auto animationDial = new QDial(settingsWidget);
	animationDial->setRange(0, 999);
	animationDial->setTracking(true);
	animationDial->setWrapping(true);
	animationDial->setInvertedAppearance(true);
	connect(animationDial, &QDial::valueChanged, this, [=, this](int value) {
		float mapped = mapSlider(value, 999, 250, 2 * std::numbers::pi);
		//this->constant_ = QPointF{cos(mapped), sin(mapped)} * 0.7885f;
	});
	animationDial->setValue(250);
	animationDial->update();
	*/

	auto posLabel = new QLabel(settingsWidget);
	posLabel->setText(v3ToS("pos: ", cachedPos_));
	auto dirLabel = new QLabel(settingsWidget);
	dirLabel->setText(v3ToS("dir: ", cachedDir_));
	auto capturePosButton = new QPushButton(settingsWidget);
	capturePosButton->setText("Capture");
	connect(capturePosButton, &QPushButton::clicked, this, [=, this](int value) {
		this->cachedPos_ = this->camera_.getPosition();
		this->cachedDir_ = this->camera_.getDirection();
		posLabel->setText(v3ToS("pos: ", cachedPos_));
		dirLabel->setText(v3ToS("dir: ", cachedDir_));
	});

	auto spotLabel = new QLabel(settingsWidget);
	spotLabel->setText("spotLight: ");
	QSlider * spotAngle = createSlider(settingsWidget,
											  1000, 0, 70.0, &this->cachedAngle_, 100);
	spotColor_ = new ColorButton(settingsWidget);
	spotColor_->setColor(Qt::white);
	auto setSpotButton = new QPushButton(settingsWidget);
	setSpotButton->setText("Set pos + dir from cached");
	connect(setSpotButton, &QPushButton::clicked, this, [=, this](int value) {
		this->lightUBO_.spot(__USER_SPOT__).position_ = this->cachedPos_;
		this->lightUBO_.spot(__USER_SPOT__).direction_ = this->cachedDir_;
		this->lightUBO_.updateSpot(__USER_SPOT__);
	});

	auto spotCoeffs = new QLabel(settingsWidget);
	spotCoeffs->setText("Spot ambient, diffuse, spec.:");
	QSlider * spotAmb = createSlider(settingsWidget,
									   1000, 0, 1.0, &this->spotAmbient_, 0);
	spotAmb->setTickInterval(1000);
	QSlider * spotDiff = createSlider(settingsWidget,
									 1000, 0, 2.0, &this->spotDiffuse_, 800);
	spotDiff->setTickInterval(500);

	QSlider * spotSpec = createSlider(settingsWidget,
									 1000, 0, 2.0, &this->spotSpecular_, 500);
	spotSpec->setTickInterval(500);

	auto dirLightLabel = new QLabel(settingsWidget);
	dirLightLabel->setText("directional light: ");
	dirColor_ = new ColorButton(settingsWidget);
	dirColor_->setColor(Qt::yellow);
	auto setDirButton = new QPushButton(settingsWidget);
	setDirButton->setText("Set dir from cached");
	connect(setDirButton, &QPushButton::clicked, this, [=, this](int value) {
		this->lightUBO_.directional(__USER_DIR__).direction_ = this->cachedDir_;
		this->lightUBO_.updateDirectional(__USER_DIR__);
	});

	auto dirCoeffs = new QLabel(settingsWidget);
	dirCoeffs->setText("Dir ambient, diffuse, spec.:");
	QSlider * dirAmb = createSlider(settingsWidget,
									 1000, 0, 1.0, &this->dirAmbient_, 0);
	dirAmb->setTickInterval(1000);
	QSlider * dirDiff = createSlider(settingsWidget,
									  1000, 0, 2.0, &this->dirDiffuse_, 800);
	dirDiff->setTickInterval(500);

	QSlider * dirSpec = createSlider(settingsWidget,
									  1000, 0, 2.0, &this->dirSpecular_, 500);
	dirSpec->setTickInterval(500);

	QFrame * hLine0 = new QFrame();
	hLine0->setFrameShape(QFrame::HLine);
	QFrame * hLine1 = new QFrame();
	hLine1->setFrameShape(QFrame::HLine);
	QFrame * hLine2 = new QFrame();
	hLine2->setFrameShape(QFrame::HLine);

	auto settingLayout = new QVBoxLayout();
	settingLayout->addLayout(addAllH(posLabel));
	settingLayout->addLayout(addAllH(dirLabel));
	settingLayout->addLayout(addAllH(capturePosButton));


	settingLayout->addWidget(hLine0);
	settingLayout->addLayout(addAllH(spotLabel));
	settingLayout->addLayout(addAllH(spotAngle, spotColor_));
	settingLayout->addLayout(addAllH(setSpotButton));

	settingLayout->addLayout(addAllH(spotCoeffs));
	settingLayout->addLayout(addAllH(spotAmb));
	settingLayout->addLayout(addAllH(spotDiff));
	settingLayout->addLayout(addAllH(spotSpec));
	settingLayout->addWidget(hLine1);

	settingLayout->addLayout(addAllH(dirLightLabel, dirColor_));
	settingLayout->addWidget(setDirButton);
	settingLayout->addWidget(dirCoeffs);
	settingLayout->addWidget(dirAmb);
	settingLayout->addWidget(dirDiff);
	settingLayout->addWidget(dirSpec);
	settingLayout->addWidget(hLine2);

	// some AI-generated cpp template syntax sugar to make it work with array of arguments
	//settingLayout->addLayout(std::apply([](auto... args) { return addAllH(args...); }, colorButtons));
	//settingLayout->addLayout(addAllH(colorDecayLabel, colorDecaySlider));
	//settingLayout->addLayout(addAllH(borderDecayLabel, borderDecaySlider));

	settingsWidget->setLayout(settingLayout);


	const auto formatFPS = [](const auto value) {
		return QString("FPS: %1").arg(QString::number(value));
	};

	auto fps = new QLabel(formatFPS(0), this);
	fps->setStyleSheet("QLabel { color : white; }");


	auto layout = new QHBoxLayout();
	layout->addWidget(settingsWidget, 1, Qt::AlignTop | Qt::AlignLeft);
	layout->addWidget(fps, 1, Qt::AlignTop | Qt::AlignRight);

	setLayout(layout);

	timer_.start();

	connect(this, &Window::updateUI, [=] {
		fps->setText(formatFPS(ui_.fps));
	});

	
	//setFocusPolicy(Qt::StrongFocus);

	holdKeys_.insert({Qt::Key_W, KeyPressContainer(Qt::Key_W)});
	holdKeys_.insert({Qt::Key_S, KeyPressContainer(Qt::Key_S)});
	holdKeys_.insert({Qt::Key_A, KeyPressContainer(Qt::Key_A)});
	holdKeys_.insert({Qt::Key_D, KeyPressContainer(Qt::Key_D)});
}

Window::~Window()
{
	{
		// Free resources with context bounded.
		const auto guard = bindContext();
	}
}

void Window::onInit()
{
	qDebug() << sizeof(DirectionalLight) << " " << sizeof(SpotLight);
	lightUBO_.directional(0).color_ = {0, 0, 1};
	lightUBO_.directional(0).direction_ = QVector3D(0.0, -1.0, 0.0).normalized();
	lightUBO_.directional(0).ambientStrength = 0.1;
	lightUBO_.directional(0).diffuseStrength = 0.8;
	lightUBO_.directional(0).specularStrength = 0.5;

	lightUBO_.directional(1).color_ = {1, 0, 0};
	lightUBO_.directional(1).direction_ = QVector3D(0.0, -1.0, 0.0).normalized();
	lightUBO_.directional(1).ambientStrength = 0.1;
	lightUBO_.directional(1).diffuseStrength = 0.8;
	lightUBO_.directional(1).specularStrength = 0.5;

	lightUBO_.spot(0).color_ = {1, 1, 0};
	lightUBO_.spot(0).direction_ = QVector3D(0.0, -1.0, 0.0).normalized();
	lightUBO_.spot(0).position_ = {1, 6, 1};
	lightUBO_.spot(0).innerCutoff_ = cosFromDeg(12.5);
	lightUBO_.spot(0).outerCutoff_ = cosFromDeg(13.0f); 
	lightUBO_.spot(0).ambientStrength = 0.0;
	lightUBO_.spot(0).diffuseStrength = 0.3;
	lightUBO_.spot(0).specularStrength = 1.0;

	lightUBO_.spot(1).color_ = {0, 1, 1};
	lightUBO_.spot(1).direction_ = QVector3D(-1.0, -1.0, -1.0).normalized();
	lightUBO_.spot(1).position_ = {2, 2, 2};
	lightUBO_.spot(1).innerCutoff_ = cosFromDeg(44.5f);
	lightUBO_.spot(1).outerCutoff_ = cosFromDeg(45.0f); 
	lightUBO_.spot(1).ambientStrength = 0.0;
	lightUBO_.spot(1).diffuseStrength = 0.3;
	lightUBO_.spot(1).specularStrength = 1.0;

	lightUBO_.spot(__USER_SPOT__).ambientStrength = 0.0;
	lightUBO_.spot(__USER_SPOT__).diffuseStrength = 0.3;
	lightUBO_.spot(__USER_SPOT__).specularStrength = 1.0;


	chessProgram_ = std::make_shared<QOpenGLShaderProgram>();
	chessProgram_->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/model.vert");
	chessProgram_->addShaderFromSourceFile(QOpenGLShader::Fragment,
											":/Shaders/model.frag");

	lightUBO_.initialise();
	lightUBO_.bindToShader(chessProgram_, "LightSources");

	chess_.setShaderProgram(chessProgram_);
	chess_.loadFromGLTF(":/Models/chess_2.glb");

	//ProjectionPoint pj(10, 2);
	//pj.setFaces(chess_.faces(0));
	
	//qDebug() << pj.find(chess_.bounds(0));
	//qDebug() << pj.get();

	chessInstance_.transform_.setToIdentity();
	chessInstance_.transform_.scale(6.0);

	// LIGHT MODELS:
	//----------------
	directionalProgram_ = std::make_shared<QOpenGLShaderProgram>();
	directionalProgram_->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/model.vert");
	directionalProgram_->addShaderFromSourceFile(QOpenGLShader::Fragment,
										   ":/Shaders/shineThrough.frag");

	spotProgram_ = std::make_shared<QOpenGLShaderProgram>();
	spotProgram_->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/coneScaling.vert");
	spotProgram_->addShaderFromSourceFile(QOpenGLShader::Fragment,
												 ":/Shaders/shineThrough.frag");

	directionalModel_.setShaderProgram(directionalProgram_);
	spotModel_.setShaderProgram(spotProgram_);

	directionalModel_.loadFromGLTF(":/Models/blender_sphere.glb");
	spotModel_.loadFromGLTF(":/Models/black_bottom_cone.glb");

	for (size_t i = 0; i < __DIRECTIONAL_COUNT__; i++) {
		directionalAnimated_.push_back(false);

		directionalManagers_[i] =
			std::make_unique<__LIGHT_MODEL_M__>(&lightUBO_, LightType::Directional, i,
			&directionalModel_, "LightSources", __DIR_LIGHT_OFFSET_);
		directionalManagers_[i]->scale(5.0);
	}

	for (size_t i = 0; i < __SPOT_COUNT__; i++)
	{
		spotAnimated_.push_back(false);

		spotManagers_[i] =
			std::make_unique<__LIGHT_MODEL_M__>(&lightUBO_, LightType::Spot, i,
												&spotModel_, "LightSources", 0.0);
		spotManagers_[i]->scale(0.5);
	}
	
	directionalAnimated_[0] = true;
	directionalAnimated_[1] = true;

	//-----------------

	// Еnable depth test and face culling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// This seemed to be doing nothing?..
	//glEnable(GL_FRAMEBUFFER_SRGB);

	// Clear all FBO buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	camera_.move(0, -3.0);
}

void Window::onRender()
{
	const auto guard = captureMetrics();

	// Clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	{
		float speed = 0.01f;
		qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

		KeyPressContainer & kW = holdKeys_.at(Qt::Key_W);
		KeyPressContainer & kS = holdKeys_.at(Qt::Key_S);
		KeyPressContainer & kA = holdKeys_.at(Qt::Key_A);
		KeyPressContainer & kD = holdKeys_.at(Qt::Key_D);

		float moveRight = 0;
		float moveForward = 0;

		if (kW.isHeld() && kS.isHeld()) {
			kW.touch(currentTime);
			kS.touch(currentTime);
		}
		else {
			moveForward = speed * static_cast<float>(kW.touch(currentTime) - kS.touch(currentTime));
		}

		if (kA.isHeld() && kD.isHeld())
		{
			kA.touch(currentTime);
			kD.touch(currentTime);
		}
		else
		{
			moveRight = speed * static_cast<float>(kD.touch(currentTime) - kA.touch(currentTime));
		}

		camera_.move(moveRight, moveForward);
	}

	lightRotationCouner_ += 0.005;
	lightUBO_.directional(0).direction_ = 
		QVector3D(0.0, -std::sinf(lightRotationCouner_), -std::cosf(lightRotationCouner_)).normalized();
	lightUBO_.directional(1).direction_ =
		QVector3D(-std::sinf(lightRotationCouner_), -std::cosf(lightRotationCouner_), 0.0).normalized();
	lightUBO_.updateDirectional(0);
	lightUBO_.updateDirectional(1);

	for (size_t i = 0; i < __DIRECTIONAL_COUNT__; i++) {
		if (directionalAnimated_[i]) {
			directionalManagers_[i]->update();
			directionalManagers_[i]->render(camera_);
		}
	}
	for (size_t i = 0; i < __SPOT_COUNT__; i++)
	{
		if (spotAnimated_[i])
		{
			spotManagers_[i]->update();
			spotManagers_[i]->render(camera_);
		}
	}

	lightUBO_.spot(__USER_SPOT__).color_ = colorToV3(spotColor_->getColor());
	lightUBO_.spot(__USER_SPOT__).outerCutoff_ = cosFromDeg(cachedAngle_ + 1.0);
	lightUBO_.spot(__USER_SPOT__).innerCutoff_ = cosFromDeg(cachedAngle_ + 0.5);
	lightUBO_.spot(__USER_SPOT__).ambientStrength = spotAmbient_;
	lightUBO_.spot(__USER_SPOT__).diffuseStrength = spotDiffuse_;
	lightUBO_.spot(__USER_SPOT__).specularStrength = spotSpecular_;
	lightUBO_.updateSpot(__USER_SPOT__);

	spotManagers_[__USER_SPOT__]->update();
	spotManagers_[__USER_SPOT__]->render(camera_);

	lightUBO_.directional(__USER_DIR__).color_ = colorToV3(dirColor_->getColor());
	lightUBO_.directional(__USER_DIR__).ambientStrength = dirAmbient_;
	lightUBO_.directional(__USER_DIR__).diffuseStrength = dirDiffuse_;
	lightUBO_.directional(__USER_DIR__).specularStrength = dirSpecular_;
	lightUBO_.updateDirectional(__USER_DIR__);

	directionalManagers_[__USER_DIR__]->update();
	directionalManagers_[__USER_DIR__]->render(camera_);

	chess_.render(camera_, {&chessInstance_});

	++frameCount_;

	// Request redraw if animated
	if (animated_)
	{
		update();
	}
}

void Window::onResize(const size_t width, const size_t height)
{
	// Configure viewport
	glViewport(0, 0, static_cast<GLint>(width), static_cast<GLint>(height));

	// Configure matrix
	const auto aspect = static_cast<float>(width) / static_cast<float>(height);
	const auto zNear = 0.1f;
	const auto zFar = 100.0f;
	const auto fov = 60.0f;

	camera_.setPerspective(fov, aspect, zNear, zFar);
}

Window::PerfomanceMetricsGuard::PerfomanceMetricsGuard(std::function<void()> callback)
	: callback_{ std::move(callback) }
{
}

Window::PerfomanceMetricsGuard::~PerfomanceMetricsGuard()
{
	if (callback_)
	{
		callback_();
	}
}

auto Window::captureMetrics() -> PerfomanceMetricsGuard
{
	return PerfomanceMetricsGuard{
		[&] {
			if (timer_.elapsed() >= 1000)
			{
				const auto elapsedSeconds = static_cast<float>(timer_.restart()) / 1000.0f;
				ui_.fps = static_cast<size_t>(std::round(frameCount_ / elapsedSeconds));
				frameCount_ = 0;
				emit updateUI();
			}
		}
	};
}

void Window::captureMouse()
{
	setCursor(Qt::BlankCursor);
	QCursor::setPos(mapToGlobal(QPoint(width() / 2, height() / 2)));
	setMouseTracking(true);
}

void Window::releaseMouse()
{
	setMouseTracking(false);
	unsetCursor();
}

void Window::mousePressEvent(QMouseEvent * event)
{

	switch (event->button())
	{
		case Qt::LeftButton:
			captureMouse();
			break;
		case Qt::RightButton:
			releaseMouse();
			break;
	}
}

void Window::keyPressEvent(QKeyEvent * event)
{
	if (event->key() == Qt::Key_Escape)
	{
		releaseMouse();
	}

	if (holdKeys_.contains(event->key())) {
		holdKeys_.at(event->key()).pressed(QDateTime::currentMSecsSinceEpoch());
	}
	event->accept();
}

void Window::keyReleaseEvent(QKeyEvent * event)
{
	if (holdKeys_.contains(event->key()))
	{
		holdKeys_.at(event->key()).released(QDateTime::currentMSecsSinceEpoch());
	}
	event->accept();
}

void Window::mouseMoveEvent(QMouseEvent * event)
{
	const QPointF center = 
	{static_cast<float>(width() / 2), static_cast<float>(height() / 2)};


	QPointF diff = center - event->pos();
	diff *= 0.01;

	camera_.rotate(diff.ry(), diff.rx());

	event->accept();
	QCursor::setPos(mapToGlobal(QPoint(width() / 2, height() / 2)));
}
