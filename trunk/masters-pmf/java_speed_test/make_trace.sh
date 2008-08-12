(for i in $(seq 1000); do cat basic_record.dat; done) >tmp.dat
(for i in $(seq 500); do cat tmp.dat; done) >tmp2.dat
rm tmp.dat
(for i in $(seq 100); do cat tmp2.dat; done) >trace.dat
rm tmp2.dat
