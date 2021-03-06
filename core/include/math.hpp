#pragma once

#include <ostream>
#include <random>

namespace no {

constexpr float pi() {
	return 3.14159265359f;
}

inline float rad_to_deg(float x) {
	return x * 57.295779513082320876f; // x * (180 / pi)
}

inline double rad_to_deg(double x) {
	return x * 57.295779513082320876; // x * (180 / pi)
}

inline float deg_to_rad(float x) {
	return x * 0.0174532925199432957f; // x * (pi / 180)
}

inline double deg_to_rad(double x) {
	return x * 0.0174532925199432957; // x * (pi / 180)
}

template<typename T>
inline T clamp(T value, T min, T max) {
	if (value > max) {
		return max;
	} else if (value < min) {
		return min;
	}
	return value;
}

template<typename T>
inline T inverse_clamp(T value, T min, T max) {
	if (value > max) {
		return min;
	} else if (value < min) {
		return max;
	}
	return value;
}

class random_number_generator {
public:

	random_number_generator() {
		seed(std::random_device{}());
	}

	random_number_generator(unsigned long long seed) {
		mersianne_twister_engine.seed(seed);
	}

	void seed(unsigned long long seed) {
		current_seed = seed;
		mersianne_twister_engine.seed(seed);
	}

	unsigned long long seed() const {
		return current_seed;
	}

	// min and max are inclusive
	template<typename T>
	T next(T min, T max) {
		if constexpr (std::is_integral<T>::value) {
			std::uniform_int_distribution<T> distribution(min, max);
			return distribution(mersianne_twister_engine);
		} else if constexpr (std::is_floating_point<T>::value) {
			std::uniform_real_distribution<T> distribution(min, max);
			return distribution(mersianne_twister_engine);
		}
		static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "T is not an integral or floating point type");
	}

	// max is inclusive
	template<typename T>
	T next(T max) {
		return next<T>(static_cast<T>(0), max);
	}

	// chance must be between 0.0f and 1.0f
	// the higher chance is, the more likely this function returns true
	bool chance(float chance) {
		return chance >= next<float>(0.0f, 1.0f);
	}

private:

	std::mt19937_64 mersianne_twister_engine;
	unsigned long long current_seed = 0;

};

template<typename T>
struct vector2 {

	T x, y;

	constexpr vector2() : x((T)0), y((T)0) {}
	constexpr vector2(T i) : x(i), y(i) {}
	constexpr vector2(T x, T y) : x(x), y(y) {}

	constexpr vector2<T> operator-() const {
		return { -x, -y };
	}

	constexpr vector2<T> operator+(const vector2<T>& v) const {
		return { x + v.x, y + v.y };
	}

	constexpr vector2<T> operator-(const vector2<T>& v) const {
		return { x - v.x, y - v.y };
	}

	constexpr vector2<T> operator*(const vector2<T>& v) const {
		return { x * v.x, y * v.y };
	}

	constexpr vector2<T> operator/(const vector2<T>& v) const {
		return { x / v.x, y / v.y };
	}

	constexpr vector2<T> operator+(T s) const {
		return { x + s, y + s };
	}

	constexpr vector2<T> operator-(T s) const {
		return { x - s, y - s };
	}

	constexpr vector2<T> operator*(T s) const {
		return { x * s, y * s };
	}

	constexpr vector2<T> operator/(T s) const {
		return { x / s, y / s };
	}

	void operator+=(const vector2<T>& v) {
		x += v.x;
		y += v.y;
	}

	void operator-=(const vector2<T>& v) {
		x -= v.x;
		y -= v.y;
	}

	void operator*=(const vector2<T>& v) {
		x *= v.x;
		y *= v.y;
	}

	void operator/=(const vector2<T>& v) {
		x /= v.x;
		y /= v.y;
	}

	constexpr bool operator>(const vector2<T>& v) const {
		return x > v.x && y > v.y;
	}

	constexpr bool operator<(const vector2<T>& v) const {
		return x < v.x && y < v.y;
	}

	constexpr bool operator>=(const vector2<T>& v) const {
		return x >= v.x && y >= v.y;
	}

	constexpr bool operator<=(const vector2<T>& v) const {
		return x <= v.x && y <= v.y;
	}

	constexpr bool operator==(const vector2<T>& v) const {
		return x == v.x && y == v.y;
	}

	constexpr bool operator!=(const vector2<T>& v) const {
		return x != v.x || y != v.y;
	}

	constexpr T distance_to(const vector2<T>& v) const {
		const T dx = x - v.x;
		const T dy = y - v.y;
		return (T)std::sqrt((double)(dx * dx + dy * dy));
	}

	void floor() {
		x = std::floor(x);
		y = std::floor(y);
	}

