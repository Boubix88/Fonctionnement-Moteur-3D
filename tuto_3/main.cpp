#include <vector>
#include <cmath>
#include <iostream>
#include <unordered_map>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
Model *model = NULL;
const int width  = 800;
const int height = 800;

void line(Vec2i pt1, Vec2i pt2, TGAImage &image, TGAColor color) {
    bool steep = false;
    if (std::abs(pt1.x-pt2.x)<std::abs(pt1.y-pt2.y)) {
        std::swap(pt1.x, pt1.y);
        std::swap(pt2.x, pt2.y);
        steep = true;
    }
    if (pt1.x>pt2.x) {
        std::swap(pt1.x, pt2.x);
        std::swap(pt1.y, pt2.y);
    }

    for (int x=pt1.x; x<=pt2.x; x++) {
        float t = (x-pt1.x)/(float)(pt2.x-pt1.x);
        int y = pt1.y*(1.-t) + pt2.y*t;
        if (steep) {
            image.set(y, x, color);
        } else {
            image.set(x, y, color);
        }
    }
}

Vec3f calculBarycentrique(const Vec3f& A, const Vec3f& B, const Vec3f& C, const Vec3f& P) {
    // Calcul de l'aire du triangle ABC
    float aireABC = (float)((B.x - A.x) * (C.y - A.y) - (C.x - A.x) * (B.y - A.y));

    // Calcul des coordonnées barycentriques
    float alpha = ((B.y - C.y) * (P.x - C.x) + (C.x - B.x) * (P.y - C.y)) / aireABC;
    float beta = ((C.y - A.y) * (P.x - C.x) + (A.x - C.x) * (P.y - C.y)) / aireABC;
    float gamma = 1.0f - alpha - beta;

    return Vec3f(alpha, beta, gamma);
}

Vec2f interpolationTexture(const Vec2f& t1, const Vec2f& t2, const Vec2f& t3, float alpha, float beta, float gamma) {
    float u = alpha * t1.x + beta * t2.x + gamma * t3.x;
    float v = alpha * t1.y + beta * t2.y + gamma * t3.y;
    return Vec2f(u, v);
}
 
void triangle(Vec3f *pts, Vec2f *tex_coords, float *zbuffer, TGAImage &image, TGAImage &texture, float intensity) { 
    // On recupere la box du triangle
    Vec2i boxMin, boxMax;
    boxMin.y = std::min(pts[0].y, std::min(pts[1].y, pts[2].y));
    boxMax.y = std::max(pts[0].y, std::max(pts[1].y, pts[2].y));
    boxMin.x = std::min(pts[0].x, std::min(pts[1].x, pts[2].x));
    boxMax.x = std::max(pts[0].x, std::max(pts[1].x, pts[2].x));

    // On parcours la box du triangle
    Vec3f P;
    for (P.x = boxMin.x;P.x <= boxMax.x; P.x++) {
        for (P.y = boxMin.y; P.y <= boxMax.y; P.y++) {
            // On calcul les coordonnées barycentriques
            Vec3f coordBarycentrique = calculBarycentrique(pts[0], pts[1], pts[2], P);

            // On vérifie si le point P est à l'intérieur du triangle
            if (coordBarycentrique.x > 0 && coordBarycentrique.y > 0 && coordBarycentrique.z > 0) {
                P.z = 0.0;
                // On récupère la profondeur du triangle
                for (int i = 0; i < 3; i++) {
                    if (i == 0) P.z += pts[i].z * coordBarycentrique.x;
                    if (i == 1) P.z += pts[i].z * coordBarycentrique.y;
                    if (i == 2) P.z += pts[i].z * coordBarycentrique.z;
                }

                // On regarde le buffer z est inferieur au z du triangle
                if (zbuffer[int(P.x+P.y*width)] <= P.z) {
                    zbuffer[int(P.x+P.y*width)] = P.z;

                    // Interpolation des coordonnées de texture à l'intérieur du triangle
                    Vec2f tex_coord = interpolationTexture(tex_coords[0], tex_coords[1], tex_coords[2], coordBarycentrique.x, coordBarycentrique.y, coordBarycentrique.z);

                    // Calcul des coordonnées dans l'image de texture
                    int tex_x = int(tex_coord.x * texture.get_width());
                    int tex_y = int(tex_coord.y * texture.get_height());
                    //std::cout << "Coordonnées texture " << tex_x << " " << tex_y << "\n";

                    // On récupère du pixel sur la texture et on l'applique par rapport à l'intensité
                    TGAColor color = texture.get(texture.get_width() - tex_x, texture.get_height() - tex_y); // soustraction sinon texture inversée
                    color.r *= intensity;
                    color.g *= intensity;
                    color.b *= intensity;
                    image.set(P.x, P.y, color);
                }
            }
        }
    }
} 

// Définition d'un foncteur de hachage pour les paires d'entiers
struct PairHash {
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1, T2> &pair) const {
        auto hash1 = std::hash<T1>{}(pair.first);
        auto hash2 = std::hash<T2>{}(pair.second);
        return hash1 ^ hash2;
    }
};


int main(int argc, char** argv) {
    if (2==argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("obj/african_head.obj");
    }

    // Image resultante
    TGAImage image(width, height, TGAImage::RGB);

    // Texture
    TGAImage texture; 
    texture.read_tga_file("texture/african_head_diffuse.tga");

	Vec3f light_dir(0,0,-1); // define light_dir

    float *zbuffer = new float[width*height];

    for (int i = 0; i < width * height; i++) {
        zbuffer[i] = std::numeric_limits<int>::min();
    }

    std::vector<std::vector<Vec2f>> all_tex_coords;

	for (int i = 0; i < model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        Vec3f screen_coords[3];
        Vec3f world_coords[3];
        Vec2f tex_coords[3];

        for (int j = 0; j < 3; j++) {
            Vec3f v = model->vert(face[j]);
            screen_coords[j] = Vec3f((v.x + 1.) * width / 2., (v.y + 1.) * height / 2., v.z);
            world_coords[j] = v;

            // Coordoonnées de la texture vt dans le modele
            int vt_index = model->texture_index(i, j); // Indice de la coordonnée de texture pour ce sommet (f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3)
            tex_coords[j] = model->texture(vt_index);
            //std::cout << "vt " << vt_index << "\n";
        }
        Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
        n.normalize();
        float intensity = n * light_dir;
        if (intensity > 0) {
            triangle(screen_coords, tex_coords, zbuffer, image, texture, intensity);
        }
    }

    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("output.tga");
    delete model;
    return 0;
}

