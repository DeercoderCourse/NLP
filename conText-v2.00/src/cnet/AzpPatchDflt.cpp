/* * * * *
 *  AzpPatchDflt.cpp
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

#include "AzpPatchDflt.hpp"

/*------------------------------------------------------------*/ 
void AzpPatchDflt::set_dim_back()
{
  const char *eyec = "AzpPatchDflt::set_dim_back"; 
  int idim = i_region.get_dim();   
  int odim = o_region.get_dim(); 
  if (idim == odim) return; 
  if (idim == 1) { /* 1D data */
    /*---  def_patches and def_allinone make a 2D region with size (xsz,1).   ---*/
    /*---  change this back to a 1D region for the upper layer  ---*/    
    AzX::throw_if((odim != 2), eyec, "Unexpected output dimension");      
    AzX::throw_if((o_region.sz(1) != 1), eyec, "ysz should be 1"); 
    int xsz = o_region.sz(0); 
    o_region.reset(&xsz, 1); 
  }
  else {
    AzX::throw_if(true, eyec, "Unexpected conversion: other than 2D -> 1D?!");
  }
}
  
/*------------------------------------------------------------*/ 
void AzpPatchDflt::def_filters() 
{
  const char *eyec = "AzpPatchDflt::def_filters"; 
  int ypch_sz = p.pch_sz, ypch_step = p.pch_step, ypadding = p.padding; 
  int idim = i_region.get_dim(); 
  if (idim == 1) { /* 1D data */
    ypch_sz = 1; ypch_step = 1; ypadding = 0;  
  }
  def_patches(p.pch_sz, p.pch_step, p.padding, 
              ypch_sz, ypch_step, ypadding, 
              p.do_simple_grid); 
  set_dim_back();   
} 

/*------------------------------------------------------------*/ 
void AzpPatchDflt::def_allinone() 
{
  Az2D i_whole(&i_region); 

  tmpl.reset(i_whole.xsz, i_whole.ysz); 
  ia_x0.reset(); ia_y0.reset(); 
  ia_x0.put(0); ia_y0.put(0);  
  Az2D o_whole(1,1); 
  o_whole.copy_to(&o_region); 

  mapping_for_filtering(&pia2_out2inp, &pia2_inp2out); 
  is_allinone = true; 
  set_dim_back(); 
}

/*------------------------------------------------------------*/ 
void AzpPatchDflt::def_patches(int pch_xsz, int pch_xstep, int xpadding, 
                         int pch_ysz, int pch_ystep, int ypadding, 
                         bool do_simple_grid)
{
  const char *eyec = "AzpPatchDflt::def_patches"; 

  Az2D i_whole(&i_region); 
  if (pch_xsz < 0 || 
      pch_xsz >= i_whole.xsz && pch_ysz >= i_whole.ysz && 
      xpadding == 0 && ypadding == 0) {
    def_allinone(); 
    return; 
  }
  AzX::throw_if((xpadding < 0 || ypadding < 0), eyec, "padding should be non-negative"); 
  is_asis = false; 
  if (pch_xsz == 1 && pch_xstep == 1 && xpadding == 0 && 
      pch_ysz == 1 && pch_ystep == 1 && ypadding == 0) {
    is_asis = true;    
  }
  
  int xsz=i_whole.xsz+xpadding*2;
  int ysz=i_whole.ysz+ypadding*2; 
  tmpl.reset(MIN(xsz,pch_xsz), MIN(ysz,pch_ysz)); 

  AzIntArr ia_xx0, ia_yy0; 
  if (do_simple_grid) {
    _set_simple_grids(xsz, tmpl.xsz, pch_xstep, &ia_xx0); 
    _set_simple_grids(ysz, tmpl.ysz, pch_ystep, &ia_yy0); 
  }
  else {
    _set_grids(xsz, tmpl.xsz, pch_xstep, &ia_xx0); 
    _set_grids(ysz, tmpl.ysz, pch_ystep, &ia_yy0); 
  }
  const int *x0 = ia_xx0.point(); 
  const int *y0 = ia_yy0.point(); 
  ia_x0.reset(); ia_y0.reset(); 
  int ix1, ix2; 
  for (ix2 = 0; ix2 < ia_yy0.size(); ++ix2) {
    for (ix1 = 0; ix1 < ia_xx0.size(); ++ix1) {
      ia_x0.put(x0[ix1]-xpadding); 
      ia_y0.put(y0[ix2]-ypadding); 
    }
  }
  Az2D o_whole(ia_xx0.size(), ia_yy0.size()); 
  o_whole.copy_to(&o_region); 

  mapping_for_filtering(&pia2_out2inp, &pia2_inp2out); 
}                            
                             
/*------------------------------------------------------------*/ 
void AzpPatchDflt::_set_grids(int sz, 
                        int inp_pch_sz, 
                        int step, 
                        AzIntArr *ia_p0) const
{
  int pch_sz = MIN(inp_pch_sz, sz); 
  ia_p0->reset(); 
  int p0 = 0; 
  for ( ; p0+pch_sz <= sz; p0 += step) {
    ia_p0->put(p0); 
    if (step <= 0) break; 
  }

  if (ia_p0->size()>0) {
    /*---  cover the last part  ---*/
    int last_p0 = ia_p0->get(ia_p0->size()-1); 
    p0 = sz - pch_sz;
    if (p0 > last_p0) {
      ia_p0->put(p0); 
    }
  }
  
  AzX::throw_if((ia_p0->size() == 0), AzInputError, "AzpPatchDflt::_set_grids", 
                "Failed to define any patch.  Check filter size and stride"); 
}

