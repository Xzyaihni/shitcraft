#ifndef TYPES_H
#define TYPES_H

#include <cmath>
#include <ostream>
#include <tuple>

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
		return l;
	}

	friend vec3d<T> operator-(vec3d<T> l, const vec3d<T>& r)
	{
		l -= r;
		return l;
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

	friend std::ostream& operator<<(std::ostream& os, const vec3d<T>& rhs)
	{
		os << "[" << rhs.x << ",";
		os << rhs.y << ",";
		os << rhs.z << "]";

		return os;
	}

	vec3d<T> abs() const noexcept
	{
		return vec3d<T>{std::abs(x), std::abs(y), std::abs(z)};
	}

	float dot_product(const vec3d<T>& rhs) const noexcept
	{
		return x*rhs.x + y*rhs.y + z*rhs.z;
	}

	float magnitude() const noexcept
	{
		return std::sqrt(x*x + y*y + z*z);
	}

	vec3d<T> normalize() const noexcept
	{
		return *this/magnitude();
	}

	template<typename P>
	vec3d<P> cast() const noexcept
	{
		return vec3d<P>{static_cast<P>(x), static_cast<P>(y), static_cast<P>(z)};
	}
};

namespace ytype
{
	enum class direction {none, left, right, forward, back, down, up};

	ytype::direction direction_opposite(const ytype::direction direction);
	vec3d<int> direction_offset(const ytype::direction direction);
	vec3d<int> direction_add(const vec3d<int> add_vec, const ytype::direction direction, const int offset);

	int round_away(const float val) noexcept;
};

#endif
