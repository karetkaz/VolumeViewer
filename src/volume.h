#ifndef VOLUME_H
#define VOLUME_H

#include <cmath>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <functional>
#include <stack>

#define dbgVolume(__MSG) do { /*cout << (__MSG) << endl;*/ } while(false)

using namespace std;

// axis aligned bounding box
struct aabbox {
	int xmin, xmax;
	int ymin, ymax;
	int zmin, zmax;

	aabbox() = default;

	aabbox &addMargin(int x1, int x2, int y1, int y2, int z1, int z2) {
		this->xmin += x1;
		this->xmax -= x2;
		this->ymin += y1;
		this->ymax -= y2;
		this->zmin += z1;
		this->zmax -= z2;
		return *this;
	}

	aabbox &addMargin(int x, int y, int z) {
		return addMargin(x, x, y, y, z, z);
	}

	aabbox &addMargin(int xyz) {
		return addMargin(xyz, xyz, xyz);
	}

	void includePoint(int x, int y, int z) {
		if (this->xmin > x) {
			this->xmin = x;
		}
		if (this->xmax < x) {
			this->xmax = x;
		}
		if (this->ymin > y) {
			this->ymin = y;
		}
		if (this->ymax < y) {
			this->ymax = y;
		}
		if (this->zmin > z) {
			this->zmin = z;
		}
		if (this->zmax < z) {
			this->zmax = z;
		}
	}

	bool checkPoint(int x, int y, int z) {
		if (x < this->xmin || x >= this->xmax) {
			return false;
		}
		if (y < this->ymin || y >= this->ymax) {
			return false;
		}
		if (z < this->zmin || z >= this->zmax) {
			return false;
		}
		return true;
	}
};

