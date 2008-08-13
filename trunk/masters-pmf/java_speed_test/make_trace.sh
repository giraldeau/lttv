(for i in $(seq 1000); do cat basic_record.dat; done) >tmp.dat
(for i in $(seq 500); do cat tmp.dat; done) >trace_short.dat
rm tmp.dat
(for i in $(seq 10); do cat trace_short.dat; done) >trace_med.dat
(for i in $(seq 10); do cat trace_med.dat; done) >trace_long.dat
