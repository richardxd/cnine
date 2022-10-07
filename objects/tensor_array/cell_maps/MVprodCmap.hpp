/*
 * This file is part of cnine, a lightweight C++ tensor library. 
 *  
 * Copyright (c) 2021, Imre Risi Kondor
 *
 * This source code file is subject to the terms of the noncommercial 
 * license distributed with cnine in the file LICENSE.TXT. Commercial 
 * use is prohibited. All redistributed versions of this file (in 
 * original or modified form) must retain this copyright notice and 
 * must be accompanied by a verbatim copy of the license. 
 *
 */


#ifndef _MVprodCmap
#define _MVprodCmap

#include "Cmaps2.hpp"


namespace cnine{

  class MVprodCmap: public Masked2_cmap{
  public:
    
    int I;
    int J;

    template<typename OP, typename ARR>
    MVprodCmap(const OP& op, ARR& r, const ARR& x, const ARR& y, const int add_flag=0){
      I=x.get_adim(0);
      J=x.get_adim(1);
      assert(r.get_aasize()==I);
      assert(y.get_aasize()==J);
      if(r.dev==0){
	if(J==0) return;
	for(int i=0; i<I; i++){
	  decltype(r.get_cell(0)) t=r.cell(i);
	  if(add_flag==0) op.apply(t,x.cell({i,0}),y.cell(0),false);
	  for(int j=1-add_flag; j<J; j++){
	    op.apply(t,x.cell({i,j}),y.cell(j),true);
	  }
	}
      }
      if(r.dev==1){
	//op.accumulate(*this,r,x,y);
      }
    }

#ifdef _WITH_CUDA

    dim3 blockdims() const{
      return dim3(I);
    }

    __device__ int n_accum(const int b) const{
      return J;
    }

    __device__ int target(const int b) const{
      return b;
    }

    __device__ int lst_ptr(const int b) const{
      return 0;
    }

    __device__ thrust::tuple<int,int> source(const int lst, const int b, const int i) const{
      return thrust::make_tuple(b*J+i,i);
    }

#endif 

  };


  // ---- Templates ------------------------------------------------------------------------------------------


  template<typename OP, typename ARR>
  ARR MVprod(const ARR& x, const ARR& y){
    ARR r=ARR::zeros_like(x,x.get_adim(0));
    MVprodCmap(OP(),r,x,y);
    return r;
  }

  template<typename OP, typename ARR>
  void add_MVprod(ARR& r, const ARR& x, const ARR& y){
    MVprodCmap(OP(),r,x,y,1);
  }


}

#endif 


