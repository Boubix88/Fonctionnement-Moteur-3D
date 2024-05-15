#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "model.h"

Model::Model(const char *filename) : verts_(), faces_(), texture_(), texture_index_() {
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
            for (int i=0;i<3;i++) iss >> v.raw[i];
            verts_.push_back(v);
        } else if (!line.compare(0, 3, "vt ")) {
            iss >> trash >> trash;
            Vec2f vt;
            for (int i = 0; i < 2; i++) iss >> vt.raw[i];
            texture_.push_back(vt);
        } else if (!line.compare(0, 2, "f ")) {
            std::vector<int> f;
            std::vector<int> vt_indices; // Indices des coordonnées de texture (vt)
            int itrash, idx, vt_idx;
            iss >> trash;
            while (iss >> idx >> trash >> vt_idx >> trash >> itrash) {
                idx--; // -- car indice commence à 1
                vt_idx--;
                f.push_back(idx);
                vt_indices.push_back(vt_idx);
            }
            faces_.push_back(f);
            texture_index_.push_back(vt_indices); // On stocke les indices des coordonnées de texture pour cette face
        }
    }
    std::cerr << "# v# " << verts_.size() << " f# "  << faces_.size() << std::endl;
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