/*------------------------------------------------------------*/ 
void AzpPatchDflt::mapping_for_pooling(AzPintArr2 *pia2_out2inp, 
                                 AzPintArr2 *pia2_inp2out, 
                                 AzPintArr *pia_out2num) /* how many inputs are mapped to each output */
const
{
  const char *eyec = "AzpPatchDflt::mapping_for_pooling"; 
  Az2D i_whole(&i_region); 
  
  AzDataArr<AzIntArr> arr_out2inp(ia_x0.size()); 
  AzDataArr<AzIntArr> arr_inp2out(i_whole.size()); 
  AzIntArr ia_out2num; ia_out2num.prepare(ia_x0.size()); 
  int ox; 
  for (ox = 0; ox < ia_x0.size(); ++ox) {
    AzIntArr *ptr = arr_out2inp.point_u(ox); 
    int x0 = ia_x0.get(ox); 
    int y0 = ia_y0.get(ox); 
    int xx, yy; 
    for (yy = y0; yy < y0+tmpl.ysz; ++yy) {
      if (yy < 0 || yy >= i_whole.y1) continue;  /* added on 4/29/2014 */
      for (xx = x0; xx < x0+tmpl.xsz; ++xx) {
        if (xx < 0 || xx >= i_whole.x1) continue; /* added on 4/29/2014 */
        int loc = yy*i_whole.xsz + xx; 
        ptr->put(loc);      
        arr_inp2out.point_u(loc)->put(ox);        
      }
    }
    ia_out2num.put(ptr->size()); 
  } 
  
  pia2_out2inp->reset(&arr_out2inp); 
  pia2_inp2out->reset(&arr_inp2out); 
  pia_out2num->reset(&ia_out2num); 
}

/*------------------------------------------------------------*/ 
void AzpPatchDflt::mapping_for_filtering(AzPintArr2 *pia2_out2inp, 
                                   AzPintArr2 *pia2_inp2out)
const
{
  const char *eyec = "AzpPatchDflt::mapping_for_filtering"; 
  Az2D i_whole(&i_region); 
  
  int patch_size = tmpl.size(); 
  
  AzDataArr<AzIntArr> arr_out2inp(ia_x0.size()*patch_size); 
  AzDataArr<AzIntArr> arr_inp2out(i_whole.size()); 
  int ox; 
  for (ox = 0; ox < ia_x0.size(); ++ox) {
    /* (x0,y0) <- the left upper corner of a filter/patch */
    int x0 = ia_x0.get(ox); 
    int y0 = ia_y0.get(ox); 
    int xx, yy; 
    for (yy = y0; yy < y0+tmpl.ysz; ++yy) { /* go through all the pixels in the patch */
      if (yy < 0 || yy >= i_whole.y1) {  /* outside: zero padding */
        continue; 
      }       
      for (xx = x0; xx < x0+tmpl.xsz; ++xx) { /* outside: zero padding */
        if (xx < 0 || xx >= i_whole.x1) {
          continue; 
        }
        int iloc = yy*i_whole.xsz + xx; 
        int oloc = ox*tmpl.size() + (yy-y0)*tmpl.xsz + (xx-x0); 
        arr_out2inp.point_u(oloc)->put(iloc);
        arr_inp2out.point_u(iloc)->put(oloc);
      }
    }
  } 
  
  pia2_out2inp->reset(&arr_out2inp); 
  pia2_inp2out->reset(&arr_inp2out); 
}

/*------------------------------------------------------------*/ 
void AzpPatchDflt::mapping_for_resnorm(const AzxD *region, 
                                 int width, 
                                 AzPintArr2 *pia2_neighbors, 
                                 AzPintArr2 *pia2_whose_neighbor, 
                                 AzPintArr *pia_neighsz) /* # of neighbors */
{
  const char *eyec = "AzpPatchDflt::mapping_for_resnorm"; 
  Az2D i_whole(region); 
  int half_width = width/2; 
  
  AzIntArr ia_neighsz; 
  AzDataArr<AzIntArr> arr_neigh(i_whole.size()); 
  AzDataArr<AzIntArr> arr_whose_neigh(i_whole.size()); 
  int ix; 
  for (ix = 0; ix < i_whole.size(); ++ix) {
    int center_y = ix / i_whole.xsz; 
    int center_x = ix % i_whole.xsz; 

    int yy, xx; 
    for (yy = center_y - half_width; yy <= center_y + half_width; ++yy) {
      if (yy < 0 || yy >= i_whole.ysz) continue; 
      for (xx = center_x - half_width; xx <= center_x + half_width; ++xx) {
        if (xx < 0 || xx >= i_whole.xsz) continue; 
        int loc = yy*i_whole.xsz + xx; 
        arr_neigh.point_u(ix)->put(loc); 
        arr_whose_neigh.point_u(loc)->put(ix); 
      }
    }
    ia_neighsz.put(arr_neigh.point(ix)->size()); 
  } 
  
  pia2_neighbors->reset(&arr_neigh); 
  pia2_whose_neighbor->reset(&arr_whose_neigh); 
  pia_neighsz->reset(&ia_neighsz); 
}
