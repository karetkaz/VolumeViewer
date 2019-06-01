/* tiny vector3d and matrix3d library */

#ifndef MATH_3D_H
#define MATH_3D_H

#include <cmath>

typedef float scalar;

static inline float deg2rad(scalar deg) {
	static const scalar PI = 3.14159265358979323846;
	return (scalar)(deg * PI / 180);
}

struct vector3d {
	scalar x;
	scalar y;
	scalar z;
	scalar w;

	inline vector3d() = default;
	inline explicit vector3d(scalar s) {
		this->x = s;
		this->y = s;
		this->z = s;
		this->w = s;
	}
	inline vector3d(scalar x, scalar y, scalar z, scalar w) {
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = w;
	}
	inline vector3d(scalar x, scalar y, scalar z) {
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = 0;
	}
	inline vector3d(const vector3d &xyz, scalar w) {
		this->x = xyz.x;
		this->y = xyz.y;
		this->z = xyz.z;
		this->w = w;
	}

	scalar operator [](int index) const {
		switch (index) {
			default:
				throw "Invalid index";
			case 0:
				return this->x;
			case 1:
				return this->y;
			case 2:
				return this->z;
			case 3:
				return this->w;
		}
	}
	scalar &operator [](int index) {
		switch (index) {
			default:
				throw "Invalid index";
			case 0:
				return this->x;
			case 1:
				return this->y;
			case 2:
				return this->z;
			case 3:
				return this->w;
		}
	}
};

static inline vector3d operator -(const vector3d &rhs) {
	return vector3d(
		-rhs.x,
		-rhs.y,
		-rhs.z,
		-rhs.w
	);
}
static inline vector3d operator +(const vector3d &lhs, const vector3d &rhs) {
	return vector3d(
		lhs.x + rhs.x,
		lhs.y + rhs.y,
		lhs.z + rhs.z,
		lhs.w + rhs.w
	);
}
static inline vector3d operator -(const vector3d &lhs, const vector3d &rhs) {
	return vector3d(
		lhs.x - rhs.x,
		lhs.y - rhs.y,
		lhs.z - rhs.z,
		lhs.w - rhs.w
	);
}
static inline vector3d operator *(const vector3d &lhs, scalar rhs) {
	return vector3d(
		lhs.x * rhs,
		lhs.y * rhs,
		lhs.z * rhs,
		lhs.w * rhs
	);
}
static inline vector3d operator *(const vector3d &lhs, const vector3d &rhs) {
	return vector3d(
		lhs.x * rhs.x,
		lhs.y * rhs.y,
		lhs.z * rhs.z,
		lhs.w * rhs.w
	);
}

static inline vector3d &operator +=(vector3d &lhs, const vector3d &rhs) {
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	lhs.z += rhs.z;
	lhs.w += rhs.w;
	return lhs;
}
static inline vector3d &operator -=(vector3d &lhs, const vector3d &rhs) {
	lhs.x -= rhs.x;
	lhs.y -= rhs.y;
	lhs.z -= rhs.z;
	lhs.w -= rhs.w;
	return lhs;
}
static inline vector3d &operator *=(vector3d &lhs, const scalar rhs) {
	lhs.x *= rhs;
	lhs.y *= rhs;
	lhs.z *= rhs;
	lhs.w *= rhs;
	return lhs;
}
static inline vector3d &operator *=(vector3d &lhs, const vector3d &rhs) {
	lhs.x *= rhs.x;
	lhs.y *= rhs.y;
	lhs.z *= rhs.z;
	lhs.w *= rhs.w;
	return lhs;
}

static inline scalar dp3(vector3d lhs, vector3d rhs) {
	return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}
static inline scalar dph(vector3d lhs, vector3d rhs) {
	return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w;
}
static inline scalar dp4(vector3d lhs, vector3d rhs) {
	return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
}

static inline vector3d cross(const vector3d &lhs, const vector3d &rhs) {
	return vector3d(
		lhs.y * rhs.z - lhs.z * rhs.y,
		lhs.z * rhs.x - lhs.x * rhs.z,
		lhs.x * rhs.y - lhs.y * rhs.x,
		0
	);
}
static inline vector3d normalized(const vector3d &src) {
	scalar squareLen = dp3(src, src);
	if (squareLen != 0) {
		squareLen = 1 / sqrt(squareLen);
	}
	return src * squareLen;
}
static inline vector3d reflected(const vector3d &dir, const vector3d &nrm) {
	return dir - (nrm * 2 * dp3(nrm, dir));
}
static inline scalar eval(const vector3d &polynomial, scalar at) {
	return ((((polynomial.w) * at + polynomial.z) * at + polynomial.y) * at) + polynomial.x;
}
static inline scalar length(const vector3d &src) {
	return sqrt(dp3(src, src));
}

