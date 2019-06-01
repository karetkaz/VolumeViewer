#ifndef VOXEL_FLOAT4
#define VOXEL_FLOAT4

#include "voxel.h"
#include <cmath>

using namespace std;

struct float4 {
	float x, y, z, w;
	static const float4 zero;
	float4() {
		this->x = 0.f;
		this->y = 0.f;
		this->z = 0.f;
		this->w = 0.f;
	}

	float4(float x, float y, float z, float w) {
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = w;
	}

	inline int toRGBA(byte buff[4]) const {
		byte alpha = toByte(w);
		float r = x, g = y, b = z;
		float len = sqrt(r*r + g*g + b*b);
		if (len > 1e-10f) {
			r /= len;
			g /= len;
			b /= len;
		}
		if (!true) {
			// absolute value
			buff[0] = toByte(abs(r));
			buff[1] = toByte(abs(g));
			buff[2] = toByte(abs(b));
		} else {
			// scale from [-1, 1) to [0, 1)
			buff[0] = toByte(r / 2 + .5f);
			buff[1] = toByte(g / 2 + .5f);
			buff[2] = toByte(b / 2 + .5f);
		}
		buff[3] = alpha;

		if (w > 255) {
			return w;
		}
//		if (abs(y - z) > 0.1f) {
//			buff[0] = 0;
//			buff[1] = 0;
//			buff[2] = 0;
//			buff[3] = 0;
//			return 0;
//		}
		return 1 + toByte(abs(x - z));
	}

	void mix(float4 other, float alpha) {
		this->x += (other.x - this->x) * alpha;
		this->y += (other.y - this->y) * alpha;
		this->z += (other.z - this->z) * alpha;
		this->w += (other.w - this->w) * alpha;
	}

};

#endif
