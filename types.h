#ifndef TYPES_H
#define TYPES_H

#include <cmath>
#include <ostream>
#include <tuple>

const int chunk_size = 32;

namespace ytype
{
	enum class direction {none, left, right, forward, back, down, up};
};

template<typename T>
struct vec3d
{
	T x;
	T y;
	T z;
	
	bool operator<(const vec3d<T>& other) const
	{
		return std::tie(x, y, z) < std::tie(other.x, other.y, other.z);
	}
	
	bool operator<=(const vec3d<T>& other) const
	{
		return (*this < other) || (*this == other);
	}
	
	bool operator>(const vec3d<T>& other) const
	{
		return other < *this;
	}
	
	bool operator>=(const vec3d<T>& other) const
	{
		return (*this > other) || (*this == other);
	}
	
	bool operator==(const vec3d<T>& other) const
	{
		return x==other.x && y==other.y && z==other.z;
	}
	
	bool operator!=(const vec3d<T>& other) const
	{
		return !(*this==other);
	}
	
	vec3d<T>& operator+=(const vec3d<T>& r)
	{
		x = x + r.x;
		y = y + r.y;
		z = z + r.z;
		return *this;
	}
	
	vec3d<T>& operator-=(const vec3d<T>& r)
	{
		x = x - r.x;
		y = y - r.y;
		z = z - r.z;
		return *this;
	}
	
	friend vec3d<T> operator+(vec3d<T> l, const vec3d<T>& r)
	{
		l += r;
		return std::move(l);
	}
	
	friend vec3d<T> operator-(vec3d<T> l, const vec3d<T>& r)
	{
		l -= r;
		return std::move(l);
	}
	
	friend vec3d<T> operator*(const vec3d<T>& l, const vec3d<T>& r)
	{
		return vec3d<T>{l.x*r.x, l.y*r.y, l.z*r.z};
	}
	
	friend vec3d<T> operator*(const vec3d<T>& l, const T& r)
	{
		return vec3d<T>{l.x*r, l.y*r, l.z*r};
	}
	
	friend vec3d<T> operator*(const T& l, const vec3d<T>& r)
	{
		return r*l;
	}
	
	friend vec3d<T> operator/(const vec3d<T>& l, const vec3d<T>& r)
	{
		return vec3d<T>{l.x/r.x, l.y/r.y, l.z/r.z};
	}
	
	friend vec3d<T> operator/(const vec3d<T>& l, const T& r)
	{
		return vec3d<T>{l.x/r, l.y/r, l.z/r};
	}
	
	friend vec3d<T> operator/(const T& l, const vec3d<T>& r)
	{
		return r/l;
	}
	
	vec3d<T>& operator/=(const T& r)
	{
		x = x/r;
		y = y/r;
		z = z/r;
	
		return *this;
	}
	
	friend std::ostream& operator<<(std::ostream& os, const vec3d<T> r)
	{
		os << "[" << r.x << ",";
		os << r.y << ",";
		os << r.z << "]";
		
		return os;
	}
	
	static float dot_product(const vec3d<T> l, const vec3d<T> r)
	{
		return l.x*r.x + l.y*r.y + l.z*r.z;
	}
	
	static float magnitude(const vec3d<T> n)
	{
		return std::sqrt(n.x*n.x + n.y*n.y + n.z*n.z);
	}
};

//this isnt good code i think
template<typename A, typename B>
vec3d<A> vec3d_cvt(const B& x, const B& y, const B& z)
{
	return vec3d<A>{static_cast<const A>(x), static_cast<const A>(y), static_cast<const A>(z)};
}

template<typename A, typename B>
vec3d<A> vec3d_cvt(const vec3d<B>& n)
{
	return  vec3d<A>{static_cast<const A>(n.x), static_cast<const A>(n.y), static_cast<const A>(n.z)};
}


ytype::direction direction_opposite(const ytype::direction direction);
vec3d<int> direction_add(const vec3d<int> add_vec, const ytype::direction direction, const int offset);

#endif
