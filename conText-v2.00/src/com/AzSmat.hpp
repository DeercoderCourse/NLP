/* * * * *
 *  AzSmat.hpp 
 *  Copyright (C) 2011-2015 Rie Johnson
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

#ifndef _AZ_SMAT_HPP_
#define _AZ_SMAT_HPP_

#include "AzUtil.hpp"
#include "AzStrArray.hpp"
#include "AzReadOnlyMatrix.hpp"

/*--- 
 * 12/15/2014:                                      
 *  - Changed AZ_MTX_FLOAT to be switchable btw double and float using a compiler option.          
 *  - Changed file format for AzSmat and AzSvect to save disk space.  
 *     NOTE: Old AzSmat file format is still supported.  
 *     NOTE: Old AzSvect file format is NOT supported, as there is no known release that writes AzSvect.
 *  - Added Replaced new AzSvect(..) with AzSvect::new_svect so that the success of new is ensured.    
 ---*/
#ifdef __AZ_SMAT_SINGLE__
typedef float AZ_MTX_FLOAT; 
#else
typedef double AZ_MTX_FLOAT; 
#endif 
#define _checkVal(x) 
/* static double _too_large_ = 16777216; */
/* static double _too_small_ = -16777216; */

typedef struct { int no; AZ_MTX_FLOAT val; } AZI_VECT_ELM; 
typedef struct { int no; float val;  } AZI_VECT_ELM4;
typedef struct { int no; double val; } AZI_VECT_ELM8;  

class AzSmat; 

//! sparse vector 
class AzSvect : /* implements */ public virtual AzReadOnlyVector {
protected:
  int row_num; 
  AZI_VECT_ELM *elm; 
  AzBaseArray<AZI_VECT_ELM> a; 
  int elm_num; 

  void _release() {
    a.free(&elm); elm_num = 0; 
  }
 
  void init_reset(int inp_row_num, bool asDense=false) { 
    initialize(inp_row_num, asDense); 
  }
  void init_reset(const AzSvect *inp) {
    if (inp == NULL) return; 
    row_num = inp->row_num; set(inp); 
  }
  void init_reset(const AzReadOnlyVector *inp) {
    if (inp == NULL) return;   
    row_num = inp->rowNum(); set(inp); 
  }
  void init_reset(AzFile *file, int float_size, int rnum) {
    _read(file, float_size, rnum); 
  }
  
  /*---  new_svect: used only by AzSmat  ---*/
  static AzSvect *new_svect() {
    AzSvect *v = NULL;
    try { v = new AzSvect(); }
    catch (std::bad_alloc &ba) { throw new AzException(AzAllocError, "AzSvect::new_svect()", ba.what()); }
    if (v == NULL) throw new AzException(AzAllocError, "AzSvect::new_svect()", "new");  
    return v; 
  }     
  static AzSvect *new_svect(int _row_num, bool asDense=false) {
    AzSvect *v = new_svect(); v->init_reset(_row_num, asDense); return v; 
  }     
  static AzSvect *new_svect(const AzSvect *inp) {
    AzSvect *v = new_svect(); v->init_reset(inp); return v; 
  } 
  static AzSvect *new_svect(const AzReadOnlyVector *inp) {
    AzSvect *v = new_svect(); v->init_reset(inp); return v; 
  }   
  static AzSvect *new_svect(AzFile *file, int float_no, int rnum) {
    AzSvect *v = new_svect(); v->init_reset(file, float_no, rnum); return v; 
  }

  AzSvect(AzFile *file) /* for compatibility; called only by AzSmat */
    : row_num(0), elm(NULL), elm_num(0) { _read_old(file); }
  
public:
  friend class AzDvect;    /* to speed up set, add, innerProduct */ 
  friend class AzPmatSpa;  /* to speed up set */ 
  friend class AzSmat;     /* to let AzSmat use new_svect and _write */
  friend class AzSmatc;    /* to speed up transfer_from */
  friend class AzObjIOTools; /* for file compatibility so that AzSvect(file) can be called. */
  
  AzSvect() : row_num(0), elm(NULL), elm_num(0) {}
  AzSvect(int _row_num, bool asDense=false) 
    : row_num(0), elm(NULL), elm_num(0) { init_reset(_row_num, asDense); }  
  AzSvect(const AzSvect *inp) 
    : row_num(0), elm(NULL), elm_num(0) { init_reset(inp); }
  AzSvect(const AzReadOnlyVector *inp) 
    : row_num(0), elm(NULL), elm_num(0) { init_reset(inp); }
    
