/* * * * *
 *  AzPmat.hpp 
 *  Copyright (C) 2013-2015 Rie Johnson
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * * * * */

#ifndef _AZ_PMAT_HPP_
#define _AZ_PMAT_HPP_

#include <time.h>
#include "AzUtil.hpp"
#include "AzDmat.hpp"

#ifdef __AZ_GPU__
  #include "AzPmat_gpu.cuh"
#else 
  #include "AzPmat_cpu.hpp"
#endif 

/*
 *  Classes beginning with "_AzP" are specific to GPU/CPU.  They are in AzPmat_gpu/cpu.* .
 */

class AzPmat {
protected:
  bool locked; 
  int row_num, col_num; 

  _AzParr<AzFloat> data; 
  _AzPmat u; 

public:
  friend class AzPmatApp; 
  friend class AzPmatApp_ab;   
  friend class AzPmat_ptr; 
  friend class AzPmatVar; 
  friend class AzPrng; 
  friend class AzPmatSpa; 
  
  AzPmat() : locked(false), row_num(0), col_num(0) {}
  AzPmat(int r_num, int c_num) : locked(false), row_num(0), col_num(0) { reform(r_num, c_num); }
  AzPmat(const AzPmat *inp, double coeff=1) : locked(false), row_num(0), col_num(0) { set(inp, coeff); }
  AzPmat(const AzDmat *inp)  : locked(false), row_num(0), col_num(0) { set(inp); }
  AzPmat(const AzSmat *inp)  : locked(false), row_num(0), col_num(0) { set(inp); }  
  AzPmat(const AzDvect *inp) : locked(false), row_num(0), col_num(0) { set(inp); }  
  AzPmat(const char *fn)     : locked(false), row_num(0), col_num(0) { read(fn); }  
  AzPmat(const AzPmat &inp) {
    AzX::throw_if(true, "AzPmat(const&)", "= is prohibited on AzPmat"); 
  }
  AzPmat & operator =(const AzPmat &inp) {
    AzX::throw_if(true, "AzPmat operator = ", "= is prohibited on AzPmat"); 
  }
  inline void lock() { locked = true; }
  inline void unlock() { locked = false; }
  inline void change_dim(int r_num, int c_num) {
    const char *eyec = "AzPmat::change_dim"; 
    AzX::throw_if((r_num*c_num != row_num*col_num), eyec, "#row*#col must be the same"); 
    row_num = r_num; 
    col_num = c_num; 
  }
  inline virtual void reset() {
    check_lock("AzPmat::reset"); 
    data.free(); 
    row_num = col_num = 0; 
  }
  
  inline int size() const {
    return row_num*col_num; 
  }

  /*---  set  ---*/
  void set(const AzPmat *m, double coeff=1);                       /* this <- m*coeff */
  void set(int col1, const AzPmat *m2, int col2, double coeff=1);  /* this[,col1] <- m2[,col2]*coeff */
  void set(const AzDmat *m);  /* this <- m */
  void set(const AzSmat *ms) { AzX::throw_if_null(ms, "AzPmat::set(smat)", "ms"); AzDmat md(ms); set(&md); }
  void set(const AzDmat *m, int col0, int col1); /* this <- m[col0:col1-1] */
  void set(const AzDvect *v); 
  void set(int col, const AzDvect *v); /* this[,col] <- v */
  void set(int col1, int c_num, const AzPmat *m2, int col2=0, double coeff=1); /* this[,col1::c_num] <- m2[,col2::c_num]*coeff */ 
  void set(int col, int c_num, const AzFloat *p_host, int data_len);  
#if 0   
  void set(const AzPmat *m, const int *cols, int cnum, bool do_zero_negaindex=false, AzFloat coeff=1);  
#else  
  void set(const AzPmat *m, const int *cols, int cnum, AzFloat coeff=1); /* 8/28/2015 */
  void copy(const AzPmat *m, const int *cols, int cnum, bool do_zero_negaindex=false, AzFloat coeff=1); /* 8/28/2015 */
#endif   
  void set(const AzPmat *m2, int m2_col0, int m2_col1);  /* this <- m2[col0:col1-1] (not tested!) */

  void set(const AzDmatc *m, const int *cols, int cols_num); 
  void copy_col2col(const AzPmat *m, const int *cols, int cnum); 
  void copy_col2col(const AzPmat *m, const AzIntArr &ia_cols) { copy_col2col(m, ia_cols.point(), ia_cols.size()); }
  
  void add_d2s(const AzPmat *m, const int *cols, int cnum, double coeff=1);  
  void add_d2s(const AzPmat *m, const AzIntArr &ia, double coeff=1) { add_d2s(m, ia.point(), ia.size(), coeff); }
  void add_s2d(const AzPmat *m, const int *cols, int cnum, double coeff=1);  
  void add_s2d(const AzPmat *m, const AzIntArr &ia, double coeff=1) { add_s2d(m, ia.point(), ia.size(), coeff); }
  void add_rows_s2d(const AzPmat *m, const int *rows, int rnum, double coeff=1);
  
