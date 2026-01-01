#include "Window.h"

#include <QMouseEvent>
#include <QLabel>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QVBoxLayout>
#include <QScreen>
#include <QPushButton>
#include <QCheckBox>

#include <array>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>
#include <numbers>
#include <string>

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

	auto supersampCheck = new QCheckBox("Sumpersampling x4", settingsWidget);
	connect(supersampCheck, &QCheckBox::toggled, [this](bool checked) {
		if (checked)
		{
			this->sspipeline_.supersample(2);
		}
		else
		{
			this->sspipeline_.supersample(1);
		}
	});


	QSlider * sampleSizeSlider = new QSlider(Qt::Horizontal, settingsWidget);
	sampleSizeSlider->setRange(1, 64);
	sampleSizeSlider->setTickPosition(QSlider::TicksBelow);

	QLabel * sampleSizeLabel = new QLabel(settingsWidget);
	sampleSizeLabel->setText("Sample Kernel size [1: 64]");

	QObject::connect(sampleSizeSlider, &QSlider::valueChanged, [this](int value) {
		this->sampleSize_ = value;
		this->sampleKernel_ = genKernel(value);
		this->sampleSizeChanged_ = true;
	});
	sampleSizeSlider->setValue(8);

	QLabel * radiusLabel = new QLabel(settingsWidget);
	radiusLabel->setText("Kernel radius [0.0, 2.0]");
	QSlider * radiusSlider = createSlider(settingsWidget,
									   1000, 0, 2.0, &this->kernelRadius_, 28);
	radiusSlider->setTickInterval(250);

	QLabel * biasLabel = new QLabel(settingsWidget);
	biasLabel->setText("Depth check bias [0.0, 0.2]");
	QSlider * biasSlider = createSlider(settingsWidget,
										  1000, 0, 0.2, &this->bias_, 33);
	biasSlider->setTickInterval(250);

	auto applyAOCheck = new QCheckBox("Apply AO", settingsWidget);
	connect(applyAOCheck, &QCheckBox::toggled, [this](bool checked) {
		this->applyAO_ = checked;
	});
	applyAOCheck->toggle();

	auto showAOCheck = new QCheckBox("Show only AO", settingsWidget);
	connect(showAOCheck, &QCheckBox::toggled, [this](bool checked) {
		this->showOcclusion_ = checked;
	});

	QLabel * blurLabel = new QLabel(settingsWidget);
	blurLabel->setText("Simple blur size [0: 4]");
	QSlider * blurSlider = new QSlider(Qt::Horizontal, settingsWidget);
	blurSlider->setRange(0, 4);
	blurSlider->setTickPosition(QSlider::TicksBelow);
	QObject::connect(blurSlider, &QSlider::valueChanged, [this](int value) {
		this->blurSize_ = value;
	});
	blurSlider->setValue(2);


	QFrame * hLine0 = new QFrame();
	hLine0->setFrameShape(QFrame::HLine);
	QFrame * hLine1 = new QFrame();
	hLine1->setFrameShape(QFrame::HLine);
	QFrame * hLine2 = new QFrame();
	hLine2->setFrameShape(QFrame::HLine);
	QFrame * hLine3 = new QFrame();
	hLine3->setFrameShape(QFrame::HLine);
	QFrame * hLine4 = new QFrame();
	hLine4->setFrameShape(QFrame::HLine);

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

	settingLayout->addWidget(hLine3);
	settingLayout->addWidget(supersampCheck);
	settingLayout->addWidget(sampleSizeLabel);
	settingLayout->addWidget(sampleSizeSlider);
	settingLayout->addWidget(radiusLabel);
	settingLayout->addWidget(radiusSlider);
	settingLayout->addWidget(biasLabel);
	settingLayout->addWidget(biasSlider);
	settingLayout->addWidget(applyAOCheck);
	settingLayout->addWidget(showAOCheck);
	settingLayout->addWidget(blurLabel);
	settingLayout->addWidget(blurSlider);

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
	noiseTex_ = genNoiseTexture();

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

	auto ssLightingProgram = std::make_shared<QOpenGLShaderProgram>();
	ssLightingProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/ssLighting.vert");
	ssLightingProgram->addShaderFromSourceFile(QOpenGLShader::Fragment,
											 ":/Shaders/ssLighting.frag");
	lightUBO_.bindToShader(ssLightingProgram, "LightSources");


	#define UNIFORM_NAME(program, uniform) program##_##uniform##Uniform 


	ssLightingProgram->bind();
	auto UNIFORM_NAME(ssl, gAspectRatio) = ssLightingProgram->uniformLocation("gAspectRatio");
	auto UNIFORM_NAME(ssl, gTanHalfFOV) = ssLightingProgram->uniformLocation("gTanHalfFOV");

	auto UNIFORM_NAME(ssl, invView) = ssLightingProgram->uniformLocation("invView");
	auto UNIFORM_NAME(ssl, cameraPosWorld) = ssLightingProgram->uniformLocation("cameraPosWorld");

	auto UNIFORM_NAME(ssl, viewZConstants) = ssLightingProgram->uniformLocation("viewZConstants");
	auto UNIFORM_NAME(ssl, applyAO) = ssLightingProgram->uniformLocation("applyAO");
	ssLightingProgram->release();


	auto ssaoProgram = std::make_shared<QOpenGLShaderProgram>();
	ssaoProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/ssLighting.vert");
	ssaoProgram->addShaderFromSourceFile(QOpenGLShader::Fragment,
											   ":/Shaders/ssao.frag");

	ssaoProgram->bind();
	auto UNIFORM_NAME(ssao, gAspectRatio) = ssaoProgram->uniformLocation("gAspectRatio");
	auto UNIFORM_NAME(ssao, gTanHalfFOV) = ssaoProgram->uniformLocation("gTanHalfFOV");

	auto UNIFORM_NAME(ssao, noiseTex) = ssaoProgram->uniformLocation("noiseTex");
	auto UNIFORM_NAME(ssao, normalMatrix) = ssaoProgram->uniformLocation("normalMatrix");
	auto UNIFORM_NAME(ssao, projection) = ssaoProgram->uniformLocation("projection");

	#define KERNEL_UNIFORMS(i) ssao_sampleKernelUniforms[i]
	#define KERNEL_UNIFORMS_NAME(i) QString::fromStdString("sampleKernel[" + std::to_string(i) + "]")

	GLint ssao_sampleKernelUniforms[64];
	for (size_t i = 0; i < 64; i++)
	{
		KERNEL_UNIFORMS(i) = ssaoProgram->uniformLocation(KERNEL_UNIFORMS_NAME(i));
	}
	auto UNIFORM_NAME(ssao, sampleSize) = ssaoProgram->uniformLocation("sampleSize");
	auto UNIFORM_NAME(ssao, kernelRadius) = ssaoProgram->uniformLocation("kernelRadius");
	auto UNIFORM_NAME(ssao, bias) = ssaoProgram->uniformLocation("bias");

	auto UNIFORM_NAME(ssao, noiseScale) = ssaoProgram->uniformLocation("noiseScale");
	auto UNIFORM_NAME(ssao, viewZConstants) = ssaoProgram->uniformLocation("viewZConstants");
	ssaoProgram->release();

	auto ssShowOccOrProgram = std::make_shared<QOpenGLShaderProgram>();
	ssShowOccOrProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/ssSimple.vert");
	ssShowOccOrProgram->addShaderFromSourceFile(QOpenGLShader::Fragment,
											 ":/Shaders/ssOcclusionOrDiffuse.frag");

	ssShowOccOrProgram->bind();
	auto UNIFORM_NAME(ssso, showOcclusion) = ssShowOccOrProgram->uniformLocation("showOcclusion");
	ssShowOccOrProgram->release();

	auto ssSimpleBlurProgram = std::make_shared<QOpenGLShaderProgram>();
	ssSimpleBlurProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/ssSimple.vert");
	ssSimpleBlurProgram->addShaderFromSourceFile(QOpenGLShader::Fragment,
												":/Shaders/ssSimpleBlur.frag");

	ssSimpleBlurProgram->bind();
	auto UNIFORM_NAME(ssb, blurSize) = ssSimpleBlurProgram->uniformLocation("blurSize");
	ssSimpleBlurProgram->release();

	sspipeline_.initBuffers();
	sspipeline_.initPipeline(
		{
			{"depthTex", {QOpenGLTexture::DepthFormat, QOpenGLTexture::Depth, QOpenGLTexture::Float32}},
			{"diffuseTex", {QOpenGLTexture::RGBA8_UNorm, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8}},
			{"normalTex", {QOpenGLTexture::RGB16F, QOpenGLTexture::RGB, QOpenGLTexture::Float16}},
			{"occlusionTex", {QOpenGLTexture::R16F, QOpenGLTexture::Red, QOpenGLTexture::Float16}},
		},
		{{"depthTex", GL_DEPTH_ATTACHMENT}, {"diffuseTex", GL_COLOR_ATTACHMENT0},
			{"normalTex", GL_COLOR_ATTACHMENT1}},
		{
			{
				ssaoProgram,
				{{"depthTex"}, {"normalTex"}},
				{{"occlusionTex", GL_COLOR_ATTACHMENT0}},
				[=, this]() {
				ssaoProgram->setUniformValue(UNIFORM_NAME(ssao, gAspectRatio), this->camera_.getAspect());
				ssaoProgram->setUniformValue(UNIFORM_NAME(ssao, gTanHalfFOV), this->camera_.getTanHalfFov());

				// NOTE: arbituary hight value, keep notice. 
				ssaoProgram->setUniformValue(UNIFORM_NAME(ssao, noiseTex), 8);
				this->noiseTex_->bind(8);
				ssaoProgram->setUniformValue(UNIFORM_NAME(ssao, normalMatrix), this->camera_.getView().normalMatrix());
				auto proj = this->camera_.getProjection();
				float viewZConstants_[2] = {proj(2, 3), proj(2, 2)};
				ssaoProgram->setUniformValueArray(UNIFORM_NAME(ssao, viewZConstants), viewZConstants_, 2, 1);
				ssaoProgram->setUniformValue(UNIFORM_NAME(ssao, projection), proj);

				if (this->sampleSizeChanged_)
				{
					this->sampleSizeChanged_ = false;
					for (size_t s = 0; s < this->sampleSize_; s++)
					{
						ssaoProgram->setUniformValue(KERNEL_UNIFORMS(s), this->sampleKernel_[s]);
					}
				}
				ssaoProgram->setUniformValue(UNIFORM_NAME(ssao, sampleSize), (GLint)this->sampleSize_);
				ssaoProgram->setUniformValue(UNIFORM_NAME(ssao, kernelRadius), this->kernelRadius_);
				ssaoProgram->setUniformValue(UNIFORM_NAME(ssao, bias), this->bias_);

				ssaoProgram->setUniformValue(UNIFORM_NAME(ssao, noiseScale), this->sspipeline_.textureSize() / 4.0);
				}
			},
			{
				ssSimpleBlurProgram,
				{{"occlusionTex"}},
				{{"occlusionTex", GL_COLOR_ATTACHMENT0}},
				[=, this]() {
				ssSimpleBlurProgram->setUniformValue(UNIFORM_NAME(ssb, blurSize), this->blurSize_);
				}
			},
			{
				ssLightingProgram,
				{{"depthTex"}, {"diffuseTex"}, {"normalTex"}, {"occlusionTex"}},
				{{"diffuseTex", GL_COLOR_ATTACHMENT0}}, // writing to diffuse, because why not I guess
				[=, this]() {
				ssLightingProgram->setUniformValue(UNIFORM_NAME(ssl, gAspectRatio), this->camera_.getAspect());
				ssLightingProgram->setUniformValue(UNIFORM_NAME(ssl, gTanHalfFOV), this->camera_.getTanHalfFov());

				ssLightingProgram->setUniformValue(UNIFORM_NAME(ssl, invView), this->camera_.getView().inverted());
				ssLightingProgram->setUniformValue(UNIFORM_NAME(ssl, cameraPosWorld),
					this->camera_.getView().inverted().column(3).toVector3D());
				auto proj = this->camera_.getProjection();
				float viewZConstants_[2] = {proj(2, 3), proj(2, 2)};
				ssLightingProgram->setUniformValueArray(UNIFORM_NAME(ssl, viewZConstants), viewZConstants_, 2, 1);
				ssLightingProgram->setUniformValue(UNIFORM_NAME(ssl, applyAO), this->applyAO_);
				}
			},
			{
				ssShowOccOrProgram,
				{{"occlusionTex"}, {"diffuseTex"}},
				{},
				[=, this]() {
				ssShowOccOrProgram->setUniformValue(UNIFORM_NAME(ssso, showOcclusion), this->showOcclusion_);
				}
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
