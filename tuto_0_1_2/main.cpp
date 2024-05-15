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

Vec3f barycentric(Vec2i *pts, Vec2i P) { 
    Vec3f u = Vec3f(pts[2].x-pts[0].x, pts[1].x-pts[0].x, pts[0].x-P.x) ^ Vec3f(pts[2].y-pts[0].y, pts[1].y-pts[0].y, pts[0].y-P.y);
    /* `pts` et `P` ont des valeurs entières en tant que coordonnées,
       donc `abs(u[2])` < 1 signifie que `u[2]` est 0, ce qui signifie
       que le triangle est dégénéré, dans ce cas, renvoyer quelque chose avec des coordonnées négatives */
    if (std::abs(u.z) < 1) return Vec3f(-1, 1, 1);
    return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z); 
}

float calculAireTriangle(Vec2i p1, Vec2i p2, Vec2i p3) {
    float minY = std::min(p1.y, std::min(p2.y, p3.y));
    float maxY = std::max(p1.y, std::max(p2.y, p3.y));
    float minX = std::min(p1.x, std::min(p2.x, p3.x));
    float maxX = std::max(p1.x, std::max(p2.x, p3.x));

    float base = maxX - minX;
    float hauteur = maxY - minY;

    return (base * hauteur) /2;
}

Vec3f calculBarycentrique(const Vec2i& A, const Vec2i& B, const Vec2i& C, const Vec2i& P) {
    // Calcul de l'aire du triangle ABC
    float aireABC = (float)((B.x - A.x) * (C.y - A.y) - (C.x - A.x) * (B.y - A.y));

    // Calcul des coordonnées barycentriques
    float alpha = ((B.y - C.y) * (P.x - C.x) + (C.x - B.x) * (P.y - C.y)) / aireABC;
    float beta = ((C.y - A.y) * (P.x - C.x) + (A.x - C.x) * (P.y - C.y)) / aireABC;
    float gamma = 1.0f - alpha - beta;

    return Vec3f(alpha, beta, gamma);
}

// Interpolation linéaire de couleur entre trois couleurs Color1, Color2 et Color3
TGAColor interpolationCouleur(const TGAColor& c1, const TGAColor& c2, const TGAColor& c3, float t1, float t2) {
    // Interpolation linéaire pour chaque composante de couleur (R, G, B)
    float r, g, b;
    if (t1 <= t2) {
        r = (1 - t1) * c1.r + (t1 - t2) * c2.r + t2 * c3.r;
        g = (1 - t1) * c1.g + (t1 - t2) * c2.g + t2 * c3.g;
        b = (1 - t1) * c1.b + (t1 - t2) * c2.b + t2 * c3.b;
    } else {
        r = (1 - t2) * c1.r + t2 * (t2 - t1) * c2.r + (t1 - t2) * c3.r;
        g = (1 - t2) * c1.g + t2 * (t2 - t1) * c2.g + (t1 - t2) * c3.g;
        b = (1 - t2) * c1.b + t2 * (t2 - t1) * c2.b + (t1 - t2) * c3.b;
    }

    // Clamping des valeurs de couleur entre 0 et 255
    r = std::min(std::max(r, 0.f), 255.f);
    g = std::min(std::max(g, 0.f), 255.f);
    b = std::min(std::max(b, 0.f), 255.f);
    //std::cout << "RGB : " << r << " - " << g << " - " << b << "\n";

    return TGAColor((int)r, (int)g, (int)b, 255);
}
 
