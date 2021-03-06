#ifndef quad_H
  #define quad_H

  #include <jbmp.h>

// A way of hierarchical interpolation, ineficcient if eval is not memo-ed

void quadify(int l,int t,int r,int b,float (*eval)(int,int),void (*fulfil)(int,int,int,int,float,float,float,float),float tol,int res); // Method


/*void quadify(int l,int t,int r,int b,float (*eval)(int,int),void (*fulfil)(int,int,int,int,float,float,float,float),float tol,int res) {
  float nw=eval(l,t);
  float ne=eval(r,t);
  float sw=eval(l,b);
  float se=eval(r,b);
  float ave=(nw+ne+sw+se)/4.0;
  if (max(max(diff(ave,nw),diff(ave,ne)),max(diff(ave,sw),diff(ave,se)))>tol && max(r-l,b-t)>res) {
    int mx=(l+r)/2;
    int my=(t+b)/2;
    quadify(l,t,mx,my,eval,fulfil,tol,res);
    quadify(mx,t,r,my,eval,fulfil,tol,res);
    quadify(l,my,mx,b,eval,fulfil,tol,res);
    quadify(mx,my,r,b,eval,fulfil,tol,res);
  } else
    fulfil(l,t,r,b,nw,ne,sw,se);
}*/

extern JBmp *Quadjbmp; // Exists
extern Map2d<int> *Quadmap; // Exists
extern int (*Quadfn)(int,int); // Exists

float Quadgetpos(int i,int j); // Method


void Quadfulfil(int l,int t,int r,int b,float nw,float ne,float sw,float se); // Method


void quadify(JBmp *j,int (*colourat)(int,int),int tol,int res); // Method


void quadify(JBmp *j,int (*colourat)(int,int),int tol,int res,int size); // Method


#endif
