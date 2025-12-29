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
#include <numbers>

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
											  1000, 0, 70.0, &this->cachedAngle_, 100, &this->spotChanged_);
	spotColor_ = new ColorButton(settingsWidget);
	spotColor_->setColor(Qt::white);
	auto setSpotButton = new QPushButton(settingsWidget);
	setSpotButton->setText("Set pos + dir from cached");
	connect(setSpotButton, &QPushButton::clicked, this, [=, this](int value) {
		this->lightUBO_.spot(__USER_SPOT__).position_ = this->cachedPos_;
		this->lightUBO_.spot(__USER_SPOT__).direction_ = this->cachedDir_;
		spotChanged_ = true;
	});

	auto spotCoeffs = new QLabel(settingsWidget);
	spotCoeffs->setText("Spot ambient, diffuse, spec.:");
	QSlider * spotAmb = createSlider(settingsWidget,
									 1000, 0, 1.0, &this->spotAmbient_, 0, &this->spotChanged_);
	spotAmb->setTickInterval(1000);
	QSlider * spotDiff = createSlider(settingsWidget,
									  1000, 0, 2.0, &this->spotDiffuse_, 400, &this->spotChanged_);
	spotDiff->setTickInterval(500);

	QSlider * spotSpec = createSlider(settingsWidget,
									  1000, 0, 2.0, &this->spotSpecular_, 250, &this->spotChanged_);
	spotSpec->setTickInterval(500);

	auto dirLightLabel = new QLabel(settingsWidget);
	dirLightLabel->setText("directional light: ");
	dirColor_ = new ColorButton(settingsWidget);
	dirColor_->setColor(Qt::black);
	auto setDirButton = new QPushButton(settingsWidget);
	setDirButton->setText("Set dir from cached");
	connect(setDirButton, &QPushButton::clicked, this, [=, this](int value) {
		this->lightUBO_.directional(__USER_DIR__).direction_ = this->cachedDir_;
		dirChanged_ = true; 
	});

	auto dirCoeffs = new QLabel(settingsWidget);
	dirCoeffs->setText("Dir ambient, diffuse, spec.:");
	QSlider * dirAmb = createSlider(settingsWidget,
									1000, 0, 1.0, &this->dirAmbient_, 0, &this->dirChanged_);
	dirAmb->setTickInterval(1000);
	QSlider * dirDiff = createSlider(settingsWidget,
									 1000, 0, 2.0, &this->dirDiffuse_, 400, &this->dirChanged_);
	dirDiff->setTickInterval(500);

	QSlider * dirSpec = createSlider(settingsWidget,
									 1000, 0, 2.0, &this->dirSpecular_, 150, &this->dirChanged_);
	dirSpec->setTickInterval(500);


	auto morphLabel = new QLabel(settingsWidget);
	morphLabel->setText("Morph to sphere:");

	auto mscaleLabel = new QLabel(settingsWidget);
	mscaleLabel->setText("scale: ");
	QSlider * morphScale = createSlider(settingsWidget,
										   1000, 0, 40.0, &this->morphScale_, 10);
	morphScale->setTickInterval(25);

	auto morphModelButton = new QPushButton(settingsWidget);
	morphModelButton->setText("Set model");
	connect(morphModelButton, &QPushButton::clicked, this, [=, this](int value) {
		QQuaternion rotation = QQuaternion::fromDirection(this->cachedDir_, {0, -1, 0}) *
			QQuaternion::fromEulerAngles(0.0f, 90.0f, 180.0f); // for correct fish rotation
		this->morphInstance_.transform_.setToIdentity();
		this->morphInstance_.transform_.translate(this->cachedPos_);
		this->morphInstance_.transform_.rotate(rotation);
	});
	auto morphPointButton = new QPushButton(settingsWidget);
	morphPointButton->setText("Set point");
	connect(morphPointButton, &QPushButton::clicked, this, [=, this](int value) {
		QMatrix4x4 scaleAdjust;
		scaleAdjust.setToIdentity();
		scaleAdjust.scale(this->morphScale_);
		QMatrix4x4 model = this->morphInstance_.transform_ * scaleAdjust;
		this->morphPointModel_ = model.inverted() * this->cachedPos_;
		this->morphRadius_ = this->morphModelBoundBox_.maxDistance(this->morphPointModel_);
		qDebug() << "Is adecuate point: " << this->morphFacesChecking_.allContain(this->morphPointModel_);
	});
	QSlider * morphProgress = createSlider(settingsWidget,
										   1000, 0, 1.0, &this->morphTransition_, 0);
	morphProgress->setTickInterval(1000);


	QFrame * hLine0 = new QFrame();
	hLine0->setFrameShape(QFrame::HLine);
	QFrame * hLine1 = new QFrame();
	hLine1->setFrameShape(QFrame::HLine);
	QFrame * hLine2 = new QFrame();
	hLine2->setFrameShape(QFrame::HLine);
	QFrame * hLine3 = new QFrame();
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

	settingLayout->addWidget(morphLabel);
	settingLayout->addLayout(addAllH(mscaleLabel, morphScale));
	settingLayout->addLayout(addAllH(morphModelButton, morphPointButton));
	settingLayout->addWidget(morphProgress);

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
	// Light source settings: 
	// ---------------------
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

	lightUBO_.spot(0).color_ = {0, 1, 1};
	lightUBO_.spot(0).position_ = {1, 0.8, 0};
	lightUBO_.spot(0).direction_ = QVector3D(-1, -0.8, 0).normalized();
	lightUBO_.spot(0).innerCutoff_ = cosFromDeg(24.5f);
	lightUBO_.spot(0).outerCutoff_ = cosFromDeg(25.0f); 
	lightUBO_.spot(0).ambientStrength = 0.0;
	lightUBO_.spot(0).diffuseStrength = 0.8;
	lightUBO_.spot(0).specularStrength = 1.0;

	lightUBO_.spot(1).color_ = {1, 1, 0};
	lightUBO_.spot(1).position_ = {-1, 1, -1};
	lightUBO_.spot(1).direction_ = QVector3D(1, -1, 1).normalized();
	lightUBO_.spot(1).innerCutoff_ = cosFromDeg(9.5f);
	lightUBO_.spot(1).outerCutoff_ = cosFromDeg(10.0f);
	lightUBO_.spot(1).ambientStrength = 0.0;
	lightUBO_.spot(1).diffuseStrength = 0.8;
	lightUBO_.spot(1).specularStrength = 1.0;

	lightUBO_.spot(2).color_ = {1, 0, 1};
	lightUBO_.spot(2).position_ = {-1, 1, 1};
	lightUBO_.spot(2).direction_ = QVector3D(1, -1, -1).normalized();
	lightUBO_.spot(2).innerCutoff_ = cosFromDeg(9.5f);
	lightUBO_.spot(2).outerCutoff_ = cosFromDeg(10.0f);
	lightUBO_.spot(2).ambientStrength = 0.0;
	lightUBO_.spot(2).diffuseStrength = 0.8;
	lightUBO_.spot(2).specularStrength = 1.0;

	for (size_t i = 0; i < 3; i++) {
		lightUBO_.spot(i).position_ += {0.19, 0.0, -0.19};
	}

	lightUBO_.spot(__USER_SPOT__).ambientStrength = 0.0;
	lightUBO_.spot(__USER_SPOT__).diffuseStrength = 0.3;
	lightUBO_.spot(__USER_SPOT__).specularStrength = 1.0;
	lightUBO_.spot(__USER_SPOT__).position_ = {0, 3, 0};
	lightUBO_.spot(__USER_SPOT__).direction_ = QVector3D(0, 1.0, 0).normalized();

	lightUBO_.directional(__USER_DIR__).direction_ = {0, -1, 0};

	// ---------------------


	// Non-light models 
	// ---------------------
	chessProgram_ = std::make_shared<QOpenGLShaderProgram>();
	chessProgram_->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/model.vert");
	chessProgram_->addShaderFromSourceFile(QOpenGLShader::Fragment,
											":/Shaders/gbuffModel.frag");

	chessInstance_.transform_.setToIdentity();
	chessInstance_.transform_.scale(6.0);
	chess_.setShaderProgram(chessProgram_);
	chess_.loadFromGLTF(":/Models/chess_2.glb");

	morphProgram_ = std::make_shared<QOpenGLShaderProgram>();
	morphProgram_->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/pointBlowout.vert");
	morphProgram_->addShaderFromSourceFile(QOpenGLShader::Fragment,
										   ":/Shaders/gbuffModel.frag");
	morphInstance_.transform_.setToIdentity();
	morphInstance_.transform_.translate(0, 5, 0);
	morphModel_.setShaderProgram(morphProgram_);
	morphModel_.loadFromGLTF(__MORPH_MODEL_FILE__);

	morphModelBoundBox_ = morphModel_.bounds(0);
	morphRadius_ = morphModelBoundBox_.maxDistance(morphPointModel_);
	morphFacesChecking_.setFaces(morphModel_.faces(0));

	morphProgram_->bind();
	blowOutPointUniform_ = morphProgram_->uniformLocation("blowOutPoint");
	transitionUniform_ = morphProgram_->uniformLocation("transition");
	radiusUniform_ = morphProgram_->uniformLocation("radius");
	morphProgram_->release();
	// ---------------------

	// Binding lights to non-light models
	// ---------------------
	lightUBO_.initialise();
	lightUBO_.bindToShader(chessProgram_, "LightSources");
	lightUBO_.bindToShader(morphProgram_, "LightSources");
	// ---------------------


	// LIGHT MODELS:
	//----------------
	directionalProgram_ = std::make_shared<QOpenGLShaderProgram>();
	directionalProgram_->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/model.vert");
	directionalProgram_->addShaderFromSourceFile(QOpenGLShader::Fragment,
										   ":/Shaders/gbuffShineThrough.frag");

	spotProgram_ = std::make_shared<QOpenGLShaderProgram>();
	spotProgram_->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/coneScaling.vert");
	spotProgram_->addShaderFromSourceFile(QOpenGLShader::Fragment,
												 ":/Shaders/gbuffShineThrough.frag");

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
		directionalManagers_[i]->update();
	}

	for (size_t i = 0; i < __SPOT_COUNT__; i++)
	{
		spotAnimated_.push_back(false);

		spotManagers_[i] =
			std::make_unique<__LIGHT_MODEL_M__>(&lightUBO_, LightType::Spot, i,
												&spotModel_, "LightSources", 0.0);
		spotManagers_[i]->scale(0.5);
		spotManagers_[i]->update();
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

	camera_.setTransforms({3.80794859, 4.15898752, 5.50759220},
		-0.330000490, 4.08237696);

	//ScreenspacePipeline::Texture

	auto ssSimpleProgram = std::make_shared<QOpenGLShaderProgram>();
	ssSimpleProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/ssSimple.vert");
	ssSimpleProgram->addShaderFromSourceFile(QOpenGLShader::Fragment,
												 ":/Shaders/ssSimple.frag");

	ssSimpleProgram->bind();
	auto simpleColorUniform = ssSimpleProgram->uniformLocation("colorTex");
	ssSimpleProgram->release();

	auto ssDepthProgram = std::make_shared<QOpenGLShaderProgram>();
	ssDepthProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/ssSimple.vert");
	ssDepthProgram->addShaderFromSourceFile(QOpenGLShader::Fragment,
											 ":/Shaders/ssDepthReading.frag");

	auto ssNormalProgram = std::make_shared<QOpenGLShaderProgram>();
	ssNormalProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/ssSimple.vert");
	ssNormalProgram->addShaderFromSourceFile(QOpenGLShader::Fragment,
											":/Shaders/ssNormal.frag");

	auto ssLightingProgram = std::make_shared<QOpenGLShaderProgram>();
	ssLightingProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/ssLighting.vert");
	ssLightingProgram->addShaderFromSourceFile(QOpenGLShader::Fragment,
											 ":/Shaders/ssLighting.frag");
	lightUBO_.bindToShader(ssLightingProgram, "LightSources");

	ssLightingProgram->bind();
	auto ssl_gAspectRatioUniform = ssLightingProgram->uniformLocation("gAspectRatio");
	auto ssl_gTanHalfFOVUniform = ssLightingProgram->uniformLocation("gTanHalfFOV");

	auto ssl_projectionUniform = ssLightingProgram->uniformLocation("projection");
	auto ssl_sampleKernelUniform = ssLightingProgram->uniformLocation("sampleKernel");
	auto ssl_invViewUniform = ssLightingProgram->uniformLocation("invView");
	auto ssl_cameraPosWorldUniform = ssLightingProgram->uniformLocation("cameraPosWorld");

	auto ssl_invProjUniform = ssLightingProgram->uniformLocation("invProj");
	auto ssl_nearPlaneUniform = ssLightingProgram->uniformLocation("nearPlane");
	auto ssl_farPlaneUniform = ssLightingProgram->uniformLocation("farPlane");
	ssLightingProgram->release();


	sspipeline_.initBuffers();
	sspipeline_.initPipeline(
		{
			{"depthTex", {QOpenGLTexture::DepthFormat, QOpenGLTexture::Depth, QOpenGLTexture::Float32}},
			{"diffuseTex", {QOpenGLTexture::RGBA8_UNorm, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8}},
			{"normalTex", {QOpenGLTexture::RGB16F, QOpenGLTexture::RGB, QOpenGLTexture::Float16}},
			{"positionTex", {QOpenGLTexture::RGB16F, QOpenGLTexture::RGB, QOpenGLTexture::Float16}},
			{"colorTex", {QOpenGLTexture::RGBA8_UNorm, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8}},
		},
		{{"depthTex", GL_DEPTH_ATTACHMENT}, {"diffuseTex", GL_COLOR_ATTACHMENT0},
		{"normalTex", GL_COLOR_ATTACHMENT1}, {"positionTex", GL_COLOR_ATTACHMENT2}},
		{
			{
				ssLightingProgram,
				{{"depthTex"}, {"diffuseTex"}, {"normalTex"}, {"positionTex"}},
				{{"colorTex", GL_COLOR_ATTACHMENT0}},
				[=, this]() {
				ssLightingProgram->setUniformValue(ssl_gAspectRatioUniform, this->camera_.getAspect());
				ssLightingProgram->setUniformValue(ssl_gTanHalfFOVUniform, this->camera_.getTanHalfFov());

				ssLightingProgram->setUniformValue(ssl_projectionUniform, this->camera_.getProjection());
				//ssLightingProgram->setUniformValue(ssl_sampleKernelUniform, this->camera_.getProjection());
				ssLightingProgram->setUniformValue(ssl_invViewUniform, this->camera_.getView().inverted());
				ssLightingProgram->setUniformValue(ssl_cameraPosWorldUniform,
					this->camera_.getView().inverted().column(3).toVector3D());
				  ssLightingProgram->setUniformValue(ssl_invProjUniform, this->camera_.getProjection().inverted());
				  ssLightingProgram->setUniformValue(ssl_nearPlaneUniform, this->camera_.getNear());
				  ssLightingProgram->setUniformValue(ssl_farPlaneUniform, this->camera_.getFar());
				}
			},
			{
				ssSimpleProgram,
				{{"colorTex", simpleColorUniform}},
				{},
				[=]() { return; } // do nothing
			}
		}
	);
}

