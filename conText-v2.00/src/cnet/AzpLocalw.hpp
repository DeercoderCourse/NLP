/* * * * *
 *  AzpLocalw.hpp
 *  Copyright (C) 2014-2015 Rie Johnson
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

#ifndef _AZP_LOCALW_HPP_
#define _AZP_LOCALW_HPP_

#include "AzpLmSgd.hpp"
#include "AzPmat_ptr.hpp"
#include "AzPmatApp.hpp"

class AzpLocalw {
protected: 
  AzPmatApp app; 

public:
  AzpLocalw() {}
  AzpLocalw(const AzpLocalw &inp) {}
  AzpLocalw(const AzpLocalw *inp) {}
  void reset(const AzpLocalw *inp) {}

  /*------------------------------------------------------------*/     
  virtual void unapply(const AzBaseArr<AzpLm *> &lms, 
                       AzPmat *m_d, /* must be formatted by the caller */
                       const AzPmat *m_lossd) const {
    const char *eyec = "AzpLocalw::unapply"; 
 
    int loc_num = lms.size(); 
    int d_num = m_lossd->colNum() / loc_num; 
    AzPmats_ptrs am_v(loc_num, m_lossd->rowNum(), m_lossd->colNum()); 
    app.rearrange(loc_num, m_lossd, d_num, am_v.point_u());
    AzPmats_ptrs am_x(loc_num, m_d->rowNum(), m_d->colNum());                                

    int loc; 
    for (loc = 0; loc < loc_num; ++loc) {
      lms[loc]->unapply(am_x.point_u(loc), am_v.point(loc)); 
    } 

    app.undo_rearrange(loc_num, m_d, d_num, am_x.point());
  }
  
  /*------------------------------------------------------------*/    
  inline virtual void apply(const AzBaseArr<AzpLm *> &lms, 
                            const AzPmat *m_x, AzPmat *m_out) const {
    const char *eyec = "AzpLocalw::apply"; 

    int loc_num = lms.size(); 
    AzX::throw_if((loc_num == 0), eyec, "#loc is zero?!"); 
    int d_num = m_x->colNum() / loc_num; 

    AzPmats_ptrs am_x(loc_num, m_x->rowNum(), m_x->colNum());     
    app.rearrange(loc_num, m_x, d_num, am_x.point_u());                                              
    AzPmats_ptrs am_v(loc_num, m_out->rowNum(), m_out->colNum()); 
 
    for (int loc = 0; loc < loc_num; ++loc) {
      lms[loc]->apply(am_x.point(loc), am_v.point_u(loc)); 
    }

    app.undo_rearrange(loc_num, m_out, d_num, am_v.point());
  }   
  
   /*------------------------------------------------------------*/ 
  inline void updateDelta(const AzBaseArr<AzpLm *> &lms, 
                          const AzpLmParam &p, 
                          int d_num, /* real # of data points.  no dummy */
                          const AzPmat *m_x, 
                          const AzPmat *m_lossd) const {             
    int loc_num = lms.size(); 
    AzPmats_ptrs am_x(loc_num, m_x->rowNum(), m_x->colNum()); 
    int my_d_num = m_x->colNum() / loc_num; 
    app.rearrange(loc_num, m_x, my_d_num, am_x.point_u()); 
    AzPmats_ptrs am_v(loc_num, m_lossd->rowNum(), m_lossd->colNum()); 
    app.rearrange(loc_num, m_lossd, my_d_num, am_v.point_u());
    for (int loc = 0; loc < loc_num; ++loc) {
      lms[loc]->updateDelta(d_num, p, am_x.point(loc), am_v.point(loc)); 
    }
  } 
}; 
#endif 