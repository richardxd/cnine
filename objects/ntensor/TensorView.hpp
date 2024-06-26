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


#ifndef _CnineTensorView
#define _CnineTensorView

#include "Cnine_base.hpp"
#include "ExprTemplates.hpp"
#include "Gdims.hpp"
#include "GstridesB.hpp"
#include "Gindex.hpp"
#include "MemArr.hpp"
#include "device_helpers.hpp"
//#include "GatherMapB.hpp"

#include "Rtensor1_view.hpp"
#include "Rtensor2_view.hpp"
#include "Rtensor3_view.hpp"
#include "RtensorA.hpp"

#include "Ctensor1_view.hpp"
#include "Ctensor2_view.hpp"
#include "Ctensor3_view.hpp"

#include "Itensor1_view.hpp"
#include "Itensor2_view.hpp"
#include "Itensor3_view.hpp"

#include "tensor1_view.hpp"

#ifdef _WITH_CUDA
#include <cuda.h>
#include <cuda_runtime.h>
#endif 

#ifdef _WITH_CUBLAS
#include <cublas_v2.h>
extern cublasHandle_t cnine_cublas;
#endif 

#ifdef _WITH_EIGEN
#include <Eigen/Dense>
#endif


namespace cnine{

  template<typename TYPE> class TensorView;
  template<typename TYPE> class Tensor;
  template<typename TYPE> class TensorArrayView;
  template<typename TYPE> class TensorSArrayView;
  template<typename TYPE> class Tensor;
  template<typename TYPE> class Ltensor;
  template<typename TYPE> class BatchedTensorView;


  // this is the multiply defined functions problem
  inline Itensor1_view view1_of(const TensorView<int>& x);
  inline Rtensor1_view view1_of(const TensorView<float>& x);
  inline Rtensor1_view view1_of(const TensorView<double>& x); // hack
  inline Ctensor1_view view1_of(const TensorView<complex<float> >& x);

  inline Itensor2_view view2_of(const TensorView<int>& x);
  inline Rtensor2_view view2_of(const TensorView<float>& x);
  inline Rtensor2_view view2_of(const TensorView<double>& x); // hack
  inline Ctensor2_view view2_of(const TensorView<complex<float> >& x);

  inline Itensor3_view view3_of(const TensorView<int>& x);
  inline Rtensor3_view view3_of(const TensorView<float>& x);
  inline Rtensor3_view view3_of(const TensorView<double>& x); //hack
  inline Ctensor3_view view3_of(const TensorView<complex<float> >& x);

  inline Rtensor1_view flat_view_of(const TensorView<float>& x);


  template<typename TYPE>
  class TensorView{
  private:

    friend class Tensor<TYPE>;
    friend class TensorArrayView<TYPE>;
    friend class TensorSArrayView<TYPE>;
    friend class Tensor<TYPE>;
    friend class Ltensor<TYPE>;
    friend class BatchedTensorView<TYPE>;

    typedef std::size_t size_t;


    MemArr<TYPE> arr;
    Gdims dims;
    GstridesB strides;
    int dev;


  public: // ---- Constructors ------------------------------------------------------------------------------


    TensorView(){}

    TensorView(const MemArr<TYPE>& _arr, const Gdims& _dims, const GstridesB& _strides):
      arr(_arr),
      dims(_dims), 
      strides(_strides), 
      dev(_arr.device()){
    }


  public: // ---- Constructors for non-view derived classes --------------------------------------------------


    TensorView(const Gdims& _dims, const int fcode, const int _dev):
      //arr(1), //TODO: eliminate this
      dims(_dims),
      strides(GstridesB(_dims)),
      dev(_dev){
      FNTRACE();

      size_t N=dims.total();

      //if(vram_manager && _dev>1){
      //arr=MemArr<TYPE>(*manager,N,_dev);
      //CNINE_ASSRT(fcode<2);
      //if(fcode==0) set_zero();
      //}

      if(fcode==0){
	arr=MemArr<TYPE>(N,fill_zero(),_dev);
	return;
      }

      if(fcode==1){
	arr=MemArr<TYPE>(N,_dev);
	return;
      }

      if(fcode>1){
	arr=MemArr<TYPE>(N,0);
	dev=0;

	switch(fcode){

	  case(2): //ones 
	    for(size_t i=0; i<N; i++)
	      arr[i]=1.0;
	  break;

	  case(3): //sequential 
	    for(size_t i=0; i<N; i++)
	      arr[i]=i;
	  break;

	  case(4): //gaussian
	    normal_distribution<double> distr;
	    if constexpr(is_complex<TYPE>()){
	      for(int i=0; i<N; i++) 
		arr[i]=TYPE(distr(rndGen),distr(rndGen));
	    }else{
	      for(int i=0; i<N; i++) 
		arr[i]=distr(rndGen);
	    }
	  break;

 	}

	move_to_device(_dev);
	return;
      }

      arr=MemArr<TYPE>(_dims.total(),_dev);
    }

    /*
    TensorView(const MemoryManager& manager, const Gdims& _dims, const int fcode, const int _dev):
      arr(manager,_dims.total(),_dev),
      dims(_dims),
      strides(GstridesB(_dims)),
      dev(_dev){
      FNTRACE();

      CNINE_ASSRT(fcode<2);
      if(fcode==0) set_zero();
    }
    */

    TensorView(const Gdims& _dims, const int _dev=0): 
      TensorView(MemArr<TYPE>(_dims.total(),_dev),_dims,GstridesB(_dims)){}

    TensorView(const Gdims& _dims, const fill_raw& dummy, const int _dev=0): 
      TensorView(MemArr<TYPE>(_dims.total(),_dev),_dims,GstridesB(_dims)){}

    TensorView(const Gdims& _dims, const fill_zero& dummy, const int _dev=0): 
      TensorView(MemArr<TYPE>(_dims.total(),dummy,_dev),_dims,GstridesB(_dims)){
    }

    TensorView(const Gdims& _dims, const fill_constant<TYPE>& dummy, const int _dev=0):
      TensorView(_dims,0){
      size_t N=dims.total();
      for(size_t i=0; i<N; i++)
	arr[i]=dummy.v;
      move_to_device(_dev);
    }

    TensorView(const Gdims& _dims, const fill_identity& dummy, const int _dev=0):
      TensorView(_dims,fill_zero(),0){
      CNINE_ASSRT(ndims()==2);
      CNINE_ASSRT(dim(0)==dim(1));
      int N=dim(0);
      for(int i=0; i<N; i++)
	set(i,i,1.0);
      move_to_device(_dev);
    }

    TensorView(const Gdims& _dims, const fill_sequential& dummy, const int _dev=0):
      TensorView(_dims,0){
      size_t N=dims.total();
      for(size_t i=0; i<N; i++)
	arr[i]=i;
      move_to_device(_dev);
    }

    TensorView(const Gdims& _dims, const fill_gaussian& dummy, const int _dev=0):
      TensorView(_dims,0){
      int N=dims.total();
      if constexpr(is_complex<TYPE>()){
	normal_distribution<double> distr;
	for(int i=0; i<N; i++) 
	  arr[i]=TYPE(distr(rndGen),distr(rndGen))*dummy.c;
      }else{
	normal_distribution<double> distr;
	for(int i=0; i<N; i++) 
	  arr[i]=distr(rndGen)*dummy.c;
      }
      move_to_device(_dev);
    }


