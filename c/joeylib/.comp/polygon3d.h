#ifndef polygon3d_H
  #define polygon3d_H

  #include <line3d.h>

/*class Polygon3d {
public:
  Polygon2d pol;
  Plane pla;
  Polygon3d() { }
  Polygon3d(V3d a,V3d b,V3d c) {
    pla=Plane((a+b+c)/3.0,V3d::normcross(a-b,a-c));
    pol.vs.add(pla.orientor().disorient(a).dropz());
    pol.vs.add(pla.orientor().disorient(b).dropz());
  }
  Line3d *intersection(Plane p) {
    Line2d plni=pla.intersection(p);
    List<V2d> vs;
    for (int i=1;i<=pol.vs.len;i++) {
      Line2d l=pol.linefrom(i);
      V2d *v=l.findintersectionornull(plni);
      if (v!=NULL)
        vs.add(*v);
    }
    if (vs.len>=2) {
      Line2d l=Line2d(vs.num(1),vs.num(2));
      Line3d r=Line3d(l).orient(pla.orientor());
      return new Line3d(r);
    }
    return NULL;
  }
};*/


class Polygon3d {
public:
  List<V3d> vs; // Exists

   Polygon3d(); // Method


   Polygon3d(V3d a,V3d b,V3d c); // Method


  V3d centre(); // Method


  Line3d *intersectionheight(float y); // Method


  void getintersectionheight(float y,Line3d *l,bool *s); // Method


  Line3d *addifintersectionheight(float y,List<Line3d> *ls); // Method


  Line3d linefrom(int i); // Method


  String toString(); // Method


  void freedom(); // Method


};

#endif
