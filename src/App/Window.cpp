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
#include <vector>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <tinygltf/tiny_gltf.h>

namespace
{

constexpr std::array<GLfloat, 48u> cubeVecrticies = {
	0.5, 0.5, 0.5, 1.0, 0.0, 0.0,
	0.5, -0.5, 0.5, 0.0, 1.0, 0.0,
	-0.5, -0.5, 0.5, 1.0, 1.0, 0.0,
	-0.5, 0.5, 0.5, 0.0, 0.0, 1.0,
	0.5, 0.5, -0.5, 1.0, 0.0, 1.0,
	0.5, -0.5, -0.5, 0.0, 1.0, 1.0,
	-0.5, -0.5, -0.5, 1.0, 1.0, 1.0,
	-0.5, 0.5, -0.5, 0.1, 0.1, 0.1,
};
constexpr std::array<GLuint, 36u> cubeIndices = {
	2, 1, 0,
	3, 2, 0,
	5, 6, 4,
	6, 7, 4,
	0, 1, 5,
	0, 5, 4,
	3, 0, 4,
	3, 4, 7,
	6, 3, 7,
	2, 3, 6,
	1, 2, 5,
	2, 6, 5};

constexpr std::array<GLfloat, 30u> modelTransfroms = {
	2.0, 2.0, 2.0, 30.0, 50.0, 0.0,
	3.0, 4.5, -10.0, 180.0, 60.0, 0.0,
	-5.0, 6.0, -15.5, 90.0, 90.0, 90.0,
	3.3, 3.0, -5.3, 180.0, 0.0, 0.0,
	-2.3, 0.0, -4.1, 0.0, 0.0, 180.0
};

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
		program_.reset();
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



	lightUBO_.spot(0).color_ = {0, 1, 0};
	lightUBO_.spot(1).color_ = {0, 0, 1};

	chessProgram_ = std::make_shared<QOpenGLShaderProgram>();
	chessProgram_->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/model.vert");
	chessProgram_->addShaderFromSourceFile(QOpenGLShader::Fragment,
											":/Shaders/model.frag");

	lightUBO_.initialise();
	lightUBO_.bindToShader(chessProgram_, "LightSources");

	chess_.setShaderProgram(chessProgram_);
	chess_.loadFromGLTF(":/Models/chess_2.glb");

	chessInstance_.transform_.setToIdentity();
	chessInstance_.transform_.scale(5.0);
	//chessInstance_.transform_.translate(1, 1, 1);

	// Configure shaders
	program_ = std::make_unique<QOpenGLShaderProgram>(this);
	program_->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/diffuse.vert");
	program_->addShaderFromSourceFile(QOpenGLShader::Fragment,
									  ":/Shaders/diffuse.frag");
	program_->link();

	// Create VAO object
	vao_.create();
	vao_.bind();

	// Create VBO
	vbo_.create();
	vbo_.bind();
	vbo_.setUsagePattern(QOpenGLBuffer::StaticDraw);
	vbo_.allocate(cubeVecrticies.data(), static_cast<int>(cubeVecrticies.size() * sizeof(GLfloat)));

	// Create IBO
	ibo_.create();
	ibo_.bind();
	ibo_.setUsagePattern(QOpenGLBuffer::StaticDraw);
	ibo_.allocate(cubeIndices.data(), static_cast<int>(cubeIndices.size() * sizeof(GLuint)));

	// Bind attributes
	program_->bind();

	program_->enableAttributeArray(0);
	program_->setAttributeBuffer(0, GL_FLOAT, 0, 3, static_cast<int>(6 * sizeof(GLfloat)));

	program_->enableAttributeArray(1);
	program_->setAttributeBuffer(1, GL_FLOAT, static_cast<int>(3 * sizeof(GLfloat)), 3,
								 static_cast<int>(6 * sizeof(GLfloat)));

	mvpUniform_ = program_->uniformLocation("mvp");

	// Release all
	program_->release();

	vao_.release();

	ibo_.release();
	vbo_.release();

	// Еnable depth test and face culling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// This seemed to be doing nothing?..
	//glEnable(GL_FRAMEBUFFER_SRGB);

	// Clear all FBO buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	model_.setToIdentity();
	rotary_.setToIdentity();

	camera_.move(0, -3.0);
}

void Window::onRender()
{
	const auto guard = captureMetrics();

	// Clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//model_.rotate(5.0f, 1.0, 1.0, 0.2);
	model_.setToIdentity();

	// Bind VAO and shader program
	program_->bind();
	vao_.bind();

	//chessInstance_.transform_.translate(0, 0, 0.01);

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
	
	rotary_.rotate(5.0f, 1.0, 1.0, 0.2);
	for (size_t i = 0; i < modelTransfroms.size() / 6; i++)
	{
		model_.setToIdentity();
		model_.translate(modelTransfroms[i * 6], modelTransfroms[i * 6 + 1], modelTransfroms[i * 6 + 2]);
		model_.rotate(modelTransfroms[i * 6 + 3],
					  modelTransfroms[i * 6 + 3] / modelTransfroms[i * 6 + 3],
					  modelTransfroms[i * 6 + 4] / modelTransfroms[i * 6 + 3],
					  modelTransfroms[i * 6 + 5] / modelTransfroms[i * 6 + 3]);

		const auto mvp = camera_.getProjection() * camera_.getView() * model_ * rotary_;
		program_->setUniformValue(mvpUniform_, mvp);

		// Draw
		glDrawElements(GL_TRIANGLES, cubeIndices.size(), GL_UNSIGNED_INT, nullptr);
	}

	// Release VAO and shader program
	vao_.release();
	program_->release();


	lightRotationCouner_ += 0.005;
	lightUBO_.directional(0).direction_ = 
		QVector3D(0.0, -std::sinf(lightRotationCouner_), -std::cosf(lightRotationCouner_)).normalized();
	lightUBO_.directional(1).direction_ =
		QVector3D(-std::sinf(lightRotationCouner_), -std::cosf(lightRotationCouner_), 0.0).normalized();
	lightUBO_.updateDirectional(0);
	lightUBO_.updateDirectional(1);
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
	projection_.setToIdentity();
	projection_.perspective(fov, aspect, zNear, zFar);
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