  public: // ---- Copying -----------------------------------------------------------------------------------


    TensorView(const TensorView<TYPE>& x):
      arr(x.arr),
      dims(x.dims),
      strides(x.strides),
      dev(x.dev){
    }
        
    TensorView& operator=(const TensorView& x) const{
      CNINE_ASSRT(dims==x.dims);
      CNINE_ASSIGN_WARNING();

      if(asize()==0) return const_cast<TensorView&>(*this); 

      if(strides==x.strides){
	if(device()==0){
	  if(x.device()==0) std::copy(x.mem(),x.mem()+memsize(),mem());
	  if(x.device()==1) CUDA_SAFE(cudaMemcpy(mem(),x.mem(),memsize()*sizeof(TYPE),cudaMemcpyDeviceToHost));
	}
	if(device()==1){
	  if(x.device()==0) CUDA_SAFE(cudaMemcpy(mem(),x.mem(),memsize()*sizeof(TYPE),cudaMemcpyHostToDevice));
	  if(x.device()==1) CUDA_SAFE(cudaMemcpy(mem(),x.mem(),memsize()*sizeof(TYPE),cudaMemcpyDeviceToDevice));  
	}      
	return const_cast<TensorView&>(*this);
      }

      if(device()==0){
	CNINE_ASSRT(x.device()==0);
	for_each([&](const Gindex& ix, TYPE& v) {v=x(ix);});
      }

      if(device()==1){
	CNINE_ASSRT(x.device()==1);
	if constexpr(std::is_same<TYPE,float>::value){
	  switch(ndims()){
	  case(1): 
	    view1().set(x.view1());
	    //Rtensor1_view(*this).set(Rtensor1_view(x));
	    break;
	  case(2): 
	    //Rtensor2_view(*this).set(Rtensor2_view(x));
	    view2().set(x.view2());
	    break;
	  case(3): 
	    //Rtensor3_view(*this).set(Rtensor3_view(x));
	    view3().set(x.view3());
	    break;
	  default:
	    CNINE_UNIMPL();
	  }
	}else{
	  CNINE_UNIMPL();
	}
      }

      return const_cast<TensorView&>(*this);
    }

    void reset(const TensorView& x){
      arr=x.arr;
      dims=x.dims;
      strides=x.strides;
      dev=x.dev;
    }

    TensorView* clone() const{
      return new TensorView(*this);
    }


  public: // ---- Devices ------------------------------------------------------------------------------------


    TensorView(const TensorView<TYPE>& x, const int _dev):
      TensorView<TYPE>(MemArr<TYPE>(x.dims.total(),_dev),x.dims,GstridesB(x.dims)){
      (*this)=x;
    }


    void move_to_device(const int _dev) const{
      if(_dev==dev) return;
      TensorView t(*this,_dev);
      const_cast<MemArr<TYPE>&>(arr)=t.arr;
      const_cast<int&>(dev)=t.dev;
    }


  public: // ---- Conversions --------------------------------------------------------------------------------


    template<typename U=TYPE, typename = typename std::enable_if<std::is_same<U,float>::value, U>::type>
    operator Rtensor1_view() const{
      CNINE_ASSRT(ndims()==1);
      return Rtensor1_view(mem(),dims(0),strides(0),dev);
    }

    template<typename U=TYPE, typename = typename std::enable_if<std::is_same<U,float>::value, U>::type>
    operator Rtensor2_view() const{
      CNINE_ASSRT(ndims()==2);
      return Rtensor2_view(mem(),dims(0),dims(1),strides(0),strides(1),dev);
    }

    template<typename U=TYPE, typename = typename std::enable_if<std::is_same<U,float>::value, U>::type>
    operator Rtensor3_view() const{
      CNINE_ASSRT(ndims()==3);
      return Rtensor3_view(mem(),dims(0),dims(1),dims(2),strides(0),strides(1),strides(2),dev);
    }

    /*
    IF_FLOAT
    Rtensor1_view view1() const{
      CNINE_ASSRT(ndims()==1);
      return Rtensor1_view(mem(),dims,strides,dev);
    }
    */

    /*
    IF_FLOAT
    Rtensor2_view view2() const{
      CNINE_ASSRT(ndims()==2);
      return Rtensor2_view(mem(),dims,strides,dev);
    }

    IF_CFLOAT
    Rtensor2_view view2() const{
      CNINE_ASSRT(ndims()==2);
      return Rtensor2_view(mem(),dims,strides,dev);
    }

    IF_FLOAT
    Rtensor3_view view3() const{
      CNINE_ASSRT(ndims()==3);
      return Rtensor3_view(mem(),dims,strides,dev);
    }

    */

    auto view1() const -> decltype(view1_of(*this)){
      CNINE_ASSRT(ndims()==1);
      return view1_of(*this);
    }

    auto view2() const -> decltype(view2_of(*this)){
      CNINE_ASSRT(ndims()==2);
      return view2_of(*this);
    }

    auto view3() const -> decltype(view3_of(*this)){
      CNINE_ASSRT(ndims()==3);
      return view3_of(*this);
    }

    auto flat_view() const -> decltype(view1_of(*this)){
      return flat_view_of(*this);
    }

  

    //deprecated 
    IF_INT
    operator Itensor1_view() const{
      CNINE_ASSRT(ndims()==1);
      return Itensor1_view(mem(),dims,strides,dev);
    }

    IF_INT
    Itensor1_view view1() const{
      CNINE_ASSRT(ndims()==1);
      return Itensor1_view(mem(),dims,strides,dev);
    }

    IF_INT
    Itensor2_view view2() const{
      CNINE_ASSRT(ndims()==2);
      return Itensor2_view(mem(),dims,strides,dev);
    }
  
    IF_INT
    Itensor3_view view3() const{
      CNINE_ASSRT(ndims()==3);
      return Itensor3_view(mem(),dims,strides,dev);
    }



    IF_FLOAT
    TensorView& operator=(const RtensorA& x){
      CNINE_ASSRT(dims.size()==x.dims.size());

      /*
      if(x.dims.size()==1){
	view1().set(x.view1());
      }
      if(x.dims.size()==2){
	view2().set(x.view2());
      }
      if(x.dims.size()==3){
	view3().set(x.view3());
      }
      */

      switch(x.dims.size()){
      case 1:
	view1().set(x.view1());
	break;
      case 2:
	view2().set(x.view2());
	break;
      case 3:
	view3().set(x.view3());
	break;
      default:
	CNINE_UNIMPL();
      }

      return *this;
    }

    // TODO 
    IF_FLOAT
    RtensorA rtensor() const{
      switch(dims.size()){
      case 1:
	return RtensorA(view1());
	break;
      case 2:
	return RtensorA(view2());
	break;
      case 3:
	return RtensorA(view3());
	break;
      default:
	CNINE_UNIMPL();
      }
      return RtensorA();
    }


  public: // ---- ATen --------------------------------------------------------------------------------------


    #ifdef _WITH_ATEN

    TensorView(const at::Tensor& T):
      TensorView(Gdims(T),T.type().is_cuda()){
      operator=(T);
    }

