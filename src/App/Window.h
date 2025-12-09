#pragma once

#include <Base/GLWidget.hpp>

#include <QElapsedTimer>
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>

#include <functional>
#include <memory>
#include <unordered_map>

#include "FpvCamera.h"
#include "KeyPressContainer.h"
#include "Model.h"

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
	GLint mvpUniform_ = -1;

	QOpenGLBuffer vbo_{QOpenGLBuffer::Type::VertexBuffer};
	QOpenGLBuffer ibo_{QOpenGLBuffer::Type::IndexBuffer};
	QOpenGLVertexArrayObject vao_;

	QOpenGLVertexArrayObject lightVao_;


	QMatrix4x4 model_;
	QMatrix4x4 rotary_;
	QMatrix4x4 view_;
	QMatrix4x4 projection_;

	std::unique_ptr<QOpenGLShaderProgram> program_;

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

private:

	QElapsedTimer timer_;
	size_t frameCount_ = 0;

	struct {
		size_t fps = 0;
	} ui_;

	bool animated_ = true;
};
