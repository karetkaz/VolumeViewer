#ifndef VOLUME_FILTER_H
#define VOLUME_FILTER_H

#include "volume.h"

#define dbgKernel(__MSG) do { cout << (__MSG) << endl; } while(false)
template <class voxel> class Kernel: public Volume<voxel> {
	struct Voxels {
		voxel x, y, z;
	};
	Voxels *separable;
	const signed cx, cy, cz;

public:
	Kernel(unsigned sx, unsigned sy, unsigned sz, int cx, int cy, int cz)
		: Volume<voxel>(sx, sy, sz), separable(nullptr), cx(cx), cy(cy), cz(cz) {
		dbgVolume("ctr.new.ker(sx, sy, sz, cx, cy, cz)");
	}

	Kernel(unsigned sx, unsigned sy, unsigned sz)
		: Kernel(sx, sy, sz, sx / 2, sy / 2, sz / 2) {
		dbgVolume("ctr.new.ker(sx, sy, sz)");
	}

	explicit Kernel(unsigned size)
		: Kernel(size, size, size) {
		dbgVolume("ctr.new.ker(size)");
	}

	Kernel(const Kernel &copy)
		: Volume<voxel>(copy), cx(copy.cx), cy(copy.cy), cz(copy.cz) {
		dbgVolume("ctr.cpy.ker");
	}

	Kernel(Kernel &&move) noexcept
		: Volume<voxel>(std::move(move)), cx(move.cx), cy(move.cy), cz(move.cz) {
		dbgVolume("ctr.mov.ker");
	}

	~Kernel() {
		if (separable != nullptr) {
			delete[]separable;
		}
		dbgVolume("dtr.ker");
	}

	void fill(voxel value, bool separable = true) {
		const unsigned sx = this->sx;
		const unsigned sy = this->sy;
		const unsigned sz = this->sz;

		for (unsigned z = 0; z < sz; ++z) {
			for (unsigned y = 0; y < sy; ++y) {
				for (unsigned x = 0; x < sx; ++x) {
					this->set(x, y, z, value);
				}
			}
		}

		makeNonSeparable();
		if (separable) {
			int size = Volume<voxel>::maxDim();
			this->separable = new Voxels[size];
			for (int i = 0; i < size; ++i) {
				this->separable[i].x = cbrt(value);
				this->separable[i].y = cbrt(value);
				this->separable[i].z = cbrt(value);
			}
		}
	}

	Kernel &fillDisk(voxel value) {
		const unsigned sx = this->sx;
		const unsigned sy = this->sy;
		const unsigned sz = this->sz;
		const signed cx = this->cx;
		const signed cy = this->cy;
		const signed cz = this->cz;

		for (unsigned z = 0; z < sz; ++z) {
			for (unsigned y = 0; y < sy; ++y) {
				for (unsigned x = 0; x < sx; ++x) {
					int dx = x - cx;
					int dy = y - cy;
					int dz = z - cz;
					if (dx * dx / (sx * sx / 4.) + dy * dy / (sy * sy / 4.) + dz * dz / (sz * sz / 4.) < 1) {
						this->set(x, y, z, value);
					}
				}
			}
		}
		makeNonSeparable();
		return *this;
	}

	Kernel &fillCross(voxel value) {
		const unsigned sx = this->sx;
		const unsigned sy = this->sy;
		const unsigned sz = this->sz;
		const unsigned cx = this->cx;
		const unsigned cy = this->cy;
		const unsigned cz = this->cz;

		for (unsigned z = 0; z < sz; ++z) {
			for (unsigned y = 0; y < sy; ++y) {
				for (unsigned x = 0; x < sx; ++x) {
					int ix = x == cx;
					int iy = y == cy;
					int iz = z == cz;
					if (ix + iy + iz == 2) {
						this->set(x, y, z, value);
					}
				}
			}
		}
		makeNonSeparable();
		return *this;
	}

	Kernel &fillDiamond(voxel value) {
		const unsigned sx = this->sx;
		const unsigned sy = this->sy;
		const unsigned sz = this->sz;
		const signed cx = this->cx;
		const signed cy = this->cy;
		const signed cz = this->cz;

		for (unsigned z = 0; z < sz; ++z) {
			for (unsigned y = 0; y < sy; ++y) {
				for (unsigned x = 0; x < sx; ++x) {
					int dx = x - cx;
					int dy = y - cy;
					int dz = z - cz;
					if (abs(dx) / (sx / 2.) + abs(dy) / (sy / 2.) + abs(dz) / (sz / 2.) < 1) {
						this->set(x, y, z, value);
					}
				}
			}
		}

		makeNonSeparable();
		return *this;
	}

	Kernel &fillIdentity(voxel value) {
		const unsigned sx = this->sx;
		const unsigned sy = this->sy;
		const unsigned sz = this->sz;

		for (unsigned z = 0; z < sz; ++z) {
			for (unsigned y = 0; y < sy; ++y) {
				for (unsigned x = 0; x < sx; ++x) {
					if (x == y && y == z) {
						this->set(x, y, z, value);
					}
				}
			}
		}
		makeNonSeparable();
		return *this;
	}

	Kernel &fillEdgeDetect(int direction) {
		if (Volume<voxel>::maxDim() > 1024) {
			throw runtime_error("kernel size too large");
		}

		makeNonSeparable();
		this->separable = new Voxels[Volume<voxel>::maxDim()];
		for (size_t i = 0; i < Volume<voxel>::maxDim(); i++) {
			int x = detectedge(i, this->sx, this->cx, direction == 0);
			int y = detectedge(i, this->sy, this->cy, direction == 1);
			int z = detectedge(i, this->sz, this->cz, direction == 2);
			this->separable[i].x = voxel(x);
			this->separable[i].y = voxel(y);
			this->separable[i].z = voxel(z);
		}

		for (size_t z = 0; z < this->sz; ++z) {
			voxel dz = this->separable[z].z;
			for (size_t y = 0; y < this->sy; ++y) {
				voxel dy = this->separable[y].y;
				for (size_t x = 0; x < this->sx; ++x) {
					voxel dx = this->separable[x].x;
					this->set(x, y, z, dx * dy * dz);
				}
			}
		}
		return *this;
	}

	Kernel &fillGauss(const double sigma, int dx = 0, int dy = 0, int dz = 0, bool separable = true) {
		double kernel_x[1024];
		double kernel_y[1024];
		double kernel_z[1024];

		if (Volume<voxel>::maxDim() > 1024) {
			throw runtime_error("kernel size too large");
		}
		for (size_t i = 0; i < Volume<voxel>::maxDim(); i++) {
			kernel_x[i] = kernel_y[i] = kernel_z[i] = 0;
		}

		for (size_t i = 0; i < this->sx; i++) {
			double t = ((double)i - this->cx);
			kernel_x[i] = gauss(-t, sigma, dx);
		}
		for (size_t i = 0; i < this->sy; i++) {
			double t = ((double)i - this->cy);
			kernel_y[i] = gauss(-t, sigma, dy);
		}
		for (size_t i = 0; i < this->sz; i++) {
			double t = ((double)i - this->cz);
			kernel_z[i] = gauss(-t, sigma, dz);
		}

		// FIXME: normalization might be needed
		for (size_t z = 0; z < this->sz; ++z) {
			double dz = kernel_z[z];
			for (size_t y = 0; y < this->sy; ++y) {
				double dy = kernel_y[y];
				for (size_t x = 0; x < this->sx; ++x) {
					double dx = kernel_x[x];
					this->set(x, y, z, voxel(dx * dy * dz));
				}
			}
		}

		makeNonSeparable();
		if (separable) {
			this->separable = new Voxels[Volume<voxel>::maxDim()];
			for (size_t i = 0; i < Volume<voxel>::maxDim(); ++i) {
				this->separable[i].x = voxel(kernel_x[i]);
				this->separable[i].y = voxel(kernel_y[i]);
				this->separable[i].z = voxel(kernel_z[i]);
			}
		}
		return *this;
	}

	void filter(const Volume<voxel> &volume, Volume<voxel> &output) {

		if (this->isSeparable(1e-6)) {
			Volume<voxel> temp(output.width(), output.height(), output.depth());
			aabbox bounds = volume.bounds([](voxel value) { return value != voxel::zero; });

			// x direction: input -> output
			for (int z = bounds.zmin; z < bounds.zmax; ++z) {
				for (int y = bounds.ymin; y < bounds.ymax; ++y) {
					for (int x = bounds.xmin; x < bounds.xmax; ++x) {
						voxel value = voxel::zero;
						for (unsigned i = 0; i < this->sx; ++i) {
							int _x = x + i - this->cx;
							if (_x < bounds.xmin || _x >= bounds.xmax) {
								continue;
							}
							value += this->separable[i].x * volume.get(_x, y, z);
						}
						output.set(x, y, z, value);
					}
				}
			}

			// y direction: output -> temp
			for (int z = bounds.zmin; z < bounds.zmax; ++z) {
				for (int y = bounds.ymin; y < bounds.ymax; ++y) {
					for (int x = bounds.xmin; x < bounds.xmax; ++x) {
						voxel value = voxel::zero;
						for (unsigned i = 0; i < this->sy; ++i) {
							int _y = y + i - this->cy;
							if (_y < bounds.ymin || _y >= bounds.ymax) {
								continue;
							}
							value += this->separable[i].y * output.get(x, _y, z);
						}
						temp.set(x, y, z, value);
					}
				}
			}

			// z direction: temp -> output
			for (int z = bounds.zmin; z < bounds.zmax; ++z) {
				for (int y = bounds.ymin; y < bounds.ymax; ++y) {
					for (int x = bounds.xmin; x < bounds.xmax; ++x) {
						voxel value = voxel::zero;
						for (unsigned i = 0; i < this->sz; ++i) {
							int _z = z + i - this->cz;
							if (_z < bounds.zmin || _z >= bounds.zmax) {
								continue;
							}
							value += this->separable[i].z * temp.get(x, y, _z);
						}
						output.set(x, y, z, value);
					}
				}
			}
			return;
		}

		dbgKernel("filter.not.separable");
		return filter(volume, output, [](size_t count, voxel values[]) {
			voxel result = voxel::zero;
			for (unsigned i = 0; i < count; i++) {
				result += values[i];
			}
			return result;
		});
	}

	void erode(const Volume<voxel> &volume, Volume<voxel> &output) {
		return filter(volume, output, [](size_t count, voxel values[]) {
			return *min_element(values, values + count);
		});
	}

	void dilate(const Volume<voxel> &volume, Volume<voxel> &output) {
		return filter(volume, output, [](size_t count, voxel values[]) {
			return *max_element(values, values + count);
		});
	}

	void median(const Volume<voxel> &volume, Volume<voxel> &output) {
		return filter(volume, output, [](size_t count, voxel values[]) {
			nth_element(values, values + count / 2, values + count);
			return values[count / 2];
		});
	}

private: // helper methods
	inline bool isSeparable(float epsilon) {
		if (this->separable == nullptr) {
			return false;
		}
		for (size_t z = 0; z < this->sz; ++z) {
			voxel Z = this->separable[z].z;
			for (size_t y = 0; y < this->sy; ++y) {
				voxel Y = this->separable[y].y;
				for (size_t x = 0; x < this->sx; ++x) {
					voxel X = this->separable[x].x;
					if (!this->get(x, y, z).equals(X * Y * Z, epsilon)) {
						return false;
					}
				}
			}
		}
		return true;
	}

	inline void makeNonSeparable() {
		if (this->separable != nullptr) {
			delete[] this->separable;
			this->separable = nullptr;
		}
	}

	void filter(const Volume<voxel> &volume, Volume<voxel> &output, const function<voxel(size_t count, voxel values[])> &action) {
		// speed test: box filter (7x7x7)
		// filter.lambda(time: 21.42 sec)
		// filter.inline(time: 21.53 sec)

		voxel *values = new voxel[this->count];
		aabbox bounds = volume.bounds([](voxel value) {
			return value != voxel::zero;
		});
		for (int dz = bounds.zmin; dz <= bounds.zmax; ++dz) {
			for (int dy = bounds.ymin; dy <= bounds.ymax; ++dy) {
				for (int dx = bounds.xmin; dx <= bounds.xmax; ++dx) {
					int offs = 0;
					for (unsigned kz = 0; kz < this->sz; ++kz) {
						int sz = dz + kz - this->cz;
						if (sz < bounds.zmin || sz >= bounds.zmax) {
							continue;
						}
						for (unsigned ky = 0; ky < this->sy; ++ky) {
							int sy = dy + ky - this->cy;
							if (sy < bounds.ymin || sy >= bounds.ymax) {
								continue;
							}
							for (unsigned kx = 0; kx < this->sx; ++kx) {
								int sx = dx + kx - this->cx;
								if (sx < bounds.xmin || sx >= bounds.xmax) {
									continue;
								}
								values[offs] = this->get(kx, ky, kz) * volume.get(sx, sy, sz);
								offs++;
							}
						}
					}
					output.set(dx, dy, dz, action(offs, values));
				}
			}
		}
		delete[] values;
	}

	static double gauss(double x, double sigma, int dx) {
		constexpr double SQRT_2_PI_INV = 0.398942280401432677939946059935;
		double t = x / sigma;
		switch (dx) {
			default:
				break;
			case -1:
				return 1;
			case 0:
				return SQRT_2_PI_INV * exp(-0.5*t*t) / sigma;
			case 1:
				return -x * SQRT_2_PI_INV * exp(-0.5*t*t) / (sigma * sigma * sigma);
			case 2:
				return (x * x - sigma * sigma) * SQRT_2_PI_INV * exp(-0.5*t*t) / (sigma * sigma * sigma * sigma * sigma);
		}
		return 0;
	}

	static int detectedge(int x, int max, int mid, bool check) {
		if (!check) {
			return 1;
		}
		if (x > max) {
			return 0;
		}
		if (x > mid) {
			return 1;
		}
		if (x < mid) {
			return -1;
		}
		return 0;
	}
};

#endif