    /*
    IF_FLOAT
    TensorView& operator=(const at::Tensor& T){
      CNINE_CONVERT_FROM_ATEN_WARNING();
      CNINE_ASSRT(dims==Gdims(T));
      CNINE_ASSRT(dev==T.type().is_cuda());
      if(dev==0){
	std::copy(T.data<c10::float>(),T.data<c10::float>()+total(),
	  reinterpret_cast<c10::float*>(arr.ptr()));
      }
      if(dev==1){
	//CUDA_SAFE(cudaMemcpy(arr.ptr(),T.data<c10::c10::complex<float>>(),total()*sizeof(c10::complex<float>),cudaMemcpyDeviceToDevice));
	CUDA_SAFE(cudaMemcpy(arr.ptr(),T.data<c10::float>(),total()*sizeof(c10::float),cudaMemcpyDeviceToDevice));
      }
      return *this;
    }
    */

    IF_CFLOAT
    TensorView& operator=(const at::Tensor& T){
      CNINE_CONVERT_FROM_ATEN_WARNING();
      CNINE_ASSRT(dims==Gdims(T));
      CNINE_ASSRT(dev==T.type().is_cuda());
      if(dev==0){
	//std::copy(T.data<TYPE>(),T.data<c10::TYPE>()+total(),arr.ptr());
	std::copy(T.data<c10::complex<float>>(),T.data<c10::complex<float>>()+total(),
	  reinterpret_cast<c10::complex<float>*>(arr.ptr()));
      }
      if(dev==1){
	//CUDA_SAFE(cudaMemcpy(arr.ptr(),T.data<c10::c10::complex<float>>(),total()*sizeof(c10::complex<float>),cudaMemcpyDeviceToDevice));
	CUDA_SAFE(cudaMemcpy(arr.ptr(),T.data<c10::complex<float>>(),total()*sizeof(c10::complex<float>),cudaMemcpyDeviceToDevice));
      }
      return *this;
    }
  
    /*
    IF_FLOAT
    at::Tensor torch() const{
      CNINE_CONVERT_TO_ATEN_WARNING();
      assert(dev==0);
      int k=ndims();
      vector<int64_t> v(k); 
      for(int i=0; i<k; i++) v[i]=dims[i];
      at::Tensor R(at::zeros(v,torch::CPU(at::Float))); 
      //std::copy(arr,arr+memsize,reinterpret_cast<float*>(R.data<c10::complex<float> >()));
      std::copy(arr.ptr(),arr.ptr()+dims.total(),R.data<c10::float>());
      return R;
    }
    */

    at::Tensor torch() const{
      CNINE_CONVERT_TO_ATEN_WARNING();
      //assert(dev==0);
      int k=ndims();
      //vector<int64_t> v(k); 
      //for(int i=0; i<k; i++) v[i]=dims[i];

      if constexpr(std::is_same<TYPE,int>::value){
	if(dev==0){
	  at::Tensor R(at::zeros(dims.as_int64(),torch::CPU(at::kInt))); 
	  std::copy(arr.ptr(),arr.ptr()+dims.total(),R.data<int>());
	  return R;
	}
      }

      if constexpr(std::is_same<TYPE,float>::value){
	if(dev==0){
	  at::Tensor R(at::zeros(dims.as_int64(),torch::CPU(at::kFloat))); 
	  std::copy(arr.ptr(),arr.ptr()+dims.total(),R.data<float>());
	  return R;
	}
	if(dev==1){
	  at::Tensor R(at::zeros(dims.as_int64(),torch::CUDA(at::kFloat))); 
	  CUDA_SAFE(cudaMemcpy(R.data<float>(),arr.ptr(),dims.total()*sizeof(float),cudaMemcpyDeviceToDevice));  
	  return R;
	}
      }

      if constexpr(std::is_same<TYPE,complex<float> >::value){
	if(dev==0){
	  at::Tensor R(at::zeros(dims.as_int64(),torch::CPU(at::kComplexFloat))); 
	  std::copy(arr.ptr(),arr.ptr()+dims.total(),R.data<c10::complex<float>>());
	  return R;
	}
      }
      
      CNINE_UNIMPL();
      return at::Tensor(at::zeros(dims.as_int64(),torch::CPU(at::kFloat))); 
    }

    #endif


  public: // ---- Eigen --------------------------------------------------------------------------------------


#ifdef _WITH_EIGEN

    operator Eigen::MatrixXf() const{
      CNINE_ASSRT(ndims()==2);
      int n=dims[0];
      int m=dims[1];
      Eigen::MatrixXf A(dims[0],dims[1]);
      for(int i=0; i<n; i++) 
	for(int j=0; j<m; j++) 
	  A(i,j)=(*this)(i,j);
      return A;
    }

    operator Eigen::MatrixXd() const{
      CNINE_ASSRT(ndims()==2);
      int n=dims[0];
      int m=dims[1];
      Eigen::MatrixXd A(dims[0],dims[1]);
      for(int i=0; i<n; i++) 
	for(int j=0; j<m; j++) 
	  A(i,j)=(*this)(i,j);
      return A;
    }

#endif 

  public: // ---- Access -------------------------------------------------------------------------------------


    int device() const{
      return dev;
    }
    
    int get_dev() const{
      return dev;
    }
    
    int get_device() const{
      return dev;
    }
    
    bool is_regular() const{
      return strides.is_regular(dims);
    }

    bool is_contiguous() const{
      return strides.is_contiguous(dims);
    }

    int ndims() const{
      return dims.size();
    }

    int dim(const int i) const{
      return dims[i];
    }

    int get_dim(const int i) const{
      return dims[i];
    }

    Gdims get_dims() const{
      return dims;
    }

    int stride(const int i) const{
      return strides[i];
    }

    GstridesB get_strides() const{
      return strides;
    }

    size_t asize() const{
      return dims.asize();
    }

    size_t total() const{
      return dims.total();
    }

    size_t memsize() const{
      return strides.memsize(dims);
    }

    // replace with get_arro?
    //TYPE* get_arr(){
    //return arr.get_arr();
    //} 

    const TYPE* get_arr() const{
      return arr.get_arr();
    } 

    TYPE* get_arr(){
      return arr.get_arr();
    } 

    //TYPE* get_arro(){
    //return arr.get_arr()+strides.offset;
    //} 

    //const TYPE* get_arro() const{
    //return arr.get_arr()+strides.offset;
    //} 

    TYPE* mem() const{
      return const_cast<TYPE*>(arr.get_arr());
    }

    template<typename TYPE2>
    TYPE2* mem_as() const{
      return const_cast<TYPE2*>(arr.template ptr_as<TYPE2>());
    }

    //TYPE& mem(const int i) const{
    //return *(const_cast<TYPE*>(arr.get_arr())+strides.offset);
    //}

    bool operator==(const TensorView<TYPE>& x) const{
      if(get_dev()!=(0)){
	auto a=TensorView<TYPE>(*this,0);
	return a==x;
      }
      if(x.get_dev()!=0){
	auto a=TensorView<TYPE>(x,0);
	return (*this)==a;
      }
      if(dims!=x.dims) return false;
      if(is_regular() && x.is_regular()){
	int N=asize();
	for(int i=0; i<N; i++)
	  if(arr[i]!=x.arr[i]) return false;
      }else{
	CNINE_UNIMPL();
	//for_each([&x](const Gindex& ix, const TYPE v){
	//  if(x(ix)!=v) return false;});
      }
      return true;
    }

