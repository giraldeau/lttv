
   consist in AND, OR and NOT nested expressions, forming a tree with 
   simple relations as leaves. The simple relations test is a field
   in an event is equal, not equal, smaller, smaller or equal, larger, or
   larger or equal to a specified value. */

typedef enum _lttv_expression_op
{ LTTV_FIELD_EQ,
  LTTV_FIELD_NE,
  LTTV_FIELD_LT,
  LTTV_FIELD_LE,
  LTTV_FIELD_GT,
  LTTV_FIELD_GE
} lttv_expression_op;

typedef _lttv_simple_expression
{ lttv_expression_op op;
  char *field_name;
  char *value;
} lttv_simple_expression;

typedef _lttv_expression_type
{ LTTV_EXPRESSION,
  LTTV_SIMPLE_EXPRESSION

}
typedef struct _lttv_expression 
{ bool or;
  bool not;
  bool simple_expression;
  union 
  { lttv_expression *e;
    lttv_field_relation *se;
  } e;
} lttv_expression;

read_token

read_expression
  ( read expr )
  simple expr [ op expr ]

read_simple_expression
  read_field_path [ rel value ]

read_field_path
  read_field_component [. field path]

read_field_component
  name [ \[ value \] ]

data struct:
and/or(left/right)
not(child)
op(left/right)
path(component...) -> field


