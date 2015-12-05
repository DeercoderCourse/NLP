/* * * * *
 *  AzPmat_ptr.hpp 
 *  Copyright (C) 2013 Rie Johnson
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

#ifndef _AZ_PMAT_PTR_HPP_
#define _AZ_PMAT_PTR_HPP_

#include "AzPmat.hpp"

/*************************************************************************/
class AzPmat_ptr : public virtual /* extends */ AzPmat {
protected:
  AzFloat *_dev_ptr; 

public:
  friend class AzPmatApp; 
  friend class AzPmats_ptrs; 
  AzPmat_ptr() : _dev_ptr(NULL) {}

protected: 
  /*---  point a column  ---*/
  inline AzFloat *_dptr_u(int cx, const char *msg2="") {
    check_col(cx, "AzPmat_ptr::_dptr_u", msg2); 
    return _dev_ptr + row_num*cx; 
  }
  inline const AzFloat *_dptr(int cx, const char *msg2="") const {
    check_col(cx, "AzPmat::_dptr", msg2); 
    return _dev_ptr + row_num*cx; 
  }
  /*---  point all  ---*/
  inline const AzFloat *_dptr() const {
    return _dev_ptr; 
  }
  inline AzFloat *_dptr_u() {
    return _dev_ptr; 
  }
  
    
  inline void reset() {
    _dev_ptr = NULL; 
    row_num = col_num = 0; 
  }
  
  void reset(AzPmat *owner, int col, int cnum) {
    AzX::throw_if((col < 0 || col+cnum > owner->colNum()), "AzPmat_ptr::reset", "out of range"); 
    _dev_ptr = owner->_dptr_u(col); 
    row_num = owner->row_num; 
    col_num = cnum; 
  }

  /*---  reform  ---*/
  inline void reform_noinit(int r_num, int c_num) {
    if (row_num == r_num && col_num == c_num) return; 
    AzX::throw_if(true, "AzPmat_ptr::reform_noinit", "wrong shape"); 
  }  
}; 

class AzPmats_ptrs {
protected: 
  AzPmat m_owner; 
  AzDataArr<AzPmat_ptr> am; 
  
public: 
  AzPmats_ptrs() {}
  AzPmats_ptrs(int num, int rnum, int cnum) {
    reset(num, rnum, cnum); 
  }
  int size() const {
    return am.size(); 
  }  
  void reset(int num) {
    am.reset(num); 
    m_owner.unlock(); 
  }
  void reset(int num, int rnum, int cnum) {
    reset(num); 
    reset(rnum, cnum); 
  }
  void reset(int rnum, int cnum) {
    int num = am.size(); 
    AzX::throw_if((num == 0), "AzPmats_ptrs::reset(rnum,cnum)", "num=0: it has not be initialized yet"); 
    AzX::throw_if((cnum % num != 0), "AzPmats_ptrs::reset(rnum,cnum)", "#column is wrong"); 
    int dnum = cnum / num; 
    m_owner.unlock(); 
    m_owner.reform_noinit(rnum, cnum); 
    int ix; 
    for (ix = 0; ix < num; ++ix) {
      AzPmat_ptr *m = am.point_u(ix); 
      m->reset(&m_owner, ix*dnum, dnum); 
    }
    m_owner.lock();    
  }
  const AzPmat *point() const {
    return &m_owner; 
  }
  AzPmat *point_u() {
    return &m_owner; 
  }
  const AzPmat *point(int index) const {
    return am.point(index); 
  }
  AzPmat *point_u(int index) {
    return am.point_u(index); 
  }
}; 
#endif 
