#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"

class Model {
private:
	std::vector<Vec3f> verts_;
	std::vector<std::vector<int> > faces_;
	std::vector<Vec2f> texture_;
	std::vector<std::vector<int>> texture_index_;
	std::vector<Vec3f> normal_;
	std::vector<std::vector<int>> normal_index_;
public:
	Model(const char *filename);
	~Model();
	int nverts();
	int nfaces();
	Vec3f vert(int i);
	std::vector<int> face(int idx);
	Vec2f texture(int i);
	int texture_index(int face_idx, int vert_idx);
	Vec3f normal(int i);
	int normal_index(int face_idx, int vert_idx);
};

#endif //__MODEL_H__