void triangle(Vec2i *pts, TGAImage &image, TGAColor color1, TGAColor color2, TGAColor color3) { 
    /*Vec2i bboxmin(image.get_width()-1,  image.get_height()-1); 
    Vec2i bboxmax(0, 0); 
    Vec2i clamp(image.get_width()-1, image.get_height()-1); 
    for (int i=0; i<3; i++) { 
        bboxmin.x = std::max(0, std::min(bboxmin.x, pts[i].x));
	bboxmin.y = std::max(0, std::min(bboxmin.y, pts[i].y));

	bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, pts[i].x));
	bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, pts[i].y));
    } 
    Vec2i P; 
    for (P.x=bboxmin.x; P.x<=bboxmax.x; P.x++) { 
        for (P.y=bboxmin.y; P.y<=bboxmax.y; P.y++) { 
            Vec3f bc_screen  = barycentric(pts, P); 
            if (bc_screen.x<0 || bc_screen.y<0 || bc_screen.z<0) continue; 
            image.set(P.x, P.y, color); 
        } 
    } */

    // On recupere la box du triangle
    Vec2i boxMin, boxMax;
    boxMin.y = std::min(pts[0].y, std::min(pts[1].y, pts[2].y));
    boxMax.y = std::max(pts[0].y, std::max(pts[1].y, pts[2].y));
    boxMin.x = std::min(pts[0].x, std::min(pts[1].x, pts[2].x));
    boxMax.x = std::max(pts[0].x, std::max(pts[1].x, pts[2].x));

    // On parcours la box du triangle
    Vec2i P;
    for (P.x = boxMin.x;P.x <= boxMax.x; P.x++) {
        for (P.y = boxMin.y; P.y <= boxMax.y; P.y++) {
            // On calcul les coordonnées barycentriques
            Vec3f coordBarycentrique = calculBarycentrique(pts[0], pts[1], pts[2], P);

            // On vérifie si le point P est à l'intérieur du triangle
            if (coordBarycentrique.x >= 0 && coordBarycentrique.y >= 0 && coordBarycentrique.z >= 0) {
                float t1 = (float)(P.x - boxMin.x) / (float)(boxMax.x - boxMin.x); // On calul t1 qui varie entre 0 et 1 selon P.x
                float t2 = (float)(P.y - boxMin.y) / (float)(boxMax.y - boxMin.y); // On calul t2 qui varie entre 0 et 1 selon P.y
                TGAColor couleurInterpolee = interpolationCouleur(color1, color2, color3, t1, t2);
                image.set(P.x, P.y, couleurInterpolee);
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

    TGAImage image(width, height, TGAImage::RGB);
    /*for (int i=0; i<model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
		Vec2i points[3];

        for (int j=0; j<3; j++) {
            Vec3f v = model->vert(face[j]);
            int x = (v.x+1.)*width/2.;
            int y = (v.y+1.)*height/2.;

			points[j].x = x;
			points[j].y = y;
        }

		triangle(points, image, white);
    }*/

    // Définissez une structure pour stocker les couleurs des sommets
    /*std::unordered_map<std::pair<int, int>, TGAColor, PairHash> vertex_color_map;

    // Parcourez tous les sommets du modèle
    for (int i = 0; i < model->nfaces(); ++i) {
        std::vector<int> face = model->face(i);

        // Pour chaque sommet du triangle
        for (int j = 0; j < 3; ++j) {
            int vertex_index = face[j];
            Vec3f v = model->vert(vertex_index);
            std::pair<int, int> vertex_coords = std::make_pair((int)v.x, (int)v.y);

            // Vérifiez si le sommet a déjà une couleur attribuée
            if (vertex_color_map.find(vertex_coords) == vertex_color_map.end()) {
                // S'il n'a pas de couleur attribuée, attribuez-lui une nouvelle couleur
                TGAColor vertex_color(rand() % 255, rand() % 255, rand() % 255, 255);
                vertex_color_map[vertex_coords] = vertex_color;
            }
        }
    }*/


	Vec3f light_dir(0,0,-1); // define light_dir

	for (int i=0; i<model->nfaces(); i++) { 
		std::vector<int> face = model->face(i); 
		Vec2i screen_coords[3]; 
		Vec3f world_coords[3]; 
        //TGAColor triangle_colors[3]; // Tableau pour stocker les couleurs des sommets du triangle

		for (int j=0; j<3; j++) { 
			Vec3f v = model->vert(face[j]); 
			screen_coords[j] = Vec2i((v.x+1.)*width/2., (v.y+1.)*height/2.); 
			world_coords[j]  = v; 

             // Assignez la couleur du sommet au tableau des couleurs du triangle
            //triangle_colors[j] = vertex_colors[face[j]]; // Utilisez l'indice du sommet pour obtenir sa couleur attribuée
            /*std::pair<int, int> vertex_coords = std::make_pair((int)v.x, (int)v.y);
            triangle_colors[j] = vertex_color_map[vertex_coords];*/
		} 
		Vec3f n = (world_coords[2]-world_coords[0])^(world_coords[1]-world_coords[0]); 
		n.normalize(); 
		float intensity = n*light_dir; 
		if (intensity>0) { 
            triangle(screen_coords, image, TGAColor(rand() % 255 * intensity, rand() % 255 * intensity, rand() % 255 * intensity, 255), TGAColor(rand() % 255 * intensity, rand() % 255 * intensity, rand() % 255 * intensity, 255), TGAColor(rand() % 255 * intensity, rand() % 255 * intensity, rand() % 255 * intensity, 255));
            //triangle(screen_coords, image, triangle_colors[0], triangle_colors[1], triangle_colors[2]);
		}
	}

    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("output.tga");
    delete model;
    return 0;
}

