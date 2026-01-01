#pragma once

#include <random>
#include <vector>
#include <cmath>
#include <memory>

#include <QVector3D>
#include <QOpenGLTexture>

std::vector<QVector3D> genKernel(size_t size);

std::unique_ptr<QOpenGLTexture> genNoiseTexture();