#pragma once

#include <QVector3D>

#include <utility>
#include <vector>

struct Cuboid{
	QVector3D min_;
	QVector3D max_;

	Cuboid(QVector3D min, QVector3D max);

	QVector3D point(size_t idx) const;
	QVector3D center() const;

	// may return more than partiotions^3, iterate by actual size
	std::vector<Cuboid> partition(size_t partitions) const;

	float size() const;

	float maxDistance(QVector3D x);
};

struct Halfspace {
	QVector3D norm_;
	QVector3D point_;

	Halfspace(QVector3D norm, QVector3D point);
	
	bool contains(QVector3D x);

	bool intersects(const Cuboid & cuboid);
};


// find -> get
class ProjectionPoint
{
	const size_t maxDepth_;
	const size_t partitions_;

	std::vector<Halfspace> faces_;

	QVector3D cache_;

public:
	ProjectionPoint(size_t maxDepth, size_t partitions);

	void setFaces(std::vector<Halfspace> faces);

	bool find(Cuboid bounds);
	QVector3D get();


	bool allContain(const QVector3D & x);

private:
	bool allIntersect(const Cuboid & bounds);

	bool finding(const Cuboid & bounds, size_t depth);
};