  AzSvect(const AzSvect &inp) 
    : row_num(0), elm(NULL), elm_num(0) { init_reset(&inp); }
  AzSvect & operator =(const AzSvect &inp) {
    if (this == &inp) return *this; 
    _release(); 
    row_num = inp.row_num; 
    set(&inp); 
    return *this; 
  }
  
  ~AzSvect() {}

  void read(AzFile *file) {
    _release(); 
    _read(file, -1, -1); 
  }

  void resize(int new_row_num); /* new #row must be greater than #row */
  void reform(int new_row_num, bool asDense=false); 
  void change_rowno(int new_row_num, const AzIntArr *ia_old2new, bool do_zero_negaindex=false); 
  
  void write(AzFile *file) { _write(file, -1); }

  inline int rowNum() const { return row_num; }  
  void load(const AzIntArr *ia_row, double val, bool do_ignore_negative_rowno=false); 
  void load(const AzIFarr *ifa_row_val); 
  void load(AzIFarr *ifa_row_val) {
    ifa_row_val->sort_Int(true); 
    load((const AzIFarr *)ifa_row_val); 
  }   
  bool isZero() const;
  bool isOneOrZero() const; 

  void cut(double min_val); 
  void only_keep(int num); /* keep this number of components with largest fabs */
  void zerooutNegative(); /* not tested */
  void nonZero(AzIFarr *ifq, const AzIntArr *ia_sorted_filter) const; 

  void filter(const AzIntArr *ia_sorted, /* must be sorted; can have duplications */
              /*---  output  ---*/
              AzIFarr *ifa_nonzero, 
              AzIntArr *ia_zero) const; 

  int nonZeroRowNo() const; /* returns the first one */
  void nonZero(AzIFarr *ifa) const; 
  void all(AzIFarr *ifa) const;  /* not tested */
  void zeroRowNo(AzIntArr *ia) const;  /* not tested */
  void nonZeroRowNo(AzIntArr *intq) const; 
  int nonZeroRowNum() const; 

  void set_inOrder(int row_no, double val); 
  void set(int row_no, double val); 
  void set(double val); 
  void set(const AzSvect *vect1, double coefficient=1); 
  void set(const AzReadOnlyVector *vect1, double coefficient=1);  
  
  void rawset(const AZI_VECT_ELM *_elm, int _elm_num);
  
  double get(int row_no) const; 

  double sum() const; 
  double absSum() const; 

  void add(int row_no, double val); 

  void multiply(int row_no, double val); 
  void multiply(double val); 
  inline void divide(double val) {
    if (val == 0) throw new AzException("AzSvect::divide", "division by zero"); 
    multiply((double)1/val); 
  }
 
  void plus_one_log();  /* x <- log(x+1) */

  void scale(const double *vect1); 

  double selfInnerProduct() const; 
  inline double squareSum() const { return selfInnerProduct(); }
  
  void log_of_plusone(); /* log(x+1) */
  double normalize(); 
  double normalize1(); 
  void binarize();  /* posi -> 1, nega -> -1 */
  void binarize1(); /* nonzero -> 1 */
  
  void clear(); 
  void zeroOut(); 

  int next(AzCursor &cursor, double &out_val) const; 

  double minPositive(int *out_row_no = NULL) const; 
  double min(int *out_row_no = NULL, 
                     bool ignoreZero=false) const; 
  double max(int *out_row_no = NULL, 
                    bool ignoreZero=false) const; 
  double maxAbs(int *out_row_no = NULL, double *out_real_val = NULL) const; 

  void polarize(); 
  
  void dump(const AzOut &out, const char *header, 
            const AzStrArray *sp_row = NULL, 
            int cut_num = -1) const; 

  void clear_prepare(int num); 
  bool isSame(const AzSvect *inp) const; 
  void cap(double cap_val); 

  static double first_positive(const AZI_VECT_ELM *elm, int elm_num, int *row=NULL); 
  double first_positive(int *row=NULL) const { return first_positive(elm, elm_num, row); }
  
protected:
  void _write(AzFile *file, int rnum); 
  void check_rowno() const; 
  void _read_old(AzFile *file); /* read old version */
  void _read(AzFile *file, int float_size, int rnum); 
  void _swap(); 

  void initialize(int inp_row_num, bool asDense); 
  int to_insert(int row_no); 
  int find(int row_no, int from_this = -1) const; 
  int find_forRoom(int row_no, 
                   int from_this = -1, 
                    bool *out_isFound = NULL) const; 

  void _dump(const AzOut &out, const AzStrArray *sp_row, 
             int cut_num = -1) const; 

