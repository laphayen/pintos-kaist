# threads
ALARM_SINGLE="pintos -v -- -q run alarm-single"
ALARM_MULTIPLE="pintos -v -- -q run alarm-multiple"
ALARM_SIMULTANEOUS="pintos -v -- -q run alarm-simultaneous"
ALARM_PRIORITY="pintos -v -- -q run alarm-priority"
ALARM_ZERO="pintos -v -- -q run alarm-zero"
ALARM_NEGATIVE="pintos -v -- -q run alarm-negative"
PRIORITY_CHANGE="pintos -v -- -q run priority-change"
PRIORITY_DONATE_ONE="pintos -v -- -q run priority-donate-one"
PRIORITY_DONATE_MULTI="pintos -v -- -q run priority-donate-multiple"
PRIORITY_DONATE_MULTI2="pintos -v -- -q run priority-donate-multiple2"
PRIORITY_DONATE_NEST="pintos -v -- -q run priority-donate-nest"
PRIORITY_DONATE_SEMA="pintos -v -- -q run priority-donate-sema"
PRIORITY_DONATE_LOWER="pintos -v -- -q run priority-donate-lower"
PRIORITY_FIFO="pintos -v -- -q run priority-fifo"
PRIORITY_PREEMPT="pintos -v -- -q run priority-preempt"
PRIORITY_SEMA="pintos -v -- -q run priority-sema"
PRIORITY_CONDVAR="pintos -v -- -q run priority-condvar"
PRIORITY_DONATE_CHAIN="pintos -v -- -q run priority-donate-chain"
MLFQS_LOAD_1="pintos -v -k -T 480 -m 20   -- -q  -mlfqs run mlfqs-load-1"
MLFQS_LOAD_60="pintos -v -k -T 480 -m 20   -- -q  -mlfqs run mlfqs-load-60"
MLFQS_LOAD_AVG="pintos -v -k -T 480 -m 20   -- -q  -mlfqs run mlfqs-load-avg "
MLFQS_RECENT_1="pintos -v -k -T 480 -m 20   -- -q  -mlfqs run mlfqs-recent-1"
MLFQS_FAIR_2="pintos -v -k -T 480 -m 20   -- -q  -mlfqs run mlfqs-fair-2"
MLFQS_FAIR_20="pintos -v -k -T 480 -m 20   -- -q  -mlfqs run mlfqs-fair-20"
MLFQS_NICE_2="pintos -v -k -T 480 -m 20   -- -q  -mlfqs run mlfqs-nice-2"
MLFQS_NICE_10="pintos -v -k -T 480 -m 20   -- -q  -mlfqs run mlfqs-nice-10"
MLFQS_BLOCK="pintos -v -k -T 480 -m 20   -- -q  -mlfqs run mlfqs-block"

cd userprog
make clean
make
cd build
source ../../activate
pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/userprog/multi-child-fd:multi-child-fd -p ../../tests/userprog/sample.txt:sample.txt -p tests/userprog/child-close:child-close -- -q   -f run multi-child-fd