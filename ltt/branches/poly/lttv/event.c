
void lttv_event_to_string(ltt_event *e, lttv_string *s, bool mandatory_fields)
{
  ltt_facility *facility;
  ltt_eventtype *eventtype;
  ltt_type *type;
  ltt_field *field;
  ltt_time time;

  g_string_set_size(s,0);

  facility = lttv_event_facility(e);
  eventtype = ltt_event_eventtype(e);
  field = ltt_event_field(e);

  if(mandatory_fields) {
    time = ltt_event_time(e);
    g_string_append_printf(s,"%s.%s: %ld.%ld",ltt_facility_name(facility),
        ltt_eventtype_name(eventtype), (long)time.tv_sec, time.tv_nsec);
  }

  print_field(e,f,s);
} 

void print_field(ltt_event *e, ltt_field *f, lttv_string *s) {
  ltt_type *type;
  ltt_field *element;

  int nb, i;

  type = ltt_field_type(f);
  switch(ltt_type_class(type)) {
    case LTT_INT:
      g_string_append_printf(s, " %ld", ltt_event_get_long_int(e,f));
      break;

    case LTT_UINT:
      g_string_append_printf(s, " %lu", ltt_event_get_long_unsigned(e,f));
      break;

    case LTT_FLOAT:
      g_string_append_printf(s, " %g", ltt_event_get_double(e,f));
      break;

    case LTT_STRING:
      g_string_append_printf(s, " \"%s\"", ltt_event_get_string(e,f));
      break;

    case LTT_ENUM:
      g_string_append_printf(s, " %s", ltt_enum_string_get(type,
          event_get_unsigned(e,f));
      break;

    case LTT_ARRAY:
    case LTT_SEQUENCE:
      g_string_append_printf(s, " {");
      nb = ltt_event_field_element_number(e,f);
      element = ltt_field_element(f);
      for(i = 0 ; i < nb ; i++) {
        ltt_event_field_element_select(e,f,i);
        print_field(e,element,s);
      }
      g_string_append_printf(s, " }");
      break;

    case LTT_STRUCT:
      g_string_append_printf(s, " {");
      nb = ltt_type_member_number(type);
      for(i = 0 ; i < nb ; i++) {
        element = ltt_field_member(f,i);
        print_field(e,element,s);
      }
      g_string_append_printf(s, " }");
      break;
  }
}