  void resize(int new_cnum) {
    if (new_cnum == col_num) return; 
    if (new_cnum == 0) {
      reform(row_num, 0); 
      return; 
    }
    if (new_cnum < col_num) {
      AzPmat m; m.set(this, 0, new_cnum); 
      set(&m); 
    }
    else {
      AzPmat m(this); 
      reform(row_num, new_cnum); 
      set(0, m.colNum(), &m); 
    }
  }

  void cbind(const AzPmat *m1) {
    if (size() <= 0) {
      set(m1); 
      return; 
    }
    AzX::throw_if((row_num != m1->rowNum()), "AzPmat::cbind", "#row mismatch"); 
    AzPmat m0(this); 
    reform_noinit(m1->rowNum(), m0.colNum()+m1->colNum()); 
    set(0, m0.colNum(), &m0); 
    set(m0.colNum(), m1->colNum(), m1); 
  }
  void cbind(const AzDataArr<AzPmat> *am); 

  void set_rowwise(int row1, int r_num, const AzPmat *m2, int row2); /* this[row1::r_num,] <- m2[row2::r_num,] */
  void set_submatrix(int row1, int r_num, int col1, int c_num, const AzPmat *m2, int row2, int col2); /* this[row1::r_num,col1:;c_num] <- m2[row2::r_num,col2::c_num] */
  void rbind(const AzPmat *m); 
  bool can_rbind(const AzPmat *m) {
    if (size() <= 0) return true; 
    if (m == NULL) return false; 
    return (col_num == m->col_num); 
  }
  
  void set(double val); /* set all components to val */
  void set(int col, double val); /* set all components of this[,col] to val */

  inline void zeroOut(int col=-1) {
    if (col < 0) set((double)0); 
    else         set(col, (double)0); 
  }
  inline void set_chk(const AzPmat *m, bool do_chk, double coeff=1) {
    if (do_chk) set_chk(m, coeff); 
    else        set(m, coeff); 
  }
  inline void set_chk(const AzPmat *m, double coeff=1) {
    AzX::throw_if((row_num  != m->row_num || col_num != m->col_num), "AzPmat::set_chk(Pmat)", "shape mismatch"); 
    set(m, coeff); 
  }
  inline void set_chk(const AzDmat *m) {
    AzX::throw_if((row_num  != m->rowNum() || col_num != m->colNum()), "AzPmat::set_chk(Dmat)", "shape mismatch"); 
    set(m); 
  }

  void setRow(int row, double val); /* this[row,] <- val */
  
  void setIdentity(int dim); /* set the identity matrix of size dim x dim */
  
  /*---  fill (different #row and #col, but #row*#col is the same  ---*/
  void fill(const AzPmat *m); 
  
  /*---  get  ---*/
  void get(AzDmat *md, bool do_chk=false) const;  /* md <- this */
  inline void get(AzSmat *ms, bool do_chk=false) const {  /* ms <- this */
    AzDmat md; 
    get(&md, do_chk); 
    md.convert_to(ms); 
  }
  void get(int col, AzDvect *v, bool do_chk=false) const; /* v <- this[,col] */
  void get(AzDvect *v, bool do_chk=false) const; 
  
  /*---  copy to ...  ---*/
  void copy_to(AzDmatc *m, int dmatc_col) const; 
  
  /*---  high overhead  ---*/
  double get(int row, int col) const; 
  void set(int row, int col, double val); 
  
  /*---  reform  ---*/
  inline virtual void reform_tmpl(const AzPmat *m_tmpl) { /* with template */
    AzX::throw_if((m_tmpl == NULL), "AzPmat::reform_tmpl", "no template"); 
    reform(m_tmpl->row_num, m_tmpl->col_num); 
  } 
  inline virtual void reform(int r_num, int c_num) { /* shape the matrix and fill with zero */
    reform_noinit(r_num, c_num); 
    set((double)0);   
  }
  inline virtual void reform_chk(int r_num, int c_num, bool do_chk, const char *msg) {
    AzX::throw_if((do_chk && (row_num != r_num || col_num != c_num)), "AzPmat::reform_chk", "shape mismatch", msg); 
    reform(r_num, c_num); 
  }

  /*---  read and write  ---*/
  void write(AzFile *file) const { /* write to a file directly */
    int sz = sizeof(AzFloat); 
    file->writeInt(sz); 
    file->writeInt(row_num); 
    file->writeInt(col_num); 
    data.write(file); 
  }
  void read(AzFile *file) { /* read from a file directly */
    reset(); 
    int sz = file->readInt(); 
    int exp_sz = sizeof(AzFloat); 
    if (sz != exp_sz) {
      AzBytArr s("floating-point precision mismatch: expected="); s << exp_sz << ", actual=" << sz; 
      AzX::throw_if(true, AzInputError, "AzPmat::read", s.c_str()); 
    }
    row_num = file->readInt(); 
    col_num = file->readInt(); 
    data.read(file); 
  }  
  void write(const char *fn) const { AzFile::write(fn, this); }
  void read(const char *fn) { AzFile::read(fn, this); }
  
