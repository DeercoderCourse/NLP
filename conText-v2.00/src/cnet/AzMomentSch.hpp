/* * * * *
 *  AzMomentSch.hpp
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

#ifndef _AZ_MOMENT_SCH_HPP_
#define _AZ_MOMENT_SCH_HPP_

#include "AzUtil.hpp"
#include "AzParam.hpp"
#include "AzTools.hpp"

/*---  change the momentum after every iteration  ---*/

  #define AzMomentSch_Type_None 'N'
  #define AzMomentSch_Type_Tinv 'T'

class AzMomentSch {
protected:
  AzBytArr s_typ; 
  char typ; 
  double mommax, momcoeff; 
  int lastfew; /* to lower the momentum for last few iterations */
  
public: 
  AzMomentSch() : typ(AzMomentSch_Type_None), mommax(-1), momcoeff(-1), lastfew(-1) {}

  #define kw_momtyp "momentum_scheduler="
  #define kw_mommax "momentum_max="
  #define kw_momcoeff "momentum_coeff="
  #define kw_lastfew "momentum_last="
  
  void resetParam(AzParam &azp, const char *pfx="") {
    const char *eyec = "AzpMomentSch::resetParam"; 
    azp.reset_prefix(pfx); 
    typ = AzMomentSch_Type_None; 
    
    azp.vStr(kw_momtyp, &s_typ); 
    if (s_typ.length() > 0) {
      typ = *s_typ.point(); 
    }
    if (typ == AzMomentSch_Type_None) {
      azp.reset_prefix(); 
      return; 
    }

    if (typ == AzMomentSch_Type_Tinv) {
      /*---  Tinv (t inverse): min(mommax, 1 - (1/2)(1/(a t + 1))) ---*/
      azp.vFloat(kw_mommax, &mommax); 
      azp.vFloat(kw_momcoeff, &momcoeff); 
      azp.vInt(kw_lastfew, &lastfew); 
      AzXi::throw_if_nonpositive(mommax, eyec, kw_mommax); 
      AzXi::throw_if_nonpositive(momcoeff, eyec, kw_momcoeff); 
    }
    azp.reset_prefix(); 
  }
  void printParam(const AzOut &out, const char *pfx="") const {
    AzPrint o(out, pfx); 
    o.printV_if_not_empty(kw_momtyp, s_typ); 
    o.printV(kw_mommax, mommax); 
    o.printV(kw_momcoeff, momcoeff); 
    o.printEnd(); 
  }
  
  void init() {

  }
    
  /*------------------------------------------------------------*/ 
  double get_momentum(int tt) {
    double momentum = -1; 
    if (typ == AzMomentSch_Type_Tinv) {   
      /*---  Tinv (t inverse): min(mommax, 1 - (1/2)(1/(a t + 1))) ---*/
      if (lastfew > 0 && tt >= lastfew) {
        momentum = 0.9; 
      }
      else {
        momentum = MIN(mommax, 1 - 0.5*(1/(momcoeff * tt + 1))); 
      }
    }
    return momentum; 
  }
};   
 
#endif 
  