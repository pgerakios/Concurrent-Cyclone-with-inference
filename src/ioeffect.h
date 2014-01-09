/* IOEffect 
   Copyright (C) 2008 Prodromos Gerakios, NTUA
   This file is part of the Cyclone compiler.

   The Cyclone compiler is free software; you can redistribute it
   and/or modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   The Cyclone compiler is distributed in the hope that it will be
   useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with the Cyclone compiler; see the file COPYING. If not,
   write to the Free Software Foundation, Inc., 59 Temple Place -
   Suite 330, Boston, MA 02111-1307, USA. */
#ifndef _IOEFFECT_ANALYSIS_H_
#define _IOEFFECT_ANALYSIS_H_

#include "absyn.h"
#include "jump_analysis.h"

namespace IOEffect 
{
 void analysis(JumpAnalysis::jump_anal_res_t tables,List::list_t<Absyn::decl_t,`H> tds);
 Absyn::effect_t	summarize_all( Position::seg_t loc,Absyn::effect_t  f );

}
#endif