/* * * * *
 *  AzPrepText.hpp 
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

#ifndef _AZ_PREP_TEXT_HPP_
#define _AZ_PREP_TEXT_HPP_
 
#include "AzUtil.hpp"
#include "AzStrPool.hpp" 
#include "AzPrint.hpp"
#include "AzSmat.hpp"
#include "AzDic.hpp"

class AzPrepText {
protected: 
  AzOut out; 
public:
  AzPrepText(const AzOut &_out) : out(_out) {}
  void gen_vocab(int argc, const char *argv[]) const; 
  void gen_regions(int argc, const char *argv[]) const; 
  void show_regions(int argc, const char *argv[]) const; 
  void gen_nbw(int argc, const char *argv[]) const; 
  void gen_nbwfeat(int argc, const char *argv[]) const; 
  void gen_b_feat(int argc, const char *argv[]) const; 
  void split_text(int argc, const char *argv[]) const;
  
protected:                         
  /*---  for gen_vocab  ---*/
  void put_in_voc(int nn, 
                  AzStrPool &sp_voc, 
                  const AzStrPool &sp_words, 
                  int wx, /* position in sp_words */
                  AZint8 count, 
                  const AzStrPool *sp_stop, 
                  bool do_remove_number) const; 
  static AzByte gen_1byte_index(const AzStrPool *sp_words, int wx); 
  static int write_vocab(const char *fn, const AzStrPool *sp, 
                          int max_num, int min_count, bool do_write_count); 
  
  /*---  for gen_regions  ---*/
  void gen_nobow_regions(const AzIntArr *ia_tokno, int dic_sz, 
                       int pch_sz, int pch_step, int padding,  
                       bool do_allow_zero, bool do_skip_unk, 
                       /*---  output  ---*/
                       AzSmat *m_feat, 
                       AzIntArr *ia_pos) const; /* patch position: may be NULL */                        
  void gen_nobow_regions2(const AzIntArr *ia_tokno, int dic_sz, int pch_sz, const AzIntArr &ia_tx0,  
                          AzSmat *m_feat, AzIntArr *ia_pos) const; 
  void gen_bow_regions(const AzIntArr *ia_tokno, int dic_sz, 
                    int pch_sz, int pch_step, int padding,  
                    bool do_skip_stopunk, bool do_allow_zero, 
                    /*---  output  ---*/
                    AzSmat *m_feat, 
                    AzIntArr *ia_pos,  /* patch position: may be NULL */ 
                    /*---  optional input  ---*/
                    int unk_id = -1) const; 
  void gen_bow_regions2(const AzIntArr *ia_tokno, int dic_sz, int pch_sz, const AzIntArr &ia_tx0,  
                        AzSmat *m_feat, AzIntArr *ia_pos, int unk_id = -1) const; 
  void gen_bow_ngram_regions(const AzDataArr<AzIntArr> &aia_tokno, 
                       const AzIntArr &ia_nn, 
                       int dic_sz, 
                       int pch_sz, int pch_step, int padding,  
                       bool do_skip_stopunk, bool do_allow_zero, 
                       /*---  output  ---*/
                       AzSmat *m_feat, 
                       AzIntArr *ia_pos) const; /* patch position: may be NULL */                    
  void gen_bow_ngram_regions2(const AzDataArr<AzIntArr> &aia_tokno, 
                       const AzIntArr &ia_nn, 
                       int dic_sz, int pch_sz, 
                       const AzIntArr *ia_tx0, 
                       /*---  output  ---*/
                       AzSmat *m_feat, AzIntArr *ia_pos) const;                        
  static void gen_nobow_dic(const AzDic *inp_dic, int pch_sz, AzDic *out_dic); 
  template <class M> 
  static void gen_nobow_dic(const AzDic *inp_dic, const M *mat, AzDic *out_dic) {
    int row_num = mat->rowNum(); 
    int pch_sz = row_num / inp_dic->size(); 
    if (row_num % inp_dic->size() != 0) {
      throw new AzException("AzPrepText::gen_nobow_dic", "#rows must be a multiple of dic size"); 
    }
    gen_nobow_dic(inp_dic, pch_sz, out_dic); 
    if (out_dic->size() != row_num) {
      throw new AzException("AzPrepText::gen_nobow_dic", "something is wrong ..."); 
    }
  }    
  void check_batch_id(const AzBytArr &s_batch_id) const; 
  
  /*---  for show_regions  ---*/
  void _show_regions(const AzSmatVar *mv, const AzDic *dic, bool do_wordonly) const; 
  
  /*---  for nbw  ---*/
  static void scale(AzSmat *ms, const AzDvect *v);                          
  static void smat_to_smatvar(const AzSmat *ms, AzSmatVar *msv); 
  
  /*---  ---*/
  void write_Y(AzSmat &m_y, const AzBytArr &s_y_fn, const AzBytArr *s_batch_id=NULL) const; 
  void write_X(AzSmat &m_x, const AzBytArr &s_x_fn, const AzBytArr *s_batch_id=NULL) const; 
  
  void check_y_ext(const AzBytArr &s_y_ext, const char *eyec) const; 
  int add_unkw(AzDic &dic) const; 

  template <class Mat> /* Mat: AzSmat | AzSmatc */
  void check_size(const Mat &m) const; 
}; 
#endif 