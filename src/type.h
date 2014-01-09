#ifndef _TYPE_H_
#define _TYPE_H_

#ifdef TYPE_CYC
#define EXTERN_ABSYN
#else
#define EXTERN_ABSYN extern
#endif

#include <list.h>
#include <position.h>

namespace Type {

  typedef enum FLOAT_SIZE { FLOAT_SZ, DOUBLE_SZ, LONG_DOUBLE_SZ } float_size_t;

  datatype TypeCon {
    VarType(tvar_t);
    VoidType;  // MemKind
    IntType(sign_t,size_of_t); // MemKind for all but int sizes
    FloatType(float_size_t);   // MemKind
    ArrayType;                 // MemKind * IntKind -> MemKind
    PointerType;               
    TupleType(unsigned int);   // MemKind1 * ... * MemKindn -> MemKind
    DefDatatypeType(datatypedecl_t@); // K1*...*Kn -> AbsKind
    UndefDatatypeType(qvar_t name, bool is_extensible);
    DefDatatypeFieldType(datatypedecl_t, datatypefield_t);
    UndefDatatypeFieldType(struct UnknownDatatypeFieldInfo);
    DefAggregateType(aggrdecl_t@);
    UndefAggregateType(aggr_kind_t, typedef_name_t, opt_t<bool> tagged);
    AnonAggrType(aggr_kind_t, list_t<aggrfield_t>);
    EnumType(typedef_name_t,struct Enumdecl *);
    AnonEnumType(list_t<enumfield_t>);
    RgnHandleType;
    DynRgnType;  // omit?
    TagType;
    HeapRgn;
    UniqueRgn;
    RefCntRgn;
    AccessEff;
    JoinEff;
    RgnsEff;
    BuiltinType(string_t,kind_t);
    TrueType;
    FalseType;
    NotType;
    OrType;
    AndType;
    TypedefType(typedef_name_t,struct Typedefdecl *);
    TypeDeclType(type_decl_t);
    TypeofType(exp_t);
    FatType;
    ThinType;
  };
  typedef struct TypeCon type_con_t;

  datatype TypeConstraint {
    TypeKind;
    IntKind;
    BoolKind;
    RegionKind;
    EffectKind;
    BoxedKind;
    ArithmeticKind;
    IntegralKind;
    TypeOfKnownSize;
    HasMember(field_name_t,type_t);
    SubType(type_t);
    SuperType(type_t);
    CoercesToType(type_t);
    CoercesFromType(type_t);
    IsWritable;
    SubEffect(type_t);
    SubRegion(type_t);
    ScopedVars(list_t<tvar_t>);
  };

  datatype RawType {
    UnknownType;
    AppType(type_con_t,list_t<type_t>);
    FnType(fn_info_t);
    ValueofType(exp_t);
  };
  typedef datatype RawType @raw_type_t;

  typedef struct Type @type_t, *type_opt_t;

  struct Type {
    type_opt_t equiv_class;  // forwarding pointer to equivalence class
    list_t<constraint_t> constraints; 
    raw_type_t defn;         // the definition of the type
    tqual_t    qualifiers;   // qualifiers (only make sense on certain kinds)
    seg_t      loc;          // location where the type was constructed
  };


  void add_constraint_to_type(tenv_t tenv, constraint_t c, type_t t) {
    switch (t->defn) {
    case &Unknown: return add_constraint_to_unknown(tenv,c,t);
    default:
      switch (c) {
      case &TypeKind: 
        if (!has_type_kind(tenv,t)) fail(tenv,badcon(c,t));
        return;
      case &IntKind:
        if (!has_int_kind(tenv,t)) fail(tenv,badcon(c,t));
        return;
      case &BoolKind:        
        if (!has_bool_kind(tenv,t)) fail(tenv,badcon(c,t));
        return;
      case &HasMember(f,t):
        
      }
    }
  }

  void equate(tenv_t tenv, type_t t1, type_t t2) {
    if (t1 == t2) return;
    t1 = compress(t1);
    t2 = compress(t2);
    switch $(t1->defn, t2->defn) {
    case $(&UnknownType, _): 
      check_occurs(tenv,t1,t2);
      trail_set(tenv, t1->equiv_class, t2);
      for (let cs = t1->constraints; cs != NULL; cs = cs->tl)
        add_constraint_to_type(tenv, cs->hd, t2);
      return;
    case $(_, &UnknownType): return equate(t2,t1);
    case $(&AppType(c1,ts1),&AppType(c2,ts2)):
      if (!cmp_tycon(c1,c2)) fail(tenv,mismatched_con(t1,t2));
      for (; ts1 != NULL && ts2 != NULL)
        equate(tenv,ts1->hd,ts2->hd);
      return;
    case $(&FnType(fninfo1),&FnType(fninfo2)):
      ...
    case $(&ValueofType(e1),&ValueofType(e2)):
      if (!Evexp:same_const_exp(e1,e2)) fail(trail,unequal_exps(t1,t2));
      return;
    }
  }

#endif
