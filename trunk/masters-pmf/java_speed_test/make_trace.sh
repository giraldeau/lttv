(for i in $(seq 1000); do cat basic_record.dat; done) >tmp.dat
(for i in $(seq 10); do cat tmp.dat; done) >trace.dat
