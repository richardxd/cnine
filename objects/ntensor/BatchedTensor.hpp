/*
 * This file is part of cnine, a lightweight C++ tensor library. 
 *  
 * Copyright (c) 2023, Imre Risi Kondor
 *
 * This source code file is subject to the terms of the noncommercial 
 * license distributed with cnine in the file LICENSE.TXT. Commercial 
 * use is prohibited. All redistributed versions of this file (in 
 * original or modified form) must retain this copyright notice and 
 * must be accompanied by a verbatim copy of the license. 
 *
 */


#ifndef _CnineBatchedTensor
#define _CnineBatchedTensor

#include "Cnine_base.hpp"
#include "TensorVirtual.hpp"
#include "TensorView.hpp"

#ifdef _WITH_CUDA
#include <cuda.h>
#include <cuda_runtime.h>
#endif 

#ifdef _WITH_CUBLAS
#include <cublas_v2.h>
extern cublasHandle_t cnine_cublas;
#endif 


namespace cnine{

  template<typename TYPE>
  class BatchedTensor: public TensorVirtual<TensorViewB<TYPE> >{
  public:

    typedef TensorVirtual<TensorViewB<TYPE> > VTensor;
    typedef TensorViewB<TYPE> BatchedTensorView;

    using TensorView::TensorView;
    using TensorView::arr;
    using TensorView::dims;
    using TensorView::strides;
    using TensorView::dev;

    using TensorView::operator=;
    using TensorView::ndims;


  public: // ---- Constructors ------------------------------------------------------------------------------


    BatchedTensor(){};

    BatchedTensor(const int _b, const Gdims& _dims, const int _dev=0):
      VTensor(_dims.prepend(_b),_dev){}

    template<typename FILLTYPE, typename = typename 
	     std::enable_if<std::is_base_of<cnine::fill_pattern, FILLTYPE>::value, FILLTYPE>::type>
    BatchedTensor(const int _b, const Gdims& _dims, const FILLTYPE& fill, const int _dev=0):
      VTensor(_dims.prepend(_b),_dev){}


  public: // ---- Named constructors ------------------------------------------------------------------------


    static BatchedTensor zero(const int _b, const Gdims& _dims, const int _dev=0){
      return BatchedTensor(_b,_dims,fill_zero(),_dev);
    }

    static BatchedTensor sequential(const int _b, const Gdims& _dims, const int _dev=0){
      return BatchedTensor(_b,_dims,fill_sequential(),_dev);
    }

    static BatchedTensor gaussian(const int _b, const Gdims& _dims, const int _dev=0){
      return BatchedTensor(_b,_dims,fill_gaussian(),_dev);
    }


  public: // ---- Copying -----------------------------------------------------------------------------------


  public: // ---- Transport -----------------------------------------------------------------------------------


  public: // ---- ATen --------------------------------------------------------------------------------------


  public: // ---- Views -------------------------------------------------------------------------------------


  public: // ---- Operations --------------------------------------------------------------------------------


    BatchedTensor operator*(const BatchedTensorView& y) const{
      CNINE_ASSERT(ndims()==1||ndims()==2,"first operand of product must be a vector or a matrix");
      CNINE_ASSERT(y.ndims()==1||y.ndims()==2,"second operand of product must be a vector or a matrix");

      if(ndims()==1 && y.ndims()==2){
	BatchedTensor R=zero({y.dim(1)},dev);
	R.add_mvprod_T(y,*this);
	return R;
      }

      if(ndims()==2 && y.ndims()==1){
	BatchedTensor R=zero({dim(0)},dev);
	R.add_mvprod(*this,y);
	return R;
      }

      if(ndims()==2 && y.ndims()==2){
	BatchedTensor R=zero({dim(0),y.dim(1)},dev);
	R.add_mprod(*this,y);
	return R;
      }

      return BatchedTensor();
    }


  public: // ---- I/O ---------------------------------------------------------------------------------------


    string classname() const{
      return "BatchedTensor";
    }

    string describe() const{
      ostringstream oss;
      oss<<"BatchedTensor"<<dims<<" ["<<strides<<"]"<<endl;
      return oss.str();
    }

    friend ostream& operator<<(ostream& stream, const BatchedTensor<TYPE>& x){
      stream<<x.str(); return stream;
    }

  };


}

#endif
    /*
    BatchedTensor(const TensorView& x):
      BatchedTensor(x.dims,x.dev){
      CNINE_CONVERT_WARNING();
      view()=x;
    }

    TensorView view(){
      return TensorView(*this);
    }

    const TensorView view() const{
      return TensorView(*this);
    }
    */


    /*
    BatchedTensor(const TensorView& x, const int _dev):
      BatchedTensor(x.dims,_dev){
      CNINE_COPY_WARNING();
      view()=x;
    }

    void move_to_device(const int _dev) const{
      if(dev==_dev) return;
      const_cast<BatchedTensor&>(*this)=BatchedTensor(*this,_dev);
    }
    */