struct matrix3d {
	vector3d x;
	vector3d y;
	vector3d z;
	vector3d w;

	inline matrix3d() = default;
	inline explicit matrix3d(scalar s) {
		this->x = vector3d(s, 0, 0, 0);
		this->y = vector3d(0, s, 0, 0);
		this->z = vector3d(0, 0, s, 0);
		this->w = vector3d(0, 0, 0, s);
	}
	inline explicit matrix3d(const vector3d &v) {
		this->x = vector3d(v.x, 0, 0, 0);
		this->y = vector3d(0, v.y, 0, 0);
		this->z = vector3d(0, 0, v.z, 0);
		this->w = vector3d(0, 0, 0, v.w);
	}
	inline matrix3d(const vector3d &x, const vector3d &y, const vector3d &z, const vector3d &w) {
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = w;
	}

	const vector3d &operator [](int index) const {
		switch (index) {
			default:
				throw "Invalid index";
			case 0:
				return this->x;
			case 1:
				return this->y;
			case 2:
				return this->z;
			case 3:
				return this->w;
		}
	}
	vector3d &operator [](int index) {
		switch (index) {
			default:
				throw "Invalid index";
			case 0:
				return this->x;
			case 1:
				return this->y;
			case 2:
				return this->z;
			case 3:
				return this->w;
		}
	}
};

static inline matrix3d operator -(const matrix3d &rhs) {
	return matrix3d(
		-rhs.x,
		-rhs.y,
		-rhs.z,
		-rhs.w
	);
}
static inline matrix3d operator +(const matrix3d &lhs, const matrix3d &rhs) {
	return matrix3d(
		lhs.x + rhs.x,
		lhs.y + rhs.y,
		lhs.z + rhs.z,
		lhs.w + rhs.w
	);
}
static inline matrix3d operator -(const matrix3d &lhs, const matrix3d &rhs) {
	return matrix3d(
		lhs.x - rhs.x,
		lhs.y - rhs.y,
		lhs.z - rhs.z,
		lhs.w - rhs.w
	);
}
static inline matrix3d operator *(const matrix3d &lhs, scalar rhs) {
	return matrix3d(
		lhs.x * rhs,
		lhs.y * rhs,
		lhs.z * rhs,
		lhs.w * rhs
	);
}
static inline matrix3d operator *(const matrix3d &lhs, const matrix3d &rhs) {
	matrix3d result(0);
	for(int row = 0; row < 4; ++row) {
		for(int col = 0; col < 4; ++col) {
			result[row][col] = \
				lhs[row][0] * rhs[0][col]+
				lhs[row][1] * rhs[1][col]+
				lhs[row][2] * rhs[2][col]+
				lhs[row][3] * rhs[3][col];
		}
	}
	return result;
}
static inline matrix3d &operator *= (matrix3d &lhs, const matrix3d& rhs) {
	return lhs = lhs * rhs;
}


static inline vector3d vp3(const matrix3d &mat, const vector3d &vec) {
	return vector3d(
		dp3(mat.x, vec),
		dp3(mat.y, vec),
		dp3(mat.z, vec),
		1
	);
}
static inline vector3d vph(const matrix3d &mat, const vector3d &vec) {
	return vector3d(
		dph(mat.x, vec),
		dph(mat.y, vec),
		dph(mat.z, vec),
		dph(mat.w, vec)
	);
}
static inline vector3d vp4(const matrix3d &mat, const vector3d &vec) {
	return vector3d(
		dp4(mat.x, vec),
		dp4(mat.y, vec),
		dp4(mat.z, vec),
		dp4(mat.w, vec)
	);
}

static inline matrix3d rotation(scalar ang, const vector3d &dir) {
	scalar sin_t = sinf(ang);
	scalar cos_t = cosf(ang);
	scalar one_c = 1 - cos_t;

	scalar xx = dir.x * dir.x;
	scalar yy = dir.y * dir.y;
	scalar zz = dir.z * dir.z;
	scalar xy = dir.x * dir.y;
	scalar xz = dir.x * dir.z;
	scalar yz = dir.y * dir.z;

	vector3d tmp = dir * sin_t;
	matrix3d result(1);
	result.x.x = one_c * xx + cos_t;
	result.x.y = one_c * xy - tmp.z;
	result.x.z = one_c * xz + tmp.y;
	result.y.x = one_c * xy + tmp.z;
	result.y.y = one_c * yy + cos_t;
	result.y.z = one_c * yz - tmp.x;
	result.z.x = one_c * xz - tmp.y;
	result.z.y = one_c * yz + tmp.x;
	result.z.z = one_c * zz + cos_t;
	return result;
}
static inline matrix3d scale(scalar cnt, const vector3d &dir) {
	vector3d tmp = dir * cnt;
	matrix3d result(1);
	result.x.x = tmp.x;
	result.y.y = tmp.y;
	result.z.z = tmp.z;
	return result;
}
static inline matrix3d translate(scalar cnt, const vector3d &dir) {
	vector3d tmp = dir * cnt;
	matrix3d result(1);
	result.x.w = tmp.x;
	result.y.w = tmp.y;
	result.z.w = tmp.z;
	return result;
}

