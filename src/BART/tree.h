/*
 *  BART: Bayesian Additive Regression Trees
 *  Copyright (C) 2017 Robert McCulloch and Rodney Sparapani
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, a copy is available at
 *  https://www.R-project.org/Licenses/GPL-2
 */



#ifndef GUARD_tree_h
#define GUARD_tree_h
#include "common.h"

#include <map>
#include <cmath>
#include <cstddef>

//--------------------------------------------------
//xinfo xi, then xi[v][c] is the c^{th} cutpoint for variable v.
//left if x[v] < xi[v][c]
typedef std::vector<double> vec_d; //double vector
typedef std::vector<vec_d> xinfo; //vector of vectors, will be split rules

//--------------------------------------------------
//info contained in a node, used by input operator
struct node_info {
   std::size_t id; //node id
   std::size_t v;  //variable
   std::size_t c;  //cut point
   double theta;   //theta
};

//--------------------------------------------------
class tree {
public:
   //friends--------------------
   friend std::istream& operator>>(std::istream&, tree&);
   //typedefs--------------------
   typedef tree* tree_p;
   typedef const tree* tree_cp;
   typedef std::vector<tree_p> npv; 
   typedef std::vector<tree_cp> cnpv;
   //contructors,destructors--------------------
   tree(): theta(0.0),v(0),c(0),p(0),l(0),r(0) {}
   tree(const tree& n): theta(0.0),v(0),c(0),p(0),l(0),r(0) {cp(this,&n);}
   tree(double itheta): theta(itheta),v(0),c(0),p(0),l(0),r(0) {}
   void tonull(){
     size_t ts = treesize();
     //loop invariant: ts>=1
     while(ts>1) { //if false ts=1
       npv nv;
       getnogs(nv);
       for(size_t i=0;i<nv.size();i++) {
         delete nv[i]->l;
         delete nv[i]->r;
         nv[i]->l=0;
         nv[i]->r=0;
       }
       ts = treesize(); //make invariant true
     }
     theta=0.0;
     v=0;c=0;
     p=0;l=0;r=0;
   }; //like a "clear", null tree has just one node
   ~tree() {tonull();}
   //operators----------
   tree& operator=(const tree& rhs){if(&rhs != this) {
     tonull(); //kill left hand side (this)
     cp(this,&rhs); //copy right hand side to left hand side
   }
   return *this;};
   
