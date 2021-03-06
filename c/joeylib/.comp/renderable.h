#ifndef renderable_H
  #define renderable_H


// General interpolation interpolates on screen and in space and uses
// texture call to evaluate colour at each pixel

// class Texture declared at top of RGBmp

// ImageType and ColourType define in jfont.c

#ifdef USINGJFONT

class Renderable {
public:
  V3d cen; // Exists
  float size; // Exists
  Texture *tex; // Exists
  virtual V3d pos(); // Method

  virtual Renderable translated(V3d t); // Method

  virtual Renderable rotated(V3d ax,float ang); // Method

  virtual Renderable operator*(Matrix m); // Method

  virtual void render(JBmp *j,Viewpoint *v); // Method

  virtual void render(RGBmp *j,Viewpoint *v); // Method

  virtual void renderwireframe(ImageType *p,Viewpoint *v); // Method

};

Renderable operator*(Matrix m,Renderable r); // Method


class Sphere : public Renderable {
public:
  float rad; // Exists
   Sphere(V3d c,float r); // Method

  Renderable translated(V3d t); // Method

  Renderable rotated(V3d ax,float ang); // Method

  Renderable operator*(Matrix m); // Method

  void render(JBmp *j,Viewpoint *v); // Method

  void renderwireframe(ImageType *ima,Viewpoint *v); // Method

};

class Disc : public Renderable {
public:
  V3d norm; // Exists
  float rad; // Exists
   Disc(); // Method

   Disc(V3d pos,V3d nor,float f); // Method

  void renderwireframe(ImageType *p,Viewpoint *v); // Method

};

class LineRend : public Renderable {
public:
  V3d a,b; // Exists
   LineRend(V3d aa,V3d bb); // Method

  void renderwireframe(ImageType *p,Viewpoint *v); // Method

};

class Cylinder : public Renderable {
public:
  V3d a,b; // Exists
  float rad; // Exists
  Disc da,db; // Exists
   Cylinder(V3d aa,V3d bb,float r); // Method

  void renderwireframe(ImageType *p,Viewpoint *v); // Method

    
};

class TextRend : public Renderable {
public:
  String text; // Exists
  float dist; // Exists
  int fontsize; // Exists
   TextRend(String t,int s,V3d pos,float d); // Method

  void renderwireframe(ImageType *p,Viewpoint *v); // Method

};

class Triangle3d : public Renderable, public Polygon3d {
public:
  V3d a,b,c; // Exists
   Triangle3d(V3d aa,V3d bb,V3d cc); // Method

  V3d normal(); // Method

  Renderable translated(V3d t); // Method

  Renderable rotated(V3d ax,float ang); // Method

  Renderable operator*(Matrix m); // Method

  void render(JBmp *j,Viewpoint *v); // Method

};

 class Gouraud;

class Corner {
public:
  V3d pos,normal; // Exists
  List<Gouraud*> gs; // Exists
  int col; // Exists
  V3d rgb; // Exists
   Corner(); // Method

   Corner(V3d x); // Method

   Corner(V3d x,V3d n); // Method

  void findnormal(); // Method

};

float tofloat(Corner c); // Method


class Gouraud : public Renderable {
public:
  Corner *a,*b,*c; // Exists
  Texture *text; // Exists
   Gouraud(Texture *t,Corner *aa,Corner *bb,Corner *cc); // Method

   Gouraud(Corner *aa,Corner *bb,Corner *cc); // Method

  void render(JBmp *j,Viewpoint *v); // Method

  void render(RGBmp *r,Viewpoint *v); // Method

  V3d normal(); // Method

};

  /* Skipping later :: void Corner::findnormal(); // Method
*/


V3d getpos(Renderable *r); // Method


void render(Octree<Renderable*> *o,Viewpoint *v); // Method


void render(RGBmp *rgbmp,Octree<Renderable*> *o,Viewpoint *v,myRGB rgb); // Method


void render(RGBmp *rgbmp,Octree<Renderable*> *o,Viewpoint *v); // Method

  

V3d posforrend(Renderable *r); // Method


#endif

#endif