  /*---    ---*/
  inline int rowNum() const { return row_num; }  
  inline int colNum() const { return col_num; }
  inline void destroy() {
    reset();  
  }

  /*---  add: this += m*coeff  ---*/   
  void add(const AzPmat *m, double coeff=1) { add_cublas(m, coeff); }
  void add_cublas(const AzPmat *m, double coeff=1);
  void add_noblas(const AzPmat *m, double coeff=1);

  /*---  add  ---*/   
  void add(double coeff, const AzPmat *m0, double coeff0); /* this = this*coeff+m0*coeff0 */
  void add(double coeff, const AzPmat *m0, double coeff0, const AzPmat *m1, double coeff1); /* this = this*coeff+m0*coeff0+m1*coeff1 */
  void add(int col1, const AzPmat *m2, int col2, double coeff=1); /* this[,col1] += m2[,col2]*coeff */
  void add(int col1, int c_num, const AzPmat *m2, int col2=0, double coeff=1); /* this[,col1:col1+c_num-1] += m2[,col2:col2+c_num-1]*coeff */
  void add(double val); /* add val to all the components */
  void add(int col1, double val); /* add val to all the components of this[,col1] */

  void add_square(double coeff, const AzPmat *m0, double coeff0); /* this[i,j] = this[i,j]*coeff+m0[i,j]^2*coeff0 */
  
  /*---  subtraction  ---*/
  inline void sub(const AzPmat *m) { add(m,(double)(-1)); } 
  inline void sub(int col1, const AzPmat *m2, int col2) { add(col1,m2,col2, (double)(-1)); }
  inline void sub(int col1, int c_num, const AzPmat *m2, int col2=0) { add(col1,c_num,m2,col2, (double)(-1)); }
  
  /*---  scale (element-wise multiplication)  ---*/   
  void elm_multi(const AzPmat *m2, bool do_inv=false); 
  void elm_multi(int col1, const AzPmat *m2, int col2, bool do_inv=false); 
  void elm_multi(int col1, int c_num, const AzPmat *m2, int col2, bool do_inv=false); /* this[,col1:col1+c_num-1] *= m2[,col2:col2+cnum-1] */

  void scale_by_sqrt(const AzPmat *m_sq, double epsilon, bool do_inv=false); /* this[,] *= sqrt(sq[,]+epsilon) */
    
  /*---  multiply a constant: this *= val  ---*/  
  void multiply(double val) { 
    if (val == 0) {zeroOut(); return; }
    multiply_noblas(val); 
  }
  void multiply_cublas(double val); 
  void multiply_noblas(double val); 
  inline void divide(double val) { /* this /= val */
    AzX::throw_if((val == 0), "AzPmat::divide", "division by zero"); 
    multiply((double)1/val); 
  }
  void multiply(int col, double val) { multiply_noblas(col, val); } /* this[,col] *= val */
  void multiply_cublas(int col, double val); 
  void multiply_noblas(int col, double val); 
  inline void divide(int col, double val) { /* this[,col] /= val */
    AzX::throw_if((val == 0), "AzPmat::divide(col)", "division by zero");   
    multiply(col, (double)1/val); 
  }
  void multiply(int col, int cnum, double val) { multiply_noblas(col, cnum, val); } /* this[,col::cnum] *= val */
  void multiply_cublas(int col, int cnum, double val); 
  void multiply_noblas(int col, int cnum, double val); 
  inline void divide(int col, int cnum, double val) { /* this[,col::cnum] /= val */
    multiply(col, cnum, (double)1/val); 
  }
  void multiply_eachcol(const AzPmat *m2, bool do_inv=false); /* for each column c: this[,c] *= m2[0,c] */
  
  /*---  truncate into [minval, maxval]  ---*/
  void truncate(double minval, double maxval);          /* nop if minval > maxval */
  void truncate(int col, double minval, double maxval); /* nop if minval > maxval */

  /*---  repmat: make num_r x num_c tiles of src  ---*/
  void repmat_from(const AzPmat *m_src, int num_r, int num_c);   /* this <- tiles */
  void add_repmat(const AzPmat *m_src, int num_r, int num_c);   /* this += tiles */
  
  /*---  transpose_from: this <- transpose(m)  ---*/
  void transpose_from(const AzPmat *m) { transpose_from_cublas(m); }
  void transpose_from_noblas(const AzPmat *m);
  void transpose_from_cublas(const AzPmat *m);
  