    bool operator!=(const TensorView<TYPE>& x) const{
      return !((*this)==x);
    }


  public: // ---- Getters ------------------------------------------------------------------------------------


    TYPE operator()(const Gindex& ix) const{
      CNINE_CHECK_RANGE(dims.check_in_range(ix,string(__PRETTY_FUNCTION__)));
      return arr[strides.offs(ix)];
    }

    TYPE operator()(const int i0) const{
      CNINE_CHECK_RANGE(dims.check_in_range(i0,string(__PRETTY_FUNCTION__)));
      return arr[strides.offs(i0)];
    }

    TYPE operator()(const int i0, const int i1) const{
      CNINE_CHECK_RANGE(dims.check_in_range(i0,i1,string(__PRETTY_FUNCTION__)));
      return arr[strides.offs(i0,i1)];
    }

    TYPE operator()(const int i0, const int i1, const int i2) const{
      CNINE_CHECK_RANGE(dims.check_in_range(i0,i1,i2,string(__PRETTY_FUNCTION__)));
      return arr[strides.offs(i0,i1,i2)];
    }

    TYPE operator()(const int i0, const int i1, const int i2, const int i3) const{
      CNINE_CHECK_RANGE(dims.check_in_range(i0,i1,i2,i3,string(__PRETTY_FUNCTION__)));
      return arr[strides.offs(i0,i1,i2,i3)];
    }


    TYPE get(const int i0) const{
      return operator()(i0);
    }

    TYPE get(const int i0, const int i1) const{
      return operator()(i0,i1);
    }

    TYPE get_value(const int i0) const{
      return operator()(i0);
    }

    TYPE get_value(const int i0, const int i1) const{
      return operator()(i0,i1);
    }

    TYPE get_value(const int i0, const int i1, const int i2) const{
      return operator()(i0,i1,i2);
    }


  public: // ---- Setters ------------------------------------------------------------------------------------


    void set(const Gindex& ix, const TYPE x) const{
      CNINE_CHECK_RANGE(dims.check_in_range(ix,string(__PRETTY_FUNCTION__)));
      arr[strides.offs(ix)]=x;
    }

    void set(const int i0, const TYPE x) const{
      CNINE_CHECK_RANGE(dims.check_in_range(i0,string(__PRETTY_FUNCTION__)));
      arr[strides.offs(i0)]=x;
    }

    void set(const int i0, const int i1,  const TYPE x) const{
      CNINE_CHECK_RANGE(dims.check_in_range(i0,i1,string(__PRETTY_FUNCTION__)));
      arr[strides.offs(i0,i1)]=x;
    }

    void set(const int i0, const int i1, const int i2, const TYPE x) const{
      CNINE_CHECK_RANGE(dims.check_in_range(i0,i1,i2,string(__PRETTY_FUNCTION__)));
      arr[strides.offs(i0,i1,i2)]=x;
    }

    void set(const int i0, const int i1, const int i2, const int i3, const TYPE x) const{
      CNINE_CHECK_RANGE(dims.check_in_range(i0,i1,i2,i3,string(__PRETTY_FUNCTION__)));
      arr[strides.offs(i0,i1,i2,i3)]=x;
    }


    void set_value(const int i0, const TYPE x) const{
      set(i0,x);
    }

    void set_value(const int i0, const int i1, const TYPE x) const{
      set(i0,i1,x);
    }


  public: // ---- Incrementers -------------------------------------------------------------------------------


    void inc(const Gindex& ix, const TYPE x) const{
      CNINE_CHECK_RANGE(dims.check_in_range(ix,string(__PRETTY_FUNCTION__)));
      arr[strides.offs(ix)]+=x;
    }

    void inc(const int i0, const TYPE x) const{
      CNINE_CHECK_RANGE(dims.check_in_range(i0,string(__PRETTY_FUNCTION__)));
      arr[strides.offs(i0)]+=x;
    }

    void inc(const int i0, const int i1,  const TYPE x) const{
      CNINE_CHECK_RANGE(dims.check_in_range(i0,i1,string(__PRETTY_FUNCTION__)));
      arr[strides.offs(i0,i1)]+=x;
    }

    void inc(const int i0, const int i1, const int i2, const TYPE x) const{
      CNINE_CHECK_RANGE(dims.check_in_range(i0,i1,i2,string(__PRETTY_FUNCTION__)));
      arr[strides.offs(i0,i1,i2)]+=x;
    }

    void inc(const int i0, const int i1, const int i2, const int i3, const TYPE x) const{
      CNINE_CHECK_RANGE(dims.check_in_range(i0,i1,i2,i3,string(__PRETTY_FUNCTION__)));
      arr[strides.offs(i0,i1,i2,i3)]+=x;
    }


  public: // ---- Rows and columns --------------------------------------------------------------------------


    TensorView<TYPE> row(const int i) const{
      CNINE_ASSRT(ndims()==2);
      CNINE_ASSRT(i<dims[0]);
      return TensorView<TYPE>(arr+strides[0]*i,{dims[1]},{strides[1]});
    }

    TensorView<TYPE> rows(const int i, const int n) const{
      CNINE_ASSRT(ndims()==2);
      CNINE_ASSRT(i+n<=dims[0]);
      return TensorView<TYPE>(arr+strides[0]*i,{n,dims[1]},{strides[0],strides[1]});
    }

    TensorView<TYPE> col(const int i) const{
      CNINE_ASSRT(ndims()==2);
      CNINE_ASSRT(i<dims[1]);
      return TensorView<TYPE>(arr+strides[1]*i,{dims[0]},{strides[0]});
    }

    TensorView<TYPE> cols(const int i, const int n) const{
      CNINE_ASSRT(ndims()==2);
      CNINE_ASSRT(i+n<=dims[1]);
      return TensorView<TYPE>(arr+strides[1]*i,{dims[0],n},{strides[0],strides[1]});
    }

    tensor1_view<TYPE> rowv(const int i) const{
      CNINE_ASSRT(ndims()==2);
      CNINE_ASSRT(i<dims[0]);
      return tensor1_view<TYPE>(arr.ptr()+strides[0]*i,dims[1],strides[1]);
    }


  public: // ---- Lambdas -----------------------------------------------------------------------------------


    void for_each(const std::function<void(const Gindex&, TYPE& x)>& lambda) const{
      dims.for_each_index([&](const Gindex& ix){
	  lambda(ix,const_cast<MemArr<TYPE>&>(arr)[strides.offs(ix)]);});
    }

    //void for_each(const std::function<void(const Gindex&, TYPE x)>& lambda) const{
    //dims.for_each_index([&](const Gindex& ix){
    //  lambda(ix,arr[strides.offs(ix)]);});
    //}


  public: // ---- Index changes ------------------------------------------------------------------------------


    //TensorView<TYPE> transp(){
    //return TensorView<TYPE>(arr,dims.transp(),strides.transp());
    //}

    const TensorView<TYPE> transp() const{
      return TensorView<TYPE>(arr,dims.transp(),strides.transp());
    }

    TensorView<TYPE> permute_indices(const vector<int>& p){
      return TensorView<TYPE>(arr,dims.permute(p),strides.permute(p));
    }