   //interface--------------------
   //set
   void settheta(double theta) {this->theta=theta;}
   void setv(size_t v) {this->v = v;}
   void setc(size_t c) {this->c = c;}
   //get
   double gettheta() const {return theta;}
   size_t getv() const {return v;}
   size_t getc() const {return c;}
   tree_p getp() {return p;}  
   tree_p getl() {return l;}
   tree_p getr() {return r;}
   //tree functions--------------------
   tree_p getptr(size_t nid){
     if(this->nid() == nid) return this; //found it
     if(l==0) return 0; //no children, did not find it
     tree_p lp = l->getptr(nid);
     if(lp) return lp; //found on left
     tree_p rp = r->getptr(nid);
     if(rp) return rp; //found on right
     return 0; //never found it
   }; //get node pointer from node id, 0 if not there
   void pr(bool pc=true){ size_t d = depth();
     size_t id = nid();
     
     size_t pid;
     if(!p) pid=0; //parent of top node
     else pid = p->nid();
     
     std::string pad(2*d,' ');
     std::string sp(", ");
     if(pc && (ntype()=='t'))
       cout << "tree size: " << treesize() << std::endl;
     cout << pad << "(id,parent): " << id << sp << pid;
     cout << sp << "(v,c): " << v << sp << c;
     cout << sp << "theta: " << theta;
     cout << sp << "type: " << ntype();
     cout << sp << "depth: " << depth();
     cout << sp << "pointer: " << this << std::endl;
     
     if(pc) {
       if(l) {
         l->pr(pc);
         r->pr(pc);
       }
     }}; //to screen, pc is "print children"
   size_t treesize(){if(l==0) return 1;  //if bottom node, tree size is 1
   else return (1+l->treesize()+r->treesize());}; //number of nodes in tree
   size_t nnogs(){if(!l) return 0; //bottom node
   if(l->l || r->l) { //not a nog
     return (l->nnogs() + r->nnogs());
   } else { //is a nog
     return 1;
   }};    //number of nog nodes (no grandchildren nodes)
   size_t nbots(){if(l==0) { //if a bottom node
     return 1;
   } else {
     return l->nbots() + r->nbots();
   }};    //number of bottom nodes
   bool birth(size_t nid, size_t v, size_t c, double thetal, double thetar){
     tree_p np = getptr(nid);
     if(np==0) {
       cout << "error in birth: bottom node not found\n";
       return false; //did not find note with that nid
     }
     if(np->l!=0) {
       cout << "error in birth: found node has children\n";
       return false; //node is not a bottom node
     }
     
     //add children to bottom node np
     tree_p l = new tree;
     l->theta=thetal;
     tree_p r = new tree;
     r->theta=thetar;
     np->l=l;
     np->r=r;
     np->v = v; np->c=c;
     l->p = np;
     r->p = np;
     
     return true;
   };
   bool death(size_t nid, double theta) { tree_p nb = getptr(nid);
     if(nb==0) {
       cout << "error in death, nid invalid\n";
       return false;
     }
     if(nb->isnog()) {
       delete nb->l;
       delete nb->r;
       nb->l=0;
       nb->r=0;
       nb->v=0;
       nb->c=0;
       nb->theta=theta;
       return true;
     } else {
       cout << "error in death, node is not a nog node\n";
       return false;
     }};
   void birthp(tree_p np,size_t v, size_t c, double thetal, double thetar){
     tree_p l = new tree;
     l->theta=thetal;
     tree_p r = new tree;
     r->theta=thetar;
     np->l=l;
     np->r=r;
     np->v = v; np->c=c;
     l->p = np;
     r->p = np;
   };
   void deathp(tree_p nb, double theta){delete nb->l;
     delete nb->r;
     nb->l=0;
     nb->r=0;
     nb->v=0;
     nb->c=0;
     nb->theta=theta;};
   void getbots(npv& bv){if(l) { //have children
     l->getbots(bv);
     r->getbots(bv);
   } else {
     bv.push_back(this);
   }};         //get bottom nodes
   void getnogs(npv& nv){ if(l) { //have children
     if((l->l) || (r->l)) {  //have grandchildren
       if(l->l) l->getnogs(nv);
       if(r->l) r->getnogs(nv);
     } else {
       nv.push_back(this);
     }
   }};         //get nog nodes (no granchildren)
   void getnodes(npv& v){ v.push_back(this);
     if(l) {
       l->getnodes(v);
       r->getnodes(v);
     }};         //get vector of all nodes
   void getnodes(cnpv& v) const{ v.push_back(this);
     if(l) {
       l->getnodes(v);
       r->getnodes(v);
     }};  //get vector of all nodes (const)
   tree_p bn(double *x,xinfo& xi){
     //cout << "l:" << l << std::endl;
     
     
     if(l==0) return this; //no children
     //cout << "v:" << v << std::endl;
     //cout << "c:" << c << std::endl;
     //cout << "x[v]:" << x[v] << std::endl;
     //cout << "xi[v][c]:" << xi[v][c] << std::endl;
     if(x[v] < xi[v][c]) {
       return l->bn(x,xi);
     } else {
       return r->bn(x,xi);
     }
   }; //find Bottom Node
   void rg(size_t v, int* L, int* U)
   {
     if(this->p==0)  {
       return;
     }
     if((this->p)->v == v) { //does my parent use v?
       if(this == p->l) { //am I left or right child
         if((int)(p->c) <= (*U)) *U = (p->c)-1;
         p->rg(v,L,U);
       } else {
         if((int)(p->c) >= *L) *L = (p->c)+1;
         p->rg(v,L,U);
       }
     } else {
       p->rg(v,L,U);
     }
   }; //recursively find region [L,U] for var v
   //node functions--------------------
   size_t nid() const{
     if(!p) return 1; //if you don't have a parent, you are the top
     if(this==p->l) return 2*(p->nid()); //if you are a left child
     else return 2*(p->nid())+1; //else you are a right child
   }; //nid of a node
   size_t depth(){
     if(!p) return 0; //no parents
     else return (1+p->depth());
   };  //depth of a node
   char ntype(){
     if(!p) return 't';
     if(!l) return 'b';
     if(!(l->l) && !(r->l)) return 'n';
     return 'i';
   }; //node type t:top, b:bot, n:no grandchildren i:interior (t can be b)
   bool isnog(){bool isnog=true;
     if(l) {
       if(l->l || r->l) isnog=false; //one of the children has children.
     } else {
       isnog=false; //no children
     }
     return isnog;};
   size_t getbadcut(size_t v){tree_p par=this->getp();
     if(par->getv()==v)
       return par->getc();
     else
       return par->getbadcut(v);};
#ifndef NoRcpp   
  Rcpp::List tree2list(xinfo& xi, double center=0., double scale=1.){
      Rcpp::List res;
      
      // five possible scenarios
      if(l) { // tree has branches
        //double cut=xi[v][c];
        size_t var=v, cut=c;
        
        var++; cut++; // increment from 0-based (C) to 1-based (R) array index
        
        if(l->l && r->l)         // two sub-trees
          res=Rcpp::List::create(Rcpp::Named("var")=(int)var,
                                 //Rcpp::Named("cut")=cut,
                                 Rcpp::Named("cut")=(int)cut,
                                 Rcpp::Named("type")=1,
                                 Rcpp::Named("left")= l->tree2list(xi, center, scale),
                                 Rcpp::Named("right")=r->tree2list(xi, center, scale));   
        else if(l->l && !(r->l)) // left sub-tree and right terminal
          res=Rcpp::List::create(Rcpp::Named("var")=(int)var,
                                 //Rcpp::Named("cut")=cut,
                                 Rcpp::Named("cut")=(int)cut,
                                 Rcpp::Named("type")=2,
                                 Rcpp::Named("left")= l->tree2list(xi, center, scale),
                                 Rcpp::Named("right")=r->gettheta()*scale+center);    
        else if(!(l->l) && r->l) // left terminal and right sub-tree
          res=Rcpp::List::create(Rcpp::Named("var")=(int)var,
                                 //Rcpp::Named("cut")=cut,
                                 Rcpp::Named("cut")=(int)cut,
                                 Rcpp::Named("type")=3,
                                 Rcpp::Named("left")= l->gettheta()*scale+center,
                                 Rcpp::Named("right")=r->tree2list(xi, center, scale));
        else                     // no sub-trees 
          res=Rcpp::List::create(Rcpp::Named("var")=(int)var,
                                 //Rcpp::Named("cut")=cut,
                                 Rcpp::Named("cut")=(int)cut,
                                 Rcpp::Named("type")=0,
                                 Rcpp::Named("left")= l->gettheta()*scale+center,
                                 Rcpp::Named("right")=r->gettheta()*scale+center);
      }
      else // no branches
        res=Rcpp::List::create(Rcpp::Named("var")=0, // var=0 means root
                               //Rcpp::Named("cut")=0.,
                               Rcpp::Named("cut")=0,
                               Rcpp::Named("type")=0,
                               Rcpp::Named("left") =theta*scale+center,
                               Rcpp::Named("right")=theta*scale+center);
      
      return res;
  }; // create an efficient list from a single tree
  //tree list2tree(Rcpp::List&, xinfo& xi); // create a tree from a list and an xinfo  
  Rcpp::IntegerVector tree2count(size_t nvar){
    Rcpp::IntegerVector res(nvar);
    
    if(l) { // tree branches
      res[v]++;
      
      if(l->l) res+=l->tree2count(nvar); // if left sub-tree
      if(r->l) res+=r->tree2count(nvar); // if right sub-tree
    } // else no branches and nothing to do
    
    return res;
  }; // for one tree, count the number of branches for each variable
#endif
private:
   double theta; //univariate double parameter
   //rule: left if x[v] < xinfo[v][c]
   size_t v;
   size_t c;
   //tree structure
   tree_p p; //parent
   tree_p l; //left child
   tree_p r; //right child
   //utiity functions
   void cp(tree_p n,  tree_cp o)
     //assume n has no children (so we don't have to kill them)
     //recursion down
     {
       if(n->l) {
         cout << "cp:error node has children\n";
         return;
       }
       
       n->theta = o->theta;
       n->v = o->v;
       n->c = o->c;
       
       if(o->l) { //if o has children
         n->l = new tree;
         (n->l)->p = n;
         cp(n->l,o->l);
         n->r = new tree;
         (n->r)->p = n;
         cp(n->r,o->r);
       }
   }; //copy tree
};

std::ostream& operator<<(std::ostream&, const tree&);
std::istream& operator>>(std::istream&, tree&);


#endif