  /*---  element-wise operations  ---*/
  void binarize();  /* for all the components: set: 1 if x>0; -1 if x<0; 0 if x=0*/
                    /* (NOTE) binarize did current binarize2 until 2/10/2014 */                 
  void binarize1();  /* for all the components: set (x!=0)?1:0 */
  void mark_eq(AzFloat v); /* for all the components: set (x==v)?1:0 */
  void mark_positive(); /* for all the components: set (x>0)?1:0 */
  void mark_negative(); /* for all the components: set (x<0)?1:0 */  
  void mark_gt(AzFloat v);  /* for all the components: set (x>v)?1:0 */ 
  void mark_lt(AzFloat v);  /* for all the components: set (x<v)?1:0 */ 
  void mark_ge(AzFloat v);  /* for all the components: set (x>=v)?1:0 */ 
  void mark_le(AzFloat v);  /* for all the components: set (x<=v)?1:0 */   
  void mark_le_rowth(const AzDvect *v_rowth, AzFloat coeff); /* for every column: (x[r,]<=rowth[r])?coeff:0 */
  void exp(AzPmat *m_mask=NULL); /* this <- exp(this) */
  void log(); /* this <- log(this) */
  void squareroot(); /* this <- sqrt(this) */
  void square(); /* this[,] <- this[,]*this[,] */
  void inverse(); /* this[,] <- 1/this[,] if not 0 */
  
  /*---  sum  ---*/
  double sum(int col=-1) const { return sum_cublas(col); } /* sum of all the components if col < 0 */
                                                           /* sum of this[,col] if col >= 0 */
  double sum_cublas(int col=-1) const;   
  double sum_noblas(int col=-1) const; 

  inline double average(int col=-1) const {
    if (row_num <= 0 || col_num <= 0) return 0; 
    if (col < 0) {
      return sum()/(double)row_num/(double)col_num; 
    }
    else {
      return sum(col)/(double)row_num; 
    }
  }

  double norm2(int col=-1) const { return norm2_cublas(col); }
  double norm2_cublas(int col=-1) const;
  double norm2_noblas(int col=-1) const { return sqrt(squareSum_noblas(col)); }
  
  /* squareSum: square sum of all the components if col < 0 */
  /*            square sum of this[,col] if col>= 0 */  
  double squareSum(int col=-1) const { return squareSum_cublas(col); }
  double squareSum_cublas(int col=-1) const { 
    double nrm2 = norm2_cublas(col); 
    return (nrm2*nrm2); 
  }
  double squareSum_noblas(int col=-1) const; 
  
  double squareSum(int col0, int cnum) const; /* square sum of this[,col0::cnum] */
  
  /* absSum: sum of absolute values of all the components if col < 0  */  
  /*         sum of absolute values of this[,col]         if col >= 0 */    
  double absSum(int col=-1) const { return absSum_cublas(col); }
  double absSum_noblas(int col=-1) const;  
  double absSum_cublas(int col=-1) const;                                    
 
  /*---  count nonzero components  ---*/
  int nz(int col=-1) const;   
  
  /*---  min max: slow.  Do not use these on a large matrix.  absmin and absmax are faster  ---*/
  double min(int col, int *out_row=NULL) const; 
  double min(int *out_row=NULL, int *out_col=NULL) const; 
  double max(int col, int *out_row=NULL) const; 
  double max(int *out_row=NULL, int *out_col=NULL) const;   
   
  void max_eachCol(AzIntArr *ia_ind, AzPmat *m_max=NULL) const; 
  void min_eachCol(AzIntArr *ia_ind, AzPmat *m_min=NULL) const; 

  /*---  ---*/
  inline virtual void reform_noinit(int r_num, int c_num) { /* shape the matrix but don't initialize with zero */
    if (row_num == r_num && col_num == c_num) return; 
    AzX::throw_if((r_num < 0 || c_num < 0), "AzPmat::reform_noinit", "#row and #col must not be negative"); 
    check_lock("AzPmat::reform_noinit"); 
    int sz = r_num*c_num; 
    if (sz < 0) { /* overflow */
      AzBytArr s("#row x #col must not exceed 2GB.  "); s << r_num << " x " << c_num << ") was requested."; 
      AzX::throw_if(true, "AzPmat::reform_noinit", s.c_str()); 
    }
  
    data.free_alloc(sz, "AzPmat::reform_noinit"); 
    row_num = r_num; 
    col_num = c_num; 
  }
  
  inline void transpose1() {
    AzX::throw_if((row_num != 1 && col_num != 1), "AzPmat::transpose1", "Either #col or #row must be 1"); 
    int temp = row_num; row_num = col_num; col_num = temp; 
  }

  
  void colSum(const AzPmat *m_inp, bool do_chk=false); /* this <- colSum(inp) */
  void rowSum(const AzPmat *m_inp, bool do_chk=false); /* this <- rowSum(inp) */
  /* colSquareSum: for every column c: this[,c] <- sum(inp[,c]^2) */
  void colSquareSum(const AzPmat *m_inp) { colSquareSum_noblas(m_inp); }
  void colSquareSum_noblas(const AzPmat *m_inp);  
  void colSquareSum_cublas(const AzPmat *m_inp);  
  void colAbsSum(const AzPmat *m_inp);     /* for every column c: this[,c] <- sum(|inp[,c]|) */
  
