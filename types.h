#ifndef TYPES_H
#define TYPES_H

#include <cmath>
#include <ostream>
#include <tuple>

const int chunkSize = 32;

enum class Direction {none, left, right, forward, back, down, up};

template<typename T>
struct Vec3d
{
	T x;
	T y;
	T z;
	
	bool operator<(const Vec3d<T>& other) const
	{
		return std::tie(x, y, z) < std::tie(other.x, other.y, other.z);
	}
	
	bool operator<=(const Vec3d<T>& other) const
	{
		return (*this < other) || (*this == other);
	}
	
	bool operator>(const Vec3d<T>& other) const
	{
		return other < *this;
	}
	
	bool operator>=(const Vec3d<T>& other) const
	{
		return (*this > other) || (*this == other);
	}
	
	bool operator==(const Vec3d<T>& other) const
	{
		return x==other.x && y==other.y && z==other.z;
	}
	
	bool operator!=(const Vec3d<T>& other) const
	{
		return !(*this==other);
	}
	
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
		return Vec3d<T>{l.x*r.x, l.y*r.y, l.z*r.z};
	}
	
	friend Vec3d<T> operator*(const Vec3d<T>& l, const T& r)
	{
		return Vec3d<T>{l.x*r, l.y*r, l.z*r};
	}
	
	friend Vec3d<T> operator*(const T& l, const Vec3d<T>& r)
	{
		return r*l;
	}
	
	friend Vec3d<T> operator/(const Vec3d<T>& l, const Vec3d<T>& r)
	{
		return Vec3d<T>{l.x/r.x, l.y/r.y, l.z/r.z};
	}
	
	friend Vec3d<T> operator/(const Vec3d<T>& l, const T& r)
	{
		return Vec3d<T>{l.x/r, l.y/r, l.z/r};
	}
	
	friend Vec3d<T> operator/(const T& l, const Vec3d<T>& r)
	{
		return r/l;
	}
	
	Vec3d<T>& operator/=(const T& r)
	{
		x = x/r;
		y = y/r;
		z = z/r;
	
		return *this;
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
Vec3d<A> Vec3dCVT(const B& x, const B& y, const B& z)
{
	return Vec3d<A>{static_cast<const A>(x), static_cast<const A>(y), static_cast<const A>(z)};
}

template<typename A, typename B>
Vec3d<A> Vec3dCVT(const Vec3d<B>& n)
{
	return  Vec3d<A>{static_cast<const A>(n.x), static_cast<const A>(n.y), static_cast<const A>(n.z)};
}


Direction directionOpposite(Direction dir);
Vec3d<int> directionAdd(Vec3d<int> add_vec, Direction dir, int offset = 1);

#endif
