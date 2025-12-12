#include "Window.h"

#include <QMouseEvent>
#include <QLabel>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QVBoxLayout>
#include <QScreen>

#include <array>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

#include "projectionPointSearch.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <tinygltf/tiny_gltf.h>

namespace
{
}// namespace

Window::Window() noexcept
{
	const auto formatFPS = [](const auto value) {
		return QString("FPS: %1").arg(QString::number(value));
	};

	auto fps = new QLabel(formatFPS(0), this);
	fps->setStyleSheet("QLabel { color : white; }");
	fps->setAttribute(Qt::WA_TransparentForMouseEvents);
	fps->setMouseTracking(true);

	auto layout = new QVBoxLayout(this);
	layout->addWidget(fps, 1);


	setLayout(layout);

	timer_.start();

	connect(this, &Window::updateUI, [=] {
		fps->setText(formatFPS(ui_.fps));
	});

	setFocusPolicy(Qt::StrongFocus);

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
	lightUBO_.spot(0).innerCutoff_ = std::cosf(12.5f * float(std::numbers::pi) / 180.0f);
	lightUBO_.spot(0).outerCutoff_ = std::cosf(13.0f * float(std::numbers::pi) / 180.0f); 
	lightUBO_.spot(0).ambientStrength = 0.0;
	lightUBO_.spot(0).diffuseStrength = 0.3;
	lightUBO_.spot(0).specularStrength = 1.0;

	lightUBO_.spot(1).color_ = {0, 1, 1};
	lightUBO_.spot(1).direction_ = QVector3D(-1.0, -1.0, -1.0).normalized();
	lightUBO_.spot(1).position_ = {2, 2, 2};
	lightUBO_.spot(1).innerCutoff_ = std::cosf(44.5f * float(std::numbers::pi) / 180.0f);
	lightUBO_.spot(1).outerCutoff_ = std::cosf(45.0f * float(std::numbers::pi) / 180.0f); 
	lightUBO_.spot(1).ambientStrength = 0.0;
	lightUBO_.spot(1).diffuseStrength = 0.3;
	lightUBO_.spot(1).specularStrength = 1.0;


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
	//chessInstance_.transform_.translate(1, 1, 1);

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

	directionalManager0_ = std::make_unique<lightModelManager<2, 2>>(&lightUBO_, LightType::Directional, 0,
																	 &directionalModel_, "LightSources", -10.0);
	directionalManager1_ = std::make_unique<lightModelManager<2, 2>>(&lightUBO_, LightType::Directional, 1,
																	 &directionalModel_, "LightSources", -10.0);
	spotManager0_ = std::make_unique<lightModelManager<2, 2>>(&lightUBO_, LightType::Spot, 0,
															  &spotModel_, "LightSources", 0.0);
	spotManager1_ = std::make_unique<lightModelManager<2, 2>>(&lightUBO_, LightType::Spot, 1,
															  &spotModel_, "LightSources", 0.0);

	directionalManager0_->scale(5.0);
	directionalManager1_->scale(5.0);

	spotManager0_->scale(0.5);
	spotManager1_->scale(0.5);

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

	directionalManager0_->update();
	directionalManager1_->update();
	directionalManager0_->render(camera_); // this renders only light's model, 
	directionalManager1_->render(camera_); // we are without shadows, actual lights are 
	// already updated, so order between models doesn't really matter
	spotManager0_->update();
	spotManager1_->update();
	spotManager0_->render(camera_);
	spotManager1_->render(camera_);

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
