// Interpolated
// 21d centre
// gaussian distributed addition amplitude and radius
// using map to remove artefacts

#include <joeylib.c>

#define debug false

int scrwid=320; int scrhei=200;
int jump=8;
float scale=1.0;
float offrnd=1.0;
float offvdamp=0.99;
float offdamp=0.95;
float ampcen=0;

class S {
public:
  float cx,cy,rad,amp;
  S() { 
  }
  S(float a,float b,float c,float d) {
    cx=a;
    cy=b;
    rad=c;
    amp=d;
  }
  void add(S o) {
    cx=cx+o.cx;
    cy=cy+o.cy;
    rad=myabs(rad+o.rad);
    amp=amp+o.amp;
  }
};

float getheight(List<S> *xs,float x,float y) {
  float sum=0;
  for (int i=1;i<=xs->len;i++) {
    S s=xs->num(i);
    sum+=s.amp*gaussian(sqrt(mysquare(x-s.cx)+mysquare(y-s.cy))/s.rad);
  }
  sum=scale*sum/(float)(xs->len);
  return sum;
}

float radfromamp(float amp) {
  return 15+amp/1024.0*scrwid/2.0;
}

void domovement(S *v,S *x) {
  hang(&(v->cx),0,offvdamp,offrnd);
  hang(&(v->cy),0,offvdamp,offrnd);
  hang(&(v->rad),0,0.99,2);
  hang(&(v->amp),0,0.99,0.5);
  x->add(*v);
  hang(&(x->cx),scrwid/2,offdamp,0);
  hang(&(x->cy),scrhei/2,offdamp,0);
  hang(&(x->amp),0,0.95,0);
  hang(&(x->rad),radfromamp(x->amp),0.9,0);
  if (x->rad<30)
    x->rad=30;
}

void main() {
  allegrosetup(scrwid,scrhei);
  makepalette(&greypalette);
  // makepalette(&myRGB::hue);
  List<S> xs=List<S>();
  List<S> xvs=List<S>();
  List<V2d> ps=List<V2d>();
  List<V2d> pvs=List<V2d>();
  randomise();
  for (int i=0;i<10;i++) {
    float amp=0;
    S s=S(myrnd()*scrwid,myrnd()*scrhei,radfromamp(amp),0);
    S v=S(0,0,0,0);
    xs+s;
    xvs+v;
  }
  for (int rndloop=0;rndloop<10000;rndloop++) {
    for (int i=0;i<xs.len;i++) {
      S *v=xvs.p2num(i);
      S *x=xs.p2num(i);
      domovement(v,x);
    }
  }
  for (int i=1;i<=100;i++) {
    ps.add(V2d(myrnd()*scrwid,myrnd()*scrhei));
    pvs.add(V2d(0,0));
  }
  JBmp j=JBmp(scrwid,scrhei);
  Map2d<int> intmap=Map2d<int>(scrwid,scrhei);
  do {
    for (int moveloop=1;moveloop<20;moveloop++) {
      j.clear();
      for (int i=1;i<ps.len;i++) {
        V2d *p=ps.p2num(i);
        V2d *v=pvs.p2num(i);
        *p=*p+*v;
        float dhdx=getheight(&xs,p->x,p->y)-getheight(&xs,p->x-1,p->y);
        float dhdy=getheight(&xs,p->x,p->y)-getheight(&xs,p->x,p->y-1);
        *v=*v*.95-V2d(dhdx,dhdy);
        if (!j.inimage(p->x,p->y)) {
          *p=V2d(myrnd()*scrwid,myrnd()*scrhei);
          *v=V2d(0,0);
        }
        j.liner(*p,v->neg(),255);
      }
      j.writetoscreen();
      if (key[KEY_ESC])
        exit(0);
    }
    for (int i=1;i<=xs.len;i++) {
      S *v=xvs.p2num(i);
      S *x=xs.p2num(i);
      domovement(v,x);
      int c=j.point(x->cx,x->cy)+x->amp;
    }
    for (int i=1;i<=ps.len;i++) {
      if (myrnd()<0.1) {
            *(ps.p2num(i))=V2d(myrnd()*scrwid,myrnd()*scrhei);
            *(pvs.p2num(i))=V2d(0,0);
      }
      /*for (int j=1;j<=ps.len;j++) {
        if (i!=j) {
          if (V2d::dist(ps.num(i),ps.num(j))<0.1) {
            *(ps.p2num(i))=V2d(myrnd()*scrwid,myrnd()*scrhei);
            *(pvs.p2num(i))=V2d(0,0);
          }
        }
      }*/
    }
  } while (!key[KEY_ESC]);
}