/*
static inline void ortho_mat(matrix3d dst, scalar l, scalar r, scalar b, scalar t, scalar n, scalar f) {
	scalar rl = r - l;
	scalar tb = t - b;
	scalar nf = n - f;

	if (rl == 0. || tb  == 0. || nf  == 0.)
		return;

	matldf(dst,			// Projection matrix3d - orthographic
		2 / rl,			0.,				0.,				-(r+l) / rl,
		0.,				2 / tb,			0.,				-(t+b) / tb,
		0.,				0.,				2 / nf,			-(f+n) / nf,
		0.,				0.,				0.,				1.);

	/ * step by step
	//~ union matrix3d tmp;
	matldf(dst,							// scale
		2/(r-l),		0.,				0.,				0.,
		0.,				2/(t-b),		0.,				0.,
		0.,				0.,				2/(n-f),		0.,
		0.,				0.,				0.,				1.);
	matmul(dst, dst, matldf(&tmp,		// translate
		1.,				0.,				0.,				-(l+r)/2,
		0.,				1.,				0.,				-(t+b)/2,
		0.,				0.,				1.,				-(n+f)/2,
		0.,				0.,				0.,				1.));
	// * /
}

static inline void persp_mat(matrix3d dst, scalar l, scalar r, scalar b, scalar t, scalar n, scalar f) {
	scalar rl = r - l;
	scalar tb = t - b;
	scalar nf = n - f;

	if (rl == 0. || tb  == 0. || nf  == 0.)
		return;

	matldf(dst,			// Projection matrix3d - perspective
		2*n / rl,	0.,			-(r+l) / rl,	0.,
		0.,			2*n / tb,	-(t+b) / tb,	0.,
		0.,			0.,			+(n+f) / nf,	-2*n*f / nf,
		0.,			0.,			1.,				0.);

	/ * step by step
	//~ union matrix3d tmp;
	matldf(dst,							// scale
		2/(r-l),		0.,				0.,				0.,
		0.,				2/(t-b),		0.,				0.,
		0.,				0.,				2/(n-f),		0.,
		0.,				0.,				0.,				1.);
	matmul(dst, dst, matldf(&tmp,		// translate
		1.,				0.,				0.,				-(l+r)/2,
		0.,				1.,				0.,				-(t+b)/2,
		0.,				0.,				1.,				-(n+f)/2,
		0.,				0.,				0.,				1.));
	//~ --- Ortographic ---
	matmul(dst, dst, matldf(&tmp,		// perspective
		n,				0.,				0.,				0,
		0.,				n,				0.,				0,
		0.,				0.,				n+f,			-n*f,
		0.,				0.,				1.,				0.));
	// * /
}

static inline void projv_mat(matrix3d dst, scalar fovy, scalar asp, scalar n, scalar f) {
	scalar bot = 1;
	scalar nf = n - f;
	if (fovy) {		// perspective
		bot = tan(fovy * ((3.14159265358979323846 / 180)));
		asp *= bot;

		matldf(dst,
			n / asp,	0.,		0.,		0,
			0.,		n / bot,	0.,		0,
			0.,		0.,		(n+f) / nf,	-2*n*f / nf,
			0.,		0.,		1.,		0);
	}
	else {			// orthographic
		matldf(dst,
			1 / asp,	0.,		0.,		0,
			0.,		1 / bot,	0.,		0,
			0.,		0.,		2 / nf,		-(f+n) / nf,
			0.,		0.,		0.,		1.);
	}
}

static scalar det3x3(scalar x1, scalar x2, scalar x3, scalar y1, scalar y2, scalar y3, scalar z1, scalar z2, scalar z3) {
	return	x1 * (y2 * z3 - z2 * y3)-
			x2 * (y1 * z3 - z1 * y3)+
			x3 * (y1 * z2 - z1 * y2);
}

static inline scalar matdet(matrix3d src) {
	scalar* x = (scalar*)&src->x.v;
	scalar* y = (scalar*)&src->y.v;
	scalar* z = (scalar*)&src->z.v;
	scalar* w = (scalar*)&src->w.v;
	return	x[0] * det3x3(y[1], y[2], y[3], z[1], z[2], z[3], w[1], w[2], w[3])-
		x[1] * det3x3(y[0], y[2], y[3], z[0], z[2], z[3], w[0], w[2], w[3])+
		x[2] * det3x3(y[0], y[1], y[3], z[0], z[1], z[3], w[0], w[1], w[3])-
		x[3] * det3x3(y[0], y[1], y[2], z[0], z[1], z[2], w[0], w[1], w[2]);
}

static inline matrix3d matadj(matrix3d dst, matrix3d src) {
	struct matrix3d tmp;
	scalar* x = (scalar*)&tmp.x.v;
	scalar* y = (scalar*)&tmp.y.v;
	scalar* z = (scalar*)&tmp.z.v;
	scalar* w = (scalar*)&tmp.w.v;
	matran(&tmp, src);
	dst->m11 = +det3x3(y[1], y[2], y[3], z[1], z[2], z[3], w[1], w[2], w[3]);
	dst->m12 = -det3x3(y[0], y[2], y[3], z[0], z[2], z[3], w[0], w[2], w[3]);
	dst->m13 = +det3x3(y[0], y[1], y[3], z[0], z[1], z[3], w[0], w[1], w[3]);
	dst->m14 = -det3x3(y[0], y[1], y[2], z[0], z[1], z[2], w[0], w[1], w[2]);
	dst->m21 = -det3x3(x[1], x[2], x[3], z[1], z[2], z[3], w[1], w[2], w[3]);
	dst->m22 = +det3x3(x[0], x[2], x[3], z[0], z[2], z[3], w[0], w[2], w[3]);
	dst->m23 = -det3x3(x[0], x[1], x[3], z[0], z[1], z[3], w[0], w[1], w[3]);
	dst->m24 = +det3x3(x[0], x[1], x[2], z[0], z[1], z[2], w[0], w[1], w[2]);
	dst->m31 = +det3x3(x[1], x[2], x[3], y[1], y[2], y[3], w[1], w[2], w[3]);
	dst->m32 = -det3x3(x[0], x[2], x[3], y[0], y[2], y[3], w[0], w[2], w[3]);
	dst->m33 = +det3x3(x[0], x[1], x[3], y[0], y[1], y[3], w[0], w[1], w[3]);
	dst->m34 = -det3x3(x[0], x[1], x[2], y[0], y[1], y[2], w[0], w[1], w[2]);
	dst->m41 = -det3x3(x[1], x[2], x[3], y[1], y[2], y[3], z[1], z[2], z[3]);
	dst->m42 = +det3x3(x[0], x[2], x[3], y[0], y[2], y[3], z[0], z[2], z[3]);
	dst->m43 = -det3x3(x[0], x[1], x[3], y[0], y[1], y[3], z[0], z[1], z[3]);
	dst->m44 = +det3x3(x[0], x[1], x[2], y[0], y[1], y[2], z[0], z[1], z[2]);
	return dst;
}

static inline scalar matinv(matrix3d dst, matrix3d src) {
	scalar det = matdet(src);
	if (det) {
		matadj(dst, src);
		matsca(dst, dst, 1. / det);
	}
	return det;
}*/