  void add_colSum(const AzPmat *m_inp); /* this += colSum(inp) */
  void add_rowSum(const AzPmat *m_inp); /* this += rowSum(inp) */  
  void add_colSquareSum(const AzPmat *m_inp);  /* for every column c: this[,c] += sum(inp[,c]^2) */
  void add_colAbsSum(const AzPmat *m_inp);     /* for every column c: this[,c] += sum(|inp[,c]|) */  
  void normalize1(); /* each column is divided by column sum; meant for computing probabilities */
  
  /*****  matrix multiplication  *****/
  /* gemm: C = alpha op(A) op(B) + beta C */
  inline void prod(const AzPmat *m1, 
                   const AzPmat *m2, 
                   bool isTr1=false, /* if true, op1=transpose() */
                   bool isTr2=false, /* if true, op2=transpose() */
                   double alpha=1,
                   const AzPstreams *streams=NULL) { /* but alpha <>1 has never been tested */
    double beta=0; 
    prod_sub(m1, m2, isTr1, isTr2, streams, alpha, beta); 
  } 
  inline void add_prod(const AzPmat *m1, 
                   const AzPmat *m2, 
                   bool isTr1=false, /* if true, op1=transpose() */
                   bool isTr2=false, /* if true, op2=transpose() */
                   double alpha=1, double beta=1, 
                   const AzPstreams *streams=NULL) { /* but alpha<>1 or beta<>1 has never been tested */
    prod_sub(m1, m2, isTr1, isTr2, streams, alpha, beta); 
  } 
  
  double absmax() const { return u._absmax(_dptr(), size()); }
  double absmin() const { return u._absmin(_dptr(), size()); }
  
  /*---  ---*/
  inline void shape_chk_tmpl(const AzPmat *m_tmpl, const char *msg, const char *msg2="") const {
    AzX::throw_if((m_tmpl == NULL), "AzPmat::shape_chk_tmpl", "no template"); 
    shape_chk(m_tmpl->row_num, m_tmpl->col_num, msg, msg2); 
  }
  inline void shape_chk(int rnum, int cnum, const char *msg, const char *msg2="") const {
    if (row_num != rnum || col_num != cnum) {
      AzBytArr s; s << msg << " " << msg2 << " " << "row_num=" << row_num << " rnum=" << rnum; 
      s << " col_num=" << col_num << " cnum=" << cnum; 
      AzX::throw_if(true, "AzPmat shape check failed: ", s.c_str()); 
    }
  }

  /*---  for debug  ---*/
  inline void dump(const AzOut &out, const char *header, 
            const AzStrArray *sp_row = NULL, const AzStrArray *sp_col = NULL, 
            int cut_num = -1) const {
    AzDmat m; 
    get(&m); 
    m.dump(out, header, -1, sp_row, sp_col, cut_num); 
  }
 
protected: 
  inline void check_lock(const char *eyec) const {
    AzX::throw_if(locked, eyec, "alloc was attempted to a locked matrix"); 
  }
  inline void check_col(int col, const char *msg, const char *msg2="") const {
    AzX::throw_if((col < 0 || col >= col_num), "AzPmat col# check failed", msg, msg2); 
  }
  
  /*---     point a column     ---*/
  /*---  !!USE WITH CAUTION!!  ---*/
  inline virtual AzFloat *_dptr_u(int cx, const char *msg2="") {
    check_col(cx, "AzPmat::_dptr_u", msg2); 
    return data._dptr_u() + row_num*cx; 
  }
  inline virtual const AzFloat *_dptr(int cx, const char *msg2="") const {
    check_col(cx, "AzPmat::_dptr", msg2); 
    return data._dptr() + row_num*cx; 
  }
  /*---       point all       ---*/
  /*--- !!USE WITH CAUTION!!  ---*/
  inline virtual const AzFloat *_dptr() const {
    return data._dptr(); 
  }
  inline virtual AzFloat *_dptr_u() {
    return data._dptr_u(); 
  }    

  /*---  for matrix multiplication  ---*/
  inline void prod_sub(const AzPmat *m1, 
                    const AzPmat *m2, 
                    bool isTr1, bool isTr2, 
                    const AzPstreams *streams, 
                    double alpha, double beta) {
    if (!isTr1) {
      if (!isTr2) prod00(m1, m2, streams, alpha, beta); 
      else        prod01(m1, m2, streams, alpha, beta); 
    }
    else {
      if (!isTr2) prod10(m1, m2, streams, alpha, beta); 
      else        AzX::throw_if(true, "AzPmat::prod_sub", "no support for t(m1)*t(m2)"); 
    }
  } 
  void prod00(const AzPmat *m1, const AzPmat *m2, const AzPstreams *streams, double alpha, double beta); 
  void prod01(const AzPmat *m1, const AzPmat *m2, const AzPstreams *streams, double alpha, double beta); 
  void prod10(const AzPmat *m1, const AzPmat *m2, const AzPstreams *streams, double alpha, double beta);    
      
