#ifndef _LTT_CONTROL_H
#define _LTT_CONTROL_H


//enum trace_mode {
//	TRACE_NORMAL,
//	TRACE_FLIGHT
//};

enum trace_op {
  OP_CREATE,
  OP_DESTROY,
  OP_START,
  OP_STOP,
	OP_ALIGN,
  OP_NONE
};

typedef struct lttctl_peer_msg {
	char trace_name[NAME_MAX];
	enum trace_op op;
	union ltt_control_args args;
} lttctl_peer_msg_t;

#endif //_LTT_CONTROL_H

