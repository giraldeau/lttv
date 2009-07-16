# makefile

COPTIONS=-DSAFETY


default:
	make commit_sum | tee commit_sum.log
	make read_write | tee read_write.log
	make events_lost | tee events_lost.log
	make no_events_lost | tee no_events_lost.log


no_events_lost: clean no_events_lost_ltl run

no_events_lost_ltl:
	cat defines > pan.ltl
	cat no_events_lost.def > pan.spin
	spin -f "!(`cat no_events_lost.ltl | grep -v ^//`)" >> pan.ltl



events_lost: clean events_lost_ltl run

events_lost_ltl:
	cat defines > pan.ltl
	spin -f "!(`cat events_lost.ltl | grep -v ^//`)" >> pan.ltl



read_write: clean read_write_ltl run

read_write_ltl:
	cat defines > pan.ltl
	spin -f "!(`cat read_write.ltl | grep -v ^//`)" >> pan.ltl



commit_sum: clean commit_sum_ltl run

commit_sum_ltl:
	cat defines > pan.ltl
	spin -f "!(`cat commit_sum.ltl | grep -v ^//`)" >> pan.ltl



run: pan
	./pan -v -X -m100000 -w21  -a -c1

pan: pan.c
	gcc -w -o pan -D_POSIX_SOURCE -DMEMLIM=750 -DXUSAFE -DNOFAIR  pan.c

pan.c: pan.ltl model.spin
	cat model.spin >> pan.spin
	spin -a -X -N pan.ltl pan.spin



clean:
	rm -f pan* trail.out