    TensorView<TYPE> reshape(const Gdims& _dims){
      CNINE_ASSRT(_dims.asize()==asize());
      CNINE_ASSRT(is_regular());
      return TensorView<TYPE>(arr,_dims,GstridesB(_dims));
    }

    TensorView<TYPE> inplace_reshape(const Gdims& _dims){
      CNINE_ASSRT(_dims.asize()==asize());
      CNINE_ASSRT(is_regular());
      dims=_dims;
      strides=GstridesB(_dims);
      return *this;
    }

    TensorView<TYPE> slice(const int d, const int i) const{
      CNINE_CHECK_RANGE(dims.check_in_range_d(d,i,string(__PRETTY_FUNCTION__)));
      return TensorView<TYPE>(arr+strides[d]*i,dims.remove(d),strides.remove(d)/*.inc_offset(strides[d]*i)*/);
    }

    TensorView<TYPE> slice(const Gindex& ix) const{
      const int k=ix.size();
      return TensorView<TYPE>(arr+strides.chunk(0,k)(ix),dims.chunk(k),strides.chunk(k)/*.set_offset(strides.chunk(0,k)(ix))*/);
    }

    TensorView<TYPE> slices(const int d, const int i, const int j) const{ //changed
      //CNINE_CHECK_RANGE(dims.check_in_range_d(d,i,string(__PRETTY_FUNCTION__)));
      if(i+j>0) {CNINE_CHECK_RANGE(dims.check_in_range_d(d,i+j-1,string(__PRETTY_FUNCTION__)));}
      Gdims _dims(dims);
      return TensorView<TYPE>(arr+strides[d]*i,_dims.set(d,j),strides);
    }

    TensorView<TYPE> unsqueeze(const int d) const{
      int s;
      if(d==0) s=strides.memsize(dims);
      else s=strides[d-1];
      return TensorView(arr,Gdims(dims).insert(d,1),GstridesB(strides).insert(d,s));
    }

    TensorView<TYPE> insert_dim(const int d, const int n) const{
      return TensorView(arr,Gdims(dims).insert(d,n),GstridesB(strides).insert(d,0));
    }

    TensorView<TYPE> cinflate(const int d, const int n) const{
      CNINE_ASSRT(dims[d]==1||dims[d]==n);
      if(dims[d]==n) return *this; 
      TensorView<TYPE> R(*this);
      R.dims[d]=n;
      R.strides[d]=0;
      return R;
    }

    TensorView<TYPE> block(const Gdims& _dims) const{
      CNINE_ASSRT(_dims<=dims);
      return TensorView<TYPE>(arr,_dims,strides);
    }

    TensorView<TYPE> block(const Gdims& _dims, const Gindex& offs) const{
      CNINE_ASSRT(offs+_dims<=dims);
      return TensorView<TYPE>(arr+strides.offs(offs),_dims,strides);
    }

    TensorView<TYPE> block(const int i0, const int i1, const int m0, const int m1) const{
      CNINE_ASSRT(ndims()==2);
      return block({m0,m1},{i0,i1});
    }

    TensorView<TYPE> block2(const int i0, const int i1, const int m0, const int m1) const{
      CNINE_ASSRT(ndims()==2);
      return block({m0,m1},{i0,i1});
    }

    TensorView<TYPE> diag() const{
      CNINE_ASSRT(ndims()==2);
      CNINE_ASSRT(dims[0]==dims[1]);
      return TensorView<TYPE>(arr,{dims[0]},{strides[0]+strides[1]});
    }

    TensorView<TYPE> tprod_view(const Gdims& dims1, const Gdims& dims2) const{
      int k=ndims();
      CNINE_ASSRT(dims1.size()==k);
      CNINE_ASSRT(dims2.size()==k);
      Gdims rdims(2*k,fill_raw());
      for(int i=0; i<k; i++){
	rdims[2*i]=dims1[i];
	rdims[2*i+1]=dims2[i];
      }
      GstridesB rstrides(2*k,fill_raw());
      for(int i=0; i<k; i++){
	rdims[2*i]=dims2[i]*strides[i];
	rdims[2*i+1]=strides[i];
      }
      return TensorView<TYPE>(arr,rdims,rstrides);
    }


  public: // ---- In-place Operations ------------------------------------------------------------------------


    void set_zero() const{
      if(dev==0){
	if(is_contiguous())
	  std::fill(mem(),mem()+asize(),0);
	else
	  CNINE_UNIMPL();
      }
      if(dev==1){
	if(is_contiguous()){
	  CUDA_SAFE(cudaMemset(mem(),0,asize()*sizeof(TYPE)));
	}else
	  CNINE_UNIMPL();
      }
    }


    void inplace_times(const TYPE c){
      if(dev==0){
	if(is_contiguous())
	  for(int i=0; i<asize(); i++) arr[i]*=c;
	else
	  for_each([&](const Gindex& ix, TYPE& x){x*=c;});
      }
      if(dev==1){
	if(is_contiguous()){
	  const float cr=c;
	  CUBLAS_SAFE(cublasSaxpy(cnine_cublas,asize(),&cr,mem(), 1,mem(), 1));
	}else
	  CNINE_UNIMPL();
      }
    }

    void normalize(){
      inplace_times(1.0/norm());
    }


  public: // ---- Cumulative Operations ----------------------------------------------------------------------


    void add(const TensorView& x){
      CNINE_DEVICE_SAME(x);
      CNINE_CHECK_SIZE(dims.check_eq(x.dims));
      assert(asize()==x.asize());
      if(dev==0){
	if(is_regular() && x.is_regular() && strides==x.strides){
	  TYPE* ptr=const_cast<MemArr<TYPE>&>(arr).get_arr();
	  TYPE* xptr=const_cast<MemArr<TYPE>&>(x.arr).get_arr();
	  for(size_t i=0; i<asize(); i++) ptr[i]+=xptr[i];
	}else
	  for_each([&](const Gindex& ix, TYPE& v){v+=x(ix);});
      }
      if(dev==1){
	if(is_regular() && x.is_regular() && strides==x.strides){
	  const float alpha=1.0; // todo
	  CUBLAS_SAFE(cublasSaxpy(cnine_cublas, asize(), &alpha, x.get_arr(), 1, get_arr(), 1));
	}else{
	  if(ndims()<=3){
	    if(ndims()==1) view1().add(x.view1());
	    if(ndims()==2) view2().add(x.view2());
	    if(ndims()==3) view3().add(x.view3());
	  }else{
	    cout<<strides<<x.strides<<endl;
	    CNINE_UNIMPL();
	  }
	}
      }
    }

    void operator+=(const TensorView& x){
      add(x);
    }

    void subtract(const TensorView& x) const{
      CNINE_DEVICE_SAME(x);
      CNINE_CHECK_SIZE(dims.check_eq(x.dims));
      CNINE_CPUONLY();
      assert(asize()==x.asize());
      if(dev==0){
	if(is_regular() && x.is_regular() && strides==x.strides){
	  TYPE* ptr=const_cast<MemArr<TYPE>&>(arr).get_arr();
	  TYPE* xptr=const_cast<MemArr<TYPE>&>(x.arr).get_arr();
	  for(size_t i=0; i<asize(); i++) ptr[i]-=xptr[i];
	}else
	  for_each([&](const Gindex& ix, TYPE& v){v-=x(ix);});
      }
      if(dev==1){
	if(is_contiguous() && x.is_contiguous() && strides==x.strides){
	  const float alpha=-1.0; // todo
	  //CUBLAS_SAFE(cublasSaxpy(cnine_cublas, asize(), &alpha, x.arr, 1, arr, 1));
	}else
	  CNINE_UNIMPL();
      }
    }