  inline int inc() const {
    return MIN(4096, MAX(32, elm_num)); 
  }
}; 

//! sparse matrix 
class AzSmat : /* implements */ public virtual AzReadOnlyMatrix {
protected:
  int col_num, row_num; 
  AzSvect **column; /* NULL if and only if col_num=0 */
  AzObjPtrArray<AzSvect> a; 
  AzSvect dummy_zero; 
  void _release() {
    a.free(&column); col_num = 0; 
    row_num = 0; 
  }
public: 
  AzSmat() : col_num(0), row_num(0), column(NULL) {}
  AzSmat(int inp_row_num, int inp_col_num, bool asDense=false)
    : col_num(0), row_num(0), column(NULL) {
    initialize(inp_row_num, inp_col_num, asDense); 
  }
  AzSmat(const AzSmat *inp) : col_num(0), row_num(0), column(NULL) {
    initialize(inp); 
  }
  AzSmat(const AzSmat &inp) : col_num(0), row_num(0), column(NULL) {
    initialize(&inp); 
  }
  AzSmat & operator =(const AzSmat &inp) {
    if (this == &inp) return *this; 
    _release(); 
    initialize(&inp); 
    return *this; 
  }

  AzSmat(AzFile *file) : col_num(0), row_num(0), column(NULL) {
    _read(file); 
  }
  AzSmat(const char *fn) : col_num(0), row_num(0), column(NULL) {
    read(fn); 
  }
  ~AzSmat() {}
  void read(AzFile *file) {
    _release(); 
    _read(file); 
  }
  void read(const char *fn) {
    AzFile file(fn); 
    file.open("rb"); 
    read(&file); 
    file.close(); 
  }
  inline void reset() {
    _release(); 
  }
  void resize(int new_col_num); 
  void resize(int new_row_num, int new_col_num); /* new #row must be greater than #row */
  void reform(int row_num, int col_num, bool asDense=false); 

  void change_rowno(int new_row_num, const AzIntArr *ia_old2new, bool do_zero_negaindex=false); 
  int nonZeroRowNo(AzIntArr *ia_nzrows) const; 
  
  void write(AzFile *file); 
  void write (const char *fn) {
    AzFile::write(fn, this); 
  }

  bool isZero() const; 
  bool isZero(int col_no) const; 
  bool isOneOrZero() const; 
  bool isOneOrZero(int col_no) const; 
  
  int nonZeroColNum() const; 
  AZint8 nonZeroNum(double *ratio=NULL) const; 
  
  void transpose(AzSmat *m_out, int col_begin = -1, int col_end = -1) const; 
  inline void transpose_to(AzSmat *m_out, int col_begin = -1, int col_end = -1) const {
    transpose(m_out, col_begin, col_end); 
  }
  void cut(double min_val); 
  void only_keep(int num); /* keep this number of components per column with largest fabs */
  void zerooutNegative(); /* not tested */

  void set(const AzSmat *inp); 
  void set(const AzSmat *inp, int col0, int col1); /* this <- inp[,col0:col1-1] */
  int set(const AzSmat *inp, const int *cols, int cnum, bool do_zero_negaindex=false); /* return #negatives in cols */
  void set(int col0, int col1, const AzSmat *inp, int icol0=0); /* this[col0:col1-1] <- inp[icol0::(col1-col0)] */
  inline void reduce(const AzIntArr *ia_cols) { 
    if (ia_cols == NULL) throw new AzException("AzSmat::reduce", "null input"); 
    reduce(ia_cols->point(), ia_cols->size());
  }
  void reduce(const int *cols, int cnum);  /* new2old; must be sorted; this <- selected columns of this */
  void set(const AzReadOnlyMatrix *inp);  
  void set(int row_no, int col_no, double val); 
  void set(double val); 

  void add(int row_no, int col_no, double val); 

  void multiply(int row_no, int col_no, double val); 
  void multiply(double val); 
  inline void divide(double val) {
    if (val == 0) throw new AzException("AzSmat::divide", "division by zero"); 
    multiply((double)1/val); 
  }

  void plus_one_log();  /* x <- log(x+1) */  
  double get(int row_no, int col_no) const; 

  double sum() const {
    if (column == NULL) return 0; 
    double mysum = 0; 
    int col; 
    for (col = 0; col < col_num; ++col) {
      if (column[col] != NULL) mysum += column[col]->sum(); 
    }
    return mysum; 
  }
  double sum(int col) const {
    if (col < 0 || col >= col_num) {
      throw new AzException("AzSmat::sum(col)", "invalid col#"); 
    }
    if (column == NULL) return 0; 
    if (column[col] == NULL) return 0; 
    return column[col]->sum(); 
  }
  
