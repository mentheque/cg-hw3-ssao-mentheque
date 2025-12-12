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
	std::vector<Cuboid> partition(size_t partitions) const{
		QVector3D inc = (max_ - min_) / partitions;

		std::vector<Cuboid> out;

		// may do 1 extra iteration in each, but not worsens actual search
		for (float minx = 0; minx < max_.x(); minx += inc.x())
		{
			for (float miny = 0; miny < max_.y(); miny += inc.y())
			{
				for (float minz = 0; minz < max_.z(); minz += inc.z())
				{
					QVector3D imin = {minx, miny, minz};
					out.push_back({imin, imin + inc});
				}
			}
		}

		return out;
	}

	float size() {
		return (max_ - min_).length();
	}
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

private:
	bool allContain(const QVector3D & x);

	bool allIntersect(const Cuboid & bounds);

	bool finding(const Cuboid & bounds, size_t depth);
};