	void ceil() {
		x = std::ceil(x);
		y = std::ceil(y);
	}

	template<typename U>
	constexpr vector2<U> to() const {
		return vector2<U>((U)x, (U)y);
	}

	constexpr T magnitude() const {
		return (T)std::sqrt((double)(x * x + y * y));
	}

	constexpr T squared_magnitude() const {
		return x * x + y * y;
	}

	constexpr vector2<T> normalized() const {
		const T m = magnitude();
		if (m == (T)0) {
			return {};
		}
		return { x / m, y / m };
	}

	constexpr T dot(const vector2<T>& v) const {
		return x * v.x + y * v.y;
	}

};

typedef vector2<int> vector2i;
typedef vector2<float> vector2f;
typedef vector2<double> vector2d;

template<typename T>
struct vector3 {

	union {
		struct {
			T x, y, z;
		};
		struct {
			vector2<T> xy;
			T _placeholder_z;
		};
	};

	constexpr vector3() : x((T)0), y((T)0), z((T)0) {}
	constexpr vector3(T i) : x(i), y(i), z(i) {}
	constexpr vector3(T x, T y, T z) : x(x), y(y), z(z) {}

	constexpr vector3<T> operator-() const {
		return { -x, -y, -z };
	}

	constexpr vector3<T> operator+(const vector3<T>& v) const {
		return { x + v.x, y + v.y, z + v.z };
	}

	constexpr vector3<T> operator-(const vector3<T>& v) const {
		return { x - v.x, y - v.y, z - v.z };
	}

	constexpr vector3<T> operator*(const vector3<T>& v) const {
		return { x * v.x, y * v.y, z * v.z };
	}

	constexpr vector3<T> operator/(const vector3<T>& v) const {
		return { x / v.x, y / v.y, z / v.z };
	}

	void operator+=(const vector3<T>& v) {
		x += v.x;
		y += v.y;
		z += v.z;
	}

	void operator-=(const vector3<T>& v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
	}

	void operator*=(const vector3<T>& v) {
		x *= v.x;
		y *= v.y;
		z *= v.z;
	}

	void operator/=(const vector3<T>& v) {
		x /= v.x;
		y /= v.y;
		z /= v.z;
	}

	constexpr bool operator>(const vector3<T>& v) const {
		return x > v.x && y > v.y && z > v.z;
	}

	constexpr bool operator<(const vector3<T>& v) const {
		return x < v.x && y < v.y && z < v.z;
	}

	constexpr bool operator>=(const vector3<T>& v) const {
		return x >= v.x && y >= v.y && z >= v.z;
	}

	constexpr bool operator<=(const vector3<T>& v) const {
		return x <= v.x && y <= v.y && z <= v.z;
	}

	constexpr bool operator==(const vector3<T>& v) const {
		return x == v.x && y == v.y && z == v.z;
	}

	constexpr bool operator!=(const vector3<T>& v) const {
		return x != v.x || y != v.y || z != v.z;
	}

	constexpr T distance_to(const vector3<T>& v) const {
		const T dx = x - v.x;
		const T dy = y - v.y;
		const T dz = z - v.z;
		return (T)std::sqrt((double)(dx * dx + dy * dy + dz * dz));
	}

	void floor() {
		x = std::floor(x);
		y = std::floor(y);
		z = std::floor(z);
	}

	void ceil() {
		x = std::ceil(x);
		y = std::ceil(y);
		z = std::ceil(z);
	}

	template<typename U>
	constexpr vector3<U> to() const {
		return vector3<U>((U)x, (U)y, (U)z);
	}

	constexpr T magnitude() const {
		return (T)std::sqrt((double)(x * x + y * y + z * z));
	}

	constexpr T squared_magnitude() const {
		return x * x + y * y + z * z;
	}

	constexpr vector3<T> normalized() const {
		const T m = magnitude();
		if (m == (T)0) {
			return {};
		}
		return { x / m, y / m, z / m };
	}

	constexpr T dot(const vector3<T>& v) const {
		return x * v.x + y * v.y + z * v.z;
	}

	constexpr vector3<T> cross(const vector3<T>& v) const {
		return { y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x };
	}

};

typedef vector3<int> vector3i;
typedef vector3<float> vector3f;
typedef vector3<double> vector3d;

template<typename T>
struct vector4 {

	union {
		struct {
			T x, y, z, w;
		};
		struct {
			vector2<T> xy;
			T _placeholder_z_0;
			T _placeholder_w_1;
		};
		struct {
			vector3<T> xyz;
			T _placeholder_w_2;
		};
		struct {
			T _placeholder_x_0;
			T _placeholder_y_0;
			vector2<T> zw;
		};
	};

