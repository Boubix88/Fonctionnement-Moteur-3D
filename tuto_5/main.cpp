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
const int depth  = 255;
Vec3f eye(1,1,3);
Vec3f center(0,0,0);
Vec3f light_dir = Vec3f(1,-1,1).normalize();

Vec3f matrix2vector(Matrix m) {
    return Vec3f(m[0][0]/m[3][0], m[1][0]/m[3][0], m[2][0]/m[3][0]);
}

Matrix vector2matrix(Vec3f v) {
    Matrix m(4, 1);
    m[0][0] = v.x;
    m[1][0] = v.y;
    m[2][0] = v.z;
    m[3][0] = 1.f;
    return m;
}

Matrix viewport(int x, int y, int w, int h) {
    Matrix m = Matrix::identity(4);
    m[0][3] = x+w/2.f;
    m[1][3] = y+h/2.f;
    m[2][3] = depth/2.f;

    m[0][0] = w/2.f;
    m[1][1] = h/2.f;
    m[2][2] = depth/2.f;
    return m;
}

Matrix lookat(Vec3f eye, Vec3f center, Vec3f up) {
    Vec3f z = (eye-center).normalize();
    Vec3f x = (up^z).normalize();
    Vec3f y = (z^x).normalize();
    Matrix res = Matrix::identity(4);
    for (int i=0; i<3; i++) {
        res[0][i] = x[i];
        res[1][i] = y[i];
        res[2][i] = z[i];
        res[i][3] = -center[i];
    }
    return res;
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
 
void triangle(Vec3f *pts, Vec2f *tex_coords, Vec3f *norm, float *zbuffer, TGAImage &image, TGAImage &texture, TGAImage &normale, Vec3f light_dir, float intensities[3]) { 
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

                    // Récupération des vecteurs de normale pour chaque sommet du triangle
                    Vec3f n0 = norm[0];
                    Vec3f n1 = norm[1];
                    Vec3f n2 = norm[2];

                    // Calcul de l'intensité de la lumière à chaque sommet
                    float intensity0 = n0 * light_dir;
                    float intensity1 = n1 * light_dir;
                    float intensity2 = n2 * light_dir;

                    // Interpolation de l'intensité de la lumière à l'intérieur du triangle
                    float interpolated_intensity = coordBarycentrique.x * intensity0 +
                                                   coordBarycentrique.y * intensity1 +
                                                   coordBarycentrique.z * intensity2;

                    // On vérifie que l'intensité de la lumière reste dans la plage [0, 1]
                    interpolated_intensity = std::max(0.0f, std::min(1.0f, interpolated_intensity));

                    // On applique la texture à l'image avec l'intensité de la lumière
                    /*TGAColor color = texture.get(texture.get_width() - tex_x, texture.get_height() - tex_y);
                    color.r *= interpolated_intensity;
                    color.g *= interpolated_intensity;
                    color.b *= interpolated_intensity;

                    // Affectation de la couleur au pixel dans l'image
                    image.set(P.x, P.y, color);*/

                    // Récupération de la couleur de la texture et application de l'intensité pour le Gouraud Shading
                    TGAColor color(255 * interpolated_intensity, 255 * interpolated_intensity, 255 * interpolated_intensity, 255);

                    // Affectation de la couleur au pixel dans l'image
                    image.set(P.x, P.y, color);
                }
            }
        }
    }
} 


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

    // Normale
    TGAImage normale;
    normale.read_tga_file("texture/african_head_nm.tga");

    float *zbuffer = new float[width*height];

    for (int i = 0; i < width * height; i++) {
        zbuffer[i] = std::numeric_limits<int>::min();
    }

    std::vector<std::vector<Vec2f>> all_tex_coords;
    std::vector<std::vector<Vec2f>> all_norm_coords;

    // Trucs pour la caméra
    Matrix ModelView  = lookat(eye, center, Vec3f(0,1,0));
    Matrix Projection = Matrix::identity(4);
    Matrix ViewPort   = viewport(width/8, height/8, width*3/4, height*3/4);
    Projection[3][2] = -1.f/(eye-center).norm();
    Matrix z = (ViewPort*Projection*ModelView);
    
    // On parcours les faces du modèle
	for (int i = 0; i < model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        Vec3f screen_coords[3];
        Vec3f world_coords[3];
        Vec2f tex_coords[3];
        Vec3f norm_coords[3];
        float intensities[3];

        // On parcours les sommets du triangle
        for (int j = 0; j < 3; j++) {
            Vec3f v = model->vert(face[j]);
            screen_coords[j] =  Vec3f(ViewPort*Projection*ModelView*Matrix(v));
            world_coords[j] = v;
            //std::cout << screen_coords[j].x << " " << screen_coords[j].y << " " << screen_coords[j] << "\n";

            // Coordoonnées de la texture vt dans le modele
            int vt_index = model->texture_index(i, j); // Indice de la coordonnée de texture pour ce sommet (f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3)
            tex_coords[j] = model->texture(vt_index);

            // Coordonnées de la normale vn dans le modele
            //std::cout << "Test d'affichage\n";
            int vn_index = model->normal_index(i, j);
            norm_coords[j] = model->normal(vn_index);

            // On calcul l'intensité de la lumière pour chaque sommet
            intensities[j] = v * light_dir;
        }

        // On dessine le triangle
        triangle(screen_coords, tex_coords, norm_coords, zbuffer, image, texture, normale, light_dir, intensities);
    }

    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("output.tga");
    delete model;
    return 0;
}