struct camera3d {
	vector3d forward;		// camera forward direction
	vector3d right;			// camera right direction
	vector3d up;			// camera up direction
	vector3d position;		// camera location

	// look at ...
	inline void lookAt(const vector3d &eye, const vector3d &target, const vector3d &up) {
		vector3d forward = target - eye;
		this->forward = normalized(forward);
		this->right = normalized(cross(up, forward));
		this->up = cross(this->forward, this->right);
		this->position = eye;
	}

	inline void move(const vector3d &dir, const scalar &cnt) {
		this->position += dir * cnt;
	}

	inline void rotate(const vector3d &orbit, const vector3d &dir, scalar ang) {
		matrix3d tmp = rotation(ang, dir);

		this->forward = normalized(vp3(tmp, this->forward));
		this->right = normalized(vp3(tmp, this->right));
		this->up = cross(this->forward, this->right);

		// orbit
		vector3d dir2 = orbit - this->position;
		scalar dist = sqrtf(dp3(dir2, dir2));

		vector3d x = normalized(vph(tmp, dir2));
		tmp = translate(-dist, x);

		this->position = vph(tmp, orbit);
	}

	inline void toGlMatrix(float mat[16]) {
#define glMat(__x, __y) mat[(__x) + 4 * (__y)]
		glMat(0, 0) = this->right.x;
		glMat(0, 1) = this->right.y;
		glMat(0, 2) = this->right.z;
		glMat(0, 3) = dp3(this->right, this->position);
		glMat(1, 0) = this->up.x;
		glMat(1, 1) = this->up.y;
		glMat(1, 2) = this->up.z;
		glMat(1, 3) = dp3(this->up, this->position);
		glMat(2, 0) = this->forward.x;
		glMat(2, 1) = this->forward.y;
		glMat(2, 2) = this->forward.z;
		glMat(2, 3) = dp3(this->forward, this->position);
		glMat(3, 0) = 0;
		glMat(3, 1) = 0;
		glMat(3, 2) = 0;
		glMat(3, 3) = 1;
#undef glMat
	}
};

#endif
