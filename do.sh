# threads
ALARM_SINGLE="pintos -v -k -T 60 -m 20   --fs-disk=10  --swap-disk=4 -- -q  -threads-tests -f run alarm-single"
ALARM_MULTIPLE="pintos -v -k -T 60 -m 20   --fs-disk=10  --swap-disk=4 -- -q  -threads-tests -f run alarm-multiple"
ALARM_SIMULTANEOUS="pintos -v -k -T 60 -m 20   --fs-disk=10  --swap-disk=4 -- -q  -threads-tests -f run alarm-simultaneous"
ALARM_PRIORITY="pintos -v -k -T 60 -m 20   --fs-disk=10  --swap-disk=4 -- -q  -threads-tests -f run alarm-priority"
ALARM_ZERO="pintos -v -k -T 60 -m 20   --fs-disk=10  --swap-disk=4 -- -q  -threads-tests -f run alarm-zero"
ALARM_NEGATIVE="pintos -v -k -T 60 -m 20   --fs-disk=10  --swap-disk=4 -- -q  -threads-tests -f run alarm-negative"
PRIORITY_CHANGE="pintos -v -k -T 60 -m 20   --fs-disk=10  --swap-disk=4 -- -q  -threads-tests -f run priority-change"
PRIORITY_DONATE_ONE="pintos -v -k -T 60 -m 20   --fs-disk=10  --swap-disk=4 -- -q  -threads-tests -f run priority-donate-one"
PRIORITY_DONATE_MULTI="pintos -v -k -T 60 -m 20   --fs-disk=10  --swap-disk=4 -- -q  -threads-tests -f run priority-donate-multiple"
PRIORITY_DONATE_MULTI2="pintos -v -k -T 60 -m 20   --fs-disk=10  --swap-disk=4 -- -q  -threads-tests -f run priority-donate-multiple2"
PRIORITY_DONATE_NEST="pintos -v -k -T 60 -m 20   --fs-disk=10  --swap-disk=4 -- -q  -threads-tests -f run priority-donate-nest"
PRIORITY_DONATE_SEMA="pintos -v -k -T 60 -m 20   --fs-disk=10  --swap-disk=4 -- -q  -threads-tests -f run priority-donate-sema"
PRIORITY_DONATE_LOWER="pintos -v -k -T 60 -m 20   --fs-disk=10  --swap-disk=4 -- -q  -threads-tests -f run priority-donate-lower"
PRIORITY_FIFO="pintos -v -k -T 60 -m 20   --fs-disk=10  --swap-disk=4 -- -q  -threads-tests -f run priority-fifo"
PRIORITY_PREEMPT="pintos -v -k -T 60 -m 20   --fs-disk=10  --swap-disk=4 -- -q  -threads-tests -f run priority-preempt"
PRIORITY_SEMA="pintos -v -k -T 60 -m 20   --fs-disk=10  --swap-disk=4 -- -q  -threads-tests -f run priority-sema"
PRIORITY_CONDVAR="pintos -v -k -T 60 -m 20   --fs-disk=10  --swap-disk=4 -- -q  -threads-tests -f run priority-condvar"
PRIORITY_DONATE_CHAIN="pintos -v -k -T 60 -m 20   --fs-disk=10  --swap-disk=4 -- -q  -threads-tests -f run priority-donate-chain"
COW_SIMPLE="pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/vm/cow/cow-simple:cow-simple --swap-disk=4 -- -q   -f run cow-simple"

cd threads
make clean
make
cd build
source ../../activate

pintos -v -- -q run alarm-multiple