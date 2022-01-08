#ifndef TYPES_H
#define TYPES_H

#include <cmath>
#include <compare>
#include <ostream>

const int chunkSize = 64;

enum class Direction {none, left, right, forward, back, down, up};

template<typename T>
struct Vec3d
{
	T x;
	T y;
	T z;
	
	auto operator<=>(const Vec3d<T>&) const = default;
	bool operator==(const Vec3d<T>&) const = default;
	
	Vec3d<T>& operator+=(const Vec3d<T>& r)
	{
		x = x + r.x;
		y = y + r.y;
		z = z + r.z;
		return *this;
	}
	
	Vec3d<T>& operator-=(const Vec3d<T>& r)
	{
		x = x - r.x;
		y = y - r.y;
		z = z - r.z;
		return *this;
	}
	
	friend Vec3d<T> operator+(Vec3d<T> l, const Vec3d<T>& r)
	{
		l += r;
		return std::move(l);
	}
	
	friend Vec3d<T> operator-(Vec3d<T> l, const Vec3d<T>& r)
	{
		l -= r;
		return std::move(l);
	}
	
	friend Vec3d<T> operator*(const Vec3d<T>& l, const Vec3d<T>& r)
	{
		return Vec3d<T>(l.x*r.x, l.y*r.y, l.z*r.z);
	}
	
	friend Vec3d<T> operator*(const Vec3d<T>& l, const T& r)
	{
		return Vec3d<T>(l.x*r, l.y*r, l.z*r);
	}
	
	friend Vec3d<T> operator*(const T& l, const Vec3d<T>& r)
	{
		return r*l;
	}
	
	friend Vec3d<T> operator/(const Vec3d<T>& l, const Vec3d<T>& r)
	{
		return Vec3d<T>(l.x/r.x, l.y/r.y, l.z/r.z);
	}
	
	friend Vec3d<T> operator/(const Vec3d<T>& l, const T& r)
	{
		return Vec3d<T>(l.x/r, l.y/r, l.z/r);
	}
	
	friend Vec3d<T> operator/(const T& l, const Vec3d<T>& r)
	{
		return r/l;
	}
	
	friend std::ostream& operator<<(std::ostream& os, const Vec3d<T> r)
	{
		os << "[" << r.x << ",";
		os << r.y << ",";
		os << r.z << "]";
		
		return os;
	}
	
	static float dotProduct(Vec3d<T> l, Vec3d<T> r)
	{
		return l.x*r.x + l.y*r.y + l.z*r.z;
	}
	
	static float magnitude(Vec3d<T> n)
	{
		return std::sqrt(n.x*n.x + n.y*n.y + n.z*n.z);
	}
};

//this isnt good code i think
template<typename A, typename B>
Vec3d<A> Vec3dCVT(Vec3d<B> n)
{
	return Vec3d<A>{static_cast<A>(n.x), static_cast<A>(n.y), static_cast<A>(n.z)};
}

Direction directionOpposite(Direction dir);
Vec3d<int> directionAdd(Vec3d<int> add_vec, Direction dir, int offset = 1);

#endif