  template <class DevFloat, class HostFloat>
  void mycopy10(HostFloat *p_host, const DevFloat *p_dev, int num) const {
    DevFloat *tmp = NULL; 
    AzBaseArray<DevFloat> _a; 
    _a.alloc(&tmp, num, "AzPmat::my_copy10", "tmp"); 
    u._copy10(tmp, p_dev, num); 
    int ix; 
    for (ix = 0; ix < num; ++ix) p_host[ix] = (HostFloat)tmp[ix]; 
  }
  template <class DevFloat, class HostFloat>
  void mycopy01(DevFloat *p_dev, const HostFloat *p_host, int num) const {
    DevFloat *tmp = NULL; 
    AzBaseArray<DevFloat> _a; 
    _a.alloc(&tmp, num, "AzPmat::my_copy01", "tmp"); 
    int ix; 
    for (ix = 0; ix < num; ++ix) tmp[ix] = (DevFloat)p_host[ix]; 
      
    u._copy01(p_dev, tmp, num); 
  }  
}; 

/*************************************************************************/
class AzPintArr {
protected:
  _AzParr<int> data; 
  _AzPint u; 

public:
  friend class AzPmat; 
  friend class AzPmatV_base; 
  friend class AzPmatVar; 
  friend class AzPmatApp; 

  AzPintArr() {}
  AzPintArr(const AzPintArr *inp) {
    reset(inp); 
  }
  AzPintArr(const AzPintArr &inp) {
    AzX::throw_if(true, "AzPintArr(const&)", "= is prohibited");   
  }
  AzPintArr & operator =(const AzPintArr &inp) {
    AzX::throw_if(true, "AzPintArr operator = ", "= is prohibited");   
  }
  inline int size() const {
    return data.size(); 
  }
  inline void alloc(int len) {
    data.free_alloc(len, "AzPintArr::alloc"); 
  }
  inline void free_alloc(int len) {
    data.free_alloc(len, "AzPintArr::free_alloc"); 
  }  
  inline void set(int val) {
    u._setval(data._dptr_u(), val, data.size()); 
  }
  inline void reset(const AzPintArr *inp) {
    data.reset(&inp->data); 
  }
  inline void reset(const AzIntArr *ia) {
    reset(ia->point(), ia->size()); 
  }
  void reset(const int *inp, int inp_len) { /* host pointer */
    data.free_alloc(inp_len, "AzPintArr::reset(inp,inp_len)"); 
    data.copy_from_host(inp, inp_len); 
  }
  void reset(const AzPintArr *inp, int inp_pos, int inp_len) { /* not tested (3/10/2014) */
    data.free_alloc(inp_len, "AzPintArr::reset(inp,pos,len)"); 
    inp->data.copy_to(data, 0, inp_pos, inp_len); 
  }
  void reset() {
    data.free(); 
  }
  inline void get(AzIntArr *ia) const {
    ia->reset(data.size(), 0); 
    data.copy_to_host(ia->point_u(), ia->size()); 
  }
  inline void get(int *h_data, int len) const {
    data.copy_to_host(h_data, len); 
  }
  inline int compare(const AzPintArr *pia2) const {
    AzIntArr ia, ia2; 
    get(&ia); 
    pia2->get(&ia2); 
    return ia.compare(&ia2); 
  }
  inline void write(AzFile *file) const {
    AzIntArr ia; get(&ia); 
    ia.write(file);
  }
  inline void read(AzFile *file) {
    AzIntArr ia; 
    ia.read(file); 
    reset(&ia); 
  }

  inline void add(int val) {
    u._add(data._dptr_u(), val, data.size()); 
  }
  inline void multiply(int val) {
    u._multiply(data._dptr_u(), val, data.size()); 
  }
  inline void divide(int val) {
    u._divide(data._dptr_u(), val, data.size()); 
  }

protected:
  inline const int *_dptr() const {
    return data._dptr(); 
  }
  inline int *_dptr_u() {
    return data._dptr_u(); 
  }
};

/*************************************************************************/
/* 2-dim array of integers */
class AzPintArr2 {
protected:
  _AzParr<int> data; 
  
  int wth; 
  int empty_val; 
  
public:
  friend class AzPmatApp; 
  