void Window::onRender()
{
	const auto guard = captureMetrics();
	qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

	sspipeline_.bindFbo();
	//glClearColor(1.0f, 0.0f, 0.0f, 1.0f);// Bright red
	// Clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Camera movement
	{
		float speed = 0.01f;

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


	// Updating revolving directional light models
	float angleRad = 2 * std::numbers::pi 
		* float(currentTime % __ONE_REVOLUTION_PER_) / float(__ONE_REVOLUTION_PER_);

	lightUBO_.directional(0).direction_ = 
		QVector3D(0.0, -std::sin(angleRad), -std::cos(angleRad)).normalized();
	lightUBO_.directional(1).direction_ =
		QVector3D(-std::sin(angleRad), -std::cos(angleRad), 0.0).normalized();
	lightUBO_.updateDirectional(0);
	lightUBO_.updateDirectional(1);

	// Updating user controlled lights on change: 
	if (spotChanged_
		|| colorToV3(spotColor_->getColor()) != lightUBO_.spot(__USER_SPOT__).color_)
	{
		lightUBO_.spot(__USER_SPOT__).color_ = colorToV3(spotColor_->getColor());
		lightUBO_.spot(__USER_SPOT__).outerCutoff_ = cosFromDeg(cachedAngle_ + 1.0);
		lightUBO_.spot(__USER_SPOT__).innerCutoff_ = cosFromDeg(cachedAngle_ + 0.5);
		lightUBO_.spot(__USER_SPOT__).ambientStrength = spotAmbient_;
		lightUBO_.spot(__USER_SPOT__).diffuseStrength = spotDiffuse_;
		lightUBO_.spot(__USER_SPOT__).specularStrength = spotSpecular_;
		lightUBO_.updateSpot(__USER_SPOT__);
		spotManagers_[__USER_SPOT__]->update();

		spotChanged_ = false;
	}
	if (dirChanged_
		|| colorToV3(dirColor_->getColor()) != lightUBO_.directional(__USER_DIR__).color_)
	{
		lightUBO_.directional(__USER_DIR__).color_ = colorToV3(dirColor_->getColor());
		lightUBO_.directional(__USER_DIR__).ambientStrength = dirAmbient_;
		lightUBO_.directional(__USER_DIR__).diffuseStrength = dirDiffuse_;
		lightUBO_.directional(__USER_DIR__).specularStrength = dirSpecular_;
		lightUBO_.updateDirectional(__USER_DIR__);

		directionalManagers_[__USER_DIR__]->update();
	}

	// Updating models of animated lights, rendering all
	for (size_t i = 0; i < __DIRECTIONAL_COUNT__; i++) {
		if (directionalAnimated_[i]) {
			directionalManagers_[i]->update();
		}
		directionalManagers_[i]->render(camera_);
	}
	for (size_t i = 0; i < __SPOT_COUNT__; i++)
	{
		if (spotAnimated_[i])
		{
			spotManagers_[i]->update();
		}
		spotManagers_[i]->render(camera_);
	}

	// Rendering other models
	chess_.render(camera_, {&chessInstance_});

	morphProgram_->bind();
	morphProgram_->setUniformValue(blowOutPointUniform_, morphPointModel_);
	morphProgram_->setUniformValue(transitionUniform_, morphTransition_);
	morphProgram_->setUniformValue(radiusUniform_, morphRadius_);
	morphProgram_->release();

	QMatrix4x4 scaleAdjust;
	scaleAdjust.setToIdentity();
	scaleAdjust.scale(this->morphScale_);
	Instance scaledMorph = {morphInstance_.transform_ * scaleAdjust};

	morphModel_.render(camera_, {&scaledMorph});

	sspipeline_.releaseFbo();
	sspipeline_.execute();

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
	sspipeline_.resize(width, height);
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
