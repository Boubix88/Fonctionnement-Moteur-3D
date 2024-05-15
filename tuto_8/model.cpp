#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "model.h"

Model::Model(const char *filename) : verts_(), faces_(), texture_(), texture_index_(), normal_(), normal_index_() {
    std::ifstream in;
    in.open (filename, std::ifstream::in);
    if (in.fail()) return;
    std::string line;
    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        char trash;
        if (!line.compare(0, 2, "v ")) {
            iss >> trash;
            Vec3f v;
            for (int i=0;i<3;i++) iss >> v[i];
            verts_.push_back(v);
        } else if (!line.compare(0, 3, "vt ")) { // Coordonnées textures
            iss >> trash >> trash;
            Vec2f vt;
            for (int i = 0; i < 2; i++) iss >> vt[i];
            texture_.push_back(vt);
        }else if (!line.compare(0, 3, "vn ")) { // Coordonnées normales
            iss >> trash >> trash;
            Vec3f vn;
            for (int i = 0; i < 3; i++) iss >> vn[i];
            normal_.push_back(vn);
        } else if (!line.compare(0, 2, "f ")) {
            std::vector<int> f;
            std::vector<int> vt_indices; // Indices des coordonnées de texture (vt)
            std::vector<int> vn_indices; // Indices des coordonnées de texture (vn)
            int idx, vt_idx, vn_idx;
            iss >> trash;
            while (iss >> idx >> trash >> vt_idx >> trash >> vn_idx) {
                idx--; // -- car indice commence à 1
                vt_idx--;
                vn_idx--;
                f.push_back(idx);
                vt_indices.push_back(vt_idx);
                vn_indices.push_back(vn_idx);
            }
            faces_.push_back(f);
            texture_index_.push_back(vt_indices); // On stocke les indices des coordonnées de texture pour cette face
            normal_index_.push_back(vn_indices); // On stocke les indices des coordonnées de normale pour cette face
        }
    }
    std::cerr << "# v# " << verts_.size() << " f# "  << faces_.size() << " vt# " << texture_.size() << " vn# " << normal_.size() << std::endl;
}

Model::~Model() {
}

int Model::nverts() {
    return (int)verts_.size();
}

int Model::nfaces() {
    return (int)faces_.size();
}

std::vector<int> Model::face(int idx) {
    return faces_[idx];
}

Vec3f Model::vert(int i) {
    return verts_[i];
}

Vec2f Model::texture(int i) {
    return texture_[i];
}

int Model::texture_index(int face_idx, int vert_idx) {
    // Renvoie l'indice de la coordonnée de texture pour le vert_idx-ème sommet de la face_idx-ème face
    return texture_index_[face_idx][vert_idx]; // Modifiez l'index du vecteur texture_index_
}

Vec3f Model::normal(int i) {
    return normal_[i];
}

int Model::normal_index(int face_idx, int vert_idx) {
    // Renvoie l'indice de la coordonnée de normale pour le vert_idx-ème sommet de la face_idx-ème face.
    return normal_index_[face_idx][vert_idx]; // Modifiez l'index du vecteur normal_index_
}