  AzPintArr2() : wth(0), empty_val(-1) {}
  AzPintArr2(const AzPintArr2 *inp) : wth(0), empty_val(-1) {
    reset(inp); 
  }
  AzPintArr2(const AzPintArr2 &inp) : wth(0), empty_val(-1) {
    AzX::throw_if(true, "AzPintArr2(const&)", "= is prohibited"); 
  }
  AzPintArr2 & operator =(const AzPintArr2 &inp) {
    AzX::throw_if(true, "AzPintArr2 operator =", "= is prohibited"); 
  }  
  inline int size() const { return (wth == 0) ? 0 : data.size() / wth; }
  inline int maxNum() const { return wth; }
  inline int stopper() const { return empty_val; }
  
  void reset(const AzPintArr2 *inp) {
    AzIntArr ia; inp->get_all(&ia); 
    _reset(inp->wth, inp->empty_val, ia.point(), ia.size()); 
  }
  void reset(const AzDataArr<AzIntArr> *inp, int e_val=-1) { 
    int width = 0; 
    int ix; 
    for (ix = 0; ix < inp->size(); ++ix) {
      width = MAX(width, inp->point(ix)->size()); 
    }
    AzIntArr ia; ia.prepare(width*inp->size()); 
    for (ix = 0; ix < inp->size(); ++ix) {
      const AzIntArr *inp_ia = inp->point(ix); 
      int jx; 
      for (jx = 0; jx < inp_ia->size(); ++jx) {
        int val = inp_ia->get(jx); 
        AzX::throw_if((val == empty_val), "AzPintArr2::reset(AzDataArr)", "content uses an empty value"); 
        ia.put(val); 
      }
      for ( ; jx < width; ++jx) {
        ia.put(empty_val); /* fill with stopper */
      }
    }
    _reset(width, e_val, ia.point(), ia.size()); 
  }
  void reset() {
    data.free(); 
    wth = 0; 
    empty_val = -1; 
  }
  inline void write(AzFile *file) const {
    file->writeInt(wth); 
    file->writeInt(empty_val); 
    AzIntArr ia; get_all(&ia); 
    ia.write(file);
  }
  inline void read(AzFile *file) {
    int width = file->readInt(); 
    int e_val = file->readInt(); 
    AzIntArr ia; 
    ia.read(file); 
    _reset(width, e_val, ia.point(), ia.size()); 
  }
 
  void get(int dx, AzIntArr *ia) const {
    AzX::throw_if((dx < 0 || (dx+1)*wth > size()), "AzPintArr2::get", "index is out of range"); 
    
    AzIntArr my_ia; get_all(&my_ia); 
    const int *ptr = my_ia.point() + dx*wth; 
    int ix; 
    for (ix = 0; ix < wth; ++ix) {
      if (ptr[ix] == empty_val) break;
    }
    ia->reset(ptr, ix); 
  }

  bool isSame(const AzPintArr2 *inp) const {
    if (wth != inp->wth) return false; 
    if (empty_val != inp->empty_val) return false; 
    AzIntArr ia1, ia2; 
    get_all(&ia1); 
    inp->get_all(&ia2); 
    if (ia1.compare(&ia2) != 0) return false; 
    return true; 
  }
  
protected:
  inline const int *_dptr() const {
    return data._dptr(); 
  }
  inline const int *_dptr(int dx) const {
    int offs = dx*wth; 
    AzX::throw_if((offs < 0 || offs+wth > size()), "AzPintArr2::_dptr", "index is out of range"); 
    return data._dptr() + offs; 
  }
  void _reset(int width, int e_val, const int *inp, int inp_len) {
    data.free_alloc(inp_len, "AzPintArr2::_reset"); 
    data.copy_from_host(inp, inp_len); 
    wth = width; 
    empty_val = e_val; 
    AzX::throw_if((width != 0 && inp_len % width != 0), "AzPintArr2::_reset", "length conflict"); 
  }
  void get_all(AzIntArr *ia) const {
    ia->reset(data.size(), 0); 
    data.copy_to_host(ia->point_u(), ia->size()); 
  }
};

/***************************************************************/  
class AzPmatVar {
protected:
  int data_num; 
  AzPmat m; /* row: #channel, col: item (like pixel) */
  AzIntArr ia_dcolind; /* col# in m: begin1, end1, begin2, end2, ... */
  AzPintArr pia_dcolind;
  _AzPmat u; 
  
public:
  friend class AzPmatApp; 

  AzPmatVar() : data_num(0) {}
  AzPmatVar(const AzPmatVar *inp) { set(inp); }
  inline int dataNum() const { return data_num; }
  inline int colNum() const { return m.colNum(); }
  inline int colNum(int ix) const {
    check_datano(ix, "AzPmatVar::colNum(idx)"); 
    return ia_dcolind.get(ix*2+1) - ia_dcolind.get(ix*2); 
  }
  inline int rowNum() const { return m.rowNum(); }
  void shape_str(AzBytArr &s) const { s << rowNum() << " x " << colNum() << ", #data=" << dataNum(); }
  
