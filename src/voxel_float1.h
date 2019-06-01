#ifndef VOXEL_FLOAT1
#define VOXEL_FLOAT1

#include "voxel.h"

#include <cmath>
#include <fstream>

using namespace std;

struct float1 {
	float value;
	static const float1 zero;

	float1() {
		this->value = 0.f;
	}

	explicit float1(float value) {
		this->value = value;
	}

	inline int toRGBA(byte buff[4]) const {
		byte alpha = toByte(this->value);
		buff[0] = alpha;
		buff[1] = alpha;
		buff[2] = alpha;
		buff[3] = alpha;
		if (value > 255) {
			return value;
		}
		return 1 + alpha;
	}
	void mix(float1 other, float alpha) {
		this->value += (other.value - this->value) * alpha;
	}
	inline bool equals(float1 than, float threshold) {
		return abs(this->value - than.value) <= threshold;
	}

	friend inline float1 abs(const float1 &value) {
		return float1(abs(value.value));
	}
	friend inline float1 cbrt(const float1 &value) {
		return float1(cbrt(value.value));
	}

	inline void read(ifstream &in) {
		this->value = in.get() / 255.f;
	}
	inline void write(ofstream &out) const {
		out.put(toByte(this->value));
	}

	friend inline float1 operator -(float1 lhs, float1 rhs) {
		return float1(lhs.value - rhs.value);
	}
	friend inline float1 operator *(float1 lhs, float1 rhs) {
		return float1(lhs.value * rhs.value);
	}
	friend inline float1 operator /(float1 lhs, float1 rhs) {
		return float1(lhs.value / rhs.value);
	}
	friend inline void operator +=(float1 &lhs, float1 rhs) {
		lhs.value += rhs.value;
	}

	friend inline bool operator !=(const float1 &lhs, const float1 &rhs) {
		return lhs.value != rhs.value;
	}
	friend inline bool operator <(const float1 &lhs, const float1 &rhs) {
		return lhs.value < rhs.value;
	}
	friend inline bool operator >(const float1 &lhs, const float1 &rhs) {
		return lhs.value > rhs.value;
	}
};

#endif