	constexpr vector4() : x((T)0), y((T)0), z((T)0), w((T)0) {}
	constexpr vector4(T i) : x(i), y(i), z(i), w(i) {}
	constexpr vector4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}

	constexpr vector4<T> operator-() const {
		return { -x, -y, -z, -w };
	}

	constexpr vector4<T> operator+(const vector4<T>& v) const {
		return { x + v.x, y + v.y, z + v.z, w + v.w };
	}

	constexpr vector4<T> operator-(const vector4<T>& v) const {
		return { x - v.x, y - v.y, z - v.z, w - v.w };
	}

	constexpr vector4<T> operator*(const vector4<T>& v) const {
		return { x * v.x, y * v.y, z * v.z, w * v.w };
	}

	constexpr vector4<T> operator/(const vector4<T>& v) const {
		return { x / v.x, y / v.y, z / v.z, w / v.w };
	}

	void operator+=(const vector4<T>& v) {
		x += v.x;
		y += v.y;
		z += v.z;
		w += v.w;
	}

	void operator-=(const vector4<T>& v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		w -= v.w;
	}

	void operator*=(const vector4<T>& v) {
		x *= v.x;
		y *= v.y;
		z *= v.z;
		w *= v.w;
	}

	void operator/=(const vector4<T>& v) {
		x /= v.x;
		y /= v.y;
		z /= v.z;
		w /= v.w;
	}

	constexpr bool operator>(const vector4<T>& v) const {
		return x > v.x && y > v.y && z > v.z && w > v.w;
	}

	constexpr bool operator<(const vector4<T>& v) const {
		return x < v.x && y < v.y && z < v.z && w < v.w;
	}

	constexpr bool operator>=(const vector4<T>& v) const {
		return x >= v.x && y >= v.y && z >= v.z && w >= v.w;
	}

	constexpr bool operator<=(const vector4<T>& v) const {
		return x <= v.x && y <= v.y && z <= v.z && w <= v.w;
	}

	constexpr bool operator==(const vector4<T>& v) const {
		return x == v.x && y == v.y && z == v.z && w == v.w;
	}

	constexpr bool operator!=(const vector4<T>& v) const {
		return x != v.x || y != v.y || z != v.z || w != v.w;
	}

	constexpr T distance_to(const vector4<T>& v) const {
		const T dx = x - v.x;
		const T dy = y - v.y;
		const T dz = z - v.z;
		const T dw = w - v.w;
		return (T)std::sqrt((double)(dx * dx + dy * dy + dz * dz + dw * dw));
	}

	void floor() {
		x = std::floor(x);
		y = std::floor(y);
		z = std::floor(z);
		w = std::floor(w);
	}

	void ceil() {
		x = std::ceil(x);
		y = std::ceil(y);
		z = std::ceil(z);
		w = std::ceil(w);
	}

	template<typename U>
	constexpr vector4<U> to() const {
		return vector4<U>((U)x, (U)y, (U)z, (U)w);
	}

	constexpr T magnitude() const {
		return (T)std::sqrt((double)(x * x + y * y + z * z + w * w));
	}

	constexpr T squared_magnitude() const {
		return x * x + y * y + z * z + w * w;
	}

	constexpr vector4<T> normalized() const {
		const T m = magnitude();
		if (m == (T)0) {
			return {};
		}
		return { x / m, y / m, z / m, w / m };
	}

	constexpr T dot(const vector4<T>& v) const {
		return x * v.x + y * v.y + z * v.z + w * v.w;
	}

};

typedef vector4<int> vector4i;
typedef vector4<float> vector4f;
typedef vector4<double> vector4d;

}

template<typename T>
std::ostream& operator<<(std::ostream& out, const no::vector2<T>& vector) {
	return out << vector.x << ", " << vector.y;
}

template<typename T>
no::vector2<T> operator*(T scalar, const no::vector2<T>& vector) {
	return vector * scalar;
}

template<typename T>
no::vector2<T> operator/(T scalar, const no::vector2<T>& vector) {
	return no::vector2<T>(scalar) / vector;
}

template<typename T>
std::ostream& operator<<(std::ostream& out, const no::vector3<T>& vector) {
	return out << vector.x << ", " << vector.y << ", " << vector.z;
}

template<typename T>
no::vector3<T> operator*(T scalar, const no::vector3<T>& vector) {
	return vector * scalar;
}

template<typename T>
std::ostream& operator<<(std::ostream& out, const no::vector4<T>& vector) {
	return out << vector.x << ", " << vector.y << ", " << vector.z << ", " << vector.w;
}

template<typename T>
no::vector4<T> operator*(T scalar, const no::vector4<T>& vector) {
	return vector * scalar;
}