  /*---  this never returns NULL  ---*/
  inline const AzSvect *col(int col_no) const {
    if (col_no < 0 || col_no >= col_num) {
      throw new AzException("AzSmat::col", "col# is out of range"); 
    }
    if (column[col_no] == NULL) {
      if (dummy_zero.rowNum() != row_num) {
        throw new AzException("AzSmat::col", "#col of dummy_zero is wrong"); 
      }
      return &dummy_zero; 
    }
    return column[col_no]; 
  }

  /*---  this never returns NULL  ---*/
  inline AzSvect *col_u(int col_no) {
    if (col_no < 0 || col_no >= col_num) {
      throw new AzException("AzSmat::col_u", "col# is out of range"); 
    }
    if (column[col_no] == NULL) {
      column[col_no] = AzSvect::new_svect(row_num); 
    }
    return column[col_no]; 
  }
  inline int rowNum() const { return row_num; }
  inline int colNum() const { return col_num; }
  inline int dataNum() const { return col_num; }
  inline int size() const { return row_num*col_num; }
  
  void log_of_plusone(); /* log(x+1) */
  void normalize(); 
  void normalize1(); 
  void binarize();  /* posi -> 1, nega -> -1 */
  void binarize1(); /* nonzero -> 1 */
  
  inline void destroy() {
    reform(0,0); 
  }
  inline void destroy(int col) {
    if (col >= 0 && col < col_num && column != NULL) {
      delete column[col]; 
      column[col] = NULL; 
    } 
  }

  inline void load(int col, AzIFarr *ifa_row_val) {
    col_u(col)->load(ifa_row_val); 
  }

  void clear(); 
  void zeroOut(); 

  int next(AzCursor &cursor, int col, double &out_val) const; 

  double max(int *out_row=NULL, int *out_col=NULL, 
                    bool ignoreZero=false) const;
  double min(int *out_row=NULL, int *out_col=NULL, 
                     bool ignoreZero=false) const; 

  void dump(const AzOut &out, const char *header, 
            const AzStrArray *sp_row = NULL, const AzStrArray *sp_col = NULL, 
            int cut_num = -1) const; 

  bool isSame(const AzSmat *inp) const; 

  static void concat(AzSmat *m_array[], 
                     int m_num, 
                     int col_num, 
                     /*---  output  ---*/
                     AzSmat *m_out,  /* not initialized */
                     bool destroyInput=false); 

  inline static bool isNull(const AzSmat *inp) {
    if (inp == NULL) return true; 
    if (inp->col_num == 0) return true; 
    if (inp->row_num == 0) return true; 
    return false; 
  }
  void cap(double cap_val); 

  void rbind(const AzSmat *m1); 
  void cbind(const AzSmat *m1); 

  void transfer_from(AzDataArr<AzSmat> *am, const AzIntArr &ia_dcolind, 
                     int r_num, int c_num, AZint8 e_num); 

  /*---  to match smatc  ---*/                     
  int col_size(int cx) const {
    check_colno(cx, "AzSmat::col_size"); 
    if (column == NULL || column[cx] == NULL) return 0; 
    return column[cx]->elm_num; 
  }
  const AZI_VECT_ELM *rawcol(int cx, int *out_num=NULL) const {
    check_colno(cx, "AzSmat::rawcol"); 
    int num = col_size(cx); 
    if (out_num != NULL) *out_num = num; 
    if (num > 0) return column[cx]->elm; 
    return NULL;    
  }
  AZint8 elmNum() const {
    if (column == NULL) return 0; 
    AZint8 e_num = 0; 
    for (int col = 0; col < col_num; ++col) e_num += col_size(col); 
    return e_num; 
  }
  void copy_to_smat(AzSmat *ms) const { 
    AzX::throw_if_null(ms, "AzSmat::copy_to_smat");   
    ms->set(this); 
  }
  void copy_to_smat(AzSmat *ms, const int *cxs, int cxs_num) const {
    AzX::throw_if_null(ms, "AzSmat::copy_to_smat(cols)"); 
    ms->set(this, cxs, cxs_num); 
  }
  double first_positive(int col, int *row=NULL) const { 
    return AzSvect::first_positive(rawcol(col), col_size(col), row); 
  }
                     
protected:
  void check_colno(int col, const char *eyec) const {
    if (col < 0 || col >= col_num) {
      throw new AzException(eyec, "Invalid col#");  
    }
  }
  void _read(AzFile *file); 
  void initialize(int row_num, int col_num, bool asDense); 
  void initialize(const AzSmat *inp); 
  void _transpose(AzSmat *m_out, int col_begin, int col_end) const; 