    void operator-=(const TensorView& x) const{
      add(x);
    }

    void add(const TensorView& x, const TYPE c){
      CNINE_DEVICE_SAME(x);
      CNINE_CHECK_SIZE(dims.check_eq(x.dims));
      CNINE_CPUONLY();
      assert(asize()==x.asize());
      if(dev==0){
	if(is_regular() && x.is_regular() && strides==x.strides){
	  TYPE* ptr=const_cast<MemArr<TYPE>&>(arr).get_arr();/*+strides.offset*/
	  TYPE* xptr=const_cast<MemArr<TYPE>&>(x.arr).get_arr()/*+x.strides.offset*/;
	  for(size_t i=0; i<asize(); i++) ptr[i]+=c*xptr[i];
	}else{
	  for_each([&](const Gindex& ix, TYPE& v){v+=c*x(ix);});
	}
      }
      if(dev==1){
	if(is_regular() && x.is_regular() && strides==x.strides){
	  const TYPE alpha=c;
	  //CUBLAS_SAFE(cublasSaxpy(cnine_cublas, asize(), &alpha, x.arr, 1, arr, 1));
	}else
	  CNINE_UNIMPL();
      }
    }

    void add_sum(const int d, const TensorView& x){
      CNINE_ASSRT(x.dims.size()>d);
      for(int i=0; i<x.dims[d]; i++)
	add(x.slice(d,i));
    }

    void subtract(const TensorView& x, const TYPE c){
      CNINE_DEVICE_SAME(x);
      CNINE_CHECK_SIZE(dims.check_eq(x.dims));
      CNINE_CPUONLY();
      assert(asize()==x.asize());
      if(dev==0){
	if(is_regular() && x.is_regular() && strides==x.strides){
	  TYPE* ptr=const_cast<MemArr<TYPE>&>(arr).get_arr();/*+strides.offset*/
	  TYPE* xptr=const_cast<MemArr<TYPE>&>(x.arr).get_arr()/*+x.strides.offset*/;
	  for(size_t i=0; i<asize(); i++) ptr[i]-=c*xptr[i];
	}else{
	  for_each([&](const Gindex& ix, TYPE& v){v-=c*x(ix);});
	}
      }
      if(dev==1){
	if(is_regular() && x.is_regular() && strides==x.strides){
	  const TYPE alpha=c;
	  //CUBLAS_SAFE(cublasSaxpy(cnine_cublas, asize(), &alpha, x.arr, 1, arr, 1));
	}else
	  CNINE_UNIMPL();
      }
    }

    void add_prod(const TensorView& x, const TensorView& y) const{
      CNINE_DEVICE_SAME(x);
      CNINE_DEVICE_SAME(y);
      CNINE_DIMS_SAME(x);
      CNINE_DIMS_SAME(y);
      if(dev==0){
	if(is_regular() && x.is_regular() && y.is_regular() && strides==x.strides&& strides==y.strides){
	  TYPE* ptr=const_cast<MemArr<TYPE>&>(arr).get_arr();
	  TYPE* xptr=const_cast<MemArr<TYPE>&>(x.arr).get_arr();
	  TYPE* yptr=const_cast<MemArr<TYPE>&>(y.arr).get_arr();
	  for(int i=0; i<asize(); i++) ptr[i]+=xptr[i]*yptr[i];
	}else
	  for_each([&](const Gindex& ix, TYPE& v){v+=x(ix)*y(ix);});
      }
      if(dev==1){
	CNINE_UNIMPL();
      }
    }

    void add_broadcast(const int d, const TensorView& x){
      CNINE_ASSRT(d<dims.size());
      CNINE_CPUONLY();
      if(dev==0)
	for(int i=0; i<dims[d]; i++)
	  slice(d)+=x;
    }

    void add_ReLU(const TensorView& x, const float alpha){
      CNINE_CHECK_SIZE(dims.check_eq(x.dims));
      assert(x.get_dev()==get_dev());
      int N=asize();
      if(dev==0){
	for(int i=0; i<N; i++) 
	  arr[i]+=((x.arr[i]>0)+alpha*(x.arr[i]<0))*x.arr[i];
      }
      if(dev==1){
	flat_view().add_ReLU(x.flat_view(),alpha);
      }
    }

    void add_ReLU_back(const TensorView& g, const TensorView& x, const float alpha){
      CNINE_CHECK_SIZE(dims.check_eq(g.dims));
      assert(g.get_dev()==get_dev());
      int N=asize();
      if(dev==0){
	for(int i=0; i<N; i++)
	  arr[i]+=((g.arr[i]>0)+alpha*(g.arr[i]<0))*g.arr[i];
      }
      if(dev==1){
	flat_view().add_ReLU_back(g.flat_view(),x.flat_view(),alpha);
      }
    }


  public: // ---- Matrix multiplication ---------------------------------------------------------------------


    void add_mvprod(const TensorView& x, const TensorView& y) const{
      reconcile_devices<TensorView<TYPE> >(*this,x,y,[](const TensorView<TYPE>& r, const TensorView<TYPE>& x, const TensorView<TYPE>& y){
	  CNINE_NDIMS_IS_1(r);
	  CNINE_NDIMS_IS_2(x);
	  CNINE_NDIMS_IS_1(y);
	  CNINE_ASSRT(x.dims[0]==r.dims[0]);
	  CNINE_ASSRT(x.dims[1]==y.dims[0]);

	  if(r.dev==0){
	    for(int i=0; i<r.dims[0]; i++){
	      TYPE t=0;
	      for(int k=0; k<x.dims[1]; k++)
		t+=x(i,k)*y(k);
	      r.inc(i,t);
	    }
	  }
	  if(r.dev==1){
	    CNINE_UNIMPL();
	  }
	});
    }


    void add_mvprod_T(const TensorView& x, const TensorView& y){
      reconcile_devices<TensorView<TYPE> >(*this,x,y,[](TensorView<TYPE>& r, const TensorView<TYPE>& x, const TensorView<TYPE>& y){
	  CNINE_NDIMS_IS_1(r);
	  CNINE_NDIMS_IS_2(x);
	  CNINE_NDIMS_IS_1(y);
	  CNINE_ASSRT(x.dims[1]==r.dims[0]);
	  CNINE_ASSRT(y.dims[0]==y.dims[0]);

	  if(r.dev==0){
	    for(int i=0; i<r.dims[0]; i++){
	      TYPE t=0;
	      for(int k=0; k<x.dims[1]; k++)
		t+=x(k,i)*y(k);
	      r.inc(i,t);
	    }
	  }
	  if(r.dev==1){
	    CNINE_UNIMPL();
	  }
	});
    }


    void add_mprod(const TensorView& x, const TensorView& y) const{
      reconcile_devices<TensorView<TYPE> >(*this,x,y,[]
	(const TensorView<TYPE>& r, const TensorView<TYPE>& x, const TensorView<TYPE>& y){
	  CNINE_NDIMS_IS_2(r);
	  CNINE_NDIMS_IS_2(x);
	  CNINE_NDIMS_IS_2(y);
	  CNINE_ASSRT(x.dims[0]==r.dims[0]);
	  CNINE_ASSRT(y.dims[1]==r.dims[1]);
	  CNINE_ASSRT(x.dims[1]==y.dims[0]);

	  r.view2().add_mprod(x.view2(),y.view2());

	  /*
	  if(r.dev==0){
	    for(int i=0; i<r.dims[0]; i++)
	      for(int j=0; j<r.dims[1]; j++){
		TYPE t=0;
		for(int k=0; k<x.dims[1]; k++)
		  t+=x(i,k)*y(k,j);
		r.inc(i,j,t);
	      }
	  }
	  if(r.dev==1){
	    CNINE_UNIMPL();
	  }
	  */

	});
    }


  public: // ---- Tensor multiplication ---------------------------------------------------------------------


    void add_tprod(const TensorView& x, const TensorView& y) const{
      reconcile_devices<TensorView<TYPE> >(*this,x,y,[&](const TensorView<TYPE>& r, const TensorView<TYPE>& x, const TensorView<TYPE>& y){
	  //CNINE_NDIMS_IS_2(r);
	  //CNINE_NDIMS_IS_2(x);
	  //CNINE_NDIMS_IS_2(y);
	  CNINE_ASSRT(x.ndims()==y.ndims());

	  if(x.ndims()==1){
	    CNINE_ASSRT(r.dims[0]==x.dims[0]*y.dims[0]);
	      for(int i=0; i<x.dims[0]; i++)
		block({y.dims[0]},{i*y.dims[0]}).add(y,x(i));
	    return;
	  }
	  
	  if(x.ndims()==2){
	    CNINE_ASSRT(r.dims[0]==x.dims[0]*y.dims[0]);
	    CNINE_ASSRT(r.dims[1]==x.dims[1]*y.dims[1]);
	    for(int i=0; i<x.dims[0]; i++)
	      for(int j=0; j<x.dims[1]; j++)
		block({y.dims[0],y.dims[1]},{i*y.dims[0],j*y.dims[1]}).add(y,x(i,j));
	    return;
	  }

	  if(x.ndims()==3){
	    CNINE_ASSRT(r.dims[0]==x.dims[0]*y.dims[0]);
	    CNINE_ASSRT(r.dims[1]==x.dims[1]*y.dims[1]);
	    CNINE_ASSRT(r.dims[2]==x.dims[2]*y.dims[2]);
	    for(int i=0; i<x.dims[0]; i++)
	      for(int j=0; j<x.dims[1]; j++)
		for(int k=0; k<x.dims[2]; k++)
		  block(y.dims,{i*y.dims[0],j*y.dims[1],k*y.dims[2]}).add(y,x(i,j,k));
	    return;
	  }
	    
	  CNINE_UNIMPL();
	});
    }


  public: // ---- Scaling -----------------------------------------------------------------------------------

    
    void add_scale_columns(const TensorView<TYPE>& x, const TensorView& y){
      CNINE_DEVICE_SAME(x);
      CNINE_DEVICE_SAME(y);
      CNINE_ASSRT(x.ndims()==2);
      CNINE_ASSRT(y.ndims()==1);
      int n=dim(0);
      int nc=dim(1);
      CNINE_ASSRT(x.dim(0)==n);
      CNINE_ASSRT(x.dim(1)==nc);
      CNINE_ASSRT(y.dim(0)==nc);
      if(dev==0){
	for(int i=0; i<n; i++)
	  for(int j=0; j<nc; j++)
	    arr[i*nc+j]+=x.arr[i*nc+j]*y.arr[j];
      }
      if(dev==1){
	CUBLAS_SAFE(cublasSdgmm(cnine_cublas,CUBLAS_SIDE_LEFT,nc,n,get_arr(),nc,y.get_arr(),1,get_arr(),nc));
	//CUBLAS_SAFE(cublasSdgmm(cnine_cublas,CUBLAS_SIDE_LEFT,nc,tail/nc,arrg,nc,y.arr,1,R.arrg,R.nc));
      }    
    }



  public: // ---- Not in-place operations -------------------------------------------------------------------




  public: // ---- Scalar valued operations ------------------------------------------------------------------


    TYPE max() const{
      if(asize()==0) return 0;
      TYPE t=arr[0];
      if(is_contiguous()){
	for(size_t i=0; i<asize(); i++)
	  if(arr[i]>t) t=arr[i];
      }else{
	for_each([&](const Gindex& ix, TYPE& v){
	    if(v>t) t=v;});
      }
      return t; 
    }

    TYPE min() const{
      if(asize()==0) return 0;
      TYPE t=arr[0];
      if(is_contiguous()){
	for(size_t i=0; i<asize(); i++)
	  if(arr[i]<t) t=arr[i];
      }else{
	for_each([&](const Gindex& ix, TYPE& v){
	    if(v<t) t=v;});
      }
      return t; 
    }

    auto max_abs() const -> decltype(std::real(min())){
      if(asize()==0) return 0;
      decltype(std::real(min())) t=std::real(arr[0]);
      if(is_contiguous()){
	for(int i=0; i<asize(); i++)
	  if(abs(arr[i])>t) t=abs(arr[i]);
      }else{
	CNINE_UNIMPL()
	//for_each([&](const Gindex& ix, TYPE& v){
	//  if(abs(v)>t) t=abs(v);});
      }
      return t; 
    }

    TYPE inp(const TensorView& y) const{
      CNINE_CPUONLY();
      CNINE_ASSRT(dims==y.dims);
      TYPE t=0;
      if(dev==0){
	if(is_regular() && y.is_regular()){
	  for(size_t i=0; i<asize(); i++)
	    t+=arr[i]*y.arr[i];
	  //t+=std::conj(arr[i])*y.arr[i];
	}else{
	  for_each([&](const Gindex& ix, TYPE& v){
	      t+=v*y(ix);});
	  //t+=std::conj(v)*y(ix);});
	}
      }
      return t;
    }

    TYPE norm2() const{
      CNINE_CPUONLY();
      TYPE t=0;
      if(dev==0){
	if(is_contiguous()){
	  for(size_t i=0; i<asize(); i++)
	    t+=arr[i]*arr[i];
	  //t+=std::conj(arr[i])*arr[i];
	}else{
	  for_each([&](const Gindex& ix, TYPE& v){
	      t+=v*v;});
	  //t+=std::conj(v)*v;});
	}
      }
      return t;
    }

    TYPE norm() const{
      return sqrt(norm2());
    }

    TYPE diff2(const TensorView& x) const{
      CNINE_ASSRT(x.asize()==asize());
      if(get_dev()==0 && x.get_dev()>0)
	return diff2(TensorView(x,0));
      TYPE t=0;
      if(is_regular() && x.is_regular()){
	for(size_t i=0; i<asize(); i++){
	  const TYPE a=x.arr[i]-arr[i];
	  if constexpr(is_complex<TYPE>())
	    t+=a*std::conj(a);
	  else
	    t+=a*a;
	}
      }else{
	for_each([&](const Gindex& ix, TYPE& v){
	    const TYPE a=x(ix)-(*this)(ix);
	    if constexpr(is_complex<TYPE>())
	      t+=a*std::conj(a);
	    else
	      t+=a*a;
	  });
      }
      return t;
    }

