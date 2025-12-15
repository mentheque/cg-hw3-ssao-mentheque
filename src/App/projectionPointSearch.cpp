#include "projectionPointSearch.h"

#include <deque>

/*
	So I wanted some code to automatically find the 
	point from which model can be projected into a sphere without
	intersecting itself, that being, I think, a point that is 
	interior to all faces of the model.
	Now, I didn't really want to write simplex in c++ myself, 
	and I could find I library that I would have been able to 
	integrate quickly. Having tried marrying draco and tinygltf 
	for about one evenening with no success, I don't want to do 
	the same dance again, so here is just framework for future 
	possible implementation of these methods, with the current one 
	not really working.
*/



Cuboid::Cuboid(QVector3D min, QVector3D max)
	: max_(max)
	, min_(min)
{
}

QVector3D Cuboid::point(size_t idx) const
{
	if (idx < 4)
	{
		if (idx)
		{
			QVector3D ret = min_;
			ret[idx - 1] = max_[idx - 1];
			return ret;
		}
		else
		{
			return min_;
		}
	}
	if (idx != 4)
	{
		QVector3D ret = max_;
		ret[idx - 5] = min_[idx - 5];
		return ret;
	}
	else
	{
		return max_;
	}
}

QVector3D Cuboid::center() const {
	return (min_ + max_) / 2;
}

std::vector<Cuboid> Cuboid::partition(size_t partitions) const
{
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

float Cuboid::size() const {
	return (max_ - min_).length();
}

float Cuboid::maxDistance(QVector3D x) {
	float max = 0;
	for (size_t i = 0; i < 8; i++) {
		float distance = point(i).distanceToPoint(x);
		if (distance > max) {
			max = distance;
		}
	}
	return max;
}

Halfspace::Halfspace(QVector3D norm, QVector3D point)
	: norm_(norm)
	, point_(point)
{
}

bool Halfspace::contains(QVector3D x)
{
	return QVector3D::dotProduct(x - point_, norm_) >= 0;
}

bool Halfspace::intersects(const Cuboid & cuboid)
{
	// stupid, keeping for now
	for (size_t i = 0; i < 8; i++)
	{
		if (contains(cuboid.point(i)))
		{
			return true;
		}
	}
	return false;
}

ProjectionPoint::ProjectionPoint(size_t maxDepth, size_t partitions)
	: maxDepth_(maxDepth)
	, partitions_(partitions)
	, faces_()
{
}

void ProjectionPoint::setFaces(std::vector<Halfspace> faces)
{
	faces_ = faces;
}

bool ProjectionPoint::find(Cuboid bounds)
{
	bounds.min_ -= {0.01f, 0.01f, 0.01f};
	bounds.max_ += {0.01f, 0.01f, 0.01f};

	//return finding(bounds, 0);
	

	std::deque<Cuboid> cells;
	std::deque<Cuboid> toPartition;

	cells.push_back(bounds);

	for (Cuboid cell = cells.front(); !cells.empty(); )
	{
		if (allContain(cell.center()))
		{
			cache_ = cell.center();
			return true;
		}

		if (allIntersect(cell))
		{
			if (cell.size() > 0.00001f)
			{
				toPartition.push_back(cell);
			}
		}

		cells.pop_front();

		if (cells.empty() && !toPartition.empty())
		{
			std::vector<Cuboid> partitions = toPartition.front().partition(partitions_);
			cells.insert(cells.end(), partitions.begin(), partitions.end());
			toPartition.pop_front();
		}
	}

	return false;
}

QVector3D ProjectionPoint::get()
{
	return cache_;
}

bool ProjectionPoint::allContain(const QVector3D & x)
{
	for (Halfspace & face: faces_)
	{
		if (!face.contains(x))
		{
			return false;
		}
	}
	return true;
}

bool ProjectionPoint::allIntersect(const Cuboid & bounds)
{
	for (Halfspace & face: faces_)
	{
		if (!face.intersects(bounds))
		{
			return false;
		}
	}
	return true;
}

bool ProjectionPoint::finding(const Cuboid & bounds, size_t depth)
{

	if (allContain(bounds.center()))
	{
		cache_ = bounds.center();
		return true;
	}

	if (depth == maxDepth_)
	{
		return false;
	}

	std::vector<Cuboid> partitions = bounds.partition(partitions_);



	for (const Cuboid & smaller: partitions)
	{
		if (allContain(smaller.center())) {
			cache_ = smaller.center();
			return true;
		}
	}

	for (const Cuboid & smaller: partitions)
	{
		if (allIntersect(smaller) && finding(smaller, depth + 1))
		{
			return true;
		}
	}

	return false;
}