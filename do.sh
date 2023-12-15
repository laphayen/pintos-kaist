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
COW_SIMPLE="pintos -v -- -q run cow-simple"

cd threads
make clean
make
cd build
source ../../activate

$PRIORITY_CHANGE