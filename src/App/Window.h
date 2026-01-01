#pragma once

#include <Base/GLWidget.hpp>

#include <QElapsedTimer>
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>

#include <QLineEdit>
#include <QSlider>
#include <QDial>

#include <functional>
#include <memory>
#include <unordered_map>

#include "Values.h"

#include "FpvCamera.h"
#include "KeyPressContainer.h"
#include "Model.h"
#include "Lights.h"
#include "ScreenspacePipeline.h"
#include "ssaoUniforms.h"

#include "ColorButton.h"


class Window final : public fgl::GLWidget
{
	Q_OBJECT
public:
	Window() noexcept;
	~Window() override;

public: // fgl::GLWidget
	void onInit() override;
	void onRender() override;
	void onResize(size_t width, size_t height) override;

private:
	class PerfomanceMetricsGuard final
	{
	public:
		explicit PerfomanceMetricsGuard(std::function<void()> callback);
		~PerfomanceMetricsGuard();

		PerfomanceMetricsGuard(const PerfomanceMetricsGuard &) = delete;
		PerfomanceMetricsGuard(PerfomanceMetricsGuard &&) = delete;

		PerfomanceMetricsGuard & operator=(const PerfomanceMetricsGuard &) = delete;
		PerfomanceMetricsGuard & operator=(PerfomanceMetricsGuard &&) = delete;

	private:
		std::function<void()> callback_;
	};

private:
	[[nodiscard]] PerfomanceMetricsGuard captureMetrics();

signals:
	void updateUI();

private:
	FpvCamera camera_;

private:
	void captureMouse();
	void releaseMouse();
	void mousePressEvent(QMouseEvent * event) override;
	void keyPressEvent(QKeyEvent * event) override;

	void mouseMoveEvent(QMouseEvent * event) override;
	
	std::unordered_map<int, KeyPressContainer> holdKeys_;
	void keyReleaseEvent(QKeyEvent * event) override;

private:
	Model chess_;
	Instance chessInstance_;
	std::shared_ptr<QOpenGLShaderProgram> chessProgram_;

	Model tumbleweed_;
	Instance tumbleweedInstance_;
private:
	LightUBOManager<__DIRECTIONAL_COUNT__, __SPOT_COUNT__> lightUBO_;
	float lightRotationCouner_ = 0.0;


	std::shared_ptr<QOpenGLShaderProgram> directionalProgram_;
	std::shared_ptr<QOpenGLShaderProgram> spotProgram_;


	Model directionalModel_;
	Model spotModel_;

	std::vector<std::unique_ptr<__LIGHT_MODEL_M__>> directionalManagers_ =
		std::vector<std::unique_ptr<__LIGHT_MODEL_M__>>(__DIRECTIONAL_COUNT__);
	std::vector<bool> directionalAnimated_;
	std::vector<std::unique_ptr<__LIGHT_MODEL_M__>> spotManagers_ =
		std::vector<std::unique_ptr<__LIGHT_MODEL_M__>>(__SPOT_COUNT__);
	std::vector<bool> spotAnimated_;
private:

	QElapsedTimer timer_;
	size_t frameCount_ = 0;

	struct {
		size_t fps = 0;
	} ui_;

	bool animated_ = true;

private:

	QVector3D cachedPos_ = {0.0, 0.0, 0.0};
	QVector3D cachedDir_ = {0.0, 0.0, 0.0};

	float cachedAngle_ = 15.5;

	ColorButton * spotColor_;
	float spotAmbient_ = 0.1;
	float spotDiffuse_ = 0.8;
	float spotSpecular_ = 0.5;
	bool spotChanged_ = true;

	ColorButton * dirColor_;
	float dirAmbient_ = 0.1;
	float dirDiffuse_ = 0.8;
	float dirSpecular_ = 0.5;
	bool dirChanged_ = true;


	std::shared_ptr<QOpenGLShaderProgram> morphProgram_;
	Model morphModel_;
	Cuboid morphModelBoundBox_ = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
	ProjectionPoint morphFacesChecking_ = {0, 0};
	Instance morphInstance_;
	float morphScale_ = 5.0;
	QVector3D morphPointModel_ = {0.0, 0.0, 0.0};
	float morphRadius_ = 1.0;
	float morphTransition_ = 0.0;

	GLint blowOutPointUniform_ = -1;
	GLint transitionUniform_ = -1;
	GLint radiusUniform_ = -1;

private:

	ScreenspacePipeline sspipeline_;

	std::unique_ptr<QOpenGLTexture> noiseTex_;
	size_t sampleSize_ = 0;
	std::vector<QVector3D> sampleKernel_;

	float kernelRadius_;
	float bias_;
	bool sampleSizeChanged_ = true;

	bool applyAO_ = true;
	bool showOcclusion_ = false;

	GLint blurSize_ = 0.0;
};