  void _read_cols_old(AzFile *file); /* for compatibility */
  void _read_cols(AzFile *file, int float_size); 
  void _write_cols(AzFile *file);   
}; 

#include "AzMatVar.hpp"
typedef AzMatVar<AzSmat> AzSmatVar; 

/*********************************************************************/
/* sparse matrix with compact format; no update is allowed           */
/* NOTE: The number of non-zero elements must not exceed 2GB,        */
/*       so that int can be used for locating columns.               */
class AzSmatc {
protected: 
  int row_num, col_num; 
  AzIntArr ia_begin, ia_end; 
  AZI_VECT_ELM *elm; 
  AzBaseArray<AZI_VECT_ELM> arr;   
public:
  AzSmatc() : row_num(0), col_num(0), elm(NULL) {}
  AzSmatc(int _row_num, int _col_num) 
            : row_num(0), col_num(0), elm(NULL) {
    reform(_row_num, _col_num);               
  }
  AzSmatc(const char *fn) : row_num(0), col_num(0), elm(NULL) {
    read(fn);  
  }
  void reset() {
    ia_begin.reset(); ia_end.reset(); 
    arr.free(&elm, "AzSmatc::reset()"); 
  }  
  void destroy() { reset(); }
  
  void write(const char *fn) { AzFile::write(fn, this); }
  void write(AzFile *file); /* read in the same format as AzSmat */
  void read(const char *fn) { AzFile::read(fn, this); }  
  void read(AzFile *file); /* write in the same format as AzSmat */

  int colNum() const { return col_num; }
  int rowNum() const { return row_num; }
  int dataNum() const { return col_num; }  
  AZint8 elmNum() const { return (AZint8)arr.size(); }
  int col_size(int cx) const {
    check_colno(cx); 
    return ia_end[cx]-ia_begin[cx]; 
  }
  const AZI_VECT_ELM *rawcol(int cx, int *num=NULL) const {
    check_colno(cx); 
    if (num != NULL) *num = (ia_end[cx]-ia_begin[cx]); 
    return elm+ia_begin[cx];     
  }  
  void reform(int _row_num, int _col_num) {
    if (row_num < 0 || col_num < 0) throw new AzException("AzSmatc::reform", "#row and #cold must be non-negative"); 
    arr.free(&elm); 
    row_num = _row_num; col_num = _col_num; 
    ia_begin.reset(col_num, 0); 
    ia_end.reset(col_num, 0); 
  }

  void set(const AzSmat *m);  
  void set(const AzSmatc *m);
  void set(const AzSmatc *m, const int *cxs, int cxs_num); 
  void cbind(const AzSmatc *m); 
  
  void copy_to_smat(AzSmat *ms, const int *cxs, int cxs_num) const;
  void copy_to_smat(AzSmat *ms) const {
    AzIntArr ia; ia.range(0, col_num); 
    copy_to_smat(ms, ia.point(), ia.size());     
  }
  void transfer_from(AzDataArr<AzSmat> *am, const AzIntArr &ia_dcolind, 
                            int r_num, int c_num, AZint8 e_num); 

  double first_positive(int col, int *row=NULL) const { 
    return AzSvect::first_positive(rawcol(col), col_size(col), row); 
  }
  double min() const {
    double val = ((AZint8)row_num*(AZint8)col_num > arr.size()) ? 0 : (double)9999999999; 
    for (int ex = 0; ex < arr.size(); ++ex) val = MIN(val, elm[ex].val); 
    return val; 
  }
  double max() const {
    double val = ((AZint8)row_num*(AZint8)col_num > arr.size()) ? 0 : (double)(-9999999999); 
    for (int ex = 0; ex < arr.size(); ++ex) val = MAX(val, elm[ex].val); 
    return val; 
  }
  AzSmatc & operator =(const AzSmatc &inp) {
    if (this == &inp) return *this; 
    set(&inp); 
    return *this; 
  }
  
protected:  
  void check_rowno(int where, int elm_num) const; 
  int read_svect(AzFile *file, int float_size, int where); 
  void _swap(); 
  void check_colno(int col) const {
    if (col < 0 || col >= col_num) throw new AzException("AzSmatc::check_colno", "col# is out of range");  
  }
}; 

#include "AzMatVar.hpp"
typedef AzMatVar<AzSmatc> AzSmatcVar; 
#endif 