template <class voxel> class Volume {
protected:
	// dimensions
	const unsigned sx, sy, sz;

	// positions of sparse elements (if null, not sparse)
	//size_t *positions = nullptr;
	voxel *voxels;
	size_t count;

	// map the position of x, y, z to the index of the array
	size_t position(int x, int y, int z) const {
		static const size_t invalid = static_cast<size_t>(-1);
		if (static_cast<unsigned>(x) >= this->sx) {
			return invalid;
		}
		if (static_cast<unsigned>(y) >= this->sy) {
			return invalid;
		}
		if (static_cast<unsigned>(z) >= this->sz) {
			return invalid;
		}
		size_t position = x + (this->sx * (y + this->sy * z));
		/*if (this->positions != nullptr) {
			// if the volume is sparse, do binary search
			size_t *lo = this->positions;
			size_t *hi = lo + this->count;

			while (lo < hi) {
				size_t *mid = lo + (hi - lo) / 2;
				if (position == *mid) {
					return mid - this->positions;
				}
				if (position > *mid) {
					lo = mid + 1;
				}
				else {
					hi = mid;
				}
			}
			return invalid;
		}*/
		return position;
	}

	unsigned maxDim() {
		return (sx > sy) ? (sx > sz ? sx : sz) : (sy > sz ? sy : sz);
	}

public:
	/**
	 * Construct a new volume with the given dimensions
	 */
	Volume(unsigned x, unsigned y, unsigned z)
		: sx(x), sy(y), sz(z), count((size_t) x * y * z) {
		dbgVolume("ctr.new.vol(sx, sy, sz)");
		this->voxels = new voxel[this->count];
	}

	/**
	 * Construct a new volume with the given dimension
	 */
	explicit Volume(unsigned size)
		: Volume(size, size, size) {
		dbgVolume("ctr.new.vol(size)");
	}

	/**
	 * Construct a new volume and copy data
	 * the new volume will be not sparse, neither if the copied one is.
	 */
	Volume(const Volume &copy)
		: Volume(copy.sx, copy.sy, copy.sz) {
		dbgVolume("ctr.cpy.vol");
		for (size_t pos = 0; pos < copy.count; ++pos) {
			this->voxels[pos] = copy.voxels[pos];
		}
	}

	Volume(Volume &&move) noexcept
		: sx(move.sx), sy(move.sy), sz(move.sz), count(move.count) {
		dbgVolume("ctr.mov.vol");

		this->voxels = move.voxels;
		move.voxels = nullptr;

		this->positions = move.positions;
		move.positions = nullptr;
	}

	/**
	 * Destroy the volume, free up memory
	 */
	virtual ~Volume() {
		dbgVolume("dtr.vol");
		delete[] this->voxels;
	}

	void save(const string &fileName, const function<bool(voxel value)> &sparse = nullptr) {
		ofstream out(fileName, ios::binary);
		if (!out) {
			throw runtime_error("Failed to open file: " + fileName);
		}

		uint16_t sx = this->sx, sy = this->sy, sz = this->sz;
		uint64_t count = this->count;

		size_t *positions = nullptr;
		if (sparse != nullptr) {
			count = 0;
			positions = new size_t[this->count];
			for (size_t pos = 0; pos < this->count; ++pos) {
				if (sparse(this->voxels[pos])) {
					positions[count] = pos;
					count += 1;
				}
			}
		}

		out.write((char *) &sx, sizeof(sx));
		out.write((char *) &sy, sizeof(sy));
		out.write((char *) &sz, sizeof(sz));
		out.write((char *) &count, sizeof(count));

		if (count < (size_t) sx * sy * sx) {
			out.write((char *) positions, count * sizeof(*positions));
			for (size_t pos = 0; pos < count; ++pos) {
				this->voxels[positions[pos]].write(out);
			}
		} else {
			for (size_t pos = 0; pos < count; ++pos) {
				this->voxels[pos].write(out);
			}
		}

		delete[] positions;
	}

	void open(const string &fileName) {
		ifstream in(fileName, ios::binary);
		if (!in) {
			throw runtime_error("Failed to open file: " + fileName);
		}

		uint16_t sx, sy, sz;
		uint64_t count;

		in.read((char *) &sx, sizeof(sx));
		in.read((char *) &sy, sizeof(sy));
		in.read((char *) &sz, sizeof(sz));
		in.read((char *) &count, sizeof(count));

		Volume *resize = this;
		if (sx != this->sx || sy != this->sy || sz != this->sz) {
			resize = new Volume(sx, sy, sz);
		}

		resize->fill(voxel::zero);
		if (count < (size_t) sx * sy * sz) {
			size_t *positions = new size_t[count];
			in.read((char *) positions, count * sizeof(*positions));
			for (size_t pos = 0; pos < count; ++pos) {
				resize->voxels[positions[pos]].read(in);
			}
			delete[] positions;
		}
		else {
			for (size_t pos = 0; pos < count; ++pos) {
				resize->voxels[pos].read(in);
			}
		}

		if (resize != this) {
			resize->resize(*this, 1);
			delete resize;
		}
	}

	inline int width() const { return sx; }

	inline int height() const { return sy; }

	inline int depth() const { return sz; }

	inline unsigned voxelCount() const { return sx * sy * sz; }

	voxel get(int x, int y, int z) const {
		size_t position = this->position(x, y, z);
		if (position >= this->count) {
			return voxel::zero;
		}
		return this->voxels[position];
	}

	void set(int x, int y, int z, voxel value) {
		size_t position = this->position(x, y, z);
		if (position >= this->count) {
			return;
		}
		this->voxels[position] = value;
	}

	void fill(voxel value) {
		for (size_t i = 0; i < this->count; ++i) {
			this->voxels[i] = value;
		}
	}

	void floodFill(int x, int y, int z, int max, float threshold, voxel fill) {
		static const int dx[6] = {0, 0, 1, 0, 0, -1}; // relative neighbor x coordinates
		static const int dy[6] = {0, 1, 0, 0, -1, 0}; // relative neighbor y coordinates
		static const int dz[6] = {1, 0, 0, -1, 0, 0}; // relative neighbor z coordinates

		const int ox = x;
		const int oy = y;
		const int oz = z;

		const unsigned sx = this->width();
		const unsigned sy = this->height();
		const unsigned sz = this->depth();

		if (static_cast<unsigned>(x) >= sx) {
			return;
		}
		if (static_cast<unsigned>(y) >= sy) {
			return;
		}
		if (static_cast<unsigned>(z) >= sz) {
			return;
		}
		voxel current = this->get(x, y, z);
		if (current.equals(fill, 0)) {
			return;
		}

		stack<tuple<int, int, int>> s;
		s.push(make_tuple(x, y, z));

		while (!s.empty()) {
			x = std::get<0>(s.top());
			y = std::get<1>(s.top());
			z = std::get<2>(s.top());
			s.pop();

			this->set(x, y, z, fill);
			for (int i = 0; i < 6; i++) {
				int nx = x + dx[i];
				int ny = y + dy[i];
				int nz = z + dz[i];

				if (static_cast<unsigned>(nx) >= sx) {
					continue;
				}
				if (static_cast<unsigned>(ny) >= sy) {
					continue;
				}
				if (static_cast<unsigned>(nz) >= sz) {
					continue;
				}
				voxel next = this->get(nx, ny, nz);
				if (next.equals(fill, 0)) {
					continue;
				}
				if (!next.equals(current, threshold)) {
					continue;
				}
				int dx = nx - ox;
				int dy = ny - oy;
				int dz = nz - oz;
				if (dx * dx + dy * dy + dz * dz < max * max) {
					s.push(make_tuple(nx, ny, nz));
				}
			}
		}
	}

	void forEach(const function<void(int x, int y, int z)> &action) const {
		for (size_t pos = 0; pos < this->count; ++pos) {
			size_t position = pos;
			int x = static_cast<int>(position % this->sx);
			position /= this->sx;
			int y = static_cast<int>(position % this->sy);
			position /= this->sy;
			int z = static_cast<int>(position % this->sz);
			action(x, y, z);
		}
	}

	void forEach(const function<void(voxel &value)> &action) const {
		for (size_t pos = 0; pos < this->count; ++pos) {
			action(this->voxels[pos]);
		}
	}

	aabbox bounds(const function<bool(voxel value)> &accept) const {
		aabbox result;
		result.xmin = 0;
		result.xmax = 0;
		result.ymin = 0;
		result.ymax = 0;
		result.zmin = 0;
		result.zmax = 0;

		this->forEach([&result, &accept, this](int x, int y, int z) {
			if (accept(this->voxels[this->position(x, y, z)])) {
				result.includePoint(x, y, z);
			}
		});

		result.xmax += 1;
		result.ymax += 1;
		result.zmax += 1;
		return result;
	}

	aabbox bounds() const {
		aabbox result;
		result.xmin = 0;
		result.xmax = this->sx;
		result.ymin = 0;
		result.ymax = this->sy;
		result.zmin = 0;
		result.zmax = this->sz;
		return result;
	}

	void resize(Volume<voxel> &dst, int linear) const {
		if (linear == 0) {
			unsigned dx = ((this->sx - 0) << 16) / dst.sx;
			unsigned dy = ((this->sy - 0) << 16) / dst.sy;
			unsigned dz = ((this->sz - 0) << 16) / dst.sz;
			for (unsigned z = 0, sz = dz / 2; z < dst.sz; ++z, sz += dz) {
				for (unsigned y = 0, sy = dz / 2; y < dst.sy; ++y, sy += dy) {
					for (unsigned x = 0, sx = dx / 2; x < dst.sx; ++x, sx += dx) {
						dst.set(x, y, z, this->get(sx >> 16, sy >> 16, sz >> 16));
					}
				}
			}
			return;
		}

		unsigned dx = ((this->sx - 1) << 16) / dst.sx;
		unsigned dy = ((this->sy - 1) << 16) / dst.sy;
		unsigned dz = ((this->sz - 1) << 16) / dst.sz;
		Volume<voxel> *mip = (Volume<voxel> *)this;
		if (dx > 0x20000 || dy > 0x20000 || dz > 0x20000) {
			mip = new Volume(*this);
			while (dx > 0x20000) {
				for (unsigned z = 0; z < mip->sz; ++z) {
					for (unsigned y = 0; y < mip->sy; ++y) {
						for (unsigned x = 0; x < mip->sx; ++x) {
							voxel x0 = mip->get(x * 2 + 0, y, z);
							voxel x1 = mip->get(x * 2 + 1, y, z);
							x0.mix(x1, .5f);
							mip->set(x, y, z, x0);
						}
					}
				}
				dx >>= 1;
			}
			while (dy > 0x20000) {
				for (unsigned z = 0; z < mip->sz; ++z) {
					for (unsigned y = 0; y < mip->sy; ++y) {
						for (unsigned x = 0; x < mip->sx; ++x) {
							voxel y0 = mip->get(x, y * 2 + 0, z);
							voxel y1 = mip->get(x, y * 2 + 1, z);
							y0.mix(y1, .5f);
							mip->set(x, y, z, y0);
						}
					}
				}
				dy >>= 1;
			}
			while (dz > 0x20000) {
				for (unsigned z = 0; z < mip->sz; ++z) {
					for (unsigned y = 0; y < mip->sy; ++y) {
						for (unsigned x = 0; x < mip->sx; ++x) {
							voxel z0 = mip->get(x, y, z * 2 + 0);
							voxel z1 = mip->get(x, y, z * 2 + 1);
							z0.mix(z1, .5f);
							mip->set(x, y, z, z0);
						}
					}
				}
				dz >>= 1;
			}
		}

		for (unsigned z = 0, sz = dz / 2; z < dst.sz; ++z, sz += dz) {
			unsigned hz = sz >> 16;
			float lz = (sz & 0xffff) / 65536.f;
			for (unsigned y = 0, sy = dy / 2; y < dst.sy; ++y, sy += dy) {
				unsigned hy = sy >> 16;
				float ly = (sy & 0xffff) / 65536.f;
				for (unsigned x = 0, sx = dx / 2; x < dst.sx; ++x, sx += dx) {
					unsigned hx = sx >> 16;
					float lx = (sx & 0xffff) / 65536.f;

					voxel x0y0z0 = mip->get(hx + 0, hy + 0, hz + 0);
					voxel x0y0z1 = mip->get(hx + 0, hy + 0, hz + 1);
					voxel x0y1z0 = mip->get(hx + 0, hy + 1, hz + 0);
					voxel x0y1z1 = mip->get(hx + 0, hy + 1, hz + 1);
					voxel x1y0z0 = mip->get(hx + 0, hy + 0, hz + 0);
					voxel x1y0z1 = mip->get(hx + 0, hy + 0, hz + 1);
					voxel x1y1z0 = mip->get(hx + 0, hy + 1, hz + 0);
					voxel x1y1z1 = mip->get(hx + 0, hy + 1, hz + 1);

					x0y0z0.mix(x0y0z1, lz);
					x0y1z0.mix(x0y1z1, lz);
					x1y0z0.mix(x1y0z1, lz);
					x1y1z0.mix(x1y1z1, lz);

					x0y0z0.mix(x0y1z0, ly);
					x1y0z0.mix(x1y1z0, ly);

					x0y0z0.mix(x1y0z0, lx);

					dst.set(x, y, z, x0y0z0);
				}
			}
		}
		if (mip != this) {
			delete mip;
		}
	}
};

#endif
