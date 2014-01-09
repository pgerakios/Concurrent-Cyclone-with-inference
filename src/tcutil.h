/* Utility functions for type checking.
   Copyright (C) 2001 Greg Morrisett, AT&T
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
#ifndef _TCUTIL_H_
#define _TCUTIL_H_

#include "relations-ap.h"
#include "tcenv.h"

namespace Tcutil {
using List;
using Absyn;

///////////////////  Warnings and Error messages ///////////////////////
`a impos(string_t,...inject parg_t)__attribute__((format(printf,1,2),noreturn));
void terr(seg_t, string_t,...inject parg_t)__attribute__((format(printf,2,3)));
void warn(seg_t, string_t,...inject parg_t)__attribute__((format(printf,2,3)));

////////////////  Predicates and Destructors for Types /////////////////
// Pure predicates
//
bool is_pointer_effect( type_t t);

bool is_void_type(type_t);       // void
bool is_char_type(type_t);       // char, signed char, unsigned char
bool is_any_int_type(type_t);    // char, short, int, etc.
bool is_any_float_type(type_t);  // any size or sign
bool is_integral_type(type_t);   // any_int or tag_t or enum
bool is_arithmetic_type(type_t); // integral or any_float
bool is_signed_type(type_t);     // signed
bool is_function_type(type_t);   // function type
bool is_pointer_type(type_t);    // pointer type
bool is_array_type(type_t);      // array type
bool is_boxed(type_t);           // boxed type

// Predicates that may constrain things
bool is_fat_ptr(type_t); // fat pointer type
bool is_zeroterm_pointer_type(type_t); // @zeroterm pointer type
bool is_nullable_pointer_type(type_t); // nullable pointer type
bool is_bound_one(ptrbound_t);         
// returns true if this is a t ? -- side effect unconstrained bounds
bool is_fat_pointer_type(type_t);
// returns true iff t contains only "bits" and no pointers or datatypes. 
// This is used to restrict the members of unions to ensure safety.
bool is_bits_only_type(type_t);
// does the function (or fn pointer) type have the "noreturn" attribute?
bool is_noreturn_fn_type(type_t);

// Deconstructors for types

// return the element type for a pointer type
type_t pointer_elt_type(type_t);
// return the region for a pointer type
type_t pointer_region(type_t);
// If the given type is a pointer type, returns the region it points
// into.  Returns NULL if not a pointer type.
bool rgn_of_pointer(type_t t, type_t@ rgn);
// For a thin bound, returns the associated expression, and for a fat 
// bound, returns null.  If b is unconstrained, then sets it to def.
exp_opt_t get_bounds_exp(ptrbound_t def, ptrbound_t b);
// Extracts the number of elements for an array or pointer type.
// If the bounds are unconstrained, then sets them to @thin@numelts(1).
exp_opt_t get_type_bound(type_t);
// like is_fat_pointer_type, but puts element type in elt_dest when true.
bool is_fat_pointer_type_elt(type_t t, type_t@ elt_dest);
// like above but checks to see if the pointer is zero-terminated
bool is_zero_pointer_type_elt(type_t t, type_t@ elt_type_dest);
// like above but also works for arrays and returns dynamic pointer stats
bool is_zero_ptr_type(type_t t,type_t @ptr_type,bool @is_fat,type_t @elt_type);
// if b is fat return NULL, if it's thin(e), return e, if
// it's unconstrained, constrain it to def and return that.
exp_opt_t get_bounds_exp(ptrbound_t def, ptrbound_t b);

//////////////////////// UTILITIES for Expressions ///////////////
bool is_integral(exp_t); 
bool is_numeric(exp_t);
bool is_zero(exp_t);

// returns a deep copy of a type -- note that the evars will
// still share and if the identity is set on type variables,
// they will share, otherwise they won't.
type_t copy_type(type_t);
// deep copy of an expression; uses copy_type above, so same rules apply.
// if [preserve_types], then copies the type, too; otherwise null
exp_t deep_copy_exp(bool preserve_types, exp_t);

// returns true if first-arg is a sub-kind of second-arg
bool kind_leq(kind_t,kind_t);
bool kind_eq(kind_t,kind_t);

// returns the type of a function declaration
type_t fd_type(fndecl_t); 
kind_t tvar_kind(tvar_t,kind_t def);
kind_t type_kind(type_t);
type_t compress(type_t);
void unchecked_cast(exp_t, type_t, coercion_t);
bool coerce_uint_type(exp_t);
bool coerce_sint_type(exp_t);
bool coerce_to_bool(exp_t);

bool coerce_arg(  Tcenv::tenv_t te , exp_t, type_t, bool@ alias_coercion); 
bool coerce_assign( Tcenv::tenv_t te , exp_t, type_t);
bool coerce_list( Tcenv::tenv_t te , type_t, list_t<exp_t>);
// true when expressions of type t1 can be implicitly cast to t2
bool silent_castable( Tcenv::tenv_t te ,seg_t,type_t,type_t);
// true when expressions of type t1 can be cast to t2 -- call silent first
coercion_t castable( Tcenv::tenv_t te ,seg_t,type_t,type_t);
// true when t1 is a subtype of t2 under the given list of subtype assumptions
bool subtype( Tcenv::tenv_t te , list_t<$(type_t,type_t)@`H,`H> assume, 
	     type_t t1, type_t t2);
// if t is a nullable pointer type and e is 0, changes e to null 
bool zero_to_null(type_t, exp_t);

// used to alias the given expression, assumed to have non-Aliasable type
$(decl_t,exp_t) insert_alias(exp_t e, type_t e_typ);
// prints a warning when an alias coercion is inserted
extern bool warn_alias_coerce;
// flag to control whether or not we print a warning when implicitly casting
// a pointer from one region into another due to outlives constraints.
extern bool warn_region_coerce;

// useful kinds
extern struct Kind xrk; // shareable xregion kind
extern struct Kind rk; // shareable region kind
extern struct Kind ak; // shareable abstract kind
extern struct Kind bk; // shareable boxed kind
extern struct Kind mk; // shareable mem kind
extern struct Kind ek; // effect kind
extern struct Kind ik; // int kind
extern struct Kind boolk; // boolean kind
extern struct Kind ptrbk; // pointer bound kind

extern struct Kind trk; // top region kind
extern struct Kind tak; // top abstract kind
extern struct Kind tbk; // top boxed kind
extern struct Kind tmk; // top memory kind

extern struct Kind urk;  // unique region kind
extern struct Kind uak;  // unique abstract kind
extern struct Kind ubk;  // unique boxed kind
extern struct Kind umk;  // unique memory kind

extern struct Core::Opt<kind_t> rko;
extern struct Core::Opt<kind_t> ako;
extern struct Core::Opt<kind_t> bko;
extern struct Core::Opt<kind_t> mko;
extern struct Core::Opt<kind_t> iko;
extern struct Core::Opt<kind_t> eko;
extern struct Core::Opt<kind_t> boolko;
extern struct Core::Opt<kind_t> ptrbko;

extern struct Core::Opt<kind_t> trko;
extern struct Core::Opt<kind_t> tako;
extern struct Core::Opt<kind_t> tbko;
extern struct Core::Opt<kind_t> tmko;

extern struct Core::Opt<kind_t> urko;
extern struct Core::Opt<kind_t> uako;
extern struct Core::Opt<kind_t> ubko;
extern struct Core::Opt<kind_t> umko;

Core::opt_t<kind_t> kind_to_opt(kind_t k);
kindbound_t kind_to_bound(kind_t k);

$(tvar_t,kindbound_t) swap_kind(type_t, kindbound_t);
  // for temporary kind refinement

type_t max_arithmetic_type(type_t, type_t);

  // linear order on types (needed for dictionary indexing)
int typecmp(type_t, type_t);
int aggrfield_cmp(aggrfield_t, aggrfield_t); // used in toc.cyc

type_t substitute(list_t<$(tvar_t,type_t)@`H,`H>, type_t);
  // could also have a version with two regions, but doesn't seem useful
type_t rsubstitute(region_t<`r>,list_t<$(tvar_t,type_t)@`r,`r>,type_t);
list_t<$(type_t,type_t)@> rsubst_rgnpo(region_t<`r>,
				       list_t<$(tvar_t,type_t)@`r,`r>,
				       list_t<$(type_t,type_t)@`H,`H>);

// substitute through an expression
exp_t rsubsexp(region_t<`r>, list_t<$(tvar_t,type_t)@`r,`r>, exp_t);

// true when e1 is a sub-effect of e2
bool subset_effect(bool may_constrain_evars, type_t e1, type_t e2);

// returns true when rgn is in effect -- won't side-effect any evars when
// constrain is false.
bool region_in_effect(bool constrain, type_t r, type_t e);

type_t fndecl2type(fndecl_t);

// generate an appropriate evar for a type variable -- used in
// instantiation.  The list of tvars is used to constrain the evar.
$(tvar_t,type_t)@   make_inst_var(list_t<tvar_t,`H>,tvar_t);
$(tvar_t,type_t)@`r r_make_inst_var($(list_t<tvar_t,`H>,region_t<`r>)@,tvar_t);
					   

// checks that a width given on a struct or union member is consistent
// with the type definition for the member.
void check_bitfield(seg_t, type_t field_typ, exp_opt_t width, stringptr_t fn);

void check_unique_vars(list_t<var_t,`r>, seg_t, string_t err_msg);
void check_unique_tvars(seg_t,list_t<tvar_t>);

// Check that bounds are not zero -- constrain to 1 if necessary
void check_nonzero_bound(seg_t, ptrbound_t);
// Check that bounds are greater than i -- constrain to i+1 if necessary
void check_bound(seg_t, unsigned int i, ptrbound_t, bool do_warn);

list_t<$(aggrfield_t,`a)@`r,`r> 
resolve_aggregate_designators(region_t<`r>, seg_t,
                              list_t<$(list_t<designator_t>,`a)@`r2,`r3>, 
                              aggr_kind_t, list_t<aggrfield_t>);
// is e1 of the form *ea or ea[eb] where ea is a zero-terminated pointer?
// If so, return true and set ea and eb appropriately (for *ea set eb to 0).
// Finally, if the pointer is fat, set is_fat.
bool is_zero_ptr_deref(exp_t, type_t @ptr_type, bool @is_fat, type_t @elt_type);
			      

// returns true if this a non-aliasable region, e.g. `U, `r::TR, etc.
bool is_noalias_region(type_t r, bool must_be_unique);

// returns true if this a non-aliasable pointer, e.g. *`U, *`r::TR, etc.
bool is_noalias_pointer(type_t t, bool must_be_unique);

// returns true if this expression only deferences non-aliasable pointers
// and if the ultimate result is a noalias pointer or aggregate.  The
// region is used for allocating temporary stuff.
bool is_noalias_path(exp_t);

// returns true if this expression is an aggregate that contains
// non-aliasable pointers or is itself a non-aliasable pointer
// The region is used for allocating temporary stuff
bool is_noalias_pointer_or_aggr(type_t);

// Ensure e is an lvalue or function designator -- return whether
// or not &e is const and what region e is in.
$(bool,type_t) addressof_props(exp_t);

// Given an effect, express/mutate it to have only regions(`a), `r, and joins.
type_t normalize_effect(type_t e);


// Gensym a new type variable with kind bounded by k
tvar_t new_tvar(kindbound_t);
// Get an identity for a type variable
int new_tvar_id();
// Add an identity to a type variable if it doesn't already have one
void add_tvar_identity(tvar_t);
void add_tvar_identities(list_t<tvar_t>);
// true iff t has been generated by new_tvar (#xxx)
bool is_temp_tvar(tvar_t);

// are the lists of attributes the same?  doesn't require the same order
bool same_atts(attributes_t, attributes_t);
// are the lists of attributes equivalent? regparm(0) is optional, since this is the gcc default
// but if the attribute -mregparm=n is used to override the default then this is not sound
// places in the linux kernel are lax with this attribute; need this to compile some headers
bool equiv_fn_atts(attributes_t a1, attributes_t a2);

// returns true iff e is an expression that can be evaluated at compile time
bool is_const_exp(exp_t);

// like Core::snd, but first argument is a tqual_t (not a BoxKind)
type_t snd_tqt($(tqual_t,type_t)@);

// If t is a typedef, returns true if the typedef is const, and warns
// if the flag declared_const is true.  Otherwise returns declared_const.
bool extract_const_from_typedef(seg_t, bool declared_const, type_t);

// Transfer any function type attributes from the given list to the
// function type.  
attributes_t transfer_fn_type_atts(type_t, attributes_t);

// issue a warning if the type is a typedef with non-empty qualifiers
void check_no_qual(seg_t, type_t);

// If b is a non-escaping variable binding, return a non-null pointer to
// the vardecl.
struct Vardecl *nonesc_vardecl(binding_t);

  // filters out null elements
list_t<`a> filter_nulls(list_t<`a *>);

// If t is an array type, promote it to an at-pointer type into the
// specified region.
type_t promote_array(type_t t, type_t rgn, bool convert_tag);

// does the type admit zero?
bool zeroable_type(type_t);

// try to constrain the boolean kinded type to desired and then
// convert it the type to a boolean.
bool force_type2bool(bool desired, type_t);

// unconstrained boolean-kinded type node
type_t any_bool(list_t<tvar_t,`H>);
// unconstrained pointer bound type node
type_t any_bounds(list_t<tvar_t,`H>);

  // This stuff is used only by tctyp -- still fleshing out these boundaries
  bool admits_zero(type_t);
  void replace_rops(list_t<$(var_opt_t,tqual_t,type_t)@>, 
		    Relations::reln_t);
  Core::opt_t<kindbound_t> kind_to_bound_opt(kind_t k);
  int fast_tvar_cmp(tvar_t,tvar_t);

  // This stuff is used only by unify and tcutil
  // fairly semantic equivalence
  bool same_rgn_po(list_t<$(type_t,type_t)@>, list_t<$(type_t,type_t)@>);
  int tycon_cmp(tycon_t,tycon_t);
  int star_cmp(int (@cmp)(`a@`r,`a@`r),`a*`r, `a*`r);
  type_t rgns_of(type_t t);

	bool is_tagged_union_and_needs_check( exp_t e );
}
#endif