    TYPE unitary_error() const{
      CNINE_ASSRT(dims[0]==dims[1]);
      TensorView A({dims[0],dims[0]},fill_zero());
      A.add_mprod(*this,transp());
      for(int i=0; i<dims[0]; i++)
	A.inc(i,i,-1.0);
      return A.norm();
    }


  public: // ---- Index manipulations -----------------------------------------------------------------------


    TensorView fuse01() const{
      CNINE_ASSRT(ndims()>=2);
      Gdims d=dims.remove(1); 
      d[0]*=dims[1];
      GstridesB s=strides.remove(0);
      return TensorView(arr,d,s);
    }

    TensorView split0(const int a, const int b) const{
      CNINE_ASSRT(ndims()>=1);
      CNINE_ASSRT(dims[0]==a*b);
      Gdims d=dims.insert(0,a); 
      d[1]=b; 
      GstridesB s=strides.insert(0,strides[0]*b);
      return TensorView(arr,d,s);
    }

    TensorView split1(const int a, const int b) const{
      CNINE_ASSRT(ndims()>=2);
      CNINE_ASSRT(dims[1]==a*b);
      Gdims d=dims.insert(1,a); 
      d[2]=b; 
      GstridesB s=strides.insert(1,strides[1]*b);
      return TensorView(arr,d,s);
    }



  public: // ---- I/O ---------------------------------------------------------------------------------------


    string classname() const{
      return "TensorView";
    }

    string describe() const{
      ostringstream oss;
      oss<<"TensorView"<<dims<<" ["<<strides<<"]"<<endl;
      return oss.str();
    }

    string repr() const{
      ostringstream oss;
      oss<<"TensorView"<<dims<<" ["<<strides<<"]"<<endl;
      return oss.str();
    }

    /*
    template<typename U=TYPE, typename = typename std::enable_if<std::is_same<U,double>::value, U>::type>
    float prnt_limit() const{
      return (std::max(-min(),max()))/10e5;
    }

    template<typename U=TYPE, typename = typename std::enable_if<!std::is_same<U,double>::value, U>::type>
    float prnt_limit() const{
      return 0;
    }
    */

    string str(const string indent="") const{
      if(dev>0) return TensorView(*this,0).str(indent);
      ostringstream oss;

      //TYPE largest=std::max(-min(),max());
      float limit;
      if constexpr(std::is_same<TYPE,complex<float> >::value)
	limit=std::real(max_abs())/10e5;
      else 
	limit=max_abs()/10e5;

      if(ndims()==1){
	oss<<indent<<"[ ";
	for(int i0=0; i0<dims[0]; i0++)
	  if(abs((*this)(i0))>limit) oss<<(*this)(i0)<<" ";
	  else oss<<TYPE(0)<<" ";
	oss<<"]"<<endl;
	return oss.str();
      }

      if(ndims()==2){
	for(int i0=0; i0<dims[0]; i0++){
	  oss<<indent<<"[ ";
	  for(int i1=0; i1<dims[1]; i1++)
	    if(abs((*this)(i0,i1))>limit) oss<<(*this)(i0,i1)<<" ";
	    else oss<<TYPE(0)<<" ";
	  oss<<"]"<<endl;
	}
	return oss.str();
      }

      if(ndims()>2){
	Gdims adims=dims.chunk(0,ndims()-2);
	adims.for_each_index([&](const Gindex& ix){
	    oss<<indent<<"Slice"<<ix<<":"<<endl;
	    oss<<slice(ix).str(indent+"  ")<<endl;
	  });
      }

      return oss.str();
    }

    friend ostream& operator<<(ostream& stream, const TensorView<TYPE>& x){
      stream<<x.str(); return stream;
    }

  };


  // ---- Functions ----------------------------------------------------------------------------------------------

  
  template<typename TYPE>
  TYPE inp(const TensorView<TYPE>& x, const TensorView<TYPE>& y){
    return x.inp(y);
  }

  template<typename TYPE>
  TYPE norm2(const TensorView<TYPE>& x){
    return x.norm2();
  }

  template<typename TYPE>
  TYPE norm(const TensorView<TYPE>& x){
    return x.norm();
  }


  // ---- View converters ----------------------------------------------------


  inline Itensor1_view view1_of(const TensorView<int>& x){
    return Itensor1_view(x.mem(),x.dim(0),x.stride(0),x.get_dev());}
  inline Rtensor1_view view1_of(const TensorView<float>& x){
    return Rtensor1_view(x.mem(),x.dim(0),x.stride(0),x.get_dev());}
  inline Rtensor1_view view1_of(const TensorView<double>& x){// hack!!
    return Rtensor1_view(reinterpret_cast<float*>(x.mem()),x.dim(0),x.stride(0),x.get_dev());}
  inline Ctensor1_view view1_of(const TensorView<complex<float> >& x){
    return Ctensor1_view(x.mem_as<float>(),x.mem_as<float>()+1,x.dim(0),2*x.stride(0),x.get_dev());}

  inline Itensor2_view view2_of(const TensorView<int>& x){
    return Itensor2_view(x.mem(),x.dim(0),x.dim(1),x.stride(0),x.stride(1),x.get_dev());}
  inline Rtensor2_view view2_of(const TensorView<float>& x){
    return Rtensor2_view(x.mem(),x.dim(0),x.dim(1),x.stride(0),x.stride(1),x.get_dev());}
  inline Rtensor2_view view2_of(const TensorView<double>& x){ // hack!!
    return Rtensor2_view(reinterpret_cast<float*>(x.mem()),x.dim(0),x.dim(1),x.stride(0),x.stride(1),x.get_dev());}
  inline Ctensor2_view view2_of(const TensorView<complex<float> >& x){
    return Ctensor2_view(x.mem_as<float>(),x.mem_as<float>()+1,
      x.dim(0),x.dim(1),2*x.stride(0),2*x.stride(1),x.get_dev());}

  inline Itensor3_view view3_of(const TensorView<int>& x){
    return Itensor3_view(x.mem(),x.dim(0),x.dim(1),x.dim(2),x.stride(0),x.stride(1),x.stride(2),x.get_dev());}
  inline Rtensor3_view view3_of(const TensorView<float>& x){
    return Rtensor3_view(x.mem(),x.dim(0),x.dim(1),x.dim(2),x.stride(0),x.stride(1),x.stride(2),x.get_dev());}
  inline Rtensor3_view view3_of(const TensorView<double>& x){
    return Rtensor3_view(reinterpret_cast<float*>(x.mem()),x.dim(0),x.dim(1),x.dim(2),x.stride(0),x.stride(1),x.stride(2),x.get_dev());}
  inline Ctensor3_view view3_of(const TensorView<complex<float> >& x){
    return Ctensor3_view(x.mem_as<float>(),x.mem_as<float>()+1,
      x.dim(0),x.dim(1),x.dim(2),2*x.stride(0),2*x.stride(1),2*x.stride(2),x.get_dev());}

  inline Rtensor1_view flat_view_of(const TensorView<float>& x){
    CNINE_ASSRT(x.is_contiguous());
    return Rtensor1_view(x.mem(),x.asize(),1,x.get_dev());}

}

#define _CnineTensorViewComplete

#endif


