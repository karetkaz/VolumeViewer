#ifndef VOXEL_H
#define VOXEL_H

typedef unsigned char byte;
static inline byte toByte(float value) {
	if (value >= 1) {
		return 255;
	}
	if (value < 0) {
		return 0;
	}
	return (byte) (value * 255);
}

#endif