  inline int size() const { return m.size(); }
  void reset() {
    data_num = 0; 
    ia_dcolind.reset(); 
    m.reset(); 
    pia_dcolind.reset(); 
  }
  inline void destroy() { reset(); } 
  void set(const AzPmatVar *mv0, int d0_begin, int d0_end); /* this[,0::d0_end-d0_begin] <- mv0[,d0_begin:d0_end-1] */
  void set(const AzPmatVar *mv0, const int *dx0s, int dx0s_num); /* this[0::dxs_num] <- selected data of mv0 */ 
  void set(const AzDmatcVar *mv0, const int *dx0s, int dx0s_num); /* this[0::dxs_num] <- selected data of mv0 */
  void set(const AzPmat *_m, const AzPintArr *_pia_dataind) {
    _set(-1, _m, _pia_dataind, NULL); 
  }
  void set(const AzPmat *_m, const AzIntArr *_ia_dataind) {
    _set(-1, _m, NULL, _ia_dataind); 
  }
  void reform(int cc, const AzPintArr *_pia_dataind) {
    _set(cc, NULL, _pia_dataind, NULL); 
  }
  void reform(int cc, const AzIntArr *_ia_dataind) {
    _set(cc, NULL, NULL, _ia_dataind); 
  }  
  void set(const AzPmatVar *inp) {
    data_num = inp->data_num; 
    m.set(&inp->m); 
    ia_dcolind.reset(&inp->ia_dcolind); 
    pia_dcolind.reset(&inp->pia_dcolind); 
  }
  int get_begin(int ix) const {
    check_datano(ix, "AzPmatVar::get_begin"); 
    return ia_dcolind[ix*2];   
  }
  int get_end(int ix) const {
    check_datano(ix, "AzPmatVar::get_end"); 
    return ia_dcolind[ix*2+1];   
  }
  void get_begin_end(int ix, int &begin, int &end) const {
    check_datano(ix, "AzPmatVar::get_begin_end"); 
    begin = ia_dcolind.get(ix*2); 
    end = ia_dcolind.get(ix*2+1); 
  }
  const AzPmat *data() const { return &m; }
  const AzIntArr *h_index() const { return &ia_dcolind; } 
  const AzPintArr *d_index() const { return &pia_dcolind; }
  
  AzPmat *data_u() { return &m; } /* use this with caution; don't do anything to change columns */  

  void update(const AzPmat *_m) {
    AzX::throw_if((_m->rowNum() != m.rowNum() || _m->colNum() != m.colNum()), 
                  "AzPmatVar::update(m)", "dimensionality mismatch"); 
    m.set(_m); 
  }
  void rbind(const AzPmatVar *mv) {
    if (mv == NULL) return; 
    if (m.size() <= 0) {
      set(mv); 
      return; 
    }
    AzX::throw_if(!can_rbind(mv), "AzPmatVar::rbind", "#col or data index mismatch"); 
    m.rbind(&mv->m); 
  }
  bool can_rbind(const AzPmatVar *mv) {
    if (m.size() <= 0) return true; 
    if (mv == NULL) return false; 
    return (colNum() == mv->colNum() && ia_dcolind.compare(&mv->ia_dcolind) == 0);    
  }
 
  void add(const AzPmatVar *mv) {
    const char *eyec = "AzPmatVar::add"; 
    if (mv == NULL) return; 
    AzX::throw_if((rowNum() != mv->rowNum() || colNum() != mv->colNum()), eyec, "shape mismatch"); 
    AzX::throw_if((ia_dcolind.compare(&mv->ia_dcolind) != 0), eyec, "index mismatch"); 
    m.add(&mv->m); 
  } 
  void check_colNum(const char *errmsg) const { /* call this after modifying m */
    const char *eyec = "AzPmatVar::check_colNum"; 
    int c_num = (ia_dcolind.size() <= 0) ? 0 : ia_dcolind[ia_dcolind.size()-1]; 
    AzX::throw_if((m.colNum() != c_num), eyec, "An inconsistency was detected. ", errmsg); 
  }
  
protected: 
  void _set(int cc, const AzPmat *_m, const AzPintArr *_pia_dcolind, const AzIntArr *_ia_dcolind); 
  const int *_dptr_index() const {
    return pia_dcolind._dptr(); 
  }
  void check_datano(int ix, const char *eyec) const {  
    AzX::throw_if((ix < 0 || ix >= data_num), eyec, "invalid index"); 
  }      
};

/***************************************************************/  
/* random number generator */
class AzPrng {
protected:
  _AzPrng g; 
  
public: 
  AzPrng() {}
  ~AzPrng() {}
  void reset() { g.reset(); }
  void reset_seed(long long int seed) { g.reset_seed(seed); }
  void uniform_01(AzPmat *m) {
    AzX::throw_if((m == NULL), "AzPrng::uniform_01", "null pointer"); 
    g.uniform_01(m->_dptr_u(), m->size()); 
  }
}; 

#include "AzPmatSpa.hpp"

#endif
