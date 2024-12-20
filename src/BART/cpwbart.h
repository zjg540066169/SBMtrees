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

#include "tree.h"
#include "treefuns.h"

typedef std::vector<tree> vtree;

#ifdef _OPENMP
void local_getpred(size_t nd, size_t p, size_t m, size_t np, xinfo& xi, std::vector<vtree>& tmat, double *px, Rcpp::NumericMatrix& yhat);
//void local_getpred(size_t nd, size_t p, size_t m, size_t np, xinfo xi, std::vector<vtree>& tmat, double *px, Rcpp::NumericMatrix& yhat);
#endif

void getpred(int beg, int end, size_t p, size_t m, size_t np, xinfo& xi, std::vector<vtree>& tmat, double *px, Rcpp::NumericMatrix& yhat);

RcppExport SEXP cpwbart(
   SEXP itrees_,		//treedraws list from fbart
   SEXP ix_,			//x matrix to predict at
   bool verbose = true
)
{
   if(verbose)
     Rprintf("*****In main of C++ for bart prediction\n");
   
   //--------------------------------------------------
   //get threadcount
   int tc = 1;
   if(verbose)
     cout << "tc (threadcount): " << tc << endl;
   
   //--------------------------------------------------
   //process trees
   Rcpp::List trees(itrees_);
   //return Rcpp::List();
   Rcpp::CharacterVector itrees(Rcpp::wrap(trees["trees"])); 
   std::string itv(itrees[0]);
   std::stringstream ttss(itv);

   size_t nd,m,p;
   ttss >> nd >> m >> p;
   if(verbose){
    cout << "number of bart draws: " << nd << endl;
    cout << "number of trees in bart sum: " << m << endl;
    cout << "number of x columns: " << p << endl;
   }
   //--------------------------------------------------
   //process cutpoints (from trees)
   Rcpp::List  ixi(Rcpp::wrap(trees["cutpoints"]));
   size_t pp = ixi.size();
   if(p!=pp) cout << "WARNING: p from trees and p from x don't agree\n";
   xinfo xi;
   xi.resize(p);
   for(size_t i=0;i<p;i++) {
      Rcpp::NumericVector cutv(ixi[i]);
      xi[i].resize(cutv.size());
      std::copy(cutv.begin(),cutv.end(),xi[i].begin());
   }
   //--------------------------------------------------
   //process x
   Rcpp::NumericMatrix xpred(ix_);
   //Rcout << xpred << std::endl;
   size_t np = xpred.ncol();
   if(verbose)
   cout << "from x,np,p: " << xpred.nrow() << ", " << xpred.ncol() << endl;
   //--------------------------------------------------
   //read in trees
   std::vector<vtree> tmat(nd);
   for(size_t i=0;i<nd;i++) tmat[i].resize(m);
   for(size_t i=0;i<nd;i++) {
      for(size_t j=0;j<m;j++) ttss >> tmat[i][j];
   }
   //--------------------------------------------------
   //get predictions

   Rcpp::NumericMatrix yhat(nd,np);
   std::fill(yhat.begin(), yhat.end(), 0.0);
   double *px = &xpred(0,0);
   

   if(verbose)
     cout << "***using serial code\n";
   getpred(0, nd-1, p, m, np,  xi,  tmat, px,  yhat);

//    #ifndef _OPENMP
//    if(verbose)
//    cout << "***using serial code\n";
//    getpred(0, nd-1, p, m, np,  xi,  tmat, px,  yhat);
//    #else
//    if(tc==1) {if(verbose) cout << "***using serial code\n"; getpred(0, nd-1, p, m, np,  xi,  tmat, px,  yhat);}
//    else {
//      if(verbose)cout << "***using parallel code\n";
// #pragma omp parallel num_threads(tc)
//       local_getpred(nd,p,m,np,xi,tmat,px,yhat);
//    }
//    #endif

   //--------------------------------------------------
   //Rcpp::List ret;
   //ret["yhat.test"] = yhat;
   return yhat;
}

void fit4(tree& t, xinfo& xi, size_t p, size_t n, double *x,  double* fv)
{
  tree::tree_p bn;
  for(size_t i=0;i<n;i++) {
    //cout << "i:" << i << std::endl;
    bn = t.bn(x+i*p,xi);
    
    fv[i] = bn->gettheta();
    
  }
}

void getpred(int beg, int end, size_t p, size_t m, size_t np, xinfo& xi, std::vector<vtree>& tmat, double *px, Rcpp::NumericMatrix& yhat)
{
   double *fptemp = new double[np];

   for(int i=beg;i<=end;i++) {
      for(size_t j=0;j<m;j++) {
         fit4(tmat[i][j],xi,p,np,px,fptemp);
         for(size_t k=0;k<np;k++) yhat(i,k) += fptemp[k];
      }
   }

   delete [] fptemp;
}
#ifdef _OPENMP
//void local_getpred(size_t nd, size_t p, size_t m, size_t np, xinfo xi, std::vector<vtree>& tmat, double *px, Rcpp::NumericMatrix& yhat)
void local_getpred(size_t nd, size_t p, size_t m, size_t np, xinfo& xi, std::vector<vtree>& tmat, double *px, Rcpp::NumericMatrix& yhat)
{


   int my_rank = omp_get_thread_num();
   int thread_count = omp_get_num_threads();
   int h = nd/thread_count; int beg = my_rank*h; int end = beg+h-1;
   
   getpred(beg,end,p,m,np,xi,tmat,px,yhat);
}